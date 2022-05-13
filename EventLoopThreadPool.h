#pragma once
#include <vector>
#include <memory>
#include "EventLoopThread.h"
// class EventLoopThread

class EventLoop;

class EventLoopThreadPool
{
public:
	EventLoopThreadPool(EventLoop *loop, int threadnum = 8);

	EventLoop *getNextPool();

	void threadPoolInit();

private:
	std::vector<EventLoop *> loops_;
	std::vector<std::shared_ptr<EventLoopThread>> threads_;
	int threadNum_;
	int next_;

	EventLoop *ownLoop_;
	bool started_;
	// EventLoop
};