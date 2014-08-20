#ifndef KEY_MSG_H
#define KEY_MSG_H

/* 
 * KeyMsg - key exchange message 
 */
#include <stdint.h>
#include <string>

enum {
  // The length of key exchange message 
  //  = 32 (key) + 4 (ip) + 2 (port) + 1 (type) + 1 (null) 
  KEY_MSG_PLAIN_LEN = 40, 

  // The length of base64 encoded message 
  KEY_MSG_CODE_LEN = 64
}; 

class KeyMsg {
public: 
  enum {
    KM_CLIENT_KEY = 0x1, 
    KM_SERVER_KEY = 0x2, 
    KM_CLIENT_ACK = 0x3, 
    KM_SERVER_ACK = 0x4, 
    KM_WARNING    = 0x5
  }; 

  union {
    struct {
      uint8_t  key [32]; 
      uint8_t  addr[4]; 
      uint16_t port; 
      uint8_t  type; 
    } msg; 
    unsigned char data[KEY_MSG_CODE_LEN]; 
  }; 
    
  unsigned char seq[KEY_MSG_CODE_LEN]; 
  // actually we only need 57 bytes to Base64-encode 39 bytes 

  KeyMsg(const std::string & addrstr, uint16_t p, const uint8_t * k, uint8_t t); 
  KeyMsg(const char * s, int sz); 
  operator const char *() const; 

  std::string getAddr() const; 
  uint16_t    getPort() const; 
  uint8_t     getType() const; 
  const uint8_t * getKey()  const;
}; 

#endif


