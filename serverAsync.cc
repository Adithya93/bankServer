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
#include <future>
#include <time.h>
#include "./bankRequestParser.h"
#include "./bankBackend.h"
#include "./bankResponseWriter.h"
#include "./threadPool.h"

//using namespace std;
using namespace xercesc;

int HEADER_SIZE = 8;
const char* response = "HTTP/1.1 200 OK\r\nContent-Length: 11\r\n\r\nbooyakasha\n\0";
const char* BAD_RESPONSE = "HTTP/1.1 400 BAD REQUEST\r\nContent-Length: 0\r\n\r\n\0";
int BAD_RESPONSE_LEN = (int)strlen(BAD_RESPONSE); 

int POOL_SIZE = 4;
bankBackend* backend;
threadPool* pool;
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

/*
void printByteByByte(char * ptr) {

	char * endPtr = ptr + 8;
	char * readPtr = ptr;
	while (readPtr < endPtr) {
		printf("Char: %c\n", *readPtr);
		printf("Byte: %2X\n", *readPtr++);
	}
}
*/

// ntohl is for 32 bits, but unsigned long is 64 bits, so this helper is used here instead for network long to host long
unsigned long netToHostLong64(char * ptr) {

	//printf("Header buff: %s\n", ptr);
	unsigned long * docSizePtr = (unsigned long *)ptr;
	unsigned long docSize = *docSizePtr;
	unsigned int docSizeMost4 = ntohl((uint32_t)*docSizePtr);
//	std::cout << "MSB 4 bytes of document are " << docSizeMost4 << "\n";
	unsigned int docSizeLeast4 = ntohl((uint32_t)*((int*)docSizePtr + 1));
//	std::cout << "LSB 4 bytees of document are " << docSizeLeast4 << "\n";
	docSize = (((unsigned long)(docSizeMost4)) << (8 * (int)(sizeof(int))));
	docSize = docSize | ((unsigned long)docSizeLeast4);
	//std::cout << "Doc size is  " << docSize << "\n";
	return docSize;
}

unsigned long readHeader(int connfd) {
	char headerBuff[HEADER_SIZE + 1];
	memset(headerBuff, 0, HEADER_SIZE + 1);
	int totalRead = 0;
	int readNow = 0;
	while (totalRead < HEADER_SIZE) {
		if ((readNow = read(connfd, headerBuff + totalRead, HEADER_SIZE - totalRead))) {
			//std::cout << "Read " << readNow << " bytes this time\n";
		}
		else {
			//std::cout << "Client closed socket after writing " << totalRead << " bytes\n";
			return 0; 
		}
		totalRead += readNow;
	}
	/*
	if ((numRead = read(connfd, headerBuff, HEADER_SIZE)) < HEADER_SIZE) {
		std::cout << "Only read " << numRead << " bytes\n";
		std::cout << "Invalid header, ignoring request\n";
		//close(connfd);
		return 0;
	}
	*/
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
			//std::cout << "Socket closed prematurely after " << totalRead << " bytes; failing request\n";
			//printf("String buffer so far:\n%s\n", requestBuffer);
			free(requestBuffer);
			return NULL; // incomplete request, don't service
		}
		totalRead += readNow;
	}
	//std::cout << "Read " << totalRead << " bytes from client request body\n";
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

void respond(int connfd, const char* responseStr, int responseStrLen) {
	if (write(connfd, responseStr, responseStrLen) < responseStrLen) {
		std::cout << "Did not finish writing response\n";
	}
	else {
		//std::cout << "Successfully returned response\n";
	}
}


