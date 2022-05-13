// @Author Z
// @Email 3161349290@qq.com
#include "Acceptor.h"
#include "EventLoop.h"
#include "./base/Util.h" //未修改
#include "Channel.h"
// #include"EventLoopThreadPool.h"

#include <memory>
#include <functional>
#include <iostream>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
// const
Acceptor::Acceptor(EventLoop *loop, int threadnum, int port)
	: ownerloop_(loop),
	  port_(port),
	  acceptFd_(socket_bind_listen(port)),
	  acceptChannel_(new Channel(loop, acceptFd_)),
	  threadnum_(threadnum),
	  eventLoopThreadPool_(new EventLoopThreadPool(loop, threadnum))
{
	assert(ownerloop_);
	assert(acceptFd_);
	assert(acceptChannel_);
	assert(eventLoopThreadPool_);

	setSocketNonBlocking(acceptFd_);
	setSocketNodelay(acceptFd_);
	handle_for_sigpipe(); //忽略SIGPIPE
}

void Acceptor::start()
{
	//线程开启初始化
	eventLoopThreadPool_->threadPoolInit();
	acceptChannel_->setReadCallback(std::bind(&Acceptor::handleNewConn, this));
	acceptChannel_->setConnCallback(std::bind(&Acceptor::handleThisConn, this));
	acceptChannel_->set_events(EPOLLIN | EPOLLET);
	ownerloop_->addChannel(acceptChannel_);
}

void Acceptor::handleThisConn()
{
	ownerloop_->updateChannel(acceptChannel_, 0);
}

void Acceptor::handleNewConn()
{
	struct sockaddr_in clientAddr;
	socklen_t clientAddrLen = sizeof(clientAddr);
	memset(&clientAddr, '0', sizeof(struct sockaddr_in));
	int acceptFd = 0;
	while ((acceptFd = accept(acceptFd_, (struct sockaddr *)&clientAddr, &clientAddrLen)) > 0)
	{
		if (acceptFd > MAXFDS)
		{
			close(acceptFd);
			continue;
		}
		if (setSocketNonBlocking(acceptFd) < 0)
		{
			std::cout << "Acceptor::handleNewConn()::setSocketNonBlocking error" << std::endl;
			return;
		}
		setSocketNodelay(acceptFd);

		EventLoop *loop = eventLoopThreadPool_->getNextPool();

		assert(loop);
		shared_ptr<HttpData> newHttpConn(new HttpData(loop, acceptFd));
		newHttpConn->ownchannel_->setHolder(newHttpConn);

		loop->queueInLoop(std::bind(&HttpData::newEvent, newHttpConn));
	}
	acceptChannel_->set_events(EPOLLIN | EPOLLET);
}