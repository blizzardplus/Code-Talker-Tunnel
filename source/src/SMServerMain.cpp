// SkypeMorph project
// 
// SMServerMain.cpp
// 
#include "Server.h"
#include "Util.h"
#include "version.h"
#include "debug.h"
#include <unistd.h>
#include "Config.h"
#include <boost/thread.hpp>



#ifdef SKYPE_PAPI
#include "SkypePublicAPIWrapperServer.h"
#else
#include "SkypeServer.h"
#endif



using namespace boost::asio;

namespace {

typedef struct {
	std::string skyperc;
	std::string logfile;
} Arguments;

void printUsage(char *name) {
	std::cerr << "Usage: " << name << " <options> " << std::endl << std::endl
			<< "Options:" << std::endl
			<< "-k <skyperc>        -- Skype configure file" << std::endl
			<< "-l <log file>       -- log filename" << std::endl
			<< "-h                  -- Print this help message." << std::endl << std::endl;

	exit(0);
}

int parseOptions(int argc, char **argv, Arguments *arguments) {
	int c;

	arguments->skyperc = "skyperc";
	arguments->logfile = "server.log";


	opterr = 0;

	while ((c = getopt (argc, argv, "k:l:h"/*"I:p:r:q:s:t:S:m:x:k:a:l:h"*/)) != -1) {
		switch (c) {

		case 'k':
			arguments->skyperc = optarg;
			break;
		case 'l':
			arguments->logfile = optarg;
			break;
		case 'h':
			printUsage(argv[0]);
			break;
		default:
			std::cerr << "Unknown option: " << c << std::endl;
			printUsage(argv[0]);
			break;
		}
	}

	return 1;
}

} // anonymous namespace 

int main(int argc, char** argv) {
	Arguments arguments;
	int ret = 0;

	std::cout << "skypemorph version r" << skypemorph_revision << std::endl;

	if (!parseOptions(argc, argv, &arguments)) {
		printUsage(argv[0]);
		ret = 2;
		return ret;
	}


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


#ifndef SKYPE_PAPI
	std::cout << "Running SkypeKit Runtime" << std::endl;
	/*Deprecated
	char* command = (char*) malloc(strlen((char*)cf.options["skypekitadr"])+strlen((char* )cf.options["skrtport"])+9);
	sprintf (command, "%s %s %s&",(char*)(cf.options["skypekitadr"]),"-m -p",(char* )cf.options["skrtport"]);
	system(command);
	 */
	pid_t pID = fork();
	if (pID == 0)
	{
		char* args[]={(char* )cf.options["skypekitadr"], "-m", "-p", (char* )cf.options["skrtport"], (char *) 0};
		execvp (args[0],args);
	}

#endif
	Morpher * morpher = new Morpher(atoi(cf.options["morphingmethod"] ));
	if (cf.options["morphingmethod"] != 0) {
		morpher->init(cf.options["skypetimecdf"],
				cf.options["skypesizecdf"],
				cf.options["torsizecdf"],
				cf.options["morphingmatrix"]);
	}

	std::cerr << "[main] " << "server starting.." << std::endl;

	Server * server = new Server(atoi(cf.options["udpport"]), atoi(cf.options["torport"]), cf.options["toraddr"], morpher);

#ifdef SKYPE_PAPI
	std::cout << "Communicating with Skype application" << std::endl;
	SkypePAPIServer *SPAPI = new SkypePAPIServer(cf.options["ipadr"],
			atoi(cf.options["udpport"]),server);
#else
	SkypeServer * skypeserver = new SkypeServer(cf.options["ipadr"], atoi(cf.options["udpport"]), //atoi(cf.options["skypeport"])
			cf,
			server);
#endif

	try {
#ifndef SKYPE_PAPI
		boost::thread & tskype  = skypeserver->run();
#endif
		boost::thread & tserver = server->run();

		tserver.join();
#ifndef SKYPE_PAPI
		tskype.join();
#endif
	}
	catch (...) {
		std::cerr << "[main] " << "caught exception" << std::endl;
		server->stop();
#ifndef SKYPE_PAPI
		skypeserver->stop();
#endif
		ret = -1;

		goto cleanup;
	}

	std::cerr << "[main] " << "finished" << std::endl;


	cleanup:
	delete server;
	delete morpher;

	done:
	return ret;
}

