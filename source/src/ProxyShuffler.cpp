#include "ProxyShuffler.h"
#include "Util.h"
#include "debug.h"

#include <boost/enable_shared_from_this.hpp>

using namespace boost::asio;

ProxyShuffler::ProxyShuffler(boost::shared_ptr<TunnelStream> socks,
                             boost::shared_ptr<TunnelStream> relay, 
                             boost::asio::io_service::work * wrk)
  : socks(socks), relay(relay), closed(false), work(wrk) 
{}

void ProxyShuffler::shuffle() {
  std::cerr << "proxy relaying..." << std::endl;

  socks->read(boost::bind(&ProxyShuffler::readComplete, shared_from_this(), 
                          socks, relay, socksReadBuffer, true,  _1, _2));

  relay->read(boost::bind(&ProxyShuffler::readComplete, shared_from_this(),
                          relay, socks, relayReadBuffer, false, _1, _2));
}

void ProxyShuffler::readComplete(boost::shared_ptr<TunnelStream> thisStream,
                                 boost::shared_ptr<TunnelStream> thatStream,
                                 unsigned char* thisBuffer, bool inSocks, 
                                 unsigned char* buf, std::size_t transferred) 
{
  if (closed) return;

  if (transferred == -1) {
    close(!inSocks);
    return;
  }

  assert(transferred <= PROXY_BUF_SIZE);
  memcpy(thisBuffer, buf, transferred);

#ifdef DEBUG_SHUFFLER
  if(inSocks) {
    std::cerr << " ======> " << transferred << "\tbytes" << std::endl;
  }
  else {
    std::cerr << " <====== " << transferred << "\tbytes" << std::endl;
  }
#endif

  thatStream->write(thisBuffer, transferred,
                    boost::bind(&ProxyShuffler::writeComplete, shared_from_this(),
                                thisStream, thatStream, thisBuffer, inSocks, 
                                placeholders::error));
}

void ProxyShuffler::writeComplete(boost::shared_ptr<TunnelStream> thisStream,
                                  boost::shared_ptr<TunnelStream> thatStream,
                                  unsigned char *buf, bool inSocks, 
                                  const boost::system::error_code &err)
{
  if (closed) return;

  if (err) {
    std::cerr << "ProxyShuffler::writeComplete error: " << err << std::endl;
    close();
    return;
  }

  thisStream->read(boost::bind(&ProxyShuffler::readComplete, shared_from_this(),
                               thisStream, thatStream, buf, inSocks, _1, _2));
}

void ProxyShuffler::close(bool force) {
  if (!closed) {
    relay->close(force);
    socks->close(force);
    closed = true;
    if (work) {
      delete work;
      work = 0; 
    }
  }
}
