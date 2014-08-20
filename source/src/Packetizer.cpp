#include "Packetizer.h"
#include "Util.h"
#include "debug.h"

#include <boost/lexical_cast.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <cassert>
#include <iostream>
#include <string>


using namespace boost::asio; 

int Packetizer::nextPacketID_ = 0; 

void Packetizer::write(unsigned char* buf, 
		int len, StreamWriteHandler handler)
{
	if (len <= 0) return;

	RawPacket * rawpkt = new RawPacket;
	assert((size_t)len <= sizeof(rawpkt->data));
	rawpkt->length = len;
	rawpkt->type   = MT_DATA;
	memcpy(rawpkt->data, buf, len);
	rawpkt->id = nextPacketID_++;
	{
		boost::unique_lock<boost::mutex> lock(_send_q_mutex);
		_send_q.put(rawpkt);
	}

	boost::system::error_code error;
	handler(error);
}


void Packetizer::read(StreamReadHandler handler) {
	boost::unique_lock<boost::mutex> read_lock(_read_mutex);
	_reading = true;
	_read_handler = handler;

	_cv_read.notify_all();
}


void Packetizer::streamRead() {
	while (_running) {
		boost::unique_lock<boost::mutex> read_lock(_read_mutex);
		while (!_reading) {
			_cv_read.wait(read_lock);
		}

		RawPacket * rawpkt = 0;
		{
			boost::unique_lock<boost::mutex> lock(_recv_q_mutex);

			bool isc = false;
			while (!(isc = _recv_q.completed()) && _recv_q.empty()) {
				_cv_recv_q.wait(lock);
			}

			if (isc) {
				unsigned char c[1];
				_read_handler(c, -1);

				return;
			}
			else {
				rawpkt = _recv_q.get();
			}
		}

		assert(rawpkt);
		_read_handler(rawpkt->data, rawpkt->length);

		delete rawpkt;

		_reading = false;
	}
}  

void Packetizer::close(bool force) {
	DEBUG_FUNC;
	if (!force) {
		boost::unique_lock<boost::mutex> lock(_send_q_mutex);
		while (!_send_q.empty()) {
			_cv_send_q.wait(lock);
		}

		std::cerr << "usual close()" << std::endl;
		uint32_t seq = _send_q.current_seq;
		UDPPacket * pkt = genGoodbye(seq);
		_client->send(pkt);
		delete pkt;
	}
	join();
}

void Packetizer::handleCloseComplete(UDPPacket * pkt, const boost::system::error_code& error, 
		std::size_t bytes_transferred) {
	DEBUG_FUNC;
	delete pkt;
	join();
}

void Packetizer::packetSend() {
	boost::posix_time::time_duration ack_interval(boost::posix_time::milliseconds(100));
	boost::posix_time::ptime         last_clock  (boost::posix_time::microsec_clock::local_time());

	if(_is_server)
	{
		boost::unique_lock<boost::mutex> lock(_start_send_mutex);
		std::cerr << "Waiting for the client to send a packet!" << std::endl;
		while(!_sending_started)
			_cv_start_send.wait(lock);
		std::cerr << "Client sent a packet, now sending our packets!!!" << std::endl;
	}
	while (_running) {

		std::pair<size_t, size_t> query = _morpher->get_next_time_len(512);
		long   trgt_timing = query.first;
		size_t trgt_length = query.second;
		//std::cerr << "Producing a packet of size " << trgt_length << std::endl;
		uint8_t     flag   = 0;
		uint32_t    seq    = 0;
		uint32_t    ack    = 0;
		RawPacket * rawpkt = 0;
		UDPPacket * udppkt = 0;

		if (trgt_length > PACKET_HDR_LEN) trgt_length -= PACKET_HDR_LEN;

		boost::this_thread::sleep(boost::posix_time::milliseconds(trgt_timing));
		boost::this_thread::yield();
		{
			boost::unique_lock<boost::mutex> lock(_send_q_mutex);
			if (!_send_q.empty()) {
				rawpkt = _send_q.get(trgt_length);
#ifdef DEBUG_PKTIZER
				std::cerr << "+";
#endif
				assert(rawpkt->length <= trgt_length);
			}

			if (!rawpkt) {
				// generate dummy packet
				rawpkt = new RawPacket;
				rawpkt->length = trgt_length;
				rawpkt->type   = MT_DUMMY;
			}

			seq    = rawpkt->seq_hi;   // seq should be the sequence number of the
			// last byte sent
			ack    = 0;


			boost::posix_time::ptime current(boost::posix_time::microsec_clock::local_time());
			if (last_clock + ack_interval <= current) {
				ack = _recv_seq;
				flag |= FG_ACK;

				last_clock = current;
			}
			else {
				flag = 0;
			}

			///*************************************************************************
			/// build and encrypt udp packet
			///*************************************************************************

			udppkt = new UDPPacket; // encrypted packet
			encodePacket(rawpkt, udppkt, flag, seq, ack, trgt_length - rawpkt->length);
			delete rawpkt;

			_client->send(udppkt);
			delete udppkt;

			_cv_send_q.notify_all();
		}
	}
}


