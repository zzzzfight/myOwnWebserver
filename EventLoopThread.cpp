#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>

#include "EventLoop.h"
#include "EventLoopThread.h"
#include "./base/CurrentThread.h"
#include "./base/CountDownLatch.h"

#include <iostream>
#include <functional>
namespace CurrentThread
{
	__thread int t_cachedTid = 0;
	__thread char t_tidString[32];
	__thread int t_tidStringLength = 6;
	__thread const char *t_threadName = "default";
} // namespace CurrentThread

pid_t gettid() { return static_cast<pid_t>(::syscall(SYS_gettid)); }

void CurrentThread::cacheTid()
{
	if (t_cachedTid == 0)
	{
		t_cachedTid = gettid();
		t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
	}
}

EventLoopThread::EventLoopThread()
	: ownloop_(nullptr),
	  latch_(1) {}

EventLoopThread::~EventLoopThread()
{
	if (ownloop_)
	{
		ownloop_->quit();
		pthread_detach(threadId_);
	}
}

EventLoop *EventLoopThread::threadStart()
{
	if (pthread_create(&threadId_, NULL, threadFunc, this))
	{
		started_ = false;
		return nullptr;
	}
	else
	{
		latch_.wait();
		// std::cout << pthread_self() << ":_2\n";
		started_ = true;
		return ownloop_;
	}
}

void *threadFunc(void *obj)
{

	EventLoopThread *temp = (EventLoopThread *)obj;

	EventLoop loop;

	temp->ownloop_ = &loop;
	temp->latch_.countDown();

	temp->ownloop_->loop();

	temp->ownloop_ = nullptr;
	return nullptr;
}