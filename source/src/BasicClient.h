#ifndef CLIENT_H
#define CLIENT_H

/* 
 * Basic Client class (responsible for sending and recieving data)
 */

#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <deque>
#include <cstdlib>

#include "Packet.h"

using namespace boost::asio; 

typedef boost::function<void (const boost::system::error_code&, 
                              std::size_t)> ClientSendHandler;


class BasicClient {
private: 
  boost::shared_ptr<ip::udp::socket> _udpsocket; 
  std::deque<UDPPacket *> _msg_send_q; // msg to be sent to client 
  std::deque<UDPPacket *> _msg_recv_q; // msg recved from client 
  mutable boost::mutex _recv_q_mutex;
  mutable boost::mutex _send_q_mutex;
  boost::condition_variable _cv_recv_q;
  boost::condition_variable _cv_send_q;
  ip::udp::endpoint _endpoint; 
  int _nextPktId; 
  bool _running; 

public: 
  BasicClient(boost::shared_ptr<ip::udp::socket> & s, 
         boost::asio::ip::udp::endpoint & ep) 
    : _udpsocket(s), _endpoint(ep), _nextPktId(0), _running(true)  
  {	 }

  void push_recv_msg(unsigned char * data, size_t len);
  UDPPacket * pop_recv_msg();

  void push_send_msg(UDPPacket * pkt); // FIXME: needed? 
  UDPPacket * pop_send_msg();          // FIXME: needed? 


  void async_send(UDPPacket * rawpkt, ClientSendHandler handler); 

  void send(UDPPacket * rawpkt);

  boost::thread thread; 

  ip::udp::endpoint endpoint() {
    return _endpoint;
  }


private:
  void run();                   // FIXME: needed? 

  void packetSendComplete(UDPPacket * udppkt, const boost::system::error_code& error, 
                          std::size_t bytes_transferred);
    
};

#endif
 