void Packetizer::packetRecv() {
	uint32_t last_ack  = 0;
	size_t   count_ack = 0;

	while (_running) {
		UDPPacket * udppkt = _client->pop_recv_msg();
		//std::cerr << "Got a message of size " << udppkt->length << std::endl;
		if(_is_server &&  !_sending_started)
		{
			_sending_started = true;
			_cv_start_send.notify_all();
		}

		///*************************************************************************
		/// decrypt udp packet and build raw data trunk
		///*************************************************************************

		RawPacket * rawpkt = new RawPacket; // decrypted packet

		uint32_t    ack    = 0;
		uint32_t    seq    = 0;
		uint8_t     flag   = 0;

		if (!decodePacket(udppkt, rawpkt, &flag, &seq, &ack)) {
			delete udppkt;
			delete rawpkt;

			continue;
		}

		delete udppkt;

		if (flag & FG_ACK) {
			boost::unique_lock<boost::mutex> lock(_send_q_mutex);

			if (ack == last_ack) {
				++count_ack;
			}
			else {
				last_ack = ack;
				count_ack = 0;
			}

			if (count_ack >= 3) {
				_send_q.reset_seq(ack);
			}
			else {
				_send_q.update_ack(ack);
			}
		}


		if (rawpkt->type != MT_DUMMY) {
			boost::unique_lock<boost::mutex> lock(_recv_q_mutex);
			_recv_seq = _recv_q.put(rawpkt);
#ifdef DEBUG_PKTIZER
std::cerr << "-";
#endif
lock.unlock();
_cv_recv_q.notify_all();
		}
	}
}

void Packetizer::encodePacket(const RawPacket * rawpkt, UDPPacket * udppkt, 
		uint8_t flag, uint32_t seq, uint32_t ack,
		size_t padding) {
	boost::unique_lock<boost::mutex> lock(_codec_mutex);

	UDPPacket tmppkt;                   // tmp packet holding plain data
	udppkt->id = rawpkt->id;
	udppkt->length = rawpkt->length + UDP_HDR_LEN + padding;

	UDP_PKT_TYPE(&tmppkt) = rawpkt->type; // copy plain data
	UDP_PKT_FLAG(&tmppkt) = flag;
	UDP_PKT_LEN(&tmppkt)  = rawpkt->length;
	UDP_PKT_SEQ(&tmppkt)  = seq;
	UDP_PKT_ACK(&tmppkt)  = ack;

	if (rawpkt->type != MT_DUMMY) {
		memcpy(UDP_PKT_MSG(&tmppkt), rawpkt->data, rawpkt->length);
	}

	{
		_codec->encrypt(UDP_PKT_CIPHER(udppkt),
				UDP_PKT_CIPHER(&tmppkt),
				UDP_PKT_CIPHER_LEN(udppkt),
				UDP_PKT_IV(udppkt));

		_codec->updateMAC(UDP_PKT_AUTH_MSG(udppkt),
				UDP_PKT_AUTH_LEN(udppkt),
				UDP_PKT_MAC(udppkt));
	}
}

