#include "Crypto.h"
#include "Util.h"
#include "debug.h"
#include "Packet.h"

#include "curve25519-donna.h"

#include <cstring>

void Codec::genPublicKey(uint8_t * mypublic) {
	const uint8_t basepoint[32] = {9};

	_secret_key[0] &= 248;
	_secret_key[31] &= 127;
	_secret_key[31] |= 64;

	curve25519_donna(mypublic, _secret_key, basepoint);
}

void Codec::genSharedKey(const uint8_t * theirpublic) {
	Sha256 _sha256;
	InitSha256(&_sha256);
	curve25519_donna(_shared_key0, _secret_key, theirpublic);


	//Getting the 4 keys!
	Sha256Update(&_sha256, _shared_key0, sizeof(_shared_key0));
	Sha256Final(&_sha256, _shared_key1);

	Sha256Update(&_sha256, _shared_key1, sizeof(_shared_key1));
	Sha256Final(&_sha256, _shared_key2);

	Sha256Update(&_sha256, _shared_key2, sizeof(_shared_key2));
	Sha256Final(&_sha256, _shared_key3);


	//hash shared key
	initMac();
}

void Codec::encrypt(uint8_t *out, const uint8_t *in, size_t len, uint8_t *iv) 
{
	uint8_t ivec[16] = {0};              // 16 bytes ivec for AES
	RNG_GenerateBlock(&_rng, ivec, 8);   // first 8 bytes should be random

	if(_is_server)
		AesSetKey(&_aes_out, _shared_key0, sizeof(_shared_key0), ivec, AES_ENCRYPTION);
	else
		AesSetKey(&_aes_out, _shared_key2, sizeof(_shared_key2), ivec, AES_ENCRYPTION);

	AesCtrEncrypt(&_aes_out, out, in, Util::roundTo16(len));

	memcpy(iv, ivec, 8);
}

void Codec::decrypt(uint8_t *out, const uint8_t *in, size_t len, const uint8_t *iv) 
{
	uint8_t ivec[16] = {0};             // 16 bytes iv for AES
	memcpy(ivec, iv, 8);                // retrieve the first 8 bytes iv

	if(_is_server)
		AesSetKey(&_aes_in, _shared_key2, sizeof(_shared_key2), ivec, AES_ENCRYPTION);
	else
		AesSetKey(&_aes_in, _shared_key0, sizeof(_shared_key0), ivec, AES_ENCRYPTION);

	AesCtrEncrypt(&_aes_in, out, in, Util::roundTo16(len));
}

void Codec::initMac() 
{
	if(_is_server)
	{
		HmacSetKey(&_hmac_in, SHA256, _shared_key1, sizeof(_shared_key1));
		HmacSetKey(&_hmac_out, SHA256, _shared_key3, sizeof(_shared_key3));
	}
	else
	{
		HmacSetKey(&_hmac_out, SHA256, _shared_key1, sizeof(_shared_key1));
		HmacSetKey(&_hmac_in, SHA256, _shared_key3, sizeof(_shared_key3));
	}
}

void Codec::updateMAC(const uint8_t *data, size_t len, uint8_t *mac) 
{
	uint8_t macDigest[SHA256_DIGEST_SIZE];
	HmacUpdate(&_hmac_out, data, len);
	HmacFinal(&_hmac_out, macDigest);
	memcpy(mac, macDigest, 8);
}

bool Codec::verifyMAC(const uint8_t *data, size_t len, const uint8_t *mac) 
{
	uint8_t macDigest[SHA256_DIGEST_SIZE];
	uint8_t trgt[8];
	if(len > 1500 )
	{
		std::cerr<< "Packet size "<< len <<" too big!" << std::endl;
		return false;
	}
	/*if (len < UDP_HDR_LEN)
	{
		std::cerr<< "Packet too small!" << std::endl;
		return false;
	}*/
	HmacUpdate(&_hmac_in, data, len);
	HmacFinal(&_hmac_in, macDigest);
	memcpy(trgt, macDigest, 8);

	return !memcmp(mac, trgt, 8);
}
