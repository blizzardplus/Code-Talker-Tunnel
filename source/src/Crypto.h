#ifndef CRYPTO_H
#define CRYPTO_H

/* 
 * crypto functionalities
 */

#include <cyassl/ctaocrypt/aes.h>
#include <cyassl/ctaocrypt/hmac.h>
#include <cyassl/ctaocrypt/random.h>

#include <stdint.h>


class Codec {
private:
  bool _is_server;
  RNG _rng;
  Aes _aes_in, _aes_out;
  Hmac _hmac_in, _hmac_out;

  uint8_t _secret_key[32];

  uint8_t _shared_key0[32];
  uint8_t _shared_key1[32];
  uint8_t _shared_key2[32];
  uint8_t _shared_key3[32];

public: 
  Codec(bool is_server): _is_server(is_server) {
    InitRng(&_rng);
    RNG_GenerateBlock(&_rng, _secret_key, sizeof(_secret_key));
  }

  ~Codec(){  }

  void genPublicKey(uint8_t * mypublic); 
  void genSharedKey(const uint8_t * theirpublic);

  void encrypt(uint8_t *out, const uint8_t *in, size_t len, uint8_t *iv); 
  void decrypt(uint8_t *out, const uint8_t *in, size_t len, const uint8_t *iv);

  void initMac(); 
  void updateMAC(const uint8_t *data, size_t len, uint8_t *mac); 
  bool verifyMAC(const uint8_t *data, size_t len, const uint8_t *mac); 

}; 

#endif
