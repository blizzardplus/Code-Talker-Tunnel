#ifdef SKYPE_PAPI

#ifndef SKYPEPUBLICAPIWRAPPERCLIENT_H_
#define SKYPEPUBLICAPIWRAPPERCLIENT_H_

#include "SkypePublicAPIWrapper.h"

class SkypePAPIClient: public SkypePublicAPIWrapper
{
public:
	enum ClientState {
		INITIALIZE,
		READY,
		SEND_INFO,
		SEND_INFO_1,
		SEND_INFO_2,
		RCV_INFO,
		SEND_KEY_HASH,
		RCV_OK,
		CALL,
		DONE
	};
	ClientState state;
	SkypePAPIClient(const char* srvID, const char* addr, int port);
	void handleData(const char *data);
	void runOnce(std::string * srv_addr, uint16_t * srv_port, boost::shared_ptr<Codec> & c);
	void handleServerInfo(std::string * ptr_addr, uint16_t * ptr_port , boost::shared_ptr<Codec> codec, std::string addr, uint16_t port);

private:
	const char* serverID;

};



#endif /* SKYPEPUBLICAPIWRAPPER_H_ */
#endif