bool Packetizer::decodePacket(const UDPPacket * udppkt, RawPacket * rawpkt, 
		uint8_t *flag, uint32_t *seq, uint32_t *ack) {
	UDPPacket tmppkt;                   // tmp packet holding plain data
	boost::unique_lock<boost::mutex> lock(_codec_mutex);
//std::cerr << "Decoding a message of size " << udppkt->length << "and UDP_PKT_AUTH_LEN(udppkt) is" << UDP_PKT_AUTH_LEN(udppkt)<< std::endl;
	if (!_codec->verifyMAC(UDP_PKT_AUTH_MSG(udppkt),
			UDP_PKT_AUTH_LEN(udppkt),
			UDP_PKT_MAC(udppkt))) {
		std::cerr << "MAC failed for udppkt->id = " << udppkt->id << std::endl;
		return false;
	}

	_codec->decrypt(UDP_PKT_CIPHER(&tmppkt),
			UDP_PKT_CIPHER(udppkt),
			UDP_PKT_CIPHER_LEN(udppkt),
			UDP_PKT_IV(udppkt));

	*flag = UDP_PKT_FLAG(&tmppkt);
	*seq  = UDP_PKT_SEQ(&tmppkt);
	*ack  = UDP_PKT_ACK(&tmppkt);

	rawpkt->type   = UDP_PKT_TYPE(&tmppkt);

	if (rawpkt->type != MT_DUMMY) {
		rawpkt->id = udppkt->id;               // FIXME: do we need this?
		rawpkt->length = UDP_PKT_LEN(&tmppkt);
		rawpkt->seq_hi = *seq;
		rawpkt->seq_lo = *seq + 1 - UDP_PKT_LEN(&tmppkt);
		memcpy(rawpkt->data, UDP_PKT_MSG(&tmppkt), rawpkt->length);
	}
	return true;
}

void Packetizer::run() {
	_running = true;
	send_thread = boost::thread(&Packetizer::packetSend, this);
	recv_thread = boost::thread(&Packetizer::packetRecv, this);
	read_thread = boost::thread(&Packetizer::streamRead, this);
}

void Packetizer::join() {
	_running = false;

	send_thread.interrupt();
	send_thread.join();
	recv_thread.interrupt();
	recv_thread.join();
}


UDPPacket * Packetizer::genHello() {
	UDPPacket * pkt = new UDPPacket;

	memset(pkt, 0, sizeof(UDPPacket));

	pkt->id  = nextPacketID_++;
	uint8_t * mypublic = pkt->data;
	_codec->genPublicKey(mypublic);
	pkt->length = 32;


	return pkt;
}

bool Packetizer::verifyHello(UDPPacket * pkt) {
	if (!pkt) return false;

	if (pkt->length >= 32) {
		uint8_t * theirpublic = pkt->data;
		_codec->genSharedKey(theirpublic);

		return true;
	}
	return false;
}

UDPPacket * Packetizer::genGoodbye(uint32_t seq) {
	UDPPacket * pkt = new UDPPacket;
	UDPPacket tmppkt;
	pkt->id  = nextPacketID_++;
	pkt->length = (uint16_t)(UDP_HDR_LEN);
	UDP_PKT_TYPE(&tmppkt) = MT_GOODBYE;
	UDP_PKT_SEQ(&tmppkt)  = seq;
	UDP_PKT_ACK(&tmppkt)  = _recv_seq;
	UDP_PKT_LEN(&tmppkt)  = 0;

	{
		boost::unique_lock<boost::mutex> lock(_codec_mutex);

		_codec->encrypt(UDP_PKT_CIPHER(pkt),
				UDP_PKT_CIPHER(&tmppkt),
				UDP_PKT_CIPHER_LEN(pkt),
				UDP_PKT_IV(pkt));

		_codec->updateMAC(UDP_PKT_AUTH_MSG(pkt),
				UDP_PKT_AUTH_LEN(pkt),
				UDP_PKT_MAC(pkt));
	}
	return pkt;
}

