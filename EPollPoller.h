// @Author Z
// @Email 3161349290@qq.com
#pragma once

#include <sys/epoll.h>
#include <sys/errno.h>

#include <vector>
#include <unordered_map>
#include <iostream>

#include "Channel.h"
#include "HttpData.h"
#include "Timer.h"
class EventLoop;
class EPollPoller
{
public:
	typedef std::shared_ptr<Channel> SP_Channel;
	typedef std::vector<SP_Channel> ChannelList;
	EPollPoller();
	~EPollPoller();

	void poll(ChannelList *activeChannels, size_t timeout);

	//向内核中添加监听事件
	void addChannel(SP_Channel channel, size_t timeout = 0);

	//修改相应channel的事件
	void updateChannel(SP_Channel channel, size_t timeout = 0);

	void removeChannel(SP_Channel channel);

	typedef std::vector<struct epoll_event> EventList;

	void fillActiveChannels(int numEvents, ChannelList *activeChannels);

	void addTimer(SP_Channel channel, int timeout)
	{
		std::shared_ptr<HttpData> t = channel->getHolder();
		if (t)
		{
			t->setTimerHolder(timermanager_.addTimer(t, timeout));
		}
		else
		{
			std::cout << "timer add fail\n";
		}
	}
	void handleExpired()
	{
		// std::cout << "chanels_.size():" << channels_.size() << std::endl;
		// std::cout << "timer deleting\n";
		timermanager_.handleExpiredEvent();
	}

protected:
	typedef std::unordered_map<int, SP_Channel> ChannelMap;
	ChannelMap channels_;
	// std::vector<std::shared_ptr<SP_Channel>> channels_;
	typedef std::shared_ptr<HttpData> SP_HttpData;
	typedef std::unordered_map<int, SP_HttpData> HttpDataMap;
	HttpDataMap httpdatas_;

private:
	int epollfd_;
	// EventList events_;
	static const int MAXEVENTS = 1024;
	struct epoll_event events_[MAXEVENTS];
	// EventLoop *ownerLoop_;

private:
	Timer timermanager_;
};