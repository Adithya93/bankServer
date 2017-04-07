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
#include <stdint.h>
#include "./bankRequestParser.h"
#include "./bankBackend.h"
#include "./bankResponseWriter.h"


//using namespace std;
using namespace xercesc;

int HEADER_SIZE = 8;
const char* response = "HTTP/1.1 200 OK\r\nContent-Length: 11\r\n\r\nbooyakasha\n\0";
const char* BAD_RESPONSE = "HTTP/1.1 400 BAD REQUEST\r\nContent-Length: 0\r\n\r\n\0";
int BAD_RESPONSE_LEN = (int)strlen(BAD_RESPONSE); 

int bindAndListen(int port, int backlog) {
	// initialize socket
	int listenFd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenFd < 0) {
		perror("Unable to create listening socket");
		exit(1);
	}
	std::cout << "Socket initialized, file descriptor is " << listenFd << "\n";
  // bind
	struct sockaddr_in serveraddr;	
	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(port);
	if(bind(listenFd, (struct sockaddr *) &serveraddr, (socklen_t)sizeof(serveraddr))) {
		perror("Unable to bind");
		exit(1);
  	}
	std::cout << "Bind successful, dropping root privilegees\n";
	// Should drop privileges since bind to privileged port is done
	listen(listenFd, backlog);
	std::cout << "Listening on socket " << listenFd << "\n";
	return listenFd;
}

// ntohl is for 32 bits, but unsigned long is 64 bits, so this helper is used here instead for network long to host long
unsigned long netToHostLong64(char * ptr) {
	//printf("Header buff: %s\n", ptr);
	unsigned long * docSizePtr = (unsigned long *)ptr;
	unsigned long docSize = *docSizePtr;
	unsigned int docSizeMost4 = ntohl((uint32_t)*docSizePtr);
//	std::cout << "MSB 4 bytes of document are " << docSizeMost4 << "\n";
	unsigned int docSizeLeast4 = ntohl((uint32_t)*((int*)docSizePtr + 1));
//	std::cout << "LSB 4 bytees of document are " << docSizeLeast4 << "\n";
	docSize = (((unsigned long)(docSizeMost4)) << (4 * (int)(sizeof docSizePtr))) | ((unsigned long)0);
	docSize = docSize | ((unsigned long)docSizeLeast4);
	std::cout << "Doc size is  " << docSize << "\n";
	return docSize;
}

unsigned long readHeader(int connfd) {
	char headerBuff[HEADER_SIZE + 1];
	memset(headerBuff, 0, HEADER_SIZE + 1);
	if (read(connfd, headerBuff, HEADER_SIZE) < HEADER_SIZE) {
		std::cout << "Invalid header, ignoring request\n";
		//close(connfd);
		return 0;
	}
	return netToHostLong64(headerBuff);
}

// bodySize is derived from readHeader
char* readBody(int connfd, unsigned long bodySize) {
	char* requestBuffer = (char*) malloc((bodySize + 1)* (sizeof(char)));
	if (!requestBuffer) {
		perror("Unable to allocate memory for request buffer");
		return NULL; // TO-DO : Differentiate from failure due to invalid header?
	}
	memset(requestBuffer, 0, bodySize + 1);
	int totalRead = 0;
	int readNow = 0;
	while (totalRead < bodySize) {
		readNow = read(connfd, requestBuffer + totalRead, bodySize - totalRead);
		if (readNow < 0) {
			perror("Error reading request body");
		}
		if (readNow == 0) { // socket closed?
			std::cout << "Socket closed prematurely after " << totalRead << " bytes; failing request\n";
			printf("String buffer so far:\n%s\n", requestBuffer);
			free(requestBuffer);
			return NULL; // incomplete request, don't service
		}
		totalRead += readNow;
	}
	std::cout << "Read " << totalRead << " bytes from client request body\n";
	return requestBuffer;
}

char* readRequest(int connfd) {
	unsigned long bodySize;
	if ((bodySize = readHeader(connfd)) > 0) {
		return readBody(connfd, bodySize);
	}
	std::cout << "Unable to read header\n";
	return NULL;
}

