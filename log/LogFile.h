// @Author Z
// @Email 3161349290@qq.com
#pragma once
#include "../base/MutexLock.h"
#include "../base/noncopyable.h"
#include "FileUtil.h"
#include <string>
#include <memory>
class LogFile : noncopyable
{
public:
	LogFile(const std::string &filename, int flushEveryN = 1024);
	~LogFile();
	void flush();
	void append(const char *msg, int len);

private:
	void append_unlocked(const char *msg, int len);
	const std::string filename_;
	const int flushEveryN_;
	int count_;
	std::unique_ptr<MutexLock> mutex_;
	std::unique_ptr<FileUtil> file_;
};