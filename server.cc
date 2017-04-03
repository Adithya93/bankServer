#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <xercesc/util/PlatformUtils.hpp>

using namespace std;
using namespace xercesc;

int bindAndListen(int port, int backlog) {
	// initialize socket
	int listenFd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenFd < 0) {
		perror("Unable to create listening socket");
		exit(1);
	}
	cout << "Socket initialized, file descriptor is " << listenFd << "\n";
  // bind
	struct sockaddr_in serveraddr;	
	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(port);
	if(bind(listenFd, (struct sockaddr *) &serveraddr, (socklen_t)sizeof(serveraddr))) {
		perror("Unable to bind");
  }
  cout << "Bind successful, dropping root privilegees\n";
	// Should drop privileges since bind to privileged port is done
	listen(listenFd, backlog);
	cout << "Listening on socket " << listenFd << "\n";
	return listenFd;
}
int main() {
	cout << "Booyakasha C++!\n";
	int listenFd = bindAndListen(8000, 10);
	int connfd;
	struct sockaddr_in cliaddr;
	socklen_t len;
	int handled = 0;
	/*
	try {
	    XMLPlatformUtils::Initialize();
  	}
  	catch (const XMLException& toCatch) {
    	// Do your failure processing here
    	return 1;
  	}
  	*/
	while(1) {
		if ((connfd = accept(listenFd, (struct sockaddr *)&cliaddr, &len)) == -1) {
			perror("Unable to accept connection");
  	}
		else {
			cout << "Accepted connection on socket " << connfd << "\n";
			const char * response = "HTTP/1.1 200 OK\r\nContent-Length: 11\r\n\r\nbooyakasha\n\0";
			int written;
			if((written = write(connfd, response, strlen(response))) < 0) {
				perror("Unable to write to client");
			}
			else {
				handled ++;
				cout << "Total requests handled: " << handled << "\n";
			}
			close(connfd);
		}
	}

	//XMLPlatformUtils::Terminate();

}

