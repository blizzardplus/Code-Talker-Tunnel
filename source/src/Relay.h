#ifndef RELAY_H
#define RELAY_H

/* 
 * This header defines a wrapper for the RelayStream class
 */

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <cassert>
#include <iostream>
#include <string>

#include "TunnelStream.h"
#include "RelayStream.h"

using namespace boost::asio;

typedef boost::function<void (const boost::system::error_code &error)> RelayConnectHandler;
typedef boost::function<void (boost::shared_ptr<RelayStream> stream, const boost::system::error_code &error)> RelayStreamHandler;
typedef boost::function<void (const boost::system::error_code &error)> RelayErrorHandler;

class Relay { 
 private:
  boost::asio::io_service &io_service;
  RelayErrorHandler errorHandler;

 public:
  Relay(boost::asio::io_service &io_service, 
        RelayErrorHandler errorHandler);

  void close();
  void connect(RelayConnectHandler handler);
  void openStream(std::string &host, uint16_t port, RelayStreamHandler handler);
  RelayStream * openStream(std::string &host, uint16_t port);

  void handleConnectionError(const boost::system::error_code &err);
};

#endif



