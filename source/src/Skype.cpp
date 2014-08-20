#ifndef SKYPE_PAPI

#include "Skype.h"
#include "Crypto.h"
#include "KeyMsg.h" 
#include "Util.h" 

#include <cstdio>
#include <cassert>
#include <iostream>
#include <string>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "skype-embedded_2.h"

using namespace Sid;

void MyContact::OnChange(int prop)
{
	if (prop == Contact::P_AVAILABILITY)
	{
		SEString name;
		GetPropSkypename(name);
		//printf("%s:%s\n",(const char*) name, (const char*)skype->wrapper->serverAccount);
		fflush(stdout);
		if(!strcmp((const char*) name, (const char*)skype->wrapper->serverAccount))
		{
			Contact::AVAILABILITY availability;
			GetPropAvailability(availability);
			if(availability == Contact::ONLINE && skype->state != SMClientSkype::DONE)
			{
				SEString statusAsText;
				statusAsText = tostring(availability);

				std::cerr << "[skype] " <<(const char*)name << "->" << (const char*)statusAsText << "\n";
				skype->wrapper->clientSendInfo(skype->codec);
			}
		}
	};
};

void SMAccount::OnChange(int prop) {
	if (prop == Account::P_STATUS) {
		Account::STATUS loginStatus;

		this->GetPropStatus(loginStatus);
		if (loginStatus == Account::LOGGED_IN) {
			boost::unique_lock<boost::mutex> lock(mx);
			loggedIn  = true;
			cv.notify_all();
		}
		else if (loginStatus == Account::LOGGED_OUT) {
			boost::unique_lock<boost::mutex> lock(mx);
			loggedIn = false;
			Account::LOGOUTREASON whyLogout;
			this->GetPropLogoutreason(whyLogout);
			if (whyLogout != Account::LOGOUT_CALLED) {

				std::cerr << "[skype] " << "logged out because:" << (const char *)tostring(whyLogout) << std::endl;

			}
			cv.notify_all();
		}
	}
}

void SMAccount::BlockWhileLoggingIn () {
	boost::unique_lock<boost::mutex> lock(mx);
	while (!this->loggedIn ) {
		cv.wait(lock);
	}

	if (!this->loggedIn)
	{
		std::cerr << "[skype] " << "Login failed.\n";
	}
}

void SMAccount::BlockWhileLoggingOut () {
	boost::unique_lock<boost::mutex> lock(mx);
	cv.notify_all();
	while (this->loggedIn) {
		cv.wait(lock);
	}
}

void SMClientSkype::OnConversationListChange(
		const ConversationRef &conversation,
		const Conversation::LIST_TYPE &type,
		const bool &added)
{
	// if conversation was added to inbox and was not in there already (just in case..)
	// -> append to our own inbox list
	if ( (type == Conversation::INBOX_CONVERSATIONS) && (added) && (!inbox.contains(conversation)) )
	{
		inbox.append(conversation);
	}
}

void SMServerSkype::OnConversationListChange(
		const ConversationRef &conversation,
		const Conversation::LIST_TYPE &type,
		const bool &added)
{
	// if conversation was added to inbox and was not in there already (just in case..)
	// -> append to our own inbox list
	if ( (type == Conversation::INBOX_CONVERSATIONS) && (added) && (!inbox.contains(conversation)) )
	{
		inbox.append(conversation);
	}
	if (type == Conversation::LIVE_CONVERSATIONS)
	{
		Conversation::LOCAL_LIVESTATUS liveStatus;
		conversation->GetPropLocalLivestatus(liveStatus);
		SEString liveStatusAsString = tostring(liveStatus);
		//printf("OnConversationListChange : %s\n", (const char*)liveStatusAsString);

		if (liveStatus == Conversation::RINGING_FOR_ME)
		{
			std::cerr << "[skype] " << "RING RING..\n";
			//std::cerr << "[skype] " << "Picking up call from MySkype::OnConversationListChange\n";
			// Saving the currently live conversation reference..
			liveSession = conversation->ref();
			liveSession.fetch();
			//liveSession->PickUpCall();
		};
	};
}

