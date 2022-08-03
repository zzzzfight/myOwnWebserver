#include "AsyncLogging.h"
#include <assert.h>

#include <iostream>
using namespace std;

AsyncLogging::AsyncLogging(const std::string &filename, int flushInterVal)
	: logfile_(filename),
	  running_(false),
	  filename_(filename),
	  flushInterval_(flushInterVal),
	  thread_(std::bind(&AsyncLogging::threadFunc, this), "logging thread"),
	  mutex_(new MutexLock()),
	  cond_(new Condition(*mutex_)),
	  currentBuffer_(new Buffer),
	  nextBuffer_(new Buffer),
	  latch_(1)
{
	bufferPtrs_.reserve(16);
	currentBuffer_->bzero();
	nextBuffer_->bzero();
}

void AsyncLogging::append(const char *msg, int len)
{
	MutexLockGuard lock(*mutex_);
	if (currentBuffer_->avail() >= len)
	{
		currentBuffer_->append(msg, len);
		// currentBuffer_->data()
		// cout << currentBuffer_->data() << endl;
	}
	else
	{
		bufferPtrs_.push_back(std::move(currentBuffer_));
		if (nextBuffer_)
		{
			currentBuffer_ = std::move(nextBuffer_);
		}
		else
		{
			currentBuffer_.reset(new Buffer());
		}
		currentBuffer_->append(msg, len);
	}
	cond_->notify();
}

void AsyncLogging::threadFunc()
{
	latch_.countDown();
	std::shared_ptr<Buffer> newBuffer1(new Buffer);
	std::shared_ptr<Buffer> newBuffer2(new Buffer);
	newBuffer1->bzero();
	newBuffer2->bzero();

	BufferPtrVector buffersWaitToWrite;
	buffersWaitToWrite.reserve(16);

	while (running_)
	{
		{
			MutexLockGuard lock(*mutex_);
			if (bufferPtrs_.empty())
			{
				cond_->waitForSeconds(flushInterval_);
			}
			bufferPtrs_.push_back(std::move(currentBuffer_));
			currentBuffer_ = std::move(newBuffer1);
			buffersWaitToWrite.swap(bufferPtrs_);
			cout << buffersWaitToWrite.size();
			if (!nextBuffer_)
			{
				nextBuffer_ = std::move(newBuffer2);
			}
		}

		if (bufferPtrs_.size() > 25)
		{
			buffersWaitToWrite.erase(buffersWaitToWrite.begin() + 2, buffersWaitToWrite.end());
		}

		// for (auto &curBuffer : buffersWaitToWrite)
		// {
		// 	logfile_.append(curBuffer->data(), curBuffer->length());
		// }
		for (int i = 0; i < buffersWaitToWrite.size(); ++i)
		{
			logfile_.append(buffersWaitToWrite[i]->data(), buffersWaitToWrite[i]->length());
			cout << buffersWaitToWrite[i]->data() << endl;
		}

		if (buffersWaitToWrite.size() > 2)
		{
			buffersWaitToWrite.resize(2);
		}
		if (!newBuffer1)
		{
			assert(!buffersWaitToWrite.empty());
			newBuffer1 = buffersWaitToWrite.back();
			buffersWaitToWrite.pop_back();
			newBuffer1->reset();
			// newBuffer1->bzero();
		}
		if (!newBuffer2)
		{
			assert(!buffersWaitToWrite.empty());
			newBuffer1 = buffersWaitToWrite.back();
			buffersWaitToWrite.pop_back();
			newBuffer2->reset();
			// newBuffer1->bzero();
		}
		buffersWaitToWrite.clear();
		logfile_.flush();
	}
	bufferPtrs_.push_back(std::move(currentBuffer_));
	// for (auto &bP : bufferPtrs_)
	// {
	// 	logfile_.append(bP->data(), bP->length());
	// }
	for (int i = 0; i < bufferPtrs_.size(); ++i)
	{
		logfile_.append(bufferPtrs_[i]->data(), bufferPtrs_[i]->length());
	}

	bufferPtrs_.clear();
	logfile_.flush();
}