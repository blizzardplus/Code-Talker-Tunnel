// SkypeMorph project
// 
// SMClientMain.cpp
// 
#include "UProxy.h"
#include "Morpher.h"
#include "Util.h"
#include "version.h"
#include "debug.h"
#include <cassert>
#include <iostream>
#include <string>
#include "OpenWRT-client.cpp"
#ifdef SKYPE_PAPI
	#include "SkypePublicAPIWrapperClient.h"
#else
	#include "SkypeClient.h"
#endif


#include "Config.h"

using namespace boost::asio;

namespace {

typedef struct {
	std::string skyperc;
	std::string logfile;
} Arguments;

void relayError(const boost::system::error_code &err) {
	std::cerr << "Error with relay, exiting..." << std::endl;
	exit(0);
}


void printUsage(char *name) {
	std::cerr << "Usage: " << name << " <options> " << std::endl << std::endl
			<< "Options:" << std::endl
			<< "-k <skyperc>        -- Skype configure file" << std::endl
			<< "-h                  -- Print this help message." << std::endl << std::endl
			<< "-l <log file>       -- log filename" << std::endl;

	exit(0);
}

int parseOptions(int argc, char **argv, Arguments *arguments) {
	int c;


	arguments->skyperc = "skyperc";
	arguments->logfile = "client.log";

	opterr = 0;

	while ((c = getopt (argc, argv, "k:l:h")) != -1) {
		switch (c) {
		case 'k':
			arguments->skyperc = optarg;
			break;
		case 'h':
			printUsage(argv[0]);
		case 'l':
			arguments->logfile = optarg;
			break;
		default:
			std::cerr << "Unknown option: " << c << std::endl;
			printUsage(argv[0]);
		}
	}

	return 1;
}

} // anonymous namespace



int main(int argc, char** argv) {
	Arguments arguments;


	std::cout << "skypemorph version r" << skypemorph_revision << std::endl;

	if (!parseOptions(argc, argv, &arguments)) {
		printUsage(argv[0]);

		return 2;
	}

	char* k;
	char* v;
	char* result;

	Config cf;
	if(!cf.load_cfg_file(arguments.skyperc.c_str()))
	{
		std::cerr << "cannot open config file" << arguments.skyperc.c_str()<<std::endl;
		return 0;
	}

	if (!log_init(arguments.logfile.c_str())) {
		std::cerr << "[main] " << "cannot open " << arguments.logfile
				<< ", exit now..." << std::endl;
		exit(-1);
	}




	Morpher * morpher = new Morpher(atoi(cf.options["morphingmethod"]));
	if (cf.options["morphingmethod"] != 0) {
		morpher->init(cf.options["skypetimecdf"],
				cf.options["skypesizecdf"],
				cf.options["torsizecdf"],
				cf.options["morphingmatrix"]);
	}

#ifdef SKYPE_PAPI
	std::cout << "Communicating with Skype application" << std::endl;
	SkypePAPIClient *SPAPI = new SkypePAPIClient(cf.options["serverid"],cf.options["ipadr"],
			atoi(cf.options["udpport"]));
#else
	std::cout << "Running SkypeKit Runtime" << std::endl;

	/*Deprecated
	 * 	command = (char*) malloc(BUFSIZ);
	sprintf (command, "%s %s %s&",(char*)(cf.options["skypekitadr"]),"-m -p",(char* )cf.options["skrtport"]);
	system(command);

	//	const char* options;
	//	options = (char*) malloc(BUFSIZ);
	//sprintf (options, "%s %s %s&",(char*)(cf.options["skypekitadr"]),"-m -p",(char* )cf.options["skrtport"]);

*/
	pid_t pID = fork();
	if (pID == 0)                // child
	{
		char* args[]={(char* )cf.options["skypekitadr"], "-m", "-p", (char* )cf.options["skrtport"], (char *) 0};
		execvp (args[0],args);
	}

	SkypeClient * skypeclient = new SkypeClient(cf.options["ipadr"],
			atoi(cf.options["udpport"]),
			//atoi(cf.options["skypeport"]),
			cf,
			cf.options["serverid"]);

#endif

	std::cerr << "[main] " << "proxy starting.." << std::endl;

	boost::asio::io_service io_service;
#ifdef TPROXY_ENABLED
	UDPRelay *udprelay = new UDPRelay(io_service, 0,
			boost::bind(relayError,
					placeholders::error));
#else
#ifdef OPENWRT_ENABLED
	UDPRelay *udprelay = new UDPRelay(io_service, 0,
			boost::bind(relayError,
					placeholders::error));
#else
	UDPRelay *udprelay = new UDPRelay(io_service, atoi(cf.options["udpport"]),
			boost::bind(relayError,
					placeholders::error));
#endif
#endif
	Proxy *proxy = new Proxy(udprelay, io_service, atoi(cf.options["socksport"]), morpher/*, skypeclient*/);
	std::cerr << "[main] " << "SOCKS proxy ready on " << atoi(cf.options["socksport"]) << "." << std::endl;


	//Getting info from Skype
	boost::shared_ptr<Codec> codec(new Codec(false));
	std::string host;
	uint16_t port;

#ifdef SKYPE_PAPI
	SPAPI->runOnce(&host, &port, codec);
#else
	skypeclient->runOnce(&host, &port, codec);
#endif
	std::cerr << "[main] " << "Connecting to " << host << ":" << port << std::endl;

	proxy->relay->openStreamWOSocks(host, port,
			codec, proxy->_morpher);

#ifdef TPROXY_ENABLED
	char* command;
	command = (char*) malloc(BUFSIZ);
	sprintf (command, "./iprulesetter -m 0 -p 0 -i %s -o %d -t %s -c %d -I 127.0.0.1&", host, port, (char*)(cf.options["udpport"]),(udprelay->_local_port));
	system(command);
#endif

#ifdef OPENWRT_ENABLED
	int res = sendReqRtr(0,1,cf.options["routerip"],host.c_str(),port,udprelay->_local_port,atoi(cf.options["udpport"]));
	printf("OPENWRT Result %d\n",res);
#endif


	proxy->relay->_pkzer->run();

	io_service::work work(io_service);
	io_service.run();
}