void writeBadRequest(int connfd) {
	if (write(connfd, BAD_RESPONSE, BAD_RESPONSE_LEN) < BAD_RESPONSE_LEN) {
		std::cout << "Did not finish writing error response\n";
	}
	else {
		std::cout << "Successfully returned error response\n";
	}
}


int main() {
	std::cout << "Booyakasha C++!\n";
	int listenFd = bindAndListen(8000, 10);
	int connfd;
	struct sockaddr_in cliaddr;
	socklen_t len;
	int handled = 0;
	try {
        XMLPlatformUtils::Initialize();
    }
    catch (const XMLException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
        std::cout << "Error during initialization of parser! :\n" << message << "\n";
        XMLString::release(&message);
        close(listenFd);
        exit(1);
        //return 1;
    }
    bankBackend* backend = new bankBackend(); 
	while(1) {
		if ((connfd = accept(listenFd, (struct sockaddr *)&cliaddr, &len)) == -1) {
			perror("Unable to accept connection");
  		}
		else {
			std::cout << "Accepted connection on socket " << connfd << "\n";
			char * reqBuffer;
			if ((reqBuffer = readRequest(connfd))) {
				// hand over to parser -> eventually either a separate thread, or a threadpool, etc
				/* TEMP
				int written;
				if((written = write(connfd, response, strlen(response))) < 0) {
					perror("Unable to write to client");
				}
				else {
					std::cout << "Succesfully written to client\n";
				}
				*/
				bankRequestParser* test = new bankRequestParser(reqBuffer, (unsigned long)strlen(reqBuffer)); // can put on stack instead of heap?   
			    if (test->parseRequest()) {
			    	std::cout << "Unable to parse request, must be badly formed.\n";
			    	// should return error xml
			    	writeBadRequest(connfd);// TEMP
			    }
			    else { // creates, balances, transfers and queries for this request are all available now
			    	/*
				    std::vector<std::tuple<unsigned long, float, bool, std::string>*>* testCreates = test->getCreateReqs();
				    for (std::vector<std::tuple<unsigned long, float, bool, std::string>*>::iterator it = testCreates->begin(); it < testCreates->end(); it ++ ) {
				      std::tuple<unsigned long, float, bool, std::string> testCreate = **it;
				      std::cout << "Account: " << std::get<0>(testCreate) << "; " << std::get<1>(testCreate) << "; " << std::get<2>(testCreate) << ";" << std::get<3>(testCreate) << "\n";
				    }
				    */
				    std::vector<std::tuple<bool, std::string>> createResults = backend->createAccounts(test->getCreateReqs());
				    std::vector<std::tuple<float, std::string>> balanceResults = backend->getBalances(test->getBalanceReqs());
				    std::vector<std::tuple<bool, std::string>> transferResults = backend->doTransfers(test->getTransferReqs());
				    for (std::vector<std::tuple<bool, std::string>>::iterator it = createResults.begin(); it < createResults.end(); it ++) {
				    	std::tuple<bool, std::string> createResult = *it;
				    	std::cout << "Create Result: " << std::get<0>(createResult) << "; Ref: " << std::get<1>(createResult) << "\n";
				    }

				    for (std::vector<std::tuple<float, std::string>>::iterator it = balanceResults.begin(); it < balanceResults.end(); it ++) {
				    	std::tuple<float, std::string> balanceResult = *it;
				    	std::cout << "Balance Result: " << std::get<0>(balanceResult) << " Ref: " << std::get<1>(balanceResult) << "\n";
				    }

				    for (std::vector<std::tuple<bool, std::string>>::iterator it = transferResults.begin(); it < transferResults.end(); it ++) {
				    	std::tuple<bool, std::string> transferResult = *it;
				    	std::cout << "Transfer Result: " << std::get<0>(transferResult) << "; Ref: " << std::get<1>(transferResult) << "\n";
				    }

				    test->cleanUp(); // done with request, clean up all XMLParsing resources
				}
			    delete test;
			}
			else { // bad request
				writeBadRequest(connfd);
			}
			close(connfd);
		}
	}
	close(listenFd);
	XMLPlatformUtils::Terminate();
	return 0;

}

