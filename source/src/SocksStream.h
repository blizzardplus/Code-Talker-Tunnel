#ifndef SOCKSSTREAM_H
#define SOCKSSTREAM_H

/* 
 * This header defines the incoming Socks4/5 connection
 */

#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <string>


#include "TunnelStream.h"
#include "Packet.h"

using namespace boost::asio;

typedef boost::function<void (std::string &host, 
                              uint16_t port, 
                              const boost::system::error_code &error)> 
              SocksRequestHandler;

class SocksStream : public TunnelStream, public boost::enable_shared_from_this<SocksStream> {

 private:
  boost::shared_ptr<ip::tcp::socket> socket;

  /* 
   * Format of SOCKS4A client request
   *
   *				+----+----+----+----+----+----+----+----+----+----+....+----+
   *				| VN | CD | DSTPORT |      DSTIP        | USERID       |NULL|
   *				+----+----+----+----+----+----+----+----+----+----+....+----+
   * # of bytes:	   1    1      2              4           variable       1
   *
   *
   * Format of SOCKS5 version identifier/method selection message:
   *
   *               +----+----------+----------+
   *               |VER | NMETHODS | METHODS  |
   *               +----+----------+----------+
   *               | 1  |    1     | 1 to 255 |
   *               +----+----------+----------+
   *
   * Format of SOCKS5 client request
   *
   *    +----+-----+-------+------+----------+----------+
   *    |VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
   *    +----+-----+-------+------+----------+----------+
   *    | 1  |  1  | X'00' |  1   | Variable |    2     |
   *    +----+-----+-------+------+----------+----------+
   *
   * Where:
   *
   *      o  VER    protocol version: X'05'
   *      o  CMD
   *         o  CONNECT X'01'
   *         o  BIND X'02'
   *         o  UDP ASSOCIATE X'03'
   *      o  RSV    RESERVED
   *      o  ATYP   address type of following address
   *         o  IP V4 address: X'01'
   *         o  DOMAINNAME: X'03'
   *         o  IP V6 address: X'04'
   *      o  DST.ADDR       desired destination address
   *      o  DST.PORT desired destination port in network octet
   *         order
   *
   */

  static const int VERSION_HEADER_LEN        = 1;
  static const int SOCKS4_REQUEST_HEADER_LEN = 7;
  static const int SOCKS5_METHOD_LEN         = 1;
  static const int SOCKS5_M_RESP_LEN         = 2;
  static const int SOCKS5_REQUEST_HEADER_LEN = 4;
  static const int DOMAIN_ADDRESS_HEADER_LEN = 1;
  static const int IPV4_ADDRESS_LEN          = 6;

  /* shared between SOCKS4 and SOCKS5 */
  unsigned char version;
  unsigned char command;

  /* SOCKS4 fields */
  unsigned char dstport[2];
  unsigned char dstip[4]; 

  /* SOCKS5 fields */
  unsigned char methodCount;
  unsigned char addressType;
  unsigned char domainAddressLength;
  
  std::string host;
  uint16_t port;
  unsigned char data[MAX_RAW_MSG_LEN];

  void readVersionHeaderComplete(SocksRequestHandler handler, 
				 const boost::system::error_code &err, 
				 std::size_t transferred);

  /* Socks4 request procedures */

  void getSocks4ConnectionRequest(SocksRequestHandler handler);

  void readSocks4RequestComplete(SocksRequestHandler handler,
			   const boost::system::error_code &err,
			   std::size_t transferred);

  void readUseridComplete(const boost::system::error_code &err, 
                          std::size_t transferred); 


  /* Socks5 request procedures */

  void getSocks5ConnectionRequest(SocksRequestHandler handler);

  void readSocks5RequestComplete(SocksRequestHandler handler,
			   const boost::system::error_code &err,
			   std::size_t transferred);

  void readSocks5NMethodComplete(SocksRequestHandler handler, 
				 const boost::system::error_code &err, 
				 std::size_t transferred);

  void readSocks5MethodComplete(SocksRequestHandler handler, 
			  const boost::system::error_code &err, 
			  std::size_t transferred);

  void sendSocks5ValidMethodResponse(SocksRequestHandler handler);

  void socks5MethodResponseComplete(SocksRequestHandler handler, 
			      const boost::system::error_code &err,
			      std::size_t transferred);

  void readDomainAddressComplete(SocksRequestHandler handler,
				 const boost::system::error_code &err,
				 std::size_t transferred);

  void readDomainAddressHeaderComplete(SocksRequestHandler handler,
				       const boost::system::error_code &err,
				       std::size_t transferred);

  void readDomainAddress(SocksRequestHandler handler);
  void readIPv4Address(SocksRequestHandler handler);

  void readIPv4AddressComplete(SocksRequestHandler handler,
			       const boost::system::error_code &err,
			       std::size_t transferred);

  void read(StreamReadHandler handler);
  void dataReadComplete(StreamReadHandler handler, 
			std::size_t transferred,
			const boost::system::error_code &err);

  void write(unsigned char* buf, int length, StreamWriteHandler handler);
  void respondConnectErrorComplete(const boost::system::error_code &err);
  void respondConnectedComplete(const boost::system::error_code &err);


 public:
  SocksStream(boost::shared_ptr<ip::tcp::socket> socket);
  void getRequest(SocksRequestHandler handler);
  void respondConnectError();
  void respondConnected();
  void close(bool force = false);
};

#endif


