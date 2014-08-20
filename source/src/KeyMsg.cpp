#include "KeyMsg.h"
#include "Util.h"
#include <cstdlib>
#include <cstring>
#include <string>

#include <iostream>

#include <boost/asio.hpp>
#ifdef WIN32
#else
#include <arpa/inet.h>
#endif

KeyMsg::KeyMsg(const std::string & addrstr, uint16_t p, const uint8_t * k, uint8_t t) {
  memset(data, 0, sizeof(data)); 
  memset(seq, 0, sizeof(seq));
  boost::system::error_code ec;
  long unsigned int* tmp;
  boost::asio::detail::socket_ops::inet_pton(AF_INET, addrstr.c_str(), msg.addr, tmp, ec);
//  inet_pton(AF_INET, addrstr.c_str(), msg.addr);

  msg.port = p; 
  msg.type = t; 

  memcpy(msg.key, k, 32); 

  unsigned int len = sizeof(seq) - 1; 
  Util::Base64_Encode(data, KEY_MSG_PLAIN_LEN, seq, &len);
}

KeyMsg::KeyMsg(const char * s, int sz) {
  memset(data, 0, sizeof(data));
  memset(seq, 0, sizeof(seq));
  memcpy(seq, s, sz); 
  seq[sz++] = '\n'; 

  unsigned int len = sizeof(data) - 1; 
  Util::Base64_Decode(seq, sz, data, &len);
}

KeyMsg::operator const char * () const 
{
  return reinterpret_cast<const char *>(seq);
}

std::string KeyMsg::getAddr() const {
  char addrstr[INET_ADDRSTRLEN];
  boost::system::error_code ec;
  boost::asio::detail::socket_ops::inet_ntop(AF_INET, (char*)msg.addr, addrstr,  INET_ADDRSTRLEN,0,ec);
  return addrstr; 
}

uint16_t KeyMsg::getPort() const {
  return msg.port; 
}

uint8_t KeyMsg::getType() const {
  return msg.type;
}

const uint8_t * KeyMsg::getKey() const {
  return msg.key; 
}
