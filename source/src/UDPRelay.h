#ifndef UDPRELAY_H
#define UDPRELAY_H

/* 
 * This header defines a wrapper for the Packetizer class
 */


#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <string>

#include "TunnelStream.h"
#include "Packetizer.h"
#include "BasicClient.h"
#include "ProxyShuffler.h" 
#include "Packet.h"
#include "Morpher.h"

#ifdef WIN32
#include <windows.h>
#endif

using namespace boost::asio;

typedef boost::function<void (const boost::system::error_code &error)> UDPRelayConnectHandler;
typedef boost::function<void (boost::shared_ptr<Packetizer> stream, const boost::system::error_code &error)> UDPRelayStreamHandler;
typedef boost::function<void (const boost::system::error_code &error)> UDPRelayErrorHandler;

class UDPRelay { 
public:
	boost::asio::io_service &io_service;
	uint16_t _local_port;
	boost::shared_ptr<ip::udp::socket> _udpsocket;
	boost::shared_ptr<BasicClient> _client;
	boost::shared_ptr<Packetizer> _pkzer;
	ip::udp::endpoint _sender_ep;
	ip::udp::endpoint _dest;

	unsigned char _buf[MAX_UDP_MSG_LEN];
	UDPRelayErrorHandler errorHandler;
public:
	UDPRelay(boost::asio::io_service &io_service, uint16_t local_port,
			UDPRelayErrorHandler errorHandler)
	: io_service(io_service), _local_port(local_port), errorHandler(errorHandler)
	{ }

	bool connect();
	void openStream(UDPRelayStreamHandler handler,
			boost::shared_ptr<Codec> & codec, Morpher * morpher);

	void openStreamWOSocks(std::string &host, uint16_t port,
			boost::shared_ptr<Codec> & codec, Morpher * morpher);



	void close();

	void recvUDPData();

	void handleUDPData(const boost::system::error_code &err,
			std::size_t bytes_transferred);

	void handleConnectionError(const boost::system::error_code &err);
};



#endif



