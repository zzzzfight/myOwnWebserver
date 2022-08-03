// @Author Z
// @Email 3161349290@qq.com
#include "LogFile.h"
#include <string>
LogFile::LogFile(const std::string &filename, int flushEveryN)
	: filename_(filename),
	  flushEveryN_(flushEveryN),
	  count_(0),
	  mutex_(new MutexLock()),
	  file_(new FileUtil(filename))
{
}

LogFile::~LogFile()
{
	file_->flush();
}

void LogFile::append(const char *msg, int len)
{
	MutexLockGuard lock(*mutex_);
	append_unlocked(msg, len);
}

void LogFile::append_unlocked(const char *msg, int len)
{
	file_->append(msg, len);
	++count_;
	if (count_ >= flushEveryN_)
	{
		count_ = 0;
		file_->flush();
	}
}

void LogFile::flush()
{
	MutexLockGuard lock(*mutex_);
	file_->flush();
}