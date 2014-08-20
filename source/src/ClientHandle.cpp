/*
 * ClientHandle.cpp
 *
 */

#include "ClientHandle.h"


void relayError(const boost::system::error_code &err) {
	std::cerr << "[server] " << "Error with relay, exiting..." << std::endl;
	exit(0);
};

ClientHandle::ClientHandle(boost::shared_ptr<ip::udp::socket> _udpsocket, ip::udp::endpoint ep, boost::shared_ptr<Codec> & codec, Morpher * _morpher, std::string _tor_addr, uint16_t _tor_port) :
		_basicClient (new BasicClient(_udpsocket, ep)),
		_codec (codec){
	boost::shared_ptr<Packetizer> pkzer (new Packetizer(_basicClient, codec, _morpher, true));
	_pkzer = pkzer;
	boost::shared_ptr<Relay> relay(new Relay(io_service,
			boost::bind(relayError,
					placeholders::error)));

	try {
		boost::shared_ptr<RelayStream> rstream(relay->openStream(_tor_addr, _tor_port));
		boost::asio::io_service::work * work = new boost::asio::io_service::work(io_service);
		boost::shared_ptr<ProxyShuffler> prxyshflr (new ProxyShuffler(rstream, _pkzer, work));
		_prxyshflr = prxyshflr;
	}
	catch (...) {
		std::cerr << "[server] " << " unexpected error. exit now" << std::endl;
	}

};


bool ClientHandle::run()
{
	_pkzer->run();
	_prxyshflr->shuffle();
	io_service.run();
}
