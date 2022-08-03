#pragma once
#include <stdio.h>
#include "LogStream.h"
#include <string>
#include <stdio.h>
#include <pthread.h>
enum LEVEL
{
	DEFAULT = 0,
	DEBUG,
	WARN,
	ERROR
};
void stop_thread();
class Logger
{

public:
	Logger(const char *filename, int line, LEVEL lev = DEFAULT);
	~Logger();
	LogStream &stream() { return impl_.stream_; }
	static void setLogFileName(std::string filename)
	{
		logFileName_ = filename;
	}
	static std::string getLogFileName()
	{
		return logFileName_;
	}

private:
	class Impl
	{
	public:
		Impl(const char *filename, int line, LEVEL lev);
		void formatTime();
		LogStream stream_;
		std::string filename_;
		int line_;
	};

	// public:
	Impl impl_;

	LEVEL lev_;
	static std::string logFileName_;
};

#define DEFAULT Logger(__FILE__, __LINE__, DEFAULT).stream()
#define ERROR Logger(__FILE__, __LINE__, ERROR).stream()
#define WARN Logger(__FILE__, __LINE__, WARN).stream()
#define DEBUG Logger(__FILE__, __LINE__, DEBUG).stream()

#define STOP AsyncLogger_