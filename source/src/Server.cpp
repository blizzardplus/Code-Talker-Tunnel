#include "Server.h"



using namespace boost::asio;


boost::thread & Server::run() {
	_thread = boost::thread(&Server::serverSession, this);
	return _thread;
}

void Server::stop() {
	_io_service.stop();

	_thread.join();
}


void Server::recvUDPData() {
	_udpsocket->async_receive_from(buffer(_buf, sizeof(_buf)),
			_sender_ep,
			boost::bind(&Server::handleUDPData,
					this, placeholders::error,
					placeholders::bytes_transferred));
}

void Server::handleUDPData(const boost::system::error_code &err,
		std::size_t bytes_transferred)
{
	if (err) {
		std::cerr << "[server] " << "Error accepting incoming connection: " << err << std::endl;
		recvUDPData();

		return;
	}

#ifdef DEBUG_UDP_DISPATCH
	std::cerr << "[server] " << "Got UDP " << bytes_transferred << " bytes "
			<< "from " << _sender_ep << std::endl;
#endif

	if (_clients.count(_sender_ep) > 0) {
		boost::shared_ptr<ClientHandle> c = _clients[_sender_ep];
		c->_basicClient->push_recv_msg(_buf, bytes_transferred);
	}
	else {
		std::cerr << "[server] " << "Ignore unknown client " << _sender_ep << std::endl;
	}

	recvUDPData();
}

bool Server::verifyConnectionRequest(boost::shared_ptr<BasicClient> & c,
		boost::shared_ptr<Packetizer> & pkzer) {
	UDPPacket * pkt = c->pop_recv_msg();
	if (pkzer->verifyHello(pkt)) {
		delete pkt;

		pkt = pkzer->genHello();
		_udpsocket->send_to(buffer(pkt->data, pkt->length), c->endpoint());
		delete pkt;

		std::cerr << "[server] " << "UDP connection established with " << c->endpoint() << std::endl;

		return true;
	}

	return false;
}



void Server::endClientSession(boost::shared_ptr<ClientHandle> & c) {
	//(_client_pkzer_map[c])->close();
	//(_client_prxyshflr_map[c])->close();
}


void Server::clientSession(boost::shared_ptr<ClientHandle> & cHandle,
		boost::shared_ptr<Codec> & codec) {
	std::cerr << "[server] " << "Client " << cHandle->_basicClient->endpoint()
            										<< " starts..." << std::endl;


	// if (!verifyConnectionRequest(c, pkzer)) {

	//   _clients.erase(c->endpoint());

	//   std::cerr << "[server] " << " invalid connection request. exit now" << std::endl
	//             << "Client " << c->endpoint() << " stops..." << std::endl;
	//   return;

	// }



	try{

#ifdef TPROXY_ENABLED
		char* command;
		command = (char*) malloc(BUFSIZ);
		sprintf (command, "./iprulesetter -m 1 -p 0 -t %d -c %d -o %d -i %s&",_listen_port,_skype_port, cHandle->_basicClient->endpoint().port(),cHandle->_basicClient->endpoint().address().to_string().c_str());
		system(command);
#endif
		cHandle->run();

	}
	catch (boost::system::system_error & error) {
		std::cerr << "[server] " << " openStream filed: " << error.what() << std::endl
				<< " eixt now." << std::endl;
	}
	catch (...) {
		std::cerr << "[server] " << " unexpected error. exit now" << std::endl;
	}


	_clients.erase(cHandle->_basicClient->endpoint());

	std::cerr << "[server] " << "Client " << cHandle->_basicClient->endpoint()
            										<< " stops..." << std::endl << std::endl;
}

#ifdef TPROXY_ENABLED

Server::Server(uint16_t skypePort, uint16_t torPort, std::string tor_addr, Morpher * morpher)
: _tor_port(torPort), _morpher(morpher), _skype_port(skypePort), _tor_addr(tor_addr)
{
	ip::udp::socket * socket = new ip::udp::socket(_io_service,
			ip::udp::endpoint(ip::udp::v4(), 0));
	_udpsocket = boost::shared_ptr<ip::udp::socket>(socket);
	_listen_port = _udpsocket->local_endpoint().port();
}

