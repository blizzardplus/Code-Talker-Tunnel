#ifdef SKYPE_PAPI
#include "SkypePublicAPIWrapperServer.h"

///////SERVER SECTION

SkypePublicAPIWrapper* SPAPI_Obj;

void PAPIWrapper_setObject(SkypePublicAPIWrapper* Sobj)
{
	SPAPI_Obj = Sobj;
}

void msgCallback(const struct msg_callback* mcb ,const char *data)
{
	SPAPI_Obj->handleData(data);
};


SkypePAPIServer::SkypePAPIServer(const char* addr, int port, Server * svr): SkypePublicAPIWrapper(addr, port), state(SkypePAPIServer::READY), server(svr)
{
	//	PAPIWrapper_setObject(this);
	std::cerr << "[SkypeAPI] " << "Client is using port number " << port << ". Make sure Skype application is using the same port." << std::endl;
	PAPIWrapper_setObject(this);

	if(initialize(msgCallback))
		std::cerr << "[SkypeAPI] " << "Problem connecting to the running instance of Skype! Make sure Skype is running and you are signed in!" << std::endl;
	boost::thread & listener  = this->run();

};
void SkypePAPIServer::handleData(const char *data)
{
	char message[BUFSIZ];
	std::cerr<< "[SkypeApp] "<< ": Got from SkypeApp -> " << data << "\n";

	std::string s (data);

	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(" ");
	tokenizer tokens(s, sep);
	tokenizer::iterator beg=tokens.begin();

	if (state==SkypePAPIServer::READY)
	{
		//for (tokenizer::iterator tok_iter = tokens.begin(); tok_iter != tokens.end(); ++tok_iter)
		//	std::cout << "<" << *tok_iter << "\n";

		if(*beg=="CHAT")
		{
			char tmp[BUFSIZ];
			strcpy(tmp , (*(++beg)).c_str());
			++beg;

			if (*(beg) == "NAME")
			{
				strcpy(chatID , tmp);
				state=SkypePAPIServer::GET_INFO;
				sprintf(message, "GET CHAT %s CHATMESSAGES", chatID);
				toSkype(message);
			}
			else if (*beg == "ACTIVITY_TIMESTAMP")
			{
				strcpy(chatID , tmp);
				state=SkypePAPIServer::GET_INFO;
				sprintf(message, "GET CHAT %s CHATMESSAGES", chatID);
				toSkype(message);
			}

		}
	}

	else if (state==SkypePAPIServer::GET_INFO)
	{

		if(*beg=="CHAT")
		{
			std::string tmp(chatID);
			if (*(++beg) == tmp)
			{
				if(*(++beg) == "CHATMESSAGES")
				{
					state=SkypePAPIServer::SEND_INFO;
					if(strcmp(chatMsgID, (*(++beg)).c_str()))
					{
						strcpy(chatMsgID , (*beg).c_str());
						if(chatMsgID[(*(beg)).length()-1]==',')
							chatMsgID[(*(beg)).length()-1] = '\0';
						sprintf(message, "GET CHATMESSAGE %s BODY", chatMsgID);
						toSkype(message);
					}
				}
			}

		}
	}
	else if (state==SkypePAPIServer::SEND_INFO)
	{
		std::string tmp(chatMsgID);
		if( (*beg=="CHATMESSAGE") && (*(++beg) == tmp) && (*(++beg) == "BODY"))
		{
			KeyMsg   imsg((*(++beg)).c_str(), (*(++beg)).length());
			if (imsg.getType() == KeyMsg::KM_CLIENT_KEY) {
				boost::shared_ptr<Codec> cdc(new Codec(true));
				this->codec = cdc;
				this->updateInfo(imsg.getAddr(), imsg.getPort());
				this->codec->genSharedKey(imsg.getKey());
				this->state = SkypePAPIServer::SEND_INFO1;

				uint8_t ourkey[32];
				this->codec->genPublicKey(ourkey);

				KeyMsg  omsg(addr, port, ourkey, KeyMsg::KM_SERVER_KEY);
				sprintf(message, "CHATMESSAGE %s %s", chatID, (const char*)omsg);
				toSkype(message);

			}
			else
			{
#ifdef WIN32
				Sleep(1000);
#else
				sleep(1);
#endif


				state=SkypePAPIServer::GET_INFO;
				sprintf(message, "GET CHAT %s CHATMESSAGES", chatID);
				toSkype(message);
			}
		}
	}
	else if (state==SkypePAPIServer::SEND_INFO1)
	{
		if( (*beg=="CHATMESSAGE"))
		{
			strcpy(chatMsgID , (*(++beg)).c_str());
			if((*(++beg) == "STATUS") && (*(++beg) == "RECEIVED"))
			{
				state=SkypePAPIServer::SEND_INFO2;
				sprintf(message, "GET CHATMESSAGE %s BODY", chatMsgID);
				toSkype(message);
			}
		}
	}
	else if (state==SkypePAPIServer::SEND_INFO2)
	{

		std::string tmp(chatMsgID);
		if( (*beg=="CHATMESSAGE") && (*(++beg) == tmp) && (*(++beg) == "BODY"))
		{
			KeyMsg   imsg((*(++beg)).c_str(), (*(++beg)).length());
			if (imsg.getType() == KeyMsg::KM_CLIENT_ACK) {
				uint8_t ourkey[32];
				this->codec->genPublicKey(ourkey);

				if(!memcmp(ourkey, imsg.getKey(), 32)) {
					std::cerr << "[SkypeApp] " << "key successfully obtained!" << std::endl;

					sprintf(message, "CHATMESSAGE %s OK", chatID);
					toSkype(message);
#ifdef WIN32
					Sleep(2000);
#else
					sleep(2);
#endif

					this->state = SkypePAPIServer::SEND_OK;
				}
				else {
					std::cerr << "[SkypeApp] " << "Key hash does not match!" << std::endl;

					toSkype("Unsuccessful handshake");

					this->state = SkypePAPIServer::READY;
				}
			}
		}
	}
	else if  (state==SkypePAPIServer::SEND_OK)
	{
		if( (*beg=="CALL") && (*(++(++beg)) == "STATUS") && (*(++beg) == "MISSED"))
		{
#ifdef TPROXY_ENABLED
			//TODO: Do not log out.
#else
			toSkype("SET USERSTATUS OFFLINE");
#endif
			done=true;
			server->newClientSession(theirAddr,theirPort,codec);
			server->wakeUp();
		}
	}
	cv.notify_all();
}
#endif
