#ifndef SERVER_H
#define SERVER_H

/* 
 * This header defines the socks proxy wrapper class
 */

#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <map>
#include <cassert>

#include "Packet.h"
#include "Packetizer.h"
#include "BasicClient.h"
#include "ClientHandle.h"
#include "Morpher.h"
#include "Crypto.h"
#include "Relay.h"
#include "RelayStream.h"
#include "ProxyShuffler.h"
#include "Util.h"
#include "debug.h"


using namespace boost::asio;





class Server {
private:
	mutable boost::mutex mx;
	boost::condition_variable cv;
	boost::asio::io_service              _io_service;
	std::map<ip::udp::endpoint,	boost::shared_ptr<ClientHandle> > _clients;
	//std::map<boost::shared_ptr<BasicClient>,	boost::shared_ptr<Packetizer> > _client_pkzer_map;
	//std::map<boost::shared_ptr<BasicClient>,	boost::shared_ptr<ProxyShuffler> > _client_prxyshflr_map;
	boost::shared_ptr<ip::udp::socket>   _udpsocket;
	ip::udp::endpoint                    _sender_ep;

	std::string 						 _tor_addr;
	uint16_t                             _tor_port;
	int									 _listen_port;
	int 								 _skype_port;
#ifndef TPROXY_ENABLED
	std::string  						 client_addr;
	uint16_t 							 client_port;
	boost::shared_ptr<Codec>			 client_codec;
#endif
	Morpher *                            _morpher;
	boost::thread                        _thread;
	unsigned char                        _buf[MAX_UDP_MSG_LEN];

	void serverSession();

	void recvUDPData();
	void handleUDPData(const boost::system::error_code &err,
			std::size_t bytes_transferred);
	bool verifyConnectionRequest(boost::shared_ptr<BasicClient> & c,
			boost::shared_ptr<Packetizer> & pkzer);
	void clientSession(boost::shared_ptr<ClientHandle> & cHandle, boost::shared_ptr<Codec> & codec);
	void endClientSession(boost::shared_ptr<ClientHandle> & c);

public:

	Server(uint16_t skypePort, uint16_t torPort, std::string tor_addr, Morpher * morpher);
	boost::thread & run();
	void stop();
#ifndef TPROXY_ENABLED
	void wakeUp();
#endif
	void newClientSession(const std::string & addr, uint16_t port, boost::shared_ptr<Codec> & codec);
};



#endif
