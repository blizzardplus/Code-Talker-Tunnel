/*
 * SendingBuffer.h
 *
 *  Created on: 2012-01-11
 *      Author: hmohajer
 *  Modified on: 2012-01-19
 *      Author: b5li
 */

#ifndef SENDINGBUFFER_H_
#define SENDINGBUFFER_H_

// Only for reliable transmission

#include <stdint.h>
#include <deque>
#include "Packet.h"

typedef unsigned char byte;

// Buffer class specific for reliable transport propose 
// 
//    ------------------------------------------
//    -----------###############+++++++++++++++
//    ------------------------------------------
//    ^          ^              ^              ^
//    |          |              |              seq_tail
//    |          |              -- current_seq
//    |          -- current_ack 
//    -- seq_head 
// 
// 
// * The ``-'' part will be released by updateAck() 
// * The ``#'' part are sent out but not ack-ed yet
// * The ``+'' part are nevered sent out
// 
// Internally the buffered data are stored by a std::deque 
// of RawPacket *. Whenever necessary, the data stored in 
// some RawPacket structs will be moved around 
// 
class SendingBuffer {
public:
  // init_seq - the sequence number assigned to the first byte 
  // 
  SendingBuffer(uint32_t init_seq) 
    : current_ack(init_seq), current_seq(init_seq), 
      seq_head(init_seq), seq_tail(init_seq) 
  { }

  // delete all remaining RawPacket 
  ~SendingBuffer(); 

  // put() - push one newly added RawPacket to the back of the deque
  // 
  void put(const byte*, size_t); 
  void put(RawPacket * pkt); 

  // get() - copy at most ``len'' bytes to memory ``out'' 
  //         from position current_seq; 
  //         current_seq is then incremented by ``len'' 
  //         or to reach seq_tail; 
  //         return the size of memory copied
  // 
  // get_from() - like get() but copy from position ``seq''; 
  //              current_seq will not be affected; 
  //              if ``seq'' is invalid, 0 is returned 
  // 
  size_t      get(byte* out, size_t len); 
  RawPacket * get(size_t len); 
  size_t      get_from(byte* out, size_t len, uint32_t seq);
  RawPacket * get_from(size_t len, uint32_t seq);


  // update_ack() effectively relases buffer trunks before the ack 
  void update_ack(uint32_t);


  // size()       - the total amount of data stored in this buffer 
  // 
  // size_fresh() - the amount of data not sent out (by get()) 
  // 
  // size_unack() - the amount of data not ack-ed 
  // 
  size_t size() const { return seq_tail - seq_head; } 

  size_t size_fresh() const { return seq_tail - current_seq;  }

  size_t size_unack() const { return current_seq - current_ack; } 

  // empty() - return true iff size_fresh() == 0, ie, no data to get() 
  // 
  // empty_unack() - return true iff size_unack() == 0
  // 
  bool empty() const { return size_fresh() == 0;}

  bool empty_unack() const { return size_unack() == 0; } 

  void dump() const; 

  void reset_seq(uint32_t seq) {
    if (seq < current_ack || seq >= current_seq) return; 
    current_seq = seq; 
  }
  
  uint32_t current_ack; 
  uint32_t current_seq; 

private:
  typedef std::deque<RawPacket *> deque_t; 
  deque_t _buf_q; 
  uint32_t seq_head; 
  uint32_t seq_tail; 
  void rewind_head(); 


};



#endif /* SENDINGBUFFER_H_ */
