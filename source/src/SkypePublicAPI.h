#ifdef SKYPE_PAPI


#ifndef SKYPEPUBLICAPI_H_
#define SKYPEPUBLICAPI_H_


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef __cplusplus
extern "C" {
#endif

struct msg_callback;

typedef void (*msg_callback_func)(const struct msg_callback* mcb, const char* str);

struct msg_callback {
	msg_callback_func mcb;
	const char* str;
};


static struct msg_callback callback;


int initialize(msg_callback_func cb);
int sendToSkype( const char *msg );
void startLoop();
void endLoop();


#ifdef __cplusplus
}
#endif

#endif
#endif
