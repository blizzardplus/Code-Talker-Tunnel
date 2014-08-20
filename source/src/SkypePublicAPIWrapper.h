#ifdef SKYPE_PAPI

#ifndef SKYPEPUBLICAPIWRAPPER_H_
#define SKYPEPUBLICAPIWRAPPER_H_

#include "SkypePublicAPI.h"


#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/tokenizer.hpp>
#include <string>
#include <stdint.h>
#include "Crypto.h"
#include "KeyMsg.h"
#include "Util.h"
#include "debug.h"

#ifdef WIN32
#include <windows.h>
#endif

typedef boost::function<void (boost::shared_ptr<Codec> codec, std::string addr,
		uint16_t port
)> SkypeInfoHandler;


class SkypePublicAPIWrapper {
protected:
	char chatID[BUFSIZ];
	char chatMsgID[BUFSIZ];
	SkypeInfoHandler         handler;
	std::string              addr;
	uint16_t                 port;
	std::string              theirAddr;
	uint16_t                 theirPort;
	boost::shared_ptr<Codec> codec;
	bool 					done;
	mutable boost::mutex      mx;
	mutable boost::mutex      msg_hndl_mx;
	boost::condition_variable cv;
	boost::thread        thread;

public:
	SkypePublicAPIWrapper(const char* addr, int port): addr(addr),port(port), done(false){};
	virtual void handleData(const char *data){};
	void toSkype(const char* msg)
	{
		std::cout<< "[SkypeApp] "<< ": Sending to SkypeApp -> " << msg << "\n";
		sendToSkype(msg);
	}
	void runOnce(std::string * svr_addr, uint16_t * srv_port, boost::shared_ptr<Codec> & c){};
	void updateInfo(std::string addr, uint16_t port) {
		theirAddr = addr;
		theirPort = port;
	}
	boost::thread & run()
	{
		thread = boost::thread(&SkypePublicAPIWrapper::mainLoop, this);
		return thread;
	}
	void mainLoop(){
#ifndef WIN32
		toSkype("NAME Morpher");
		toSkype("PROTOCOL 7");
#endif
		startLoop();
	}
	void invokeHandler() {
		handler(this->codec, theirAddr, theirPort);
	};
};




#endif /* SKYPEPUBLICAPIWRAPPER_H_ */
#endif