void SMClientConversation::OnMessage(const MessageRef & message) {
	Message::TYPE messageType;
	message->GetPropType(messageType);

	if (messageType == Message::POSTED_TEXT) {
		Respond(message);
	}
}


void SMServerConversation::OnMessage(const MessageRef & message) {
	Message::TYPE messageType;
	message->GetPropType(messageType);

	if (messageType == Message::POSTED_TEXT) {
		Respond(message);
	}
}

void SMServerConversation::Respond(const MessageRef & message) {
	SEIntList propIds;
	propIds.append(Message::P_AUTHOR);
	propIds.append(Message::P_BODY_XML);

	SEIntDict propValues = message->GetProps(propIds);
	SEString iuser = propValues[0];
	SEString itext = propValues[1];

	Message::Ref reply;
	if (iuser != skype->wrapper->skypeAccount) {

		std::cerr << "[skype] "
				<< (const char *)iuser  << " : "       // P_AUTHOR
				<< (const char *)itext  << std::endl;  // P_BODY_XML

		KeyMsg   imsg((const char *)itext, itext.size());

		if (skype->state == SMServerSkype::READY) {
			if (imsg.getType() == KeyMsg::KM_CLIENT_KEY) {
				skype->updateInfo(imsg.getAddr(), imsg.getPort());
				boost::shared_ptr<Codec> _codec(new Codec(true));
				skype->codec = _codec;
				skype->codec->genSharedKey(imsg.getKey());
				skype->state = SMServerSkype::SEND_INFO;

				uint8_t ourkey[32];
				skype->codec->genPublicKey(ourkey);

				KeyMsg  omsg(skype->wrapper->addr, skype->wrapper->port,
						ourkey, KeyMsg::KM_SERVER_KEY);
				SEString otext((const char *)omsg);

				std::cerr << "[skype] " << "sending : "
						<< (const char *)otext << std::endl;
				this->PostText(otext, reply, false);

				skype->state = SMServerSkype::RECV_ACK;

			}
			else {
				std::cerr << "[skype] " << "ignore message from "
						<< (const char *)iuser << std::endl;
			}
		}
		else {
			assert(skype->state == SMServerSkype::RECV_ACK);
			if (imsg.getType() == KeyMsg::KM_CLIENT_ACK) {
				uint8_t ourkey[32];
				skype->codec->genPublicKey(ourkey);

				if(!memcmp(ourkey, imsg.getKey(), 32)) {
					std::cerr << "[skype] " << "key successfully obtained!" << std::endl;
					std::cerr << "[skype] " << "sending : OK" << std::endl;


					this->PostText("OK", reply, false);
					sleep(2);
					skype->state = SMServerSkype::DONE;


				}
				else {
					std::cerr << "[skype] " << "Key hash does not match!" << std::endl;
					std::cerr << "[skype] " << "sending : Unsuccessful handshake" << std::endl;
					this->PostText("Unsuccessful handshake", reply, false);

					skype->state = SMServerSkype::RECV_ACK;
				}
			}
			else {
				std::cerr << "[skype] " << "ignore message from "
						<< (const char *)iuser << std::endl;
			}
		}
	}
}






