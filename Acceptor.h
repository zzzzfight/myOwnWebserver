// @Author Z
// @Email 3161349290@qq.com
#pragma once
#include <memory>
class Channel;

class EventLoop;
#include"EventLoopThreadPool.h"

class Acceptor
{
public:
	Acceptor(EventLoop *loop, int threadnum = 0, int port = 9006);

	void start();

	void handleNewConn();

	void handleThisConn();
	// void Handle

private:
	static const int MAXFDS = 4096;
	// std::shared_ptr<EventLoop> ownerloop_;
	EventLoop *ownerloop_;

	int port_;
	int acceptFd_;

	std::shared_ptr<Channel> acceptChannel_; //需要写在acceptFd_之后,保证构造器初始化顺序
											 //线程池相关
private:
	int threadnum_;
	std::unique_ptr<EventLoopThreadPool> eventLoopThreadPool_;
};