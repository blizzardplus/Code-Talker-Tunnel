/*
 * RecvingBuffer.cpp
 *
 *  Created on: 2012-01-23
 *      Author: b5li
 */

#include "RecvingBuffer.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>

#include <algorithm>

#include "Util.h"
#include "debug.h"

#ifdef DEBUG
# define ASSERT_INVARIANT                       \
  do {                                          \
    assert(seq_head <= seq_tail);               \
  } while (0) 
#else
# define ASSERT_INVARIANT 
#endif

uint32_t RecvingBuffer::put(RawPacket * pkt)
{
  log("rcvbuf", "p\t%u\t%u\t%u\t%hhu", pkt->length, pkt->seq_lo, pkt->seq_hi, pkt->type); 

  if (pkt->type == MT_GOODBYE) {
    assert(!_isc && "multiple GOODBYE received!"); 
    _isc = true;
    delete pkt;
    return seq_tail; 
  }
  else if (pkt->length == 0) {
    delete pkt;
    return seq_tail;
  }


  if (pkt->seq_lo < seq_head) {
    if (pkt->seq_hi < seq_head) { 
      // drop the packet if its entire content 
      // has been acked 
      delete pkt;
      return seq_tail; 
    }
    else {
      // discard already acked content 

      size_t offset = seq_head - pkt->seq_lo; 
      size_t length = pkt->length - offset; 
      assert(offset < pkt->length); 
      memmove(pkt->data, pkt->data+offset, length); 

      pkt->seq_lo = seq_head; 
      pkt->seq_hi = seq_head + length - 1; 
      pkt->length = length; 
    }
  }

  list_t::iterator it = _buf_lt.begin(); 
  while (it != _buf_lt.end()) {
    RawPacket * p = *it; 
    if (pkt->seq_lo < p->seq_lo) break; 
    ++it;
  }
  it = _buf_lt.insert(it, pkt);
  assert(seq_head <= pkt->seq_lo && "FIXME: acked content not discarded?"); 
  while (it != _buf_lt.end()) {
    pkt = *it; 
    if (pkt->seq_lo > seq_tail) break; 
    if (seq_tail <= pkt->seq_hi) seq_tail = pkt->seq_hi+1; 
    ASSERT_INVARIANT; 
    ++it; 
  }
  return seq_tail; 
}

RawPacket * RecvingBuffer::get()
{
  log("rcvbuf", "g");
  if (empty()) return 0; 

  RawPacket * pkt = _buf_lt.front(); 
  assert(pkt && pkt->seq_lo < seq_tail); 
  _buf_lt.pop_front(); 

  list_t::iterator it = _buf_lt.begin(); 
  while (it != _buf_lt.end()) {
    RawPacket * p = *it; 
    if (p->seq_lo <= pkt->seq_hi) {
      // shrink if p is larger 
      if (pkt->seq_hi < p->seq_hi) {
        size_t length = p->seq_hi - pkt->seq_hi; 
        size_t offset = p->length - length; 
        memmove(p->data, p->data+offset, length); 
        p->length = length; 
        p->seq_lo += offset; 
        ++it; 
      }
      else if (pkt->type == MT_GOODBYE) {
        assert(!"caught GOODBYE msg"); 
      }
      else {
        it = _buf_lt.erase(it); 
        delete p; 
      }
    }
    else {
      break; 
    }
  }

  seq_head += pkt->length; 
  if (_buf_lt.empty()) {
    assert(seq_tail == seq_head); 
  }
  else {
    RawPacket * p = _buf_lt.front(); 
    if (p->seq_lo != seq_head) {
      assert(p->seq_lo > seq_head);
      assert(seq_tail == seq_head); 
    }
  }

  ASSERT_INVARIANT; 
  return pkt; 
}


void RecvingBuffer::dump(bool hasContent) const {
  printf("seq_head\t\t%d\t-\t%08x\n", seq_head, seq_head); 
  printf("seq_tail\t\t%d\t-\t%08x\n", seq_tail, seq_tail); 
  printf("--------------------------------------------------\n"); 

  list_t::const_iterator it = _buf_lt.begin(); 
  int i = 0; 
  while (it != _buf_lt.end()) {
    RawPacket * pkt = *it; 
    printf("PACKET %d LENGTH %08u LO %08u HI %08u\n", i, pkt->length, pkt->seq_lo, pkt->seq_hi); 

    if (hasContent) {
      printf("\n"); 
      Util::hexDump(pkt->data, pkt->length); 
      printf("--------------------------------------------------\n");
    }
    ++i; 
    ++it; 
  }
}

RecvingBuffer::~RecvingBuffer() {
  while (!_buf_lt.empty()) {
    RawPacket * pkt = _buf_lt.front(); 
    _buf_lt.pop_front(); 
    delete pkt; 
  }
}

#ifdef UNITTEST


int main() {
  RecvingBuffer buf(0); 
  assert(buf.size() == 0); 
  assert(buf.seq_head == buf.seq_tail && buf.seq_head == 0); 

  RawPacket *pkt = new RawPacket; 
  pkt->seq_lo    = 0; 
  pkt->seq_hi    = 4; 
  pkt->length = 5; 
  memcpy(pkt->data, "abcde", 5); 
  size_t ack = buf.put(pkt); 
  assert(buf.size() == 5);
  assert(ack == 5); 

  buf.dump(); 

  RawPacket *pkt2 = new RawPacket; 
  pkt2->seq_lo    = 2; 
  pkt2->seq_hi    = 6; 
  pkt2->length = 5; 
  memcpy(pkt2->data, "cde12", 5); 
  ack = buf.put(pkt2); 
  assert(buf.size() == 7);
  assert(ack == 7); 

  buf.dump();



  pkt2 = new RawPacket; 
  pkt2->seq_lo    = 10; 
  pkt2->seq_hi    = 15; 
  pkt2->length = 6; 
  memcpy(pkt2->data, "ABCDEF", 6); 
  ack = buf.put(pkt2); 
  assert(buf.size() == 7);
  assert(ack == 7); 



  buf.dump();

  pkt2 = buf.get(); 
  assert(pkt == pkt2); 
  assert(buf.size() == 2); 

  buf.dump(); 

  pkt2 = buf.get(); 
  assert(pkt2->length == 2); 
  assert(!memcmp(pkt2->data, "12", 2)); 

  buf.dump(); 


  return 0;
}

#endif  // UNITTEST

