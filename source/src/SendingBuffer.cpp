/*
 * SendingBuffer.cpp
 *
 *  Created on: 2012-01-11
 *      Author: hmohajer
 *  Modified on: 2012-01-19
 *      Author: b5li
 */

#include "SendingBuffer.h"

#include <cstdlib>
#include <cstdio>

#include <cstring>
#include <cassert>

#include "Util.h"
#include "debug.h"

#ifdef DEBUG
# define ASSERT_INVARIANT                       \
		do {                                          \
			assert((seq_head <= current_ack)    &&      \
					(current_ack <= current_seq) &&      \
					(current_seq <= seq_tail));          \
		} while (0)
#else
# define ASSERT_INVARIANT 
#endif

void SendingBuffer::put(const byte* data, size_t len)
{
	assert(len > 0 && "FIXME: got an empty buffer trunk");
	RawPacket * pkt = new RawPacket;
	assert(len <= sizeof(pkt->data));
	memcpy(pkt->data, data, len);
	pkt->length = len;
	put(pkt);
}

void SendingBuffer::put(RawPacket * pkt)
{
	assert(pkt->length > 0 && "FIXME: got an empty RawPacket");
	pkt->seq_lo = seq_tail;
	seq_tail += pkt->length;
	pkt->seq_hi = seq_tail - 1;

	_buf_q.push_back(pkt);
}

size_t SendingBuffer::get(byte* out, size_t len) 
{

	size_t ret = get_from(out, len, current_seq);
	current_seq += ret;
	ASSERT_INVARIANT;
	return ret;

}


RawPacket * SendingBuffer::get(size_t len)
{
	RawPacket * pkt = get_from(len, current_seq);

	current_seq += pkt->length;
	ASSERT_INVARIANT;
	return pkt;
}

size_t SendingBuffer::get_from(byte* out, size_t len, uint32_t seq) 
{
	// return 0 if invalid deq
	if (seq < current_ack || seq >= seq_tail) return 0;

	deque_t::iterator it = _buf_q.begin();
	size_t ret = 0;

	while (len > 0 && it != _buf_q.end()) {
		RawPacket * pkt = *it;
		if (seq < pkt->seq_lo) {
			break;
		}
		else if (seq <= pkt->seq_hi) {
			size_t offset      = seq - pkt->seq_lo;
			size_t partial_len = pkt->length - offset;
			if (len < partial_len) partial_len = len;

			memcpy(out, pkt->data+offset, partial_len);
			out += partial_len;
			ret += partial_len;
			seq += partial_len;
			len -= partial_len;
		}

		++it;
	}

	return ret;
}



RawPacket * SendingBuffer::get_from(size_t len, uint32_t seq) 
{
	assert(len <= MAX_RAW_MSG_LEN && "get_from() asks for >1024 bytes");
	RawPacket * pkt = new RawPacket;
	size_t length   = get_from(pkt->data, len, seq);
	pkt->type   = MT_DATA;
	pkt->length = length;
	pkt->seq_lo = seq;
	pkt->seq_hi = seq + length - 1;
	return pkt;
}

void SendingBuffer::update_ack(uint32_t ack)
{
	assert(ack <= seq_tail);
	if (ack == current_ack) return; // assert(current_ack < current_seq);

	if (current_seq < ack) current_seq = ack;

	current_ack = ack;            // first update current_ack

	rewind_head();                // then rewind to release ack-ed data
	ASSERT_INVARIANT;
}

