// Sets iptable rules, needs root privilage

//#include "iprule-setter.h"
#include <stdlib.h>
#include <string>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

bool isServer, isTCP;
char theirIP[BUFSIZ];
char ourIP[BUFSIZ];
int transferToPort, currPort, otherSidePort;

void printUsage(char *name) {
	std::cerr << "Usage: " << name << " <options> " << std::endl << std::endl
			<< "Options:" << std::endl
			<< "-m <mode>	-- Usage mode: 0 for client and 1 for server" << std::endl
			<< "-p <protocol>	-- Protocol: 0 for UDP and 1 for TCP" << std::endl
			<< "-t <port>	-- Port to transfer connection to" << std::endl
			<< "-c <port>	-- Current port of the connection" << std::endl
			<< "-o <port>	-- Their port number" << std::endl
			<< "-i <ip-address>	-- Target IP address" << std::endl
			<< "-I <ip-address>	-- Our IP address" << std::endl
			<< "-h		-- Print this help message." << std::endl << std::endl;
	exit(0);
}

int parseOptions(int argc, char **argv) {
	int c;
	while ((c = getopt (argc, argv, "m:p:t:c:o:i:I:h")) != -1) {
		switch (c) {
		case 'm':
			isServer = atoi(optarg);
			break;
		case 'p':
			isTCP = atoi(optarg);
			break;
		case 't':
			transferToPort = atoi(optarg);
			break;
		case 'c':
			currPort = atoi(optarg);
			break;
		case 'o':
			otherSidePort = atoi(optarg);
			break;
		case 'i':
			strcpy(theirIP,optarg);
			break;
		case 'I':
			strcpy(ourIP,optarg);
			break;
		case 'h':
			printUsage(argv[0]);
			break;
		default:
			std::cerr << "Unknown option: " << c << std::endl;
			printUsage(argv[0]);
		}
	}
	return 1;
}


std::string exec(char* cmd) {
	FILE* pipe = popen(cmd, "r");
	if (!pipe) return "ERROR";
	char buffer[128];
	std::string result = "";
	while(!feof(pipe)) {
		if(fgets(buffer, 128, pipe) != NULL)
			result += buffer;
	}
	pclose(pipe);
	return result;
}

int main(int argc, char** argv) {

	if (!parseOptions(argc, argv)) {
		printUsage(argv[0]);
		return 2;
	}

	std::cout<<isServer<<":"<<isTCP<<std::endl;
	char cmd[BUFSIZ];
	if(isServer)
	{
		if(isTCP)
		{

		}
		else
		{
			/*
			pid_t pID = fork();
			if (pID == 0)
			{
				char* args[]={"iptables", "-L", (char *) 0};
				execvp (args[0],args);
			}
			 */
			sprintf(cmd,"iptables -t filter -A OUTPUT  -p udp -d %s --sport %d --dport %d -j DROP", theirIP, currPort,otherSidePort);
			if (exec(cmd)!="")
				std::cout<<"Command not successful! Make sure you have root access!"<<std::endl;
			sprintf(cmd,"iptables -t nat -A PREROUTING --src %s -p udp --dport %d -j REDIRECT --to-port %d", theirIP, currPort,transferToPort);
			if (exec(cmd)!="")
				std::cout<<"Command not successful! Make sure you have root access!"<<std::endl;
			sprintf(cmd,"conntrack -D -p udp -d %s", theirIP);
			if (exec(cmd)!="")
				std::cout<<"Command not successful! Make sure you have root access!"<<std::endl;
			sprintf(cmd,"conntrack -D -p udp -s %s", theirIP);
			if (exec(cmd)!="")
				std::cout<<"Command not successful! Make sure you have root access!"<<std::endl;

		}
	}
	else
	{
		if(isTCP)
		{

		}
		else
		{
			sprintf(cmd, "iptables -t nat -A POSTROUTING -d %s/32 -p udp --sport %d --dport %d -j SNAT --to-source %s:%d", theirIP, currPort, otherSidePort, ourIP,transferToPort);
			if (exec(cmd)!="")
				std::cout<<"Command not successful! Make sure you have root access!"<<std::endl;
			sprintf(cmd,"conntrack -D -p udp -d %s", theirIP);
			if (exec(cmd)!="")
				std::cout<<"Command not successful! Make sure you have root access!"<<std::endl;
			sprintf(cmd,"conntrack -D -p udp -s %s", theirIP);
			if (exec(cmd)!="")
				std::cout<<"Command not successful! Make sure you have root access!"<<std::endl;
		}

	}

}

