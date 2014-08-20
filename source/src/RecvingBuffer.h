/*
 * RecvingBuffer.h
 *
 *  Created on: 2012-01-23
 *      Author: b5li
 */

#ifndef RECVINGBUFFER_H_
#define RECVINGBUFFER_H_

#include <stdint.h>
#include <list>
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
class RecvingBuffer {
public:
  // init_seq - the sequence number assigned to the first byte 
  // 
  RecvingBuffer(uint32_t init_seq) 
    : seq_head(init_seq), seq_tail(init_seq), _isc(false) 
  { }

  // delete all remaining RawPacket 
  ~RecvingBuffer(); 

  // put() - insert one RawPacket to the list 
  // 
  uint32_t put(RawPacket * pkt); 

  // get() - copy at most ``len'' bytes to memory ``out'' 
  //         from position current_seq; 
  //         current_seq is then incremented by ``len'' 
  //         or to reach seq_tail; 
  //         return the size of memory copied
  // 
  RawPacket * get(); 

  // size()       - the total amount of data stored in this buffer 
  // 
  size_t size() const { return seq_tail > seq_head ? seq_tail - seq_head : 0; } 

  // empty() - return true iff size() == 0, ie, no data to get() 
  // 
  bool empty() const { return size() == 0; } 

  // completed() - return true iff all data are received and retrieved
  // 
  bool completed() {
    return _isc && _buf_lt.empty(); 
  }

  void dump(bool hasContent = true) const;
  
  uint32_t seq_head; 
  uint32_t seq_tail; 


private:
  typedef std::list<RawPacket *> list_t; 
  list_t _buf_lt; 
  bool   _isc; 

};

#endif /* RECVINGBUFFER_H_ */
