#include "Relay.h"
#include "Util.h"
#include "debug.h"

#include <iostream>
#include <boost/lexical_cast.hpp>

using namespace boost::asio; 

Relay::Relay(boost::asio::io_service &io_service,
		     RelayErrorHandler errorHandler) 
  : io_service(io_service), errorHandler(errorHandler)
{ }


void Relay::close() {
  DEBUG_FUNC;
}

void Relay::connect(RelayConnectHandler handler) {
  DEBUG_FUNC;
}


void Relay::openStream(std::string &host, uint16_t port, RelayStreamHandler handler) {
  uint16_t streamId  = Util::getRandomId();
  boost::shared_ptr<ip::tcp::socket> socket(new ip::tcp::socket(io_service));
  ip::tcp::resolver::query query(host, boost::lexical_cast<std::string>(port));
  ip::tcp::resolver resolver(io_service);
  ip::tcp::resolver::iterator dest = resolver.resolve(query);
  ip::tcp::resolver::iterator end ;
  ip::tcp::endpoint endpoint;

  boost::system::error_code error = error::host_not_found;
  while (error && dest != end)
  {
    socket->close();
    socket->connect(*dest++, error);
  }
  if (error) 
      throw boost::system::system_error(error);

  boost::shared_ptr<RelayStream> stream;
  if (error) {
    handler(stream, error);
    return;
  }

  stream = boost::shared_ptr<RelayStream>(new RelayStream(socket, streamId)); 

  handler(stream, error);
}


RelayStream * Relay::openStream(std::string &host, uint16_t port) {
  uint16_t streamId  = Util::getRandomId();
  boost::shared_ptr<ip::tcp::socket> socket(new ip::tcp::socket(io_service));
  ip::tcp::resolver::query query(host, boost::lexical_cast<std::string>(port));
  ip::tcp::resolver resolver(io_service);
  ip::tcp::resolver::iterator dest = resolver.resolve(query);
  ip::tcp::resolver::iterator end ;
  ip::tcp::endpoint endpoint;

  boost::system::error_code error = error::host_not_found;
  while (error && dest != end)
  {
    socket->close();
    socket->connect(*dest++, error);
  }
  if (error) 
  {
	  std::cerr <<"[Relay]"<<" Cannot bind to"<< host <<":" <<port <<"! Please make sure Tor Bridge is up and running." << std::endl;
      throw boost::system::system_error(error);
  }

  RelayStream * stream = new RelayStream(socket, streamId); 
  
  return stream; 
}


void Relay::handleConnectionError(const boost::system::error_code &err) {
  std::cerr << "Error with connection to Exit Node: " << err << std::endl;
  errorHandler(err);
}
