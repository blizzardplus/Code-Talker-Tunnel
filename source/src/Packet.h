#ifndef PACKET_H
#define PACKET_H

/* 
 * UDP packet struct
 */

#include <stdint.h>
#include <string.h>

enum {
	/* maximal length of tcp messages */
	MAX_RAW_MSG_LEN = 1460,

	/* udp crypto info length - mac (8 bytes) and iv (8 bytes) */
	UDP_CRYT_LEN    = 16,
	/* udp message header length */
	UDP_HDR_LEN     = 12 + UDP_CRYT_LEN,
	/* maximal length of udp messages */
	MAX_UDP_MSG_LEN = MAX_RAW_MSG_LEN + UDP_HDR_LEN,

	/* the ip packet header length */
	PACKET_HDR_LEN  = 42 + UDP_HDR_LEN
}; 

enum msg_type_t {
	MT_HELLO    = 0x1,
	MT_HELLOACK = 0x2,
	MT_DATA     = 0x3,
	MT_DUMMY    = 0x4,
	MT_GOODBYE  = 0x5
};

/* Raw packet */
typedef struct {
	unsigned int    id;
	size_t          length;
	uint8_t         type;
	uint32_t        seq_lo;          /* the sequence number of data[0] */
	uint32_t        seq_hi;          /* the sequence number of data[length-1] */
	unsigned char   data[MAX_RAW_MSG_LEN];
} RawPacket; 


enum msg_flag_t {
	FG_DATA    = 0x1,
	FG_ACK     = 0x2,

	FG_NUM_FLAGS
}; 

/* UDP packet  */
typedef struct {

	unsigned int    id;
	size_t          length;       /* size of the udp datagram "cell" */
	union {
		unsigned char data[MAX_UDP_MSG_LEN];

		struct {
			uint8_t  mac[8];
			uint8_t  iv[8];
			union {
				struct {
					uint8_t  type;
					uint8_t  flag;
					uint16_t len;         /* size of the real msg (minus any padding) */
					int32_t  seq;
					int32_t  ack;
					uint8_t  msg[MAX_RAW_MSG_LEN+16]; /* FIXME: round up for AES BLOCK SIZE */
				} payload;
				uint8_t    cipher[0];
			};
		} cell;
	};
} UDPPacket;


#define UDP_PKT_MSG(x) \
		(x)->cell.payload.msg

#define UDP_PKT_LEN(x) \
		(x)->cell.payload.len

#define UDP_PKT_TYPE(x) \
		(x)->cell.payload.type

#define UDP_PKT_FLAG(x) \
		(x)->cell.payload.flag

#define UDP_PKT_SEQ(x) \
		(x)->cell.payload.seq

#define UDP_PKT_ACK(x) \
		(x)->cell.payload.ack

#define UDP_PKT_MAC(x) \
		(x)->cell.mac

#define UDP_PKT_IV(x) \
		(x)->cell.iv

#define UDP_PKT_CIPHER(x) \
		(x)->cell.cipher

#define UDP_PKT_CIPHER_LEN(x) \
		((x)->length - UDP_CRYT_LEN)

#define UDP_PKT_AUTH_MSG(x) \
		((x)->data + sizeof((x)->cell.mac))

#define UDP_PKT_AUTH_LEN(x) \
		((x)->length > sizeof((x)->cell.mac)) ? ((x)->length - sizeof((x)->cell.mac)) : 0

#endif