void Server::serverSession() {
	std::cerr << "[server] " << "ready on " << _udpsocket->local_endpoint() << std::endl;
	recvUDPData();
	boost::asio::io_service::work work(_io_service);
	_io_service.run();
}


void Server::newClientSession(const std::string & addr, uint16_t port, boost::shared_ptr<Codec> & codec) {
	ip::udp::endpoint ep(ip::address::from_string(addr.c_str()), port);
	if (_clients.count(ep) != 0) {
		std::cerr << "[server] " << "session exists for " << ep << std::endl;

		return;
	}
	boost::shared_ptr<ClientHandle> cHandle(new ClientHandle(_udpsocket, ep,codec,_morpher,_tor_addr, _tor_port));
	std::cerr << "[server] " << "new client " << ep << std::endl;
	_clients[ep] = cHandle;

	/*
	 * Used for debugging

	for ( std::map<ip::udp::endpoint,
			boost::shared_ptr<Client> >::const_iterator it = _clients.begin(); it != _clients.end(); it++) {
		std::cout << "Client: " << (it->second)->endpoint() << std::endl;
	}
	 */
	cHandle->_basicClient->thread = boost::thread(&Server::clientSession, this, cHandle, codec);
}


#else



Server::Server(uint16_t skypePort, uint16_t torPort, std::string tor_addr, Morpher * morpher)
: _tor_port(torPort), _morpher(morpher), _skype_port(skypePort), _tor_addr(tor_addr)
{


	//TODO: Bind to some random port and user redirect to get the messages to that port!
	//ip::udp::socket * socket = new ip::udp::socket(_io_service,
	//                                   ip::udp::endpoint(ip::udp::v4(), listenPort+1));
	//udpsocket = boost::shared_ptr<ip::udp::socket>(socket);

	/*
	ip::udp::socket * socket = new ip::udp::socket(_io_service,
			ip::udp::endpoint(ip::udp::v4(), listenPort+1));
	_udpsocket = boost::shared_ptr<ip::udp::socket>(socket);*/

	//setting the reuse option! Doesn't work :(
	//int yes=1;
	//int client_fd = (int)socket->native_handle();
	//int error = setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR,  &yes, sizeof(int));


}

bool done = false;
void Server::serverSession() {
	//creating socket
	boost::unique_lock<boost::mutex> lock(mx);
	while (!done) cv.wait(lock);
#ifdef WIN32
	Sleep(2000);
#else
	sleep(2);
#endif

	bool connected = false;
	ip::udp::socket * socket;
	while(!connected)
	{
		try
		{
			socket = new ip::udp::socket(_io_service, ip::udp::endpoint(ip::udp::v4(), _listen_port));
			connected=true;
		}
		catch(boost::exception & e)
		{
			std::cerr << "[server] " << "Port already busy, trying again!\n";
#ifdef WIN32
			Sleep(1000);
#else
			sleep(1);
#endif
		}
	}
	//ip::udp::socket * socket = new ip::udp::socket(_io_service, ip::udp::endpoint(ip::udp::v4(), _listen_port));

	_udpsocket = boost::shared_ptr<ip::udp::socket>(socket);
	std::cerr << "[server] " << "ready on " << _udpsocket->local_endpoint() << std::endl;



	ip::udp::endpoint ep(ip::address::from_string(client_addr.c_str()), client_port);
	if (_clients.count(ep) != 0) {
		std::cerr << "[server] " << "session exists for " << ep << std::endl;

		return;
	}
	boost::shared_ptr<ClientHandle> cHandle(new ClientHandle(_udpsocket, ep,client_codec,_morpher,_tor_addr, _tor_port));
	std::cerr << "[server] " << "new client " << ep << std::endl;
	_clients[ep] = cHandle;


	cHandle->_basicClient->thread = boost::thread(&Server::clientSession, this, cHandle, client_codec);

	//receiving UDP packets!
	recvUDPData();
	boost::asio::io_service::work work(_io_service);
	_io_service.run();
}


void Server::wakeUp() {
	done=true;
	cv.notify_all();
}
void Server::newClientSession(const std::string & addr, uint16_t port, boost::shared_ptr<Codec> & codec) {
	this->client_addr = addr ;
	this->client_port = port;
	this->client_codec = codec;
}

#endif