void SendingBuffer::rewind_head() 
{
	RawPacket * pkt = _buf_q.front();
	if (!pkt) return;

	while (seq_head < current_ack && !_buf_q.empty()) {
		pkt = _buf_q.front();
		assert(seq_head == pkt->seq_lo);
		if (pkt->seq_hi < current_ack) {
			// release RawPacket at the beginning of _buf_q
			_buf_q.pop_front();

			seq_head += pkt->length;

			delete pkt;
		}
		else {
			break;
		}
	}

	if (seq_head == current_ack || _buf_q.empty()) return ;

	assert((seq_head < current_ack) && (current_ack < seq_head + pkt->length));

	size_t offset = current_ack - seq_head;
	size_t length = pkt->length - offset;

	memmove(pkt->data, pkt->data+offset, length);
	pkt->length = length;
	seq_head = current_ack;

	pkt->seq_lo = seq_head;
	pkt->seq_hi = seq_head + length - 1;
}


void SendingBuffer::dump() const {
	printf("seq_head\t\t%d\t-\t%08x\n", seq_head, seq_head);
	printf("seq_tail\t\t%d\t-\t%08x\n", seq_tail, seq_tail);
	printf("current_seq\t\t%d\t-\t%08x\n", current_seq, current_seq);
	printf("current_ack\t\t%d\t-\t%08x\n", current_ack, current_ack);
	printf("--------------------------------------------------\n");

	deque_t::const_iterator it = _buf_q.begin();
	int i = 0;
	while (it != _buf_q.end()) {
		RawPacket * pkt = *it;
		printf("\nPACKET %d LENGTH %d\n\n", i, pkt->length);

		Util::hexDump(pkt->data, pkt->length);

		printf("--------------------------------------------------\n");
		++i;
		++it;
	}

}

SendingBuffer::~SendingBuffer() {
	while (!_buf_q.empty()) {
		RawPacket * pkt = _buf_q.front();
		_buf_q.pop_front();
		delete pkt;
	}
}


#ifdef UNITTEST

int main() {
	SendingBuffer buf(0);
	assert(buf.size() == 0 && buf.size_fresh() == 0 && buf.size_unack() == 0);
	assert(buf.current_seq == buf.current_ack && buf.current_seq == 0);

	unsigned char data[1024] = "abcabcabc";

	buf.put(data, 3);
	assert(buf.size() == 3 && buf.size_fresh() == 3 && buf.size_unack() == 0);

	buf.put((byte *)"123456", 6);
	assert(buf.size() == 9 && buf.size_fresh() == 9 && buf.size_unack() == 0);

	buf.put((byte *)"1234567890", 10);
	assert(buf.size() == 19 && buf.size_fresh() == 19 && buf.size_unack() == 0);

	buf.put((byte *)"ABCDEF", 6);
	assert(buf.size() == 25 && buf.size_fresh() == 25 && buf.size_unack() == 0);
	buf.dump();

	memset(data, 0, 1024);

	buf.get(data, 5);
	assert(buf.size() == 25 && buf.size_fresh() == 20 && buf.size_unack() == 5);
	printf("++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	printf("data at line %d\n", __LINE__);
	Util::hexDump(data, 5);

	assert(!memcmp(data, "abc12", 5));
	assert(buf.current_seq == 5);
	assert(buf.current_ack == 0);

	buf.get(data, 5);
	assert(buf.size() == 25 && buf.size_fresh() == 15 && buf.size_unack() == 10);
	printf("++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	printf("data at line %d\n", __LINE__);
	Util::hexDump(data, 5);

	assert(!memcmp(data, "34561", 5));
	assert(buf.current_seq == 10);
	assert(buf.current_ack == 0);

	buf.get(data, 2);
	assert(buf.size() == 25 && buf.size_fresh() == 13 && buf.size_unack() == 12);
	printf("++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	printf("data at line %d\n", __LINE__);
	Util::hexDump(data, 2);

	assert(!memcmp(data, "23", 2));
	assert(buf.current_seq == 12);
	assert(buf.current_ack == 0);


	buf.update_ack(1);
	assert(buf.size() == 24 && buf.size_fresh() == 13 && buf.size_unack() == 11);
	printf("++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	buf.dump();


	assert(buf.current_seq == 12);
	assert(buf.current_ack == 1);

	return 0;
}

#endif  // UNITTEST

