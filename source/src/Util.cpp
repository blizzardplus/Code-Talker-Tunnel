#include "Util.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>

#include <cyassl/ctaocrypt/random.h>
#include <boost/assert.hpp>

uint16_t Util::bigEndianArrayToShort(unsigned char *buf) {
  return (uint16_t)((buf[0] & 0xff) << 8 | (buf[1] & 0xff));
}

uint32_t Util::bigEndianArrayToInt(unsigned char *buf) {
  return (uint32_t)((buf[0] & 0xff) << 24 | (buf[1] & 0xff) << 16 | 
		    (buf[2] & 0xff) << 8 | (buf[3] & 0xff));
}

void Util::int64ToArrayBigEndian(unsigned char *a, uint64_t i) {
  a[0] = (i >> 56) & 0xFF;
  a[1] = (i >> 48) & 0xFF;
  a[2] = (i >> 40) & 0xFF;
  a[3] = (i >> 32) & 0xFF;
  a[4] = (i >> 24) & 0xFF;
  a[5] = (i >> 16) & 0xFF;
  a[6] = (i >> 8)  & 0xFF;
  a[7] = i         & 0xFF;
}


void Util::int32ToArrayBigEndian(unsigned char *a, uint32_t i) {
  a[0] = (i >> 24) & 0xFF;
  a[1] = (i >> 16) & 0xFF;
  a[2] = (i >> 8)  & 0xFF;
  a[3] = i         & 0xFF;
}

void Util::int16ToArrayBigEndian(unsigned char *a, uint32_t i) {
  a[0] = (i >> 8) & 0xff;
  a[1] = i        & 0xff;
}

namespace {

enum {
  BAD         = 0xFF,  /* invalid encoding */
  PAD         = '=',
  PEM_LINE_SZ = 64
};



static const unsigned char base64Decode[] = 
  { 62, BAD, BAD, BAD, 63,   /* + starts at 0x2B */
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
    BAD, BAD, BAD, BAD, BAD, BAD, BAD,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25,
    BAD, BAD, BAD, BAD, BAD, BAD,
    26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
    36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
    46, 47, 48, 49, 50, 51
  };

static const byte base64Encode[] = 
  { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
    'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
    'u', 'v', 'w', 'x', 'y', 'z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '+', '/'
  };

} // anonymous namespace 

int Util::Base64_Encode(const unsigned char * in, unsigned int inLen, 
                        unsigned char * out, unsigned int * outLen) {
  unsigned int i = 0, j = 0, n = 0;   /* new line counter */

  unsigned int outSz = (inLen + 3 - 1) / 3 * 4;
  outSz += (outSz + PEM_LINE_SZ - 1) / PEM_LINE_SZ;  /* new lines */

  if (outSz > *outLen) return -1;
    
  while (inLen > 2) {
    unsigned char b1 = in[j++];
    unsigned char b2 = in[j++];
    unsigned char b3 = in[j++];

    /* encoded idx */
    unsigned char e1 = b1 >> 2;
    unsigned char e2 = ((b1 & 0x3) << 4) | (b2 >> 4);
    unsigned char e3 = ((b2 & 0xF) << 2) | (b3 >> 6);
    unsigned char e4 = b3 & 0x3F;

    /* store */
    out[i++] = base64Encode[e1];
    out[i++] = base64Encode[e2];
    out[i++] = base64Encode[e3];
    out[i++] = base64Encode[e4];

    inLen -= 3;

    if ((++n % (PEM_LINE_SZ / 4)) == 0 && inLen)
      out[i++] = '\n';
  }

  /* last integral */
  if (inLen) {
    int twoBytes = (inLen == 2);

    unsigned char b1 = in[j++];
    unsigned char b2 = (twoBytes) ? in[j++] : 0;

    unsigned char e1 = b1 >> 2;
    unsigned char e2 = ((b1 & 0x3) << 4) | (b2 >> 4);
    unsigned char e3 =  (b2 & 0xF) << 2;

    out[i++] = base64Encode[e1];
    out[i++] = base64Encode[e2];
    out[i++] = (twoBytes) ? base64Encode[e3] : PAD;
    out[i++] = PAD;
  } 

  out[i++] = '\n';
  if (i != outSz)
    return 1; 
  *outLen = outSz;

  return 0;
}

