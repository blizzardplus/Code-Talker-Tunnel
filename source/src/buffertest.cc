#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <stdint.h>
#include <iostream>
#include <sstream>
#include <string>

#include "SendingBuffer.h"
#include "RecvingBuffer.h"
#include "Packet.h"
#include "Util.h"

int main(int argc, char ** argv) {
  SendingBuffer send_buf(0); 
  RecvingBuffer recv_buf(0); 

  char c; 
  char buf[1500]; 
  int i; 

  if (argc < 3) return 0; 

  FILE * ifile = fopen(argv[1], "rb"); 
  FILE * ofile = fopen(argv[2], "wb"); 

  uint32_t seq_in, seq_out, ack_in, ack_out; 
  size_t len_in, len_out; 

  while (!ferror(ifile)) {
    RawPacket * pkt = new RawPacket; 
    pkt->length = fread(pkt->data, sizeof(unsigned char), 512, ifile); 
    if (pkt->length == 0) {
      delete pkt; 
      break;
    }
    else {
      send_buf.put(pkt); 
    }
  }

  while (!std::cin.eof()) {

    std::cin >> c; 

    if (c == 'r') {

      if(!fgets(buf, sizeof(buf), stdin)) exit(-1); 
      std::stringstream sstr(buf); 
      sstr >> seq_in; 
      sstr >> len_in; 
      RawPacket * pkt = send_buf.get_from(len_in, seq_in); 

      if (pkt->length == 0) {
        RawPacket * p = new RawPacket; 
        p->type = MT_GOODBYE; 
        p->length = 0; 
        recv_buf.put(p); 
      } 
      else {
        recv_buf.put(pkt); 
      }
    }
    else if (c == 'w') {
      if (recv_buf.completed()) {
        fclose(ofile); 
        break;
      }
      else if (recv_buf.empty())
        continue;

      RawPacket * pkt = recv_buf.get(); 
      fwrite(pkt->data, sizeof(unsigned char), pkt->length, ofile); 
      delete pkt; 
    }
    else if (c == 'x') {
      printf("\nSEND_BUF\n\n"); 
      send_buf.dump(); 
    }
    else if (c == 'X') {
      printf("\nRECV_BUF\n\n"); 
      recv_buf.dump();
    }
    else 
      continue; 

  }

  return 0;
}
