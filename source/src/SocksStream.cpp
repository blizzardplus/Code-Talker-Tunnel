#include "SocksStream.h"
#include "Util.h"
#include "debug.h"

#include <boost/bind.hpp>
#include <cassert>
#include <iostream>

using namespace boost::asio;

SocksStream::SocksStream(boost::shared_ptr<ip::tcp::socket> socket) 
  : socket(socket)
{}
				 
void SocksStream::getRequest(SocksRequestHandler handler) {
  boost::asio::async_read(*socket, buffer(data, VERSION_HEADER_LEN),
			  boost::bind(&SocksStream::readVersionHeaderComplete,
				      shared_from_this(), handler, 
				      placeholders::error,
				      placeholders::bytes_transferred));
}

void SocksStream::readVersionHeaderComplete(SocksRequestHandler handler, 
						const boost::system::error_code &err, 
						std::size_t transferred)
{
  if (err) {
    handler(host, port, err);
    return;
  }

  assert(transferred == VERSION_HEADER_LEN);

  version     = data[0];

  std::cerr << " version " << (int)version << std::endl; 

  if (version == 0x4) {
    getSocks4ConnectionRequest(handler);
    return;
  }
  
  if (version == 0x05) {
    boost::asio::async_read(*socket, buffer(data, SOCKS5_METHOD_LEN),
                            boost::bind(&SocksStream::readSocks5NMethodComplete, 
                                        shared_from_this(), handler, 
                                        placeholders::error,
                                        placeholders::bytes_transferred));
    return;
  }


  std::cerr << " socks version unknown" << std::endl;
  handler(host, port, boost::asio::error::access_denied);
}


void SocksStream::getSocks4ConnectionRequest(SocksRequestHandler handler) {
  async_read(*socket, boost::asio::buffer(data, SOCKS4_REQUEST_HEADER_LEN),
	     boost::bind(&SocksStream::readSocks4RequestComplete, shared_from_this(),
			 handler, placeholders::error, placeholders::bytes_transferred));
}

void SocksStream::readSocks4RequestComplete(SocksRequestHandler handler,
					  const boost::system::error_code &err,
					  std::size_t transferred) 
{
  if (err) {
    handler(host, port, err);
    return;
  }
  
  assert(transferred == SOCKS4_REQUEST_HEADER_LEN);

  command     = data[0];
  std::cerr << " command "; 
  if (command == 0x01) std::cerr << "CONNECT";
  else if (command == 0x02) std::cerr << "BIND"; 
  else std::cerr << "unknown " << (int)command; 
  std::cerr << std::endl;

  if (command != 0x01) {
    handler(host, port, boost::asio::error::access_denied);
    return;
  }  

  dstport[0]  = data[1];
  dstport[1]  = data[2];
  dstip[0]    = data[3];
  dstip[1]    = data[4];
  dstip[2]    = data[5];
  dstip[3]    = data[6]; 

  std::cerr << " dstport " << Util::bigEndianArrayToShort(dstport) << std::endl; 

  std::cerr << " dstip   " 
            << (int)dstip[0] << "." << (int)dstip[1] << "." 
            << (int)dstip[2] << "." << (int)dstip[3] << std::endl; 

  boost::asio::streambuf b;
  read_until(*socket, b, (char)0x0);

  if (dstip[0] == 0x0 && dstip[1] == 0x0 && dstip[2] == 0x0 && dstip[3] != 0x0) {
    std::cerr << "SOCKS4A request" << std::endl;
    handler(host, port, boost::asio::error::access_denied);
  } 
  else {
    std::cerr << "SOCKS4 request" << std::endl;

    boost::array<unsigned char, 4> hostBytes = {dstip[0], dstip[1], dstip[2], dstip[3]};
    host                                     = ip::address_v4(hostBytes).to_string();
    port                                     = Util::bigEndianArrayToShort(dstport);
    handler(host, port, err);
  }
}

void SocksStream::readUseridComplete(const boost::system::error_code &err, 
                                     std::size_t transferred) { }

void SocksStream::readSocks5NMethodComplete(SocksRequestHandler handler, 
						const boost::system::error_code &err, 
						std::size_t transferred)
{
  if (err) {
    handler(host, port, err);
    return;
  }

  assert(transferred == SOCKS5_METHOD_LEN);

  methodCount = data[0];

  assert(methodCount < sizeof(data));

  boost::asio::async_read(*socket, buffer(data, methodCount),
			  boost::bind(&SocksStream::readSocks5MethodComplete,
				      shared_from_this(), handler, 
				      placeholders::error,
				      placeholders::bytes_transferred));  
}

