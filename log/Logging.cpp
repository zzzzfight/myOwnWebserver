#include "Logging.h"
#include "../base/CurrentThread.h"
#include "../base/Thread.h"
#include "AsyncLogging.h"
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

static pthread_once_t once_control_ = PTHREAD_ONCE_INIT;
static AsyncLogging *AsyncLogger_;

std::string Logger::logFileName_ = "./WebServer.log";

void once_init()
{
	AsyncLogger_ = new AsyncLogging(Logger::getLogFileName());
	AsyncLogger_->start();
}

void output(const char *msg, int len)
{
	pthread_once(&once_control_, once_init);
	AsyncLogger_->append(msg, len);
}

void stop_thread()
{
	AsyncLogger_->stop();
}

Logger::Impl::Impl(const char *filename, int line, LEVEL lev)
	: stream_(),
	  line_(line),
	  filename_(filename)
{
	formatTime();
	switch (lev)
	{
	case 1:
	{
		stream_ << "[DEBUG]:  ";
		break;
	}
	case 2:
	{
		stream_ << "[WARN]:   ";
		break;
	}
	case 3:
	{
		stream_ << "[ERROR]:  ";
		break;
	}
	default:
	{
		stream_ << "[DEFAULT]: ";
	}
	}
}

void Logger::Impl::formatTime()
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

Logger::Logger(const char *filename, int line, LEVEL lev)
	: impl_(filename, line, lev)
{
}

Logger::~Logger()
{
	impl_.stream_ << " -- " << impl_.filename_ << ':' << impl_.line_ << '\n';

	const LogStream::Buffer &buf(stream().buffer());
	output(buf.data(), buf.length());
}
