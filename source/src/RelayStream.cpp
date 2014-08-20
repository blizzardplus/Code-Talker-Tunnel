#include "RelayStream.h"
#include "Util.h"
#include "debug.h"

#include <boost/lexical_cast.hpp>

using namespace boost::asio; 

void RelayStream::write(unsigned char* buf, 
                        int len, StreamWriteHandler handler) 
{
  if (len <= 0) return;
  socket->async_write_some(buffer(buf, len), 
              boost::bind(handler, placeholders::error));
}

void RelayStream::read(StreamReadHandler handler) {
  socket->async_read_some(buffer(data, sizeof(data)),
                          boost::bind(&RelayStream::dataReadComplete, shared_from_this(), 
                                      handler, 
                                      placeholders::bytes_transferred, 
                                      placeholders::error));
}

void RelayStream::dataReadComplete(StreamReadHandler handler, 
				       std::size_t transferred,
				       const boost::system::error_code &err)
{
  if (err) handler(data, -1);
  else     handler(data, transferred);
}

void RelayStream::close(bool force) {
  DEBUG_FUNC;
  socket->close(); 
}




  
