#ifdef OPENWRT_ENABLED

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#include <boost/asio.hpp>

#ifdef WIN32
#else

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#endif

#define PORT "3490" // the port this application will be connecting to


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int exec(char* cmd) {
	FILE* pipe = popen(cmd, "r");
	if (!pipe) return -1;
	char buffer[BUFSIZ];
	while(!feof(pipe)) {
		if(fgets(buffer, BUFSIZ, pipe) != NULL)
			printf("%s",buffer);
	}
	pclose(pipe);
	return 0;
}


/* Sends a request for iptable setting to router
 * isBrdg -> indicates whether this is bridge or client
 * isAddRule -> this is an iptable add request or delete request
 * local_real_port -> current port that connection is actually on
 * local_masq_port -> port to masquerade
 */
int sendReqRtr(int isBrdg, int isAddRule, const char* router_ip, const char* remote_ip, int remote_port, int local_real_port, int local_masq_port)
{
	int sockfd, numbytes;
	char buf[BUFSIZ];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	//	if (argc != 2) {
	//		fprintf(stderr,"usage:hostname\n");
	//		exit(1);
	//	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(router_ip, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}


	boost::system::error_code ec;
	boost::asio::detail::socket_ops::inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s,0,ec);
//	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	//Syntax: isSrv, isAddRule, Our local IP, Remote Destination IP, Remote Destination port, Real port we are sending on, Masquerade port to fake
	sprintf(buf,"%d %d %s %d %d %d",isBrdg, isAddRule, remote_ip,  remote_port,  local_real_port,  local_masq_port);
	printf("Sending: %s\n",buf);
	if (send(sockfd, buf, strlen(buf), 0) == -1)
	{
		perror("send");
		close(sockfd);
		exit(0);
	}


	if ((numbytes = recv(sockfd, buf, BUFSIZ-1, 0)) == -1) {
		perror("recv");
		exit(1);
	}

	buf[numbytes] = '\0';

	printf("client: received '%s'\n",buf);

	close(sockfd);

	if(strcmp(buf,"success")) return 0;
	else return -1;
}

#endif
