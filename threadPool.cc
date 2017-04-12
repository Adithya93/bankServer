#include "./threadPool.h"

	threadPool::threadPool(int size, void (*fn)(int), void * args) { // initializes thread pool of given size
		this->task = fn;
		this->args = args;
		active = true;
		totalServiced = 0;
		peakQueueLength = 0;
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
		if (requests.size() > peakQueueLength) {
			peakQueueLength = requests.size();
		}
		// unlock
		queueLock.unlock();
	}

	void threadPool::dequeueRequest() {
		int threadServiced = 0;
		while (active) {
			// acquire lock
			std::unique_lock<std::mutex> queueLock(queueMutex);//, std::defer_lock);			
			// wait while queue is empty
			while (requests.empty() && active) {
				std::cout << "Request queue empty; Worker thread going to sleep\n";
				queueCV.wait(queueLock);
			}
			if (active) {
				//std::cout << "Sleeper has awakened!\n";
				// unlock once connfd extracted
				int requestFd = requests.front();
				requests.pop_front();
				queueLock.unlock();
				// execute func
				task(requestFd);
				threadServiced ++;
				//std::cout << "Thread " << std::this_thread::get_id() << " has now serviced " << threadServiced << " requests\n";
			}
			else {
				std::cout << "Thread awakened to exit\n";
				queueLock.unlock();
			}
		}
		//std::cout << "Thread serviced total of " << threadServiced << " requests\n";
		// acquire exit lock
		std::unique_lock<std::mutex> exitLock(exitMutex);//, std::defer_lock);
		totalServiced += threadServiced;
		if (--liveThreads == 0) { // notify main thread that pool is ready for shutdown
			exitCV.notify_one();
			std::cout << "All threads exited, ready for shutdown\n";
		}
		else {
			//std::cout << liveThreads << " threads left\n";
		}
		// release exit lock
		//exitLock.unlock(); exitLock will get automatically unlocked as it goes out of scope
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
		std::cout << "Main thread about to reap dead threads\n";
		for (std::deque<std::thread*>::iterator it = workers.begin(); it < workers.end(); it ++) {
			(*it)->join();
			//std::cout << "About to delete thread\n";
			delete(*it);
		}
		std::cout << "Threadpool serviced a total of " << totalServiced << " requests. Deallocated all threadpool memory. Exiting.\n";
		std::cout << "Peak queue length: " << peakQueueLength << "\n";
		return totalServiced;
	}
