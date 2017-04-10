#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
//#include <pthread.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>

class threadPool {

	public:
		threadPool(int size, void (*fn)(int), void * args = NULL); // initializes thread pool of given size

		void enqueueRequest(int connfd);

		void dequeueRequest();

		int shutdown(); // deletes all threads and queue

	private:
		void (*task)(int);
		void * args;
		bool active;
		std::deque<int> requests;
		std::deque<std::thread*> workers;
		std::mutex queueMutex;
		std::mutex exitMutex;
		std::condition_variable queueCV;
		std::condition_variable exitCV;
		int totalServiced;
		int liveThreads;

};