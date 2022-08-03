// @Author Z
// @Email 3161349290@qq.com

#pragma once
#include <memory>
#include <functional>
#include <vector>
#include <string>
#include "LogStream.h"
#include "LogFile.h"

#include "../base/MutexLock.h"
#include "../base/Thread.h"
#include "../base/noncopyable.h"
#include "../base/CountDownLatch.h"

class AsyncLogging : noncopyable
{
public:
	AsyncLogging(const std::string &filename, int flushInterval = 2);
	~AsyncLogging()
	{
		if (running_)
			stop();
	}

	void start()
	{
		running_ = true;
		thread_.start();
		latch_.wait();
	}

	void stop()
	{
		running_ = false;
		cond_->notify();
		thread_.join();
	}

	void append(const char *msg, int len);

	void threadFunc();

public:
	static std::string logFileName_;

private:
	using Buffer = FixedBuffer<kLargerBuffer>;
	using BufferPtr = std::shared_ptr<Buffer>;
	using BufferPtrVector = std::vector<BufferPtr>;
	LogFile logfile_;
	bool running_;
	const std::string filename_;
	const int flushInterval_;
	Thread thread_;
	std::unique_ptr<MutexLock> mutex_;
	std::unique_ptr<Condition> cond_;
	BufferPtr currentBuffer_;
	BufferPtr nextBuffer_;
	BufferPtrVector bufferPtrs_;
	CountDownLatch latch_;
};
