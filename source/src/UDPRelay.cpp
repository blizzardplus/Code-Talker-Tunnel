#include "UDPRelay.h"
#include "Crypto.h"
#include "Util.h"
#include "debug.h"
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <cassert>

using namespace boost::asio; 

void UDPRelay::close() {
	DEBUG_FUNC;
}

bool UDPRelay::connect() {
	UDPPacket * pkt = _pkzer->genHello();
	_udpsocket->send_to(buffer(pkt->data, pkt->length), _dest);
	delete pkt;

	UDPPacket recv_pkt;
	while (true) {
		size_t len = _udpsocket->receive_from(buffer(recv_pkt.data,
				sizeof(recv_pkt.data)),
				_sender_ep);

		if (_sender_ep == _dest) {
			recv_pkt.length = len;
			break;
		}
	}

	if (_pkzer->verifyHello(&recv_pkt)) {
		std::cerr << "UDP connection established with " << _dest << std::endl;
		return true;
	}

	std::cerr << "verify hello failed with " << _dest << std::endl;
	return false;
}

void UDPRelay::openStream(UDPRelayStreamHandler handler,
		boost::shared_ptr<Codec> & codec, Morpher * morpher) {


	boost::system::error_code error;

	handler(_pkzer, error);
}

void UDPRelay::openStreamWOSocks(std::string &host, uint16_t port,boost::shared_ptr<Codec> & codec, Morpher * morpher) {


	bool connected = false;
	while(!connected)
	{
		try
		{
			_udpsocket = boost::shared_ptr<ip::udp::socket>(new ip::udp::socket(io_service,
						ip::udp::endpoint(ip::udp::v4(), _local_port)));
			connected=true;
		}
		catch(boost::exception & e)
		{
			std::cerr << "[server] " << "Port already busy, trying again!\n";
#ifdef WIN32
			Sleep(1000);
#else
			sleep(1);
#endif
		}
	}

	std::cerr << "[server] " << "Successfully connected on local port " << _local_port << "!\n";
	_local_port = _udpsocket->local_endpoint().port();

	ip::udp::resolver::query query(ip::udp::v4(), host, boost::lexical_cast<std::string>(port));
	ip::udp::resolver resolver(io_service);
	ip::udp::resolver::iterator dest_itor = resolver.resolve(query);

	_dest = *dest_itor;

	_client = boost::shared_ptr<BasicClient>(new BasicClient(_udpsocket, _dest));
	_pkzer  = boost::shared_ptr<Packetizer>(new Packetizer(_client, codec, morpher, false));

	boost::system::error_code error;
	// while (!connect()) ;          // FIXME: set an upper bound on # of looping ?
	recvUDPData();
}



void UDPRelay::recvUDPData() {
	_udpsocket->async_receive_from(buffer(_buf, sizeof(_buf)),
			_sender_ep,
			boost::bind(&UDPRelay::handleUDPData,
					this, placeholders::error,
					placeholders::bytes_transferred));
}

void UDPRelay::handleUDPData(const boost::system::error_code &err, 
		std::size_t bytes_transferred)
{
	if (err) {
		std::cerr << "Error accepting incoming connection: " << err << std::endl;
		recvUDPData();
		return;
	}

	if (_sender_ep != _dest) {
		std::cerr << "ignore UDP data from " << _sender_ep << std::endl;
	}
	else {
#ifdef DEBUG_UDP_DISPATCH
		std::cerr << "Got UDP " << bytes_transferred << " bytes "
				<< "from " << _sender_ep << std::endl;
#endif
		_client->push_recv_msg(_buf, bytes_transferred);
	}

	recvUDPData();
}

void UDPRelay::handleConnectionError(const boost::system::error_code &err) {
	std::cerr << "Error with connection to Exit Node: " << err << std::endl;
	errorHandler(err);
}
