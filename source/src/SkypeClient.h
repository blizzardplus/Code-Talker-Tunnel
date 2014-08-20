#ifndef SKYPE_CLIENT_H
#define SKYPE_CLIENT_H

/* 
 * SkypeClient - The manager class for Skype interfaces on the client side 
 */

#ifndef SKYPE_PAPI
#include "Skype.h"

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <string>
#include <stdint.h>

#include "skype-embedded_2.h"

class SkypeClient : public SkypeKitWrapper {
private: 
  SEString                  serverAccount;
  bool             done;

public: 
  /*
  SkypeClient(SEString accountName,
              SEString accountPwd,
              std::string a,
              uint16_t    p,
              uint16_t    sport,
              SEString    rtaddr,
              uint        rtport,
              std::string rtpath,
              std::string keyfilename,
              SEString svrAccount)
    : SkypeKitWrapper(accountName, accountPwd, a, p, sport, rtaddr, rtport, rtpath, keyfilename, serverAccount),
      serverAccount(svrAccount), done(false) { }
*/
  SkypeClient(std::string addr, uint16_t port,// uint16_t sport,
		  Config c,
              SEString svrAccount)
    : SkypeKitWrapper(addr, port, //sport,
    		c), serverAccount(svrAccount),done(false) {
  }



  void runOnce(std::string * svr_addr, uint16_t * srv_port, boost::shared_ptr<Codec> c);

  void handleServerInfo(std::string * ptr_addr, uint16_t * ptr_port, boost::shared_ptr<Codec> codec, std::string addr, uint16_t port);

};

#endif

#endif
