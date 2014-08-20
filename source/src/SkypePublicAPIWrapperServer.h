#ifdef SKYPE_PAPI

#ifndef SKYPEPUBLICAPIWRAPPERSERVER_H_
#define SKYPEPUBLICAPIWRAPPERSERVER_H_

#include "SkypePublicAPIWrapper.h"
#include "Server.h"


class SkypePAPIServer: public SkypePublicAPIWrapper
{
private:
	  boost::shared_ptr<Server> server;
public:
	enum ServerState {
		READY,
		GET_INFO,
		SEND_INFO,
		SEND_INFO1,
		SEND_INFO2,
		SEND_INFO3,
		SEND_OK
	};
	ServerState state;
	SkypePAPIServer(const char* addr, int port, Server * svr);
	void handleData(const char *data);
//	void runOnce(std::string * svr_addr, uint16_t * srv_port, boost::shared_ptr<Codec> & c);

};

#endif /* SKYPEPUBLICAPIWRAPPER_H_ */
#endif
