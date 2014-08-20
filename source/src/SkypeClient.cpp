#ifndef SKYPE_PAPI
#include "SkypeClient.h"
#include "Crypto.h"
#include "KeyMsg.h"
#include "Util.h"
#include "debug.h"

#include <cstring>
#include <cassert>
#include <iostream>
#include <string>
#include <boost/function.hpp>
#include <boost/bind.hpp>



void SkypeClient::runOnce(std::string * srv_addr, 
		uint16_t * srv_port,
		boost::shared_ptr<Codec> codec) {
	std::cerr << "[skype] " << "Creating skype" << std::endl;

	// exec skrt
	skype = new SMClientSkype(this, codec, boost::bind(&SkypeClient::handleServerInfo,
			this, srv_addr, srv_port, _1, _2, _3));

	std::cerr << "[skype] " << "Submitting application token" << std::endl;

	readKeyPair();
	skype->init(keyBuf, skrtAddr, skrtPort, "skypeclnt.log");
	skype->start();

	std::cerr << "[skype] " << "Retrieving account" << std::endl;



	if(skypeLogin()) {
		SMClientSkype * ck = dynamic_cast<SMClientSkype *>(skype);
		assert(ck);
		skype->GetConversationList(ck->inbox, Conversation::INBOX_CONVERSATIONS);

		fetch(ck->inbox);
		ContactGroup::Ref skypeNamesContactGroup;
		skype->GetHardwiredContactGroup(ContactGroup::SKYPE_BUDDIES, skypeNamesContactGroup);

		MyContact::Refs contactList;
		skypeNamesContactGroup->GetContacts(contactList);
		fetch(contactList);

		std::cerr << "[skype] " << "waiting for the server to become online" << std::endl;
		skype->wrapper->pingServer();
		//clientSendInfo(codec);

		{
			boost::unique_lock<boost::mutex> lock(mx);
			while (!this->done) cv.wait(lock);
		}
#ifdef TPROXY_ENABLED
		return;
#else
		skypeLogout();
#endif
	}
	else
	{
		std::cerr << "[skype] " << "Login failed" << std::endl;
	}
	std::cerr << "[skype] " << "finished" << std::endl;
	skype->stop();
	delete skype;

}


void SkypeClient::handleServerInfo(std::string * ptr_addr, uint16_t * ptr_port, boost::shared_ptr<Codec> codec,
		std::string addr, uint16_t port) {

	assert(ptr_addr && ptr_port);
	*ptr_addr = addr;
	*ptr_port = port;
	std::cerr << "[skype] " << "handleServerInfo " << addr << ":" << port << std::endl;
	{
		boost::unique_lock<boost::mutex> lock(mx);

		this->done = true;
		cv.notify_all();
	}
}


#endif
