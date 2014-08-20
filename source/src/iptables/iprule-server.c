/*
 ** server.c -- a stream socket server demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

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

int remote_port, local_real_port, local_masq_port, isBrdg, isAddRule;
char *router_ip;
char client_ip[BUFSIZ];
char remote_ip[BUFSIZ];

int get_router_ip(char** router_ip)
{
	printf("%s\n","Trying to get router IP address.");
	int res;
	FILE* pipe = popen("uci -P/var/state get network.wan.ipaddr", "r");
	if (!pipe) return -1;
	char buffer[BUFSIZ];
	if(!feof(pipe) && (fgets(buffer, BUFSIZ, pipe) != NULL))
	{
		strncpy(*router_ip,buffer,strlen(buffer)-1);
		printf("Router IP address is %s!\n",*router_ip);
		res = 0;
	}
	else
	{
		printf("%s\n","Unsuccessful! Trying again.");
		pipe = popen(". /lib/functions/network.sh; network_get_ipaddr ip wan; echo $ip", "r");
		if (!pipe || feof(pipe) || (fgets(buffer, BUFSIZ, pipe) == NULL))
		{
			printf("Router IP address was not determined successfully! Closing!\n");
			res = -1;
		}
		else
		{
			strncpy(*router_ip,buffer,strlen(buffer)-1);
			printf("Router IP address is %s!\n",*router_ip);
			res = 0;
		}
	}
	pclose(pipe);
	return res;
}


int run_cmd()
{

	//TODO: add for bridge and rule remove
	char cmd[BUFSIZ];
	if(!isBrdg)
	{
		if(isAddRule)
		{
			sprintf(cmd,"iptables --table nat --insert zone_wan_nat  1 --jump SNAT -s %s/32 -d %s/32 -p udp --sport %d --dport %d --to-source %s:%d"
					, client_ip, remote_ip, local_real_port, remote_port, router_ip,local_masq_port);
			printf("Command:%s\n",cmd);
			if(exec(cmd)) return -1;

			sprintf(cmd,"iptables --table filter --insert zone_lan_forward  1 --jump ACCEPT -s %s/32 -d %s/32 -p udp --sport %d --dport %d"
					, client_ip, remote_ip, local_real_port, remote_port);
			printf("Command:%s\n",cmd);
			if(exec(cmd)) return -1;

			sprintf(cmd,"conntrack -D -p udp -d %s", remote_ip);
			if(exec(cmd)) return -1;

			sprintf(cmd,"conntrack -D -p udp -s %s", remote_ip);
			if(exec(cmd)) return -1;

			return 0;
		}
	}

}

/*
 * Process a request for iptable setting
 * Message should have the following format:
 *
 * (isBrdg, isAddRule, remote_ip, remote_port, local_real_port, local_masq_port)
 *
 * isBrdg -> indicates whether this is bridge or client
 * isAddRule -> this is an iptable add request or delete request
 * local_real_port -> current port that connection is actually on
 * local_masq_port -> port to masquerade
 */


int procMsg(char *msg, char* client_ip_adr)
{
	int done = 0;
	strcpy(client_ip,client_ip_adr);

	char *ch;
	ch = strtok(msg, " ");
	if (ch != NULL) {
		isBrdg = atoi(ch);

		ch = strtok(NULL, " ");
		isAddRule = atoi(ch);

		ch = strtok(NULL, " ,");
		strcpy(remote_ip,ch);

		ch = strtok(NULL, " ,");
		remote_port = atoi(ch);

		ch = strtok(NULL, " ,");
		local_real_port= atoi(ch);

		ch = strtok(NULL, " ,");
		local_masq_port= atoi(ch);

		done = 1;
	}
	if(!done)
	{
		return -1;
		printf("done not true closing\n");
	}
	int s = run_cmd();
	return (s);
}

int main()
{

	//Determining router ip address
	router_ip = (char*)malloc(BUFSIZ*sizeof(char*));
	if(get_router_ip(&router_ip)) return -1;

	int sockfd, new_fd, numbytes;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	char buf[BUFSIZ];
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
				get_in_addr((struct sockaddr *)&their_addr),
				s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			if ((numbytes = recv(new_fd, buf, BUFSIZ-1, 0)) == -1) {
				perror("recv");
				exit(1);
			}

			buf[numbytes] = '\0';

			printf("server: received '%s'\n",buf);
			if (!procMsg(buf,s))
				strcpy(buf,"success");
			else
				strcpy(buf,"failure");

			if (send(new_fd, buf, 8, 0) == -1)
				perror("send");
			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}

