// @Author Z
// @Email 3161349290@qq.com
#include "Channel.h"
#include <sys/epoll.h>

Channel::Channel(EventLoop *loop, int fd)
	: ownerLoop_(loop),
	  fd_(fd)
{
}

void Channel::handleEvents()
{
	// events_
	eventHandling_ = true;
	if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
	{
		events_ = 0;
		if (closeCallback_)
			closeCallback_();
		return;
	}
	if (revents_ & EPOLLERR)
	{
		if (errorCallback_)
			errorCallback_();
		return;
	}
	if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
	{
		if (readCallback_)
			readCallback_();
	}
	if (revents_ & EPOLLOUT)
	{
		if (writeCallback_)
			writeCallback_();
	}
	if (connCallback_)
		connCallback_();
	eventHandling_ = false;
	std::cout << "this->http_.use_count():" << this->http_.use_count() << std::endl;
	return;
}