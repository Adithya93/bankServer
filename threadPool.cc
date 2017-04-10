#include "./threadPool.h"

	threadPool::threadPool(int size, void (*fn)(int), void * args) { // initializes thread pool of given size
		this->task = fn;
		this->args = args;
		active = true;
		totalServiced = 0;
		for (int workerNum = 0; workerNum < size; workerNum ++) {
			workers.push_back(new std::thread(&threadPool::dequeueRequest, this));
		}
		liveThreads = size;
		std::cout << "Initialized threads\n";
	}
	
	void threadPool::enqueueRequest(int connfd) {
		// acquire lock
		std::unique_lock<std::mutex> queueLock(queueMutex);
		// TO-DO : WAIT WHILE QUEUE IS AT MAX QUEUE LENGTH?
		// push connfd onto queue
		requests.push_front(connfd);
		// signal
		queueCV.notify_one();
		// unlock
		queueLock.unlock();
	}

	void threadPool::dequeueRequest() {
		int threadServiced = 0;
		while (active) {
			// acquire lock
			std::unique_lock<std::mutex> queueLock(queueMutex);//, std::defer_lock);			
			// wait while queue is empty
			while (requests.empty()) {
				std::cout << "Request queue empty; Worker thread going to sleep\n";
				queueCV.wait(queueLock);
			}
			std::cout << "Sleeper has awakened!\n";
			// unlock once connfd extracted
			int requestFd = requests.front();
			requests.pop_front();
			queueLock.unlock();
			// execute func
			task(requestFd);
			threadServiced ++;
			std::cout << "Thread has now serviced " << threadServiced << " requests\n";
		}
		std::cout << "Thread serviced total of " << threadServiced << " requests\n";
		// acquire exit lock
		std::unique_lock<std::mutex> exitLock(exitMutex);//, std::defer_lock);
		totalServiced += threadServiced;
		if (--liveThreads == 0) { // notify main thread that pool is ready for shutdown
			exitCV.notify_one();
			std::cout << "All threads exited, ready for shutdown\n";
		}
		else {
			std::cout << liveThreads << " threads left\n";
		}
		// release exit lock
		exitLock.unlock();
		// thread exits
	}

	int threadPool::shutdown() { // deletes all threads and queue
		// acquire exit lock
		std::unique_lock<std::mutex> exitLock(exitMutex);
		// acquire queue lock
		std::unique_lock<std::mutex> queueLock(queueMutex);
		active = false;
		// broadcast to threads
		queueCV.notify_all();
		// unlock queue lock
		queueLock.unlock();
		// wait on exit CV signal from last exiting thread
		exitCV.wait(exitLock);
		for (std::deque<std::thread*>::iterator it = workers.begin(); it < workers.end(); it ++) {
			delete(*it);
		}
		std::cout << "Threadpool serviced a total of " << totalServiced << " requests. Deallocated all threadpool memory. Exiting.\n";
		return totalServiced;
	}
