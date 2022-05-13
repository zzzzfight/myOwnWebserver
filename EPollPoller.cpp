// @Author Z
// @Email 3161349290@qq.com
#include "EPollPoller.h"
#include "Channel.h"

#include <assert.h>
#include <unistd.h>
#include <errno.h>
// #include <sys/epoll.h>
#include <iostream>
#include <sys/epoll.h>
#include "Timer.h"

EPollPoller::EPollPoller()
	: epollfd_(epoll_create1(EPOLL_CLOEXEC))
{
	assert(epollfd_ > 0);
}

EPollPoller::~EPollPoller()
{
	close(epollfd_);
}
void EPollPoller::poll(ChannelList *activeChannels, size_t timeout)
{
	// int numEvents = epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeout);
	// int numEvents = epoll_wait(epollfd_, &*events_.begin(), 1024, timeout);
	int numEvents = epoll_wait(epollfd_, events_, 1024, timeout);

	int saveErrno = errno;
	while (true)
	{
		if (numEvents > 0)
		{
			this->fillActiveChannels(numEvents, activeChannels);
		}
		else if (numEvents == 0)
		{
			// std::cout << "nothing happened" << std::endl;
		}
		else
		{
			if (errno != EINTR)
				// std::cout << "EPollPoller::poll errno:" << (int)errno << std::end;
				errno = saveErrno;
		}
		if (activeChannels->size() > 0)
			return;
	}
}

void EPollPoller::addChannel(SP_Channel channel, size_t timeout)
{
	int fd = channel->fd();
	if (timeout > 0)
	{
		//计时器管理 http类
		httpdatas_[fd] = channel->getHolder();
		addTimer(channel, timeout);
	}
	struct epoll_event event;

	assert(fd);

	event.data.fd = fd;
	event.events = channel->events();

	assert(event.events != 0);
	assert(epollfd_);

	int ret = epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &event);

	assert(ret == 0);

	channels_[fd] = channel;
}

//修改相应channel的事件
void EPollPoller::updateChannel(SP_Channel channel, size_t timeout)
{
	int fd = channel->fd();

	if (timeout > 0)
	{
		//计时器管理 http类
		addTimer(channel, timeout);
		httpdatas_[fd] = channel->getHolder();
	}

	assert(fd);

	struct epoll_event event;
	event.data.fd = fd;
	event.events = channel->events();

	assert(event.events != 0);
	assert(epollfd_);

	int ret = epoll_ctl(epollfd_, EPOLL_CTL_MOD, fd, &event);

	assert(ret == 0);

	channels_[fd] = channel;
}

void EPollPoller::removeChannel(SP_Channel channel)
{
	int fd = channel->fd();

	assert(fd);

	struct epoll_event event;
	event.data.fd = fd;
	event.events = channel->events();

	int ret = epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd, &event);

	assert(ret == 0);

	channels_.erase(fd);
	httpdatas_.erase(fd);
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels)
{
	for (int i = 0; i < numEvents; ++i)
	{
		int fd = events_[i].data.fd;

		ChannelMap::const_iterator it = channels_.find(fd);
		assert(it != channels_.end());

		SP_Channel channel = channels_[fd];
		channel->set_revents(events_[i].events);
		activeChannels->push_back(channel);
	}
}