void serviceRequest(int connfd) {
	bankRequestParser parser;
	bankResponseWriter writer;
	std::string errorResponse = writer.getParseErrorResponse(); // should find a way to avoid repetition
	char * reqBuffer;
	clock_t start;
	clock_t end;
	clock_t t;
	clock_t dbStart;
	clock_t dbEnd;
	clock_t dbTime;
  	start = clock();
	if ((reqBuffer = readRequest(connfd))) {
		end = clock();
		t = end - start;
		//std::cout << "Time elapsed reading request: " << t << " clicks; " << (((float)t)/CLOCKS_PER_SEC) << " secs\n";
		// hand over to parser -> eventually either a separate thread, or a threadpool, etc
		start = clock();
		parser.initialize(reqBuffer, (unsigned long)strlen(reqBuffer));
		end = clock();
		t = end - start;
		//std::cout << "Time elapsed initializing parser: " << t << " clicks; " << (((float)t)/CLOCKS_PER_SEC) << " secs\n";
	    start = clock();
	    if (parser.parseRequest()) { // parser returned error while reading document
	    	std::cout << "Unable to parse request, must be badly formed.\n";
	    	// should return error xml
			respond(connfd, errorResponse.c_str(), (int)errorResponse.size());			    	
	    }
	    else { // creates, balances, transfers and queries for this request are all available now
		    end = clock();
		    t = end - start;
		    //std::cout << "Time elapsed parsing DOM: " << t << " clicks; " << (((float)t)/CLOCKS_PER_SEC) << " secs\n";
		    start = clock();
		    std::vector<std::tuple<unsigned long, float, bool, std::string>> createReqs = parser.getCreateReqs();
		    std::vector<std::tuple<unsigned long, std::string>> balanceReqs = parser.getBalanceReqs();
		    std::vector<std::tuple<unsigned long, unsigned long, float, std::string, std::vector<std::string>>> transferReqs = parser.getTransferReqs();
		    std::vector<std::tuple<std::string, std::string>> queryReqs = parser.getQueryReqs();
		    end = clock();
		    t = end - start;
		    //std::cout << "Time elapsed copying parse data: " << t << " clicks; " << (((float)t)/CLOCKS_PER_SEC) << " secs\n";
		    /*
		    std::vector<std::tuple<bool, std::string>> createResults = backend->createAccounts(parser.getCreateReqs());
		    std::vector<std::tuple<float, std::string>> balanceResults = backend->getBalances(parser.getBalanceReqs());
		    std::vector<std::tuple<int, std::string>> transferResults = backend->doTransfers(parser.getTransferReqs());
		    std::vector<std::tuple<bool, std::string, std::vector<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>>>>> queryResults = backend->doQueries(parser.getQueryReqs());
			*/
			start = clock();
			dbStart = clock();
			//std::vector<std::tuple<bool, std::string>> createResults = backend->createAccounts(createReqs);
			std::future<std::vector<std::tuple<bool, std::string>>> createResultsFuture = std::async(std::launch::async, &bankBackend::createAccounts, backend, createReqs);
			end = clock();
		    t = end - start;
		    //std::cout << "Time elapsed launching thread for cache/database: " << t << " clicks; " << (((float)t)/CLOCKS_PER_SEC) << " secs\n";
		    start = clock();
			//std::vector<std::tuple<float, std::string>> balanceResults = backend->getBalances(balanceReqs);
			std::future<std::vector<std::tuple<float, std::string>>> balanceResultsFuture = std::async(std::launch::async, &bankBackend::getBalances, backend, balanceReqs);
			end = clock();
		    t = end - start;
		    //std::cout << "Time elapsed launching thread for cache/database: " << t << " clicks; " << (((float)t)/CLOCKS_PER_SEC) << " secs\n";
		    start = clock();
			//std::vector<std::tuple<int, std::string>> transferResults = backend->doTransfers(transferReqs);
			std::future<std::vector<std::tuple<int, std::string>>> transferResultsFuture = std::async(std::launch::async, &bankBackend::doTransfers, backend, transferReqs);
			end = clock();
		    t = end - start;
		    //std::cout << "Time elapsed launching thread for cache/database: " << t << " clicks; " << (((float)t)/CLOCKS_PER_SEC) << " secs\n";
		    start = clock();
			//std::vector<std::tuple<bool, std::string, std::vector<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>>>>> queryResults = backend->doQueries(queryReqs);
			std::future<std::vector<std::tuple<bool, std::string, std::vector<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>>>>>> queryResultsFuture = std::async(std::launch::async, &bankBackend::doQueries, backend, queryReqs);
			end = clock();
		    t = end - start;
		    //std::cout << "Time elapsed launching thread for cache/database: " << t << " clicks; " << (((float)t)/CLOCKS_PER_SEC) << " secs\n";
			//start = clock()
			//std::cout << "Launched async tasks, waiting on them\n";
			start = clock();
			std::vector<std::tuple<bool, std::string>> createResults = createResultsFuture.get();
			end = clock();
		    t = end - start;
		    //std::cout << "Time elapsed creating accounts: " << t << " clicks; " << (((float)t)/CLOCKS_PER_SEC) << " secs\n";
			start = clock();
			std::vector<std::tuple<float, std::string>> balanceResults = balanceResultsFuture.get();
			end = clock();
		    t = end - start;
		    //std::cout << "Time elapsed getting balances: " << t << " clicks; " << (((float)t)/CLOCKS_PER_SEC) << " secs\n";
			start = clock();
			std::vector<std::tuple<int, std::string>> transferResults = transferResultsFuture.get();
			end = clock();
		    t = end - start;
		    //std::cout << "Time elapsed doing transfers: " << t << " clicks; " << (((float)t)/CLOCKS_PER_SEC) << " secs\n";
		    start = clock();
			std::vector<std::tuple<bool, std::string, std::vector<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>>>>> queryResults = queryResultsFuture.get();
			dbEnd = clock();
			dbTime = dbEnd - dbStart;
			//std::cout << "Total time on database by async approach: " << dbTime << " clicks; " << (((float)dbTime)/CLOCKS_PER_SEC) << " secs\n";
			end = clock();
			t = end - start;
		    //std::cout << "Time elapsed performing queries: " << t << " clicks; " << (((float)t)/CLOCKS_PER_SEC) << " secs\n";
		    start = clock();
		    std::string successResponse = writer.constructResponse(&createResults, &balanceResults, &transferResults, &queryResults);
		    end = clock();
		    t = end - start;
		    //std::cout << "Time elapsed constructing XML data string: " << t << " clicks; " << (((float)t)/CLOCKS_PER_SEC) << " secs\n";
		    start = clock();
		    respond(connfd, successResponse.c_str(), (int)successResponse.size());
			end = clock();
			t = end - start;
		    //std::cout << "Time elapsed writing out data to socket: " << t << " clicks; " << (((float)t)/CLOCKS_PER_SEC) << " secs\n";
		}
	    //delete test;
	    parser.cleanUp(); // done with request, clean up all XMLParsing resources
		free(reqBuffer);
	}
	else { // bad request
		respond(connfd, errorResponse.c_str(), (int)errorResponse.size());
	}
	close(connfd);
}

void cleanUp() {
	std::cout << "Main thread about to clean up\n";
	backend->cleanUp();
	delete backend;
	int totalServiced = pool->shutdown(); // Allow threadpool to reclaim its resources
	std::cout << "Total requests serviced : " << totalServiced << "\n";
	delete pool;
	close(listenFd);
	XMLPlatformUtils::Terminate();
	exit(1);
}

void handleSignal(int num) {
	std::cout << "Handling signal\n";
	cleanUp();
}

int main() {
	std::cout << "Booyakasha C++!\n";
	listenFd = bindAndListen(8000, 10);
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
    std::cout << "About to spawn threads\n";
    pool = new threadPool(POOL_SIZE, &serviceRequest); // threads created and waiting for requests
	//while(running) {
	while (1) {
		if ((connfd = accept(listenFd, (struct sockaddr *)&cliaddr, &len)) == -1) {
			perror("Unable to accept connection");
  		}
		else {
			//std::cout << "Accepted connection on socket " << connfd << ", enqueueing on threadpool's queue\n";
			pool->enqueueRequest(connfd); // Will be picked up by threads of pool
		}
	}
}

