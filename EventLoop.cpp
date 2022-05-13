// @Author Z
// @Email 3161349290@qq.com
#include "EventLoop.h"
#include "EPollPoller.h"
#include "./base/MutexLock.h"
#include "./base/CurrentThread.h"

#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/syscall.h>

#include <iostream>

int createEventFd()
{
	int evtfd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
	if (evtfd < 0)
	{
		abort();
	}
	return evtfd;
}

__thread EventLoop *t_loopInThisThread = 0;

// namespace CurrentThread
// {
// 	__thread int t_cachedTid = 0;
// 	__thread char t_tidString[32];
// 	__thread int t_tidStringLength = 6;
// 	__thread const char *t_threadName = "default";
// }

// pid_t gettid() { return static_cast<pid_t>(::syscall(SYS_gettid)); }

// void CurrentThread::cacheTid()
// {
// 	if (t_cachedTid == 0)
// 	{
// 		t_cachedTid = gettid();
// 		t_tidStringLength =
// 			snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
// 	}
// }

EventLoop::EventLoop()
	: poller_(new EPollPoller()),
	  looping_(false),
	  eventHanding_(false),
	  callingPendingFunctors_(false),
	  threadId_(CurrentThread::tid()),
	  wakeUpFd_(createEventFd()),
	  wakeUpChannel_(new Channel(this, wakeUpFd_))
{
	if (!t_loopInThisThread)
	{
	}
	else
	{
		t_loopInThisThread = this;
	}
	wakeUpChannel_->set_events(EPOLLIN | EPOLLET);
	wakeUpChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
	wakeUpChannel_->setConnCallback(std::bind(&EventLoop::handleConn, this));
	poller_->addChannel(wakeUpChannel_, 0);
}

EventLoop::~EventLoop()
{
	close(wakeUpFd_);
	t_loopInThisThread = NULL;
}

void EventLoop::loop()
{
	looping_ = true;
	while (looping_)
	{
		activeChannels_.clear();
		poller_->poll(&activeChannels_, EPOLLTIME);
		// cout << "poller" << endl;
		eventHanding_ = true;
		for (auto activechannel : activeChannels_)
		{
			activechannel->handleEvents();
		}
		eventHanding_ = false;

		doPendingFunctors();

		//计时器删除阶段
		poller_->handleExpired();
	}
}

void EventLoop::quit()
{
	looping_ = false;
	if (!isInLoopThread())
	{
		wakeUp();
	}
}

void EventLoop::runInLoop(Functor cb)
{
	if (isInLoopThread())
	{
		cb();
	}
	else
	{
		queueInLoop(std::move(cb));
	}
}

void EventLoop::queueInLoop(Functor cb)
{
	std::cout << "new task\n";

	{
		MutexLockGuard lock(mutex_);
		pendingFunctors_.push_back(std::move(cb));
	}
	if (!isInLoopThread() || callingPendingFunctors_)
	{
		wakeUp();
	}
}

void EventLoop::doPendingFunctors()
{
	std::vector<Functor> functors;
	callingPendingFunctors_ = true;
	{
		MutexLockGuard lock(mutex_);
		functors.swap(pendingFunctors_);
	}
	for (const Functor &functor : functors)
	{
		functor();
	}
	callingPendingFunctors_ = false;
}

void EventLoop::addChannel(SP_Channel channel, int timeout)
{
	poller_->addChannel(channel, timeout);
}

void EventLoop::updateChannel(SP_Channel channel, int timeout)
{
	poller_->updateChannel(channel, timeout);
}
void EventLoop::removeChannel(SP_Channel channel)
{
	poller_->removeChannel(channel);
}