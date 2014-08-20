#ifndef SKYPE_SERVER_H
#define SKYPE_SERVER_H
#ifndef SKYPE_PAPI
/* 
 * SkypeServer - The manager class for Skype interfaces on the server side 
 */

#include "Skype.h"

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <string>
#include <stdint.h>
#include "Config.h"

#include "skype-embedded_2.h"

#include "Server.h"

class SkypeServer : public SkypeKitWrapper {
private: 
  boost::shared_ptr<Server> server; 
  boost::thread        thread; 
  bool                 running; 
  bool                 done; 
  bool				   logged_in;

public: 

  SkypeServer(SEString accountName, 
              SEString accountPwd, 
              std::string a, 
              uint16_t    p, 
              uint16_t    sport,
              SEString    rtaddr, 
              uint        rtport, 
              std::string rtpath, 
              std::string keyfilename, 
              boost::shared_ptr<Server> & svr) 
    : SkypeKitWrapper(accountName, accountPwd, a, p, sport, rtaddr, rtport, rtpath, keyfilename, serverAccount),
      server(svr), running(false), done(false), logged_in(false)
  { }


  SkypeServer(std::string addr, uint16_t port,// uint16_t sport,
		  Config c,
              Server * svr) 
    : SkypeKitWrapper(addr, port,// sport,
    		c), server(svr), running(false), done(false) { }

  boost::thread & run(); 

  void stop(); 

  void handleClientInfo(boost::shared_ptr<Codec> codec, std::string addr, uint16_t port); 

private: 

  void serverSession(); 

};

#endif

#endif
