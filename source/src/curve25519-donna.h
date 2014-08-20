#ifndef CURVE25519
#define CURVE25519


#include <stdint.h>

typedef uint8_t u8;
typedef int32_t s32;
typedef int64_t limb;

#ifdef __cplusplus
    extern "C" {
#endif

int
curve25519_donna(u8 *mypublic, const u8 *secret, const u8 *basepoint);

#ifdef __cplusplus
    }
#endif

#endif

