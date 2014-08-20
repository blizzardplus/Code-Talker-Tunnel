#include "UProxy.h"

using namespace boost::asio;

Proxy::Proxy(UDPRelay *relay, io_service &io_service, int listenPort, Morpher * morpher/*, SkypeClient * skc*/)
  : acceptor(io_service, ip::tcp::endpoint(ip::tcp::v4(), listenPort)), relay(relay), _morpher(morpher)/*, skypeclient(skc)*/
{
  acceptIncomingConnection();
}

void Proxy::acceptIncomingConnection() {
  boost::shared_ptr<ip::tcp::socket> socket(new ip::tcp::socket(acceptor.get_io_service()));
  acceptor.async_accept(*socket, boost::bind(&Proxy::handleIncomingConnection,
					     this, socket, placeholders::error));
}


void Proxy::handleIncomingConnection(boost::shared_ptr<ip::tcp::socket> socket,
					const boost::system::error_code &err) 
{
  if (err) {
    std::cerr << "[proxy] " << "Error accepting incoming connection: " << err << std::endl;
    acceptIncomingConnection();
    return;
  }

  std::cerr << "[proxy] " << "Got SOCKS Connection..." << std::endl;

  boost::shared_ptr<SocksStream> connection(new SocksStream(socket));
  connection->getRequest(boost::bind(&Proxy::handleSocksRequest, this,
				     connection, _1, _2, _3));

  acceptIncomingConnection();
}

void Proxy::handleSocksRequest(boost::shared_ptr<SocksStream> connection,
				  std::string &host,
				  uint16_t port,
				  const boost::system::error_code &err)
{
  std::cerr << "[proxy] " << "Got SOCKS Request: " << host << ":" << port << std::endl;

  if (err) {
    connection->close();
  }
  else     {
    boost::shared_ptr<Codec> codec(new Codec(false)); 
    //skypeclient->runOnce(&host, &port, codec);

    std::cerr << "[proxy] " << "Skype server at " << host << ":" << port << std::endl; 

    relay->openStream(boost::bind(&Proxy::handleStreamOpen,
                                  this, connection, _1, _2), 
                      codec, _morpher);
  }
}

void Proxy::handleStreamOpen(boost::shared_ptr<SocksStream> socks,
                             boost::shared_ptr<Packetizer> stream,
                             const boost::system::error_code &err)
{
  if (err) {
    std::cerr << "[proxy] " << "Error opening stream: " << err << std::endl;
    socks->respondConnectError();
    socks->close();
    return;
  }
  std::cerr << "[proxy] " << "Successfully opened relay..." << std::endl;

  socks->respondConnected();

  boost::shared_ptr<ProxyShuffler> proxyShuffler(new ProxyShuffler(socks, stream));
  //stream->run();
  proxyShuffler->shuffle();
}

