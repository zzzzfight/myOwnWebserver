#include <fcntl.h>
#include <sys/unistd.h>
// #include <string>
#include <stdio.h>
#include <fstream>
#include <string>
#include <iostream>
#include <vector>
#include <memory>
#include "LogStream.h"
// #include <thread>
#include "Thread.h"
#include "MutexLock.h"
#include "Condition.h"
#include <assert.h>
#include <functional>
#include <sys/time.h>
using namespace std;

const int LEN = 40000;
class FileUtil
{
public:
	FileUtil(string filename) : filename_(filename), fp_(fopen(filename.c_str(), "ae"))
	{
		// ofs.open(filename, std::ofstream::app | std::ofstream::out);
		setbuffer(fp_, buffer, sizeof buffer);
	}
	~FileUtil()
	{
		fflush(fp_);
	}
	void flush()
	{
		fflush(fp_);
	}
	void append(const char *logline, int len)
	{
		int n = fwrite_unlocked(logline, 1, len, fp_);
		int remain = len - n;
		while (remain)
		{
			int x = fwrite_unlocked(logline + n, 1, len, fp_);
			if (x == 0)
			{
				cout << "fwrite error\n";
				break;
			}
			remain -= x;
			// logline += n;
			n += x;
		}
	}

	void thread_func()
	{
		vector<shared_ptr<FixedBuffer<LEN>>> bufferstoWrite;
	}

private:
	string filename_;
	// FILE *fp_;
	FILE *fp_;
	ofstream ofs;
	char buffer[20];
};

class Async
{
public:
	Async(string filename)
		: fu_(filename.c_str()),
		  filename_(filename.c_str()),
		  mutex_(new MutexLock),
		  cond_(new Condition(*mutex_)),
		  thread_(bind(&Async::threadfunc, this), "Logging"),
		  running_(false),
		  latch_(1),
		  currentBuffer_(new FixedBuffer<LEN>),
		  nextBuffer_(new FixedBuffer<LEN>)
	{
		currentBuffer_->bzero();
		nextBuffer_->bzero();
		Buffers_.reserve(16);
	}
	~Async()
	{
		if (running_)
		{
			stop();
		}
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

private:
	FileUtil fu_;

	const char *filename_;

public:
	void append(const char *msg, int len)
	{
		MutexLockGuard lock(*mutex_);
		if (currentBuffer_->avail() >= len)
		{
			currentBuffer_->append(msg, len);
		}
		else
		{
			Buffers_.push_back(move(currentBuffer_));
			// currentBuffer_.reset();
			if (nextBuffer_)
			{
				currentBuffer_ = std::move(nextBuffer_);
			}
			else
			{
				currentBuffer_.reset(new FixedBuffer<LEN>);
			}
			currentBuffer_->append(msg, len);
		}
		cond_->notify();
	}

	void threadfunc()
	{
		latch_.countDown();
		shared_ptr<FixedBuffer<LEN>> newbuffer1(new FixedBuffer<LEN>());
		shared_ptr<FixedBuffer<LEN>> newbuffer2(new FixedBuffer<LEN>());
		newbuffer1->bzero();
		newbuffer2->bzero();

		vector<shared_ptr<FixedBuffer<LEN>>> bufferstowrite;
		bufferstowrite.reserve(16);
		while (running_)
		{
			{
				MutexLockGuard lock(*mutex_);
				if (Buffers_.empty())
				{
					cond_->waitForSeconds(20);
				}
				Buffers_.push_back(move(currentBuffer_));
				currentBuffer_ = move(newbuffer1);
				bufferstowrite.swap(Buffers_);
				if (!nextBuffer_)
				{
					nextBuffer_ = move(newbuffer2);
				}
			}
			// flush();
			if (Buffers_.size() > 25)
			{
				bufferstowrite.erase(bufferstowrite.begin() + 2, bufferstowrite.end());
			}
			for (size_t i = 0; i < bufferstowrite.size(); ++i)
			{
				fu_.append(bufferstowrite[i]->data(), bufferstowrite[i]->length());
			}
			if (bufferstowrite.size() > 2)
			{
				bufferstowrite.resize(2);
			}

			if (!newbuffer1)
			{
				assert(!bufferstowrite.empty());
				newbuffer1 = bufferstowrite.back();
				bufferstowrite.pop_back();
				newbuffer1->reset();
			}
			if (!newbuffer2)
			{
				assert(!bufferstowrite.empty());
				newbuffer2 = bufferstowrite.back();
				bufferstowrite.pop_back();
				newbuffer2->reset();
			}
			bufferstowrite.clear();
			fu_.flush();
		}
		if (currentBuffer_)
		{
			Buffers_.push_back(move(currentBuffer_));
		}
		if (Buffers_.size() > 0)
		{
			for (int i = 0; i < Buffers_.size(); ++i)
			{
				fu_.append(Buffers_[i]->data(), Buffers_[i]->length());
			}
		}
		Buffers_.clear();
		fu_.flush();
	}

	shared_ptr<FixedBuffer<LEN>> currentBuffer_;
	shared_ptr<FixedBuffer<LEN>> nextBuffer_;
	vector<shared_ptr<FixedBuffer<LEN>>> Buffers_;
	Thread thread_;
	unique_ptr<MutexLock> mutex_;
	unique_ptr<Condition> cond_;
	bool running_;
	CountDownLatch latch_;
	static string logFileName_;
};
// class Logger;
// static pthread_once_t once_control_ = PTHREAD_ONCE_INIT;
// static Async *AsyncLogger_;

void once_init();
void output(const char *msg, int len);

class Logger
{
private:
public:
	Logger(const char *fileName, int line) : impl_(fileName, line)
	{
	}
	~Logger()
	{
		impl_.stream_ << " -- " << impl_.basename_ << ':' << impl_.line_ << '\n';
		const LogStream::Buffer &buf(stream().buffer());
		output(buf.data(), buf.length());
	}

	static void setLogFileName(std::string filename)
	{
		logFileName_ = filename;
	}
	static string getLogFileName()
	{
		return logFileName_;
	}
	LogStream &stream()
	{
		return impl_.stream_;
	}

private:
	class Impl
	{
	public:
		Impl(const char *fileName, int line)
			: line_(line),
			  basename_(fileName)
		{
			formatTime();
		}
		void formatTime()
		{
			struct timeval tv;
			time_t time;
			char str_t[26] = {0};
			gettimeofday(&tv, NULL);
			time = tv.tv_sec;
			struct tm *p_time = localtime(&time);
			strftime(str_t, 26, "%Y-%m-%d %H:%M:%S\n", p_time);
			stream_ << str_t;
		}
		int line_;
		string basename_;
		LogStream stream_;
	};
	Impl impl_;
	static string logFileName_;
};

static Async *AsyncLogger_;
static pthread_once_t once_control_ = PTHREAD_ONCE_INIT;
void once_init()
{
	AsyncLogger_ = new Async(Logger::getLogFileName());
	AsyncLogger_->start();
}

void output(const char *msg, int len)
{
	pthread_once(&once_control_, once_init);
	AsyncLogger_->append(msg, len);
}

std::string Logger::logFileName_ = "./test.txt";
#define LOG Logger(__FILE__, __LINE__).stream()

int main()
{
	// printf("%s,%d", __FILE__, __LINE__);

	for (int i = 0; i < 100000; ++i)
	{
		LOG << "HELLOWORLD";
	}
	AsyncLogger_->stop();
	// sleep(20);
}