void SMClientConversation::Respond(const MessageRef & message)
{
	SEIntList propIds;
	propIds.append(Message::P_AUTHOR);
	propIds.append(Message::P_BODY_XML);
	SEIntDict propValues = message->GetProps(propIds);
	SEString iuser = propValues[0];
	SEString itext = propValues[1];

	Message::Ref reply;

	if (iuser != skype->wrapper->skypeAccount)
	{
		std::cerr << "[skype] "
				<< (const char *)iuser  << " : "       // P_AUTHOR
				<< (const char *)itext  << std::endl;  // P_BODY_XML

		if (skype->state == SMClientSkype::RECV_INFO)
		{
			KeyMsg   imsg((const char *)itext, itext.size());
			if (imsg.getType() == KeyMsg::KM_SERVER_KEY) {
				skype->updateInfo(imsg.getAddr(), imsg.getPort());
				skype->codec->genSharedKey(imsg.getKey());
				skype->state = SMClientSkype::SEND_ACK;

				KeyMsg   omsg("0.0.0.0", 0, imsg.getKey(), KeyMsg::KM_CLIENT_ACK);
				SEString otext((const char *)omsg);

				this->PostText(otext, reply, false);
				std::cerr << "[skype] " << "sending : " << (const char *)otext << std::endl;
			}
			else {
				std::cerr << "[skype] " << "ignore message from"
						<< (const char *)iuser << std::endl;
			}
		}
		else
		{
			assert(skype->state == SMClientSkype::SEND_ACK);
			if(!strcmp("OK", (const char *)itext))
			{
				std::cerr << "[skype] " << "handshake successful!" << std::endl;
				skype->state = SMClientSkype::DONE;

				//Making call
				MyConversation::Ref conv;
				bool convOk = skype->GetConversationByIdentity(skype->wrapper->serverAccount, conv);
				if (!convOk)
				{
					printf("Unable to create conversation with %s\n", (const char*)skype->wrapper->serverAccount);
				}
				else
				{
					conv->RingOthers();

					srand((unsigned)time(0));
					int t = rand()%5 + 5;

					std::cerr << "[skype] " << "Started calling! Waiting for " << t << " seconds."<<std::endl;
					//char tmp[BUFSIZ];
					//sprintf(tmp,"Ringing for %s seconds.\n", t);
					//SEString otext(tmp);
					//this->PostText(otext, reply, false);

					// Block for some seconds
					sleep(t);
					conv->LeaveLiveSession();
				}

				//call finished, proceed

				skype->invokeHandler();
			}

			else
			{
				std::cerr << "[skype] " << "handshake unsuccessful! Try Again!" << std::endl;
				skype->state = SMClientSkype::SEND_ACK;
			}
		}
	}
};

void SMServerConversation::OnChange(int prop)
{
	if(skype->state==SMServerSkype::DONE)
	{

		if (prop == Conversation::P_LOCAL_LIVESTATUS)
		{
			Conversation::LOCAL_LIVESTATUS liveStatus;
			this->GetPropLocalLivestatus(liveStatus);
			if (liveStatus == Conversation::RINGING_FOR_ME)
			{
				std::cerr << "[skype] " <<"RING RING..\n";
				//std::cerr << "[skype] " << "Picking up call from MyConversation::OnChange\n";

			};

			if (liveStatus == Conversation::IM_LIVE)
			{
				// Saving the currently live conversation reference..
				skype->liveSession = this->ref();
				printf("Live session is up.\n");
			};

			if ((liveStatus == Conversation::RECENTLY_LIVE) || (liveStatus == Conversation::NONE))
			{
				std::cerr << "[skype] " << "Call has ended..\n";
				//Setting the port is not effective since it is not
				//used imediately! We have to logout!
				//skype->SetInt(SETUPKEY_PORT, 31031);
				skype->isLiveSessionUp = false;
				skype->state = SMServerSkype::READY;
				sleep(2);
				skype->invokeHandler();

			};
		};
	};
};

//----------------------------------------------------------------------------
// Main


SkypeKitWrapper::SkypeKitWrapper(std::string addr, uint16_t port, //uint16_t sport,
		Config c)
: addr(addr), port(port)
//,  sport(sport)
{


	skypeAccount = c.options["skypeid"];

	skypePasswd  = c.options["skypepwd"];

	skrtAddr = c.options["skrtip"];

	skrtPort = atoi(c.options["skrtport"]);

	skrtPath = c.options["skrtport"];

	keyFileName = c.options["keyfilename"];

	serverAccount = c.options["serverid"];
}


#include <errno.h>

