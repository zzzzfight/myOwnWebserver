#include "EventLoopThreadPool.h"

#include <assert.h>
#include <iostream>
#include "EventLoop.h"
using namespace std;

EventLoopThreadPool::EventLoopThreadPool(EventLoop *loop, int threadnum)
	: ownLoop_(loop),
	  threadNum_(threadnum),
	  next_(0),
	  started_(false)
{
	// std::cout << "EventLoopThreadPool_loop:" << loop << endl;
	// std::cout << "EventLoopThreadPool_baseloop:" << ownLoop_ << std::endl;
	// std::cout << "EventLoopThreadPool_baseloop->_threadid" << ownLoop_->_threadid << std::endl;
}

void EventLoopThreadPool::threadPoolInit()
{

	ownLoop_->assertInLoopThread();
	started_ = true;
	for (int i = 0; i < threadNum_; i++)
	{
		std::shared_ptr<EventLoopThread> curthread(new EventLoopThread());
		threads_.push_back(curthread);
		EventLoop *loop = curthread->threadStart();
		loops_.push_back(loop);
	}
}

EventLoop *EventLoopThreadPool::getNextPool()
{
	ownLoop_->assertInLoopThread();
	assert(started_);

	EventLoop *loop = ownLoop_;
	// std::cout << "getNextPool:ownLoop_" << ownLoop_ << std::endl;
	// std::cout << "threadnum:" << threadNum_ << endl;
	if (!loops_.empty())
	{
		loop = loops_[next_];
		next_ = (next_ + 1) % threadNum_;
	}
	// std::cout << "curloop:" << loop << endl;

	return loop;
}
