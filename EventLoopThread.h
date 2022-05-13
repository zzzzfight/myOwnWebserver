#pragma once

#include <pthread.h>

#include "./base/CountDownLatch.h"
class EventLoop;
// #include "EventLoop.h"

void *threadFunc(void *obj);

class EventLoopThread
{
public:
	EventLoopThread();
	~EventLoopThread();

	//线程开启运行，返回启动的loop地址
	EventLoop *threadStart();

public:
//或者将threadFunc 写成类中的static函数，然后私有化这些成员
	CountDownLatch latch_;
	EventLoop *ownloop_;

private:
	bool started_;
	pthread_t threadId_;
};