bool SkypeKitWrapper::readKeyPair() {
	FILE* f = 0;
	size_t fsize = 0;

	f = fopen(keyFileName.c_str(), "r");
	if (f != 0) {

		fseek(f, 0, SEEK_END);
		fsize = ftell(f);
		rewind(f);
		size_t keyLen = fsize + 1;
		keyBuf = new char[keyLen];
		size_t read = fread(keyBuf,1,fsize,f);
		if (read != fsize)
		{
			std::cerr << "[skype] " << "error reading key file '" << keyFileName.c_str() << "'" << std::endl;
			return false;
		}
		keyBuf[fsize]=0; //cert should be null terminated
		fclose(f);
		return true;
	}

	perror(keyFileName.c_str());

	return false;
}


void SkypeKitWrapper::pingServer() {
	Conversation::Ref conv;

	SEStringList partnerNames;
	partnerNames.append(serverAccount);

	std::cerr << "[skype] " << "Pinging "<< (const char*)serverAccount << std::endl;
	fflush(stdout);

	if (skype->GetConversationByParticipants(partnerNames, conv, true, false))
	{
		SEString displayName;
		conv->GetPropDisplayname(displayName);
		Message::Ref reply;

		SEString text((const char *)"ping");
		std::cerr << "[skype] " << "sending:" << (const char*)text << std::endl;
		conv->PostText(text,reply,false);
	}
}


void SkypeKitWrapper::clientSendInfo(const boost::shared_ptr<Codec> & codec) {
	Conversation::Ref conv;

	SEStringList partnerNames;
	partnerNames.append(serverAccount);
	fflush(stdout);

	if (skype->GetConversationByParticipants(partnerNames, conv, true, false))
	{
		SEString displayName;
		conv->GetPropDisplayname(displayName);
		Message::Ref reply;

		uint8_t ourkey[32];
		codec->genPublicKey(ourkey);
		KeyMsg msg(addr, port, ourkey, KeyMsg::KM_CLIENT_KEY);
		SEString text((const char *)msg);
		std::cerr << "[skype] " << "sending:" << (const char*)text << std::endl;
		conv->PostText(text,reply,false);
		((SMClientSkype *)skype)->state = SMClientSkype::RECV_INFO;
	}
}


int SkypeKitWrapper::skypeLogin() {


	if (skype->GetAccount(skypeAccount, account))
	{
		//bind to given UDP port
		try
		{
			skype->SetInt(SETUPKEY_PORT, port);
		}
		catch(...) {
			std::cerr << "[skype] " << "unable to bind to port " << port << std::endl;
			return 0;
		}
		std::cerr << "[skype] " << "Logging in..\n";
		std::cerr << "[skype] " << skypeAccount <<  ":*****\n";
		account->LoginWithPassword(skypePasswd, false, true);
		account->BlockWhileLoggingIn();

		if (account->loggedIn)
		{

			std::cerr << "[skype] " << "Loggin complete.\n";
			return 1;
		}
		else
		{
			std::cerr << "[skype] " << "Loggin unsuccessful! Please try again\n";
			return 0;
		}
	}
	else
	{
		std::cerr << "[skype] " << "Account does not exist. Check the keypair certificate!\n";
		exit(-1);
	}
	return 0;
}

void SkypeKitWrapper::skypeLogout()
{
	std::cerr << "[skype] " << "Logging out..\n";
	account->Logout(false);
	account->BlockWhileLoggingOut();
	logged_in=false;
	cv.notify_all();
	std::cerr << "[skype] " << "Logout complete.\n";
}

#ifdef UNITTEST


int main() {
	uint8_t key[32] = "abc";

	KeyMsg imsg("0.0.0.0", 0, key);

	assert(!memcpy(key, imsg.getKey(), 32));

	const char * code = (const char *)imsg;

	assert(code[strlen(code)-1] == '\n');

	KeyMsg omsg(code, strlen(code)-1);

	assert(!memcpy(key, omsg.getKey(), 32));

	return 0;
}

#endif

#endif
