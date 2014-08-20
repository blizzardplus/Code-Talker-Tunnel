#ifndef SKYPE_H
#define SKYPE_H

#ifndef SKYPE_PAPI



/* 
 * Skype interfaces (using SkypeKit)
 */

#include <stdint.h>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include "skype-embedded_2.h"
#include "Config.h"

class Codec; 
class SMSkype; 
class SMClientSkype; 
class SMServerSkype; 
class SkypeKitWrapper; 


class MyContact : public Contact
{
	SMClientSkype* skype;
public:
	typedef DRef<MyContact, Contact> Ref;
	typedef DRefs<MyContact, Contact> Refs;
	MyContact(unsigned int oid, SERootObject* root, SMClientSkype * sk) : Contact(oid, root) , skype(sk)  {};

	void OnChange(int prop);
};


class MyConversation : public Conversation
{
public:
	typedef DRef<MyConversation, Conversation> Ref;
	typedef DRefs<MyConversation, Conversation> Refs;
	MyConversation(unsigned int oid, SERootObject* root) : Conversation(oid, root) {};

	void OnChange(int prop);
};

class SMAccount : public Account
{
public:

	typedef DRef<SMAccount, Account> Ref;
	typedef DRefs<SMAccount, Account> Refs;
	SMAccount(unsigned int oid, SERootObject* root)
	: Account(oid, root), loggedIn(false) { }

	void OnChange(int prop);
	void BlockWhileLoggingIn();
	void BlockWhileLoggingOut();

	// private:
	bool loggedIn;

private:
	mutable boost::mutex mx;
	boost::condition_variable cv;
};

class SMClientConversation : public Conversation {
private:
	SMClientSkype * skype;

public:
	typedef DRef<SMClientConversation, Conversation> Ref;
	typedef DRefs<SMClientConversation, Conversation> Refs;
	SMClientConversation(unsigned int oid, SERootObject* root, SMClientSkype * sk)
	: Conversation(oid, root), skype(sk) { }

	void OnMessage(const MessageRef & message);

	void Respond(const MessageRef & message);
};

class SMServerConversation : public Conversation {
private:
	SMServerSkype * skype;

public:
	typedef DRef<SMServerConversation, Conversation> Ref;
	typedef DRefs<SMServerConversation, Conversation> Refs;

	SMServerConversation(unsigned int oid, SERootObject* root, SMServerSkype * sk)
	: Conversation(oid, root), skype(sk) { }

	void OnMessage(const MessageRef & message);
	void Respond(const MessageRef & message);
	void OnChange(int prop);
};

typedef boost::function<void (boost::shared_ptr<Codec> codec, std::string addr,
		uint16_t port
)> SkypeInfoHandler;

class SMSkype : public Skype {
protected: 


public:
	SkypeKitWrapper        * wrapper;
	boost::shared_ptr<Codec> codec;
	SkypeInfoHandler         handler;
	std::string              theirAddr;
	uint16_t                 theirPort;


	SMSkype(SkypeKitWrapper * skw, boost::shared_ptr<Codec> & c, SkypeInfoHandler h)
	: wrapper(skw), codec(c), handler(h) {}

	Account* newAccount(int oid) {return new SMAccount(oid, this);}

	virtual Conversation::Refs getInbox() const = 0;

	void updateInfo(std::string addr, uint16_t port) {
		theirAddr = addr;
		theirPort = port;
	}

	void invokeHandler() {
		handler(codec, theirAddr, theirPort);
	}

};

class SMClientSkype : public SMSkype {
public: 
	enum State {
		READY,
		RECV_INFO,
		SEND_ACK,
		DONE
	};
	State state;

	SMClientConversation::Refs     inbox;

	SMClientSkype(SkypeKitWrapper * skw, boost::shared_ptr<Codec> & c, SkypeInfoHandler h)
	: SMSkype(skw, c, h), state(READY)
	{ }


	Conversation* newConversation(int oid)  {
		return new SMClientConversation(oid, this, this);
	}

	Conversation::Refs getInbox() const {
		return inbox;
	}

	Contact* newContact(int oid) {
		return new MyContact(oid, this, this);
	}


	void OnConversationListChange(const ConversationRef &conversation,
			const Conversation::LIST_TYPE &type,
			const bool &added);

	friend class SMClientConversation;

};


class SMServerSkype : public SMSkype {
public: 
	enum State {
		READY,
		SEND_INFO,
		RECV_ACK,
		DONE
	};
	State state;
	SMServerConversation::Ref liveSession;
	bool isLiveSessionUp;

	SMServerConversation::Refs     inbox;

	SMServerSkype(SkypeKitWrapper * skw, boost::shared_ptr<Codec> & c, SkypeInfoHandler h)
	: SMSkype(skw, c, h), state(READY)
	{ }


	Conversation* newConversation(int oid)  {
		return new SMServerConversation(oid, this, this);
	}

	Conversation::Refs getInbox() const {
		return inbox;
	}

	void OnConversationListChange(const ConversationRef &conversation,
			const Conversation::LIST_TYPE &type,
			const bool &added);

	friend class SMServerConversation;

};




class SkypeKitWrapper
{
public: 
	bool 		logged_in;
	SEString    skypeAccount;
	SEString    skypePasswd;
	SEString serverAccount;
	SEString    skrtAddr;
	uint16_t    skrtPort;
	std::string skrtPath;
	SMSkype   * skype;
	char      * keyBuf;
	std::string keyFileName;
	SMAccount::Ref account;

public: 
	enum Role {
		CLIENT,
		SERVER
	};
	std::string addr;
	uint16_t    port;
	//	uint16_t    sport;
	Role        role;

	SkypeKitWrapper(SEString accountName,
			SEString accountPwd,
			std::string addr,
			uint16_t    port,
			uint16_t    sport,
			SEString    rtaddr,
			uint        rtport,
			std::string rtpath,
			std::string keyfilename, SEString serverAcc)
	: skypeAccount(accountName),
	  skypePasswd(accountPwd),
	  skrtAddr(rtaddr),
	  skrtPort(rtport),
	  skrtPath(rtpath),
	  keyFileName(keyfilename),
	  addr(addr),
	  port(port),
	  //sport(sport),
	  serverAccount(serverAcc) {
	}


	SkypeKitWrapper(std::string addr, uint16_t port, //uint16_t sport,
			Config c);

	int  skypeLogin();
	void skypeLogout();
	void clientSendInfo(const boost::shared_ptr<Codec> & codec);
	void pingServer();

protected:
	bool readKeyPair();
	mutable boost::mutex      mx;
	boost::condition_variable cv;

public: 

	friend class SMSkype;

};

#endif




#endif