void SocksStream::readSocks5MethodComplete(SocksRequestHandler handler, 
					 const boost::system::error_code &err, 
					 std::size_t transferred)
{
  if (err) {
    handler(host, port, err);
    return;
  }
  
  assert(transferred == methodCount);

  int i;
  for (i=0;i<methodCount;i++) {
    if (data[i] == 0x00) {
      sendSocks5ValidMethodResponse(handler);
      return;
    }
  }

  handler(host, port, boost::asio::error::access_denied);  
}


void SocksStream::sendSocks5ValidMethodResponse(SocksRequestHandler handler) {
  data[0] = 0x05;
  data[1] = 0x00;

  async_write(*socket, boost::asio::buffer(data, SOCKS5_M_RESP_LEN),
	      boost::bind(&SocksStream::socks5MethodResponseComplete, shared_from_this(),
			  handler, placeholders::error, placeholders::bytes_transferred));
}


void SocksStream::socks5MethodResponseComplete(SocksRequestHandler handler, 
					     const boost::system::error_code &err,
					     std::size_t transferred) 
{
  if (err) handler(host, port, err);
  else     getSocks5ConnectionRequest(handler);
}




void SocksStream::getSocks5ConnectionRequest(SocksRequestHandler handler) {
  async_read(*socket, boost::asio::buffer(data, SOCKS5_REQUEST_HEADER_LEN),
	     boost::bind(&SocksStream::readSocks5RequestComplete, shared_from_this(),
			 handler, placeholders::error, placeholders::bytes_transferred));
}

void SocksStream::readSocks5RequestComplete(SocksRequestHandler handler,
					  const boost::system::error_code &err,
					  std::size_t transferred) 
{
  if (err) {
    handler(host, port, err);
    return;
  }
  
  assert(transferred == SOCKS5_REQUEST_HEADER_LEN);
  assert(data[0] == 0x05);
  assert(data[2] == 0x00);

  command     = data[1];
  addressType = data[3];

  std::cerr << " command "; 
  if (command == 0x01) std::cerr << "CONNECT";
  else if (command == 0x02) std::cerr << "BIND"; 
  else std::cerr << "unknown " << (int)command; 
  std::cerr << std::endl;
  
  if (command != 0x01) {
    handler(host, port, boost::asio::error::access_denied);
    return;
  }  

  if      (addressType == 0x01) readIPv4Address(handler);
  else if (addressType == 0x03) readDomainAddress(handler);
  else                          handler(host, port, boost::asio::error::access_denied);  
}


void SocksStream::readDomainAddressComplete(SocksRequestHandler handler,
						const boost::system::error_code &err,
						std::size_t transferred)
{
  if (err) {
    handler(host, port, err);
    return;
  }

  assert(transferred == domainAddressLength + 2);
  host = std::string((const char*)data, (std::size_t)domainAddressLength);
  port = Util::bigEndianArrayToShort(data+domainAddressLength);
  handler(host, port, err);
}

void SocksStream::readDomainAddressHeaderComplete(SocksRequestHandler handler,
						      const boost::system::error_code &err,
						      std::size_t transferred)
{
  if (err) {
    handler(host, port, err);
    return;
  }

  assert(transferred == DOMAIN_ADDRESS_HEADER_LEN);

  domainAddressLength = data[0];

  async_read(*socket, buffer(data, domainAddressLength + 2),
	     boost::bind(&SocksStream::readDomainAddressComplete, shared_from_this(),
			 handler, placeholders::error, placeholders::bytes_transferred));
}

void SocksStream::readDomainAddress(SocksRequestHandler handler) {

  async_read(*socket, buffer(data, DOMAIN_ADDRESS_HEADER_LEN), 
	     boost::bind(&SocksStream::readDomainAddressHeaderComplete, shared_from_this(),
			 handler, placeholders::error, placeholders::bytes_transferred));
}

void SocksStream::readIPv4Address(SocksRequestHandler handler) {
  async_read(*socket, buffer(data, IPV4_ADDRESS_LEN), 
	     boost::bind(&SocksStream::readIPv4AddressComplete,
			 shared_from_this(), handler, 
			 placeholders::error, 
			 placeholders::bytes_transferred));
}
				      
void SocksStream::readIPv4AddressComplete(SocksRequestHandler handler,
					      const boost::system::error_code &err,
					      std::size_t transferred) 
{
  if (err) {
    handler(host, port, err);
    return;
  }

  assert(transferred == IPV4_ADDRESS_LEN);

  boost::array<unsigned char, 4> hostBytes = {data[0], data[1], data[2], data[3]};
  host                                     = ip::address_v4(hostBytes).to_string();
  port                                     = Util::bigEndianArrayToShort(data+4);

  handler(host, port, err);
}

