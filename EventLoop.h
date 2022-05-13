// @Author Z
// @Email 3161349290@qq.com
#pragma once
#include <vector>
#include <memory>
#include <functional>
#include <assert.h>
#include <sys/epoll.h>
#include "Channel.h"
#include "./base/Util.h"
#include "EPollPoller.h"
#include "./base/MutexLock.h"
#include "./base/CurrentThread.h"
class EventLoop
{
public:
	typedef std::function<void()> Functor;
	typedef std::shared_ptr<Channel> SP_Channel;
	EventLoop();

	~EventLoop();

	//事件池的主体函数
	void loop();

	void quit();

	void runInLoop(Functor cb);

	void queueInLoop(Functor cb);

	void doPendingFunctors();

	void addChannel(SP_Channel channel, int timeout = 0);
	void updateChannel(SP_Channel channel, int timeout = 0);
	void removeChannel(SP_Channel channel);

	bool isInLoopThread() const
	{
		return threadId_ == CurrentThread::tid();
	}
	void assertInLoopThread() { assert(isInLoopThread()); }

	void wakeUp()
	{
		uint64_t one = 1;
		ssize_t n = writen(wakeUpFd_, (char *)(&one), sizeof(one));
		if (n != sizeof(one))
		{
			// log
		}
	}

	void handleRead()
	{
		uint64_t one = 1;
		ssize_t n = readn(wakeUpFd_, &one, sizeof(one));
		wakeUpChannel_->set_events(EPOLLIN | EPOLLET);
	}

	void handleConn()
	{
		updateChannel(wakeUpChannel_, 0);
	}

private:
	static const int EPOLLTIME = 30000;	  //设置epoll_wait的阻塞时间
	std::shared_ptr<EPollPoller> poller_; //管理的EPollPoller类
	bool looping_ = false;
	bool eventHanding_ = false;

	std::vector<Functor> pendingFunctors_;
	bool callingPendingFunctors_ = false;
	MutexLock mutex_;

	// std::vector<Channel>
	std::vector<SP_Channel> activeChannels_;

private:
	int wakeUpFd_;
	std::shared_ptr<Channel> wakeUpChannel_;

private:
	const pid_t threadId_;
};