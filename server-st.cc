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
#include <signal.h>
#include "./bankRequestParser.h"
#include "./bankBackend.h"
#include "./bankResponseWriter.h"

using namespace xercesc;

int HEADER_SIZE = 8;
const char* response = "HTTP/1.1 200 OK\r\nContent-Length: 11\r\n\r\nbooyakasha\n\0";
const char* BAD_RESPONSE = "HTTP/1.1 400 BAD REQUEST\r\nContent-Length: 0\r\n\r\n\0";
int BAD_RESPONSE_LEN = (int)strlen(BAD_RESPONSE); 

bankBackend* backend;
int PORT = 3000;
int BACKLOG = 128;
int listenFd;

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
	unsigned long * docSizePtr = (unsigned long *)ptr;
	unsigned long docSize = *docSizePtr;
	unsigned int docSizeMost4 = ntohl((uint32_t)*docSizePtr);
	unsigned int docSizeLeast4 = ntohl((uint32_t)*((int*)docSizePtr + 1));
	docSize = (((unsigned long)(docSizeMost4)) << (8 * (int)(sizeof(int))));
	docSize = docSize | ((unsigned long)docSizeLeast4);
	return docSize;
}


unsigned long readHeader(int connfd) {
	char headerBuff[HEADER_SIZE + 1];
	memset(headerBuff, 0, HEADER_SIZE + 1);
	if (read(connfd, headerBuff, HEADER_SIZE) < HEADER_SIZE) {
		std::cout << "Invalid header, ignoring request\n";
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
			free(requestBuffer);
			return NULL; // incomplete request, don't service
		}
		totalRead += readNow;
	}
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

//void respond(int connfd, const char* responseStr, int responseStrLen) {
void respond(int connfd, char* responseStr, int responseStrLen) {
	if (write(connfd, responseStr, responseStrLen) < responseStrLen) {
		std::cout << "Did not finish writing response\n";
	}
	else {
	}
}


void cleanUp() {
	std::cout << "Main thread about to clean up\n";
	backend->cleanUp();
	delete backend;
	close(listenFd);
	XMLPlatformUtils::Terminate();
	exit(0);
}


void handleSignal(int num) {
	std::cout << "Handling signal\n";
	cleanUp();
}


int main() {
	std::cout << "Booyakasha C++!\n";
	listenFd = bindAndListen(PORT, BACKLOG);
	int connfd;
	struct sockaddr_in cliaddr;
	socklen_t len;
	signal(SIGINT, handleSignal); // Register signal handler
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
    }
    backend = new bankBackend(); // cache shared among threads
    bankRequestParser parser;
    bankResponseWriter writer;
	while(1) {
		if ((connfd = accept(listenFd, (struct sockaddr *)&cliaddr, &len)) == -1) {
			perror("Unable to accept connection");
  		}
		else {
			bankRequestParser parser;
			bankResponseWriter writer;
			//std::string errorResponse = writer.getParseErrorResponse(); // should find a way to avoid repetition
			int errorResponseLen;
			char* errorResponse = writer.getParseErrorResponse(&errorResponseLen); 
			char * reqBuffer;
			if ((reqBuffer = readRequest(connfd))) {
		// hand over to parser -> eventually either a separate thread, or a threadpool, etc
				parser.initialize(reqBuffer, (unsigned long)strlen(reqBuffer));
			    
			    if (parser.parseRequest()) { // parser returned error while reading document
			    	std::cout << "Unable to parse request, must be badly formed.\n";
			    	// should return error xml
					//respond(connfd, errorResponse.c_str(), (int)errorResponse.size());			    	
			    	respond(connfd, errorResponse, errorResponseLen);
			    }
	    		else { // creates, balances, transfers and queries for this request are all available now
				    clock_t start, end, t;
				    std::vector<std::tuple<unsigned long, float, bool, std::string>> createReqs = parser.getCreateReqs();
				    std::vector<std::tuple<unsigned long, std::string>> balanceReqs = parser.getBalanceReqs();
				    std::vector<std::tuple<unsigned long, unsigned long, float, std::string, std::vector<std::string>>> transferReqs = parser.getTransferReqs();
				    std::vector<std::tuple<std::string, std::string>> queryReqs = parser.getQueryReqs();

					start = clock();
				    std::vector<std::tuple<bool, std::string>> createResults = backend->createAccounts(createReqs);
					std::vector<std::tuple<float, std::string>> balanceResults = backend->getBalances(balanceReqs);
					std::vector<std::tuple<int, std::string>> transferResults = backend->doTransfers(transferReqs);
					std::vector<std::tuple<bool, std::string, std::vector<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>>>>> queryResults = backend->doQueries(queryReqs);
					end = clock();
					t = end - start;
				    //std::string successResponse = writer.constructResponse(&createResults, &balanceResults, &transferResults, &queryResults);
				    //respond(connfd, successResponse.c_str(), (int)successResponse.size());
					int responseLen;
					char* successResponse = writer.constructResponse(&createResults, &balanceResults, &transferResults, &queryResults, &responseLen);
					respond(connfd, successResponse, responseLen);
				}
		    	parser.cleanUp(); // done with request, clean up all XMLParsing resources
				free(reqBuffer);
			}
			else { // bad request
				//respond(connfd, errorResponse.c_str(), (int)errorResponse.size());
				respond(connfd, errorResponse, errorResponseLen);
			}
		close(connfd);
		}
	}
	exit(1);
}
