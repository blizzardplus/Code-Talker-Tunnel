#ifndef UTIL_H
#define UTIL_H

/* 
 * The utilities used in other componenets
 */

#include <stdint.h>
#include <string>
#include <vector>


class Util {

 public:
  static void int64ToArrayBigEndian(unsigned char *a, uint64_t i);
  static void int32ToArrayBigEndian(unsigned char *a, uint32_t i);
  static void int16ToArrayBigEndian(unsigned char *a, uint32_t i);

  static uint16_t bigEndianArrayToShort(unsigned char *buf);
  static uint32_t bigEndianArrayToInt(unsigned char *buf);

  static int Base64_Encode(const unsigned char * in, unsigned int inLen,
                    unsigned char * out, unsigned int * outLen);

  static int Base64_Decode(const unsigned char* in, unsigned int inLen,
                    unsigned char* out, unsigned int* outLen);

  static uint16_t roundTo16(uint16_t);

  static void hexDump(const void *buf, int length);
  static void hexStringToChar(unsigned char *buffer, int len, std::string &hexString);
  static std::string charToHexString(const unsigned char* buffer, size_t size);

  static uint16_t getRandomId();
  static uint32_t getRandom();
};

#endif