void SocksStream::read(StreamReadHandler handler) {
  socket->async_read_some(buffer(data, sizeof(data)), 
			  boost::bind(&SocksStream::dataReadComplete, shared_from_this(), 
				      handler, 
				      placeholders::bytes_transferred, 
				      placeholders::error));
}

void SocksStream::dataReadComplete(StreamReadHandler handler, 
				       std::size_t transferred,
				       const boost::system::error_code &err)
{
  if (err) handler(data, -1);
  else     handler(data, transferred);
}

void SocksStream::write(unsigned char* buf, int length, StreamWriteHandler handler)
{
  if (length <= 0) return;
  async_write(*socket, buffer(buf, length), boost::bind(handler, placeholders::error));
}

void SocksStream::close(bool force) {
  DEBUG_FUNC;

  socket->close();
}

void SocksStream::respondConnectedComplete(const boost::system::error_code &err) {}

// Reply format of SOCKS4
// 				+----+----+----+----+----+----+----+----+
// 				| VN | CD | DSTPORT |      DSTIP        |
// 				+----+----+----+----+----+----+----+----+
//  # of bytes:	   1    1      2              4
// 
// VN is the version of the reply code and should be 0. CD is the result
// code with one of the following values:
// 
// 	90: request granted
// 	91: request rejected or failed
// 	92: request rejected becasue SOCKS server cannot connect to
// 	    identd on the client
// 	93: request rejected because the client program and identd
// 	    report different user-ids
// 
// Reply format of SOCKS5
//        +----+-----+-------+------+----------+----------+
//        |VER | REP |  RSV  | ATYP | BND.ADDR | BND.PORT |
//        +----+-----+-------+------+----------+----------+
//        | 1  |  1  | X'00' |  1   | Variable |    2     |
//        +----+-----+-------+------+----------+----------+
//
//     Where:
//
//          o  VER    protocol version: X'05'
//          o  REP    Reply field:
//             o  X'00' succeeded
//             o  X'01' general SOCKS server failure
//             o  X'02' connection not allowed by ruleset
//             o  X'03' Network unreachable
//             o  X'04' Host unreachable
//             o  X'05' Connection refused
//             o  X'06' TTL expired
//             o  X'07' Command not supported
//             o  X'08' Address type not supported
//             o  X'09' to X'FF' unassigned
//          o  RSV    RESERVED
//          o  ATYP   address type of following address
//
// 
void SocksStream::respondConnected() {
  ip::tcp::endpoint local = socket->local_endpoint(); 
  if (version == 0x4) {
    data[0] = 0x00;
    data[1] = 90;

    Util::int16ToArrayBigEndian(data+2, port);

    boost::array< unsigned char, 4 > localAddress = local.address().to_v4().to_bytes();
    data[4] = localAddress[0];
    data[5] = localAddress[1];
    data[6] = localAddress[2];
    data[7] = localAddress[3];

    async_write(*socket, buffer(data, 8), 
                boost::bind(&SocksStream::respondConnectedComplete,
                            shared_from_this(), placeholders::error));
  } 
  else if (version == 0x5) {
    data[0] = 0x05;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x01;

    boost::array< unsigned char, 4 > localAddress = local.address().to_v4().to_bytes();
    data[4] = localAddress[0];
    data[5] = localAddress[1];
    data[6] = localAddress[2];
    data[7] = localAddress[3];

    Util::int16ToArrayBigEndian(data+8, local.port());

    async_write(*socket, buffer(data, 10), 
                boost::bind(&SocksStream::respondConnectedComplete,
                            shared_from_this(), placeholders::error));
  }
}

void SocksStream::respondConnectErrorComplete(const boost::system::error_code &err) {}


void SocksStream::respondConnectError() {
  if (version == 0x4) {
    data[0] = 0x00;
    data[1] = 91;

    async_write(*socket, buffer(data, 2), 
                boost::bind(&SocksStream::respondConnectErrorComplete,
                            shared_from_this(), placeholders::error));
  }
  else if (version == 0x5) {
    data[0] = 0x05;
    data[1] = 0x04;
    data[2] = 0x00;
    data[3] = 0x01;
    data[4] = 0x7F;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x01;
    data[8] = 0x04;
    data[9] = 0x00;

    async_write(*socket, buffer(data, 10), 
                boost::bind(&SocksStream::respondConnectErrorComplete,
                            shared_from_this(), placeholders::error));
  }
}