int Util::Base64_Decode(const unsigned char* in, unsigned int inLen, 
                        unsigned char* out, unsigned int* outLen) {

  unsigned int i = 0;
  unsigned int j = 0;
  unsigned int plainSz = inLen - ((inLen + (PEM_LINE_SZ - 1)) / PEM_LINE_SZ );

  plainSz = (plainSz * 3 + 3) / 4;
  if (plainSz > *outLen) return -1;

  while (inLen > 3) {
    unsigned char b1, b2, b3;
    unsigned char e1 = in[j++];
    unsigned char e2 = in[j++];
    unsigned char e3 = in[j++];
    unsigned char e4 = in[j++];

    int pad3 = 0;
    int pad4 = 0;

    if (e1 == 0)            /* end file 0's */
      break;
    if (e3 == PAD)
      pad3 = 1;
    if (e4 == PAD)
      pad4 = 1;

    e1 = base64Decode[e1 - 0x2B];
    e2 = base64Decode[e2 - 0x2B];
    e3 = (e3 == PAD) ? 0 : base64Decode[e3 - 0x2B];
    e4 = (e4 == PAD) ? 0 : base64Decode[e4 - 0x2B];

    b1 = (e1 << 2) | (e2 >> 4);
    b2 = ((e2 & 0xF) << 4) | (e3 >> 2);
    b3 = ((e3 & 0x3) << 6) | e4;

    out[i++] = b1;
    if (!pad3)
      out[i++] = b2;
    if (!pad4)
      out[i++] = b3;
    else
      break;
        
    inLen -= 4;
    if (in[j] == ' ' || in[j] == '\r' || in[j] == '\n') {
      unsigned char endLine = in[j++];
      inLen--;
      while (endLine == ' ') {   /* allow trailing whitespace */
        endLine = in[j++];
        inLen--;
      }
      if (endLine == '\r') {
        endLine = in[j++];
        inLen--;
      }
      if (endLine != '\n') {
        return 1;
      }
    }
  }
  *outLen = i;

  return 0;
}

uint16_t Util::roundTo16(uint16_t u) {
  if ((u & 0xf) == 0) return u; 
  else                return (u & 0xfff0) + 0x10; 
}

/*
 * Debugging function to print a hexdump of data with ascii, for example:
 * 00000000  74 68 69 73 20 69 73 20  61 20 74 65 73 74 20 6d  this is  a test m
 * 00000010  65 73 73 61 67 65 2e 20  62 6c 61 68 2e 00        essage.  blah..
 */


void Util::hexDump(const void *ptr, int len) {
  unsigned char * data = (unsigned char *)ptr; 
  int line;
  int max_lines = (len / 16) + (len % 16 == 0 ? 0 : 1);
  int i;
    
  for(line = 0; line < max_lines; line++)
  {
    printf("%08x  ", line * 16);

    /* print hex */
    for(i = line * 16; i < (8 + (line * 16)); i++)
    {
      if(i < len)
              printf("%02x ", (uint8_t)data[i]);
      else
              printf("   ");
    }
    printf(" ");
    for(i = (line * 16) + 8; i < (16 + (line * 16)); i++)
    {
      if(i < len)
              printf("%02x ", (uint8_t)data[i]);
      else
              printf("   ");
    }

    printf(" ");
        
    /* print ascii */
    for(i = line * 16; i < (8 + (line * 16)); i++)
    {
      if(i < len)
      {
        if(32 <= data[i] && data[i] <= 126)
                printf("%c", data[i]);
        else
                printf(".");
      }
      else
              printf(" ");
    }
    printf(" ");
    for(i = (line * 16) + 8; i < (16 + (line * 16)); i++)
    {
      if(i < len)
      {
        if(32 <= data[i] && data[i] <= 126)
                printf("%c", data[i]);
        else
                printf(".");
      }
      else
              printf(" ");
    }

    printf("\n");
  }
}

void Util::hexStringToChar(unsigned char *buffer, int len, std::string &hexString) {
  assert(hexString.length() / 2 == len);

  char nibble[9];
  nibble[8] = '\0';

  uint32_t nibbleConversion;

  for (int i=0;i<len/4;i++) {
    memcpy(nibble, hexString.c_str()+(i*8), 8);
    sscanf(nibble, "%x", &nibbleConversion);
    Util::int32ToArrayBigEndian(buffer+(i*4), nibbleConversion);
  }

  memset(nibble, 0, sizeof(nibble));
  nibbleConversion = 0;
}

std::string Util::charToHexString(const unsigned char* buffer, size_t size) {
  std::stringstream str;
  str.setf(std::ios_base::hex, std::ios::basefield);
  str.fill('0');

  for (size_t i=0;i<size;++i) {
    str << std::setw(2) << (unsigned short)(unsigned char)buffer[i];
  }

  return str.str();
}


uint16_t Util::getRandomId() {
  unsigned char id[2];
  RNG rng;
  InitRng(&rng); 
  RNG_GenerateBlock(&rng, id, sizeof(id)); 
  
  return bigEndianArrayToShort(id);
}

uint32_t Util::getRandom() {
  unsigned char bytes[4];
  RNG rng;
  InitRng(&rng); 
  RNG_GenerateBlock(&rng, bytes, sizeof(bytes));

  return bigEndianArrayToInt(bytes);
}

