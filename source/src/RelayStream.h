#ifndef RELAYSTREAM_H
#define RELAYSTREAM_H

/* 
 * This header defines the outgoing connection stream
 */

#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <cassert>
#include <iostream>
#include <string>

#include "TunnelStream.h"
#include "Packet.h"

using namespace boost::asio;

typedef boost::function<void (std::string &host, 
                              uint16_t port, 
                              const boost::system::error_code &error)> 
              SocksRequestHandler;

class RelayStream : public TunnelStream, public boost::enable_shared_from_this<RelayStream> {

 private:
  boost::shared_ptr<ip::tcp::socket> socket;
  uint16_t streamId;
  unsigned char data[MAX_RAW_MSG_LEN];

 public:
  RelayStream(boost::shared_ptr<ip::tcp::socket> s, uint16_t id) 
    : socket(s), streamId(id) 
  { }

  void read(StreamReadHandler handler);

  void dataReadComplete(StreamReadHandler handler, 
			std::size_t transferred,
			const boost::system::error_code &err);

  void write(unsigned char* buf, int length, StreamWriteHandler handler);

  void close(bool force = false);

  ip::tcp::endpoint getLocalEndpoint() {
    return socket->local_endpoint(); 
  }
};

#endif

