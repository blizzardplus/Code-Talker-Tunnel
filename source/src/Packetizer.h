#ifndef PACKETIZER_H
#define PACKETIZER_H

/* 
 * This header defines the outgoing connection stream
 */

#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "TunnelStream.h"
#include "Packet.h"

#include "BasicClient.h"
#include "Crypto.h"
#include "SendingBuffer.h" 
#include "RecvingBuffer.h"
#include "Morpher.h" 

using namespace boost::asio;

class Packetizer : public TunnelStream, 
                   public boost::enable_shared_from_this<Packetizer> {
private:
  SendingBuffer         _send_q;       /* udp packets waiting for sending out */
  RecvingBuffer         _recv_q;       /* udp packets received from other stream */
  mutable boost::mutex _send_q_mutex; 
  mutable boost::mutex _recv_q_mutex;
  mutable boost::mutex _read_mutex; 
  mutable boost::mutex _codec_mutex;
  mutable boost::mutex _start_send_mutex;
  boost::condition_variable _cv_send_q; 
  boost::condition_variable _cv_recv_q; 
  boost::condition_variable _cv_read;
  boost::condition_variable _cv_start_send;
  boost::shared_ptr<BasicClient> _client;
  boost::shared_ptr<Codec>  _codec;
  volatile uint32_t _recv_seq; 
  volatile bool     _running; 
  volatile bool     _reading;
  volatile bool     _sending_started;
  bool				_is_server;
  StreamReadHandler _read_handler; 
  Morpher *_morpher;

public:

  Packetizer(boost::shared_ptr<BasicClient> & c, 
             boost::shared_ptr<Codec> & codec, 
             Morpher * morpher, bool is_server)

    : _send_q(0), _recv_q(0), _client(c), _codec(codec), _recv_seq(0), 
      _running(false), _reading(false), _sending_started(false), _morpher(morpher), _is_server(is_server)
  { }

  void read(StreamReadHandler handler);

  void write(unsigned char* buf, int length, StreamWriteHandler handler);

  void close(bool force = false);


  // send / receive packet
  boost::thread send_thread; 
  boost::thread recv_thread; 
  boost::thread read_thread; 

  void packetSend(); 
  void packetRecv(); 

  inline void encodePacket(const RawPacket * rawpkt, UDPPacket * udppkt, 
                           uint8_t flag, uint32_t seq, uint32_t ack, size_t padding); 
  inline bool decodePacket(const UDPPacket * udppkt, RawPacket * rawpkt, 
                           uint8_t *flag, uint32_t *seq, uint32_t *ack); 

  void run(); 
  void join();

  UDPPacket * genHello(); 
  bool verifyHello(UDPPacket * pkt); 
  UDPPacket * genGoodbye(uint32_t seq); 

private:
  void streamRead();
  void handleCloseComplete(UDPPacket * pkt, const boost::system::error_code& error, 
                           std::size_t bytes_transferred); 

protected: 

  static int nextPacketID_; 

};

#endif
