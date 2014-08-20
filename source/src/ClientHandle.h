/*
 * ClientHandle.h
 *
 */

#ifndef CLIENTHANDLE_H
#define CLIENTHANDLE_H

#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <map>
#include <cassert>

#include "Packet.h"
#include "Packetizer.h"
#include "BasicClient.h"
#include "Morpher.h"
#include "Crypto.h"
#include "Relay.h"
#include "RelayStream.h"
#include "ProxyShuffler.h"
#include "Util.h"
#include "debug.h"

class ClientHandle {
private:
	boost::shared_ptr<Packetizer> _pkzer;
	boost::shared_ptr<ProxyShuffler> _prxyshflr;
	boost::shared_ptr<Codec> _codec;
	boost::asio::io_service io_service;
public:
	boost::shared_ptr<BasicClient> _basicClient;
	ClientHandle(boost::shared_ptr<ip::udp::socket> _udpsocket, ip::udp::endpoint ep, boost::shared_ptr<Codec> & codec, Morpher * _morpher, std::string _tor_addr, uint16_t _tor_port);
	bool run();
	bool close();
};

#endif /* CLIENTHANDLE_H_ */
