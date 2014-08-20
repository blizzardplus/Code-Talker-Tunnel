#ifndef SKYPE_PAPI
#include "SkypeServer.h"
#include "Crypto.h"
#include "Util.h"
#include "debug.h"

#include <cstring>
#include <cassert>
#include <iostream>
#include <string>
#include <boost/function.hpp>
#include <boost/bind.hpp>



void SkypeServer::serverSession() {
	while (running) {

		std::cerr << "[skype] " << "Creating skype" << std::endl;

		// exec skrt

		boost::shared_ptr<Codec> codec(new Codec(true));

		std::string clientAddr;
		uint16_t    clientPort;
		skype = new SMServerSkype(this, codec, boost::bind(&SkypeServer::handleClientInfo,
				this, _1, _2, _3));

		std::cerr << "[skype] " << "Submitting application token" << std::endl;

		readKeyPair();
		skype->init(keyBuf, skrtAddr, skrtPort, "skypesrv.log");
		skype->start();

		std::cerr << "[skype] " << "Retrieving account" << std::endl;


		if(skypeLogin())
		{
			logged_in=true;
			SMServerSkype * sk = dynamic_cast<SMServerSkype *>(skype);
			assert(sk);
			skype->GetConversationList(sk->inbox, Conversation::INBOX_CONVERSATIONS);
			fetch(sk->inbox);
			{
				boost::unique_lock<boost::mutex> lock(mx);

				while (!done) cv.wait(lock);
			}
#ifndef TPROXY_ENABLED
			skypeLogout();
			cv.notify_all();
			server->wakeUp();
#endif
		}
		else
		{
			std::cerr << "[skype] " << "Login failed" << std::endl;
		}
		skype->stop();
		delete skype; running = false;
	}
}

boost::thread & SkypeServer::run() {
	running = true;

	thread = boost::thread(&SkypeServer::serverSession, this);
	return thread;
}

void SkypeServer::stop() {
	running = false;
	thread.interrupt();
	thread.join();
}


void SkypeServer::handleClientInfo(boost::shared_ptr<Codec> codec,  
		std::string addr, uint16_t port) {
	std::cerr << "[skype] " << "handleClientInfo " << addr << ":" << port << std::endl;
	{
		server->newClientSession(addr, port, codec);
#ifndef TPROXY_ENABLED
		boost::unique_lock<boost::mutex> lock(mx);
		done = true;
		cv.notify_all();
#endif

	}


}

#endif
