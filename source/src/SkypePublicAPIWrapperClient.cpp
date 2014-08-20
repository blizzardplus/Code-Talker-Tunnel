#ifdef SKYPE_PAPI
#include "SkypePublicAPIWrapperClient.h"
SkypePublicAPIWrapper* SPAPI_Obj;

void PAPIWrapper_setObject(SkypePublicAPIWrapper* Sobj)
{
	SPAPI_Obj = Sobj;
}

void msgCallback(const struct msg_callback* mcb ,const char *data)
{
	SPAPI_Obj->handleData(data);
};

///////CLIENT SECTION
SkypePAPIClient::SkypePAPIClient(const char* srvID, const char* addr, int port): SkypePublicAPIWrapper(addr, port), serverID(srvID),state(SkypePAPIClient::INITIALIZE)
{
	//	PAPIWrapper_setObject(this);
	std::cerr << "[SkypeAPI] " << "Client is using port number " << port << ". Make sure Skype application is using the same port." << std::endl;
	PAPIWrapper_setObject(this);

	if(initialize(msgCallback))
		std::cerr << "[SkypeAPI] " << "Problem connecting to the running instance of Skype! Make sure Skype is running and you are signed in!" << std::endl;
	boost::thread & listener  = this->run();
};


//char *EnumString[] = {"INITIALIZE","READY","SEND_INFO","SEND_INFO_1","SEND_INFO_2","RCV_INFO","SEND_KEY_HASH","RCV_OK","CALL","DONE"};
void SkypePAPIClient::handleData(const char *data)
{
	char message[BUFSIZ];
	char tmp[BUFSIZ];
	int i;

	std::cerr<< "[SkypeApp] " /*<< EnumString[state]*/ << ": Got from SkypeApp -> " << data << "\n";

	if (state== SkypePAPIClient::INITIALIZE)
	{
#ifdef WIN32
		if(!strcmp("authorization success", data))
			state = SkypePAPIClient::READY;
#else
		if(!strcmp("PROTOCOL 7", data))
			state = SkypePAPIClient::READY;
#endif
	}
	else if(state == SkypePAPIClient::READY)
	{

		if(strlen(data)<18 || strncmp(data,"CHAT #",6))
			return;
		state = SkypePAPIClient::RCV_INFO;
		strncpy(chatID,data + 5,strlen(data)-18);
		chatID[strlen(data)-19]='\0';
		sprintf(message,"OPEN CHAT %s",chatID);
		toSkype(message);

		char message2[BUFSIZ];
		uint8_t ourkey[32];
		codec->genPublicKey(ourkey);
		KeyMsg msg(addr, port, ourkey, KeyMsg::KM_CLIENT_KEY);
		sprintf(message2,"CHATMESSAGE %s %s", chatID, (const char*)(msg));
		toSkype(message2);
	}
	else if (state==SkypePAPIClient::RCV_INFO)
	{
		if(strlen(data)<11)
			return;
		/*
		if(!strncmp(data,"CHATMESSAGE",11))
		{
			while (isdigit(data[i]))
				i++;
			strncpy(tmp,data+12,i-12);
			if(!strcmp(data+i+1,"STATUS SENT"))
				state=SkypePAPIClient::SEND_INFO_2;
		}

		char tmp[BUFSIZ];
		int i = 12;
		 */
		if(!strncmp(data,"CHAT ",5))
		{
			char tmp1[BUFSIZ];
			sprintf(tmp, "CHAT %s ACTIVITY_TIMESTAMP",chatID);
			sprintf(tmp1, "CHAT %s CHATMESSAGES",chatID);
			if(!strncmp(data,tmp,strlen(tmp)))
			{
				sprintf(message,"GET CHAT %s CHATMESSAGES",chatID);
				toSkype(message);
			}
			else if(!strncmp(data,tmp1,strlen(tmp1)))
			{
				char tmp2[BUFSIZ];
				i = strlen(tmp1)+1;
				while (isdigit(data[i]))
					i++;
				strncpy(tmp2,data+strlen(tmp1)+1,i);
				tmp2[i-strlen(tmp1)-1]='\0';
				sprintf(message,"GET CHATMESSAGE %s BODY",tmp2);
				toSkype(message);
			}
		}
		else if((!strncmp(data,"MESSAGE ",8)) || (!strncmp(data,"CHATMESSAGE ",12)))
		{
			memset(tmp, 0, sizeof(tmp));
			int j=(!strncmp(data,"MESSAGE ",8))?8:12;
			i=j;
			while (isdigit(data[i]))
				i++;
			strncpy(tmp,data+j,i-j);
			if(!strcmp(data+i+1,"STATUS RECEIVED"))
			{
				//state=SkypePAPIClient::RCV_INFO;
				sprintf(message,"GET CHATMESSAGE %s BODY",tmp);
				printf("This is msg!!!:\"%s\"\n",message);
				toSkype(message);
			}
			else if(!strncmp(data+i+1,"BODY",4))
			{
				char msg[BUFSIZ];
				strcpy(msg,data+i+6);
				KeyMsg   imsg((const char *)msg, strlen(msg));
				if (imsg.getType() == KeyMsg::KM_SERVER_KEY) {
					this->updateInfo(imsg.getAddr(), imsg.getPort());
					this->invokeHandler();
					this->codec->genSharedKey(imsg.getKey());
					//std::cerr<< "GOT : " << imsg.getAddr() << imsg.getPort() << std::endl;
					KeyMsg omsg("0.0.0.0", 0, imsg.getKey(), KeyMsg::KM_CLIENT_ACK);
					state=SkypePAPIClient::RCV_OK;
					sprintf(message,"CHATMESSAGE %s %s", chatID, (const char*)(omsg));
					toSkype(message);
				}
				else {
					std::cerr<< "[SkypeApp] : " << "ignore message " << data << std::endl;
				}
			}
		}
	}
	else if (state==SkypePAPIClient::RCV_OK)
	{
		if((!strncmp(data,"MESSAGE ",8)) || (!strncmp(data,"CHATMESSAGE ",12)))
		{
			int j=(!strncmp(data,"MESSAGE ",8))?8:12;
			i=j;
			while (isdigit(data[i]))
				i++;
			strncpy(tmp,data+i,i-j);
			if(!strcmp(data+i+1,"STATUS RECEIVED"))
			{
				state=SkypePAPIClient::RCV_OK;
				sprintf(message,"GET CHATMESSAGE %s BODY",tmp);
				toSkype(message);
			}
			else if(!strncmp(data+i+1,"BODY",4))
			{
				char msg[BUFSIZ];
				strcpy(msg,data+i+6);
				if(!strcmp("OK", msg))
				{
					state=SkypePAPIClient::CALL;
					srand((unsigned)time(0));
					// Block for some seconds
					sprintf(message,"call %s", serverID);
					toSkype(message);
				}
			}
		}
		else if(!strncmp(data,"CHAT ",5))
		{
			char tmp1[BUFSIZ];
			sprintf(tmp, "CHAT %s ACTIVITY_TIMESTAMP",chatID);
			sprintf(tmp1, "CHAT %s CHATMESSAGES",chatID);
			if(!strncmp(data,tmp,strlen(tmp)))
			{
				sprintf(message,"GET CHAT %s CHATMESSAGES",chatID);
				toSkype(message);
			}
			else if(!strncmp(data,tmp1,strlen(tmp1)))
			{
				char tmp2[BUFSIZ];
				i = strlen(tmp1)+1;
				while (isdigit(data[i]))
					i++;
				strncpy(tmp2,data+strlen(tmp1)+1,i);
				tmp2[i-strlen(tmp1)-1]='\0';
				sprintf(message,"GET CHATMESSAGE %s BODY",tmp2);
				toSkype(message);
			}
		}
		else
		{
			sprintf(message,"GET CHAT %s CHATMESSAGES",chatID);
			toSkype(message);
		}
	}
	else if (state==SkypePAPIClient::CALL)
	{
		char tmp[BUFSIZ] = "";
		int i = 5;
		if(!strncmp(data,"CALL",4))
		{
			while (isdigit(data[i]))
				i++;
			strncpy(tmp,data+5,i-5);
			if(!strcmp(data+i+1,"STATUS RINGING"))
			{
#ifdef WIN32
				Sleep((rand()%20+5)*1000);
#else
				sleep(rand()%20+5);
#endif
				sprintf(message,"ALTER CALL %s HANGUP", tmp);
				toSkype(message);
#ifdef TPROXY_ENABLED
				//TODO: Do not log out.
#else
#ifdef OPENWRT_ENABLED
				//TODO: Do not log out.
#else
				toSkype("SET USERSTATUS OFFLINE");
#endif
#endif
				done=true;
			}
		}

	}
	cv.notify_all();
};

void SkypePAPIClient::handleServerInfo(std::string * ptr_addr, uint16_t * ptr_port, boost::shared_ptr<Codec> codec,
		std::string addr, uint16_t port) {

	assert(ptr_addr && ptr_port);
	*ptr_addr = addr;
	*ptr_port = port;
	std::cerr << "[SkypeApp] " << "handleServerInfo " << addr << ":" << port << std::endl;
}

void SkypePAPIClient::runOnce(std::string * srv_addr, uint16_t * srv_port, boost::shared_ptr<Codec> & c)
{
	boost::unique_lock<boost::mutex> lock(mx);
	while(state != SkypePAPIClient::READY)
		cv.wait(lock);

	handler = boost::bind(&SkypePAPIClient::handleServerInfo,this, srv_addr, srv_port, _1, _2, _3);

	this->codec = c;
	char message[BUFSIZ];
	sprintf(message,"CHAT CREATE %s",serverID);
	toSkype(message);
	cv.notify_all();
	while (!this->done) cv.wait(lock);
	std::cerr<< "[SkypeApp] "<< "Ending the loop!" << "\n";
	endLoop();
}


#endif
