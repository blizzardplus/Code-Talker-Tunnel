#include "BasicClient.h"
#include "Util.h"
#include "debug.h"


using namespace boost::asio; 

void BasicClient::push_recv_msg(unsigned char * data, size_t len) { 
  UDPPacket * udppkt = new UDPPacket; 
  udppkt->id = _nextPktId++; 
  udppkt->length = len; 
  //std::cerr << "Pushing a message of size " << len << " for client" << _endpoint << std::endl;

  assert(len <= sizeof(udppkt->data)); 
  memcpy(udppkt->data, data, len); 

  {
    boost::unique_lock<boost::mutex> lock(_recv_q_mutex);
    _msg_recv_q.push_back(udppkt);
  }
  _cv_recv_q.notify_all(); 
}


UDPPacket * BasicClient::pop_recv_msg() {
  boost::unique_lock<boost::mutex> lock(_recv_q_mutex);

  while (_msg_recv_q.empty()) {
    // std::cerr << "pop_recv_msg waiting..." << std::endl;
    _cv_recv_q.wait(lock); 
  }
  
  UDPPacket * udppkt = _msg_recv_q.front();
  //std::cerr << "Popping a message of size " << udppkt->length << std::endl;
  _msg_recv_q.pop_front(); 
  return udppkt;
}

void BasicClient::push_send_msg(UDPPacket * pkt) {
  if (!pkt) return; 

  {
    boost::unique_lock<boost::mutex> lock(_send_q_mutex);
    _msg_send_q.push_back(pkt);
  }
  _cv_send_q.notify_all();
}

UDPPacket * BasicClient::pop_send_msg() {
  boost::unique_lock<boost::mutex> lock(_send_q_mutex);

  while (_msg_send_q.empty()) {
    _cv_send_q.wait(lock); 
  }
  
  UDPPacket * udppkt = _msg_send_q.front();
  _msg_send_q.pop_front(); 
  return udppkt;
}

void BasicClient::async_send(UDPPacket * udppkt, ClientSendHandler handler) {
  _udpsocket->async_send_to(boost::asio::buffer(udppkt->data, udppkt->length), 
                            _endpoint, handler);
}

void BasicClient::send(UDPPacket * udppkt) {
  _udpsocket->send_to(boost::asio::buffer(udppkt->data, udppkt->length), _endpoint); 
}

void BasicClient::run() {
  while (_running) {
    UDPPacket * pkt = pop_send_msg(); 

    async_send(pkt, boost::bind(&BasicClient::packetSendComplete, 
                                this, pkt, placeholders::error, 
                                placeholders::bytes_transferred));

  }
}


void BasicClient::packetSendComplete(UDPPacket * udppkt, 
                                    const boost::system::error_code& error, 
                                    std::size_t bytes_transferred) {
  delete udppkt;
}

