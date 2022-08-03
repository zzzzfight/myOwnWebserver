// @Author Z
// @Email 3161349290@qq.com

//日志系统中最底层的类，直接进行文件操作
#pragma once

#include <stdio.h>
#include <string>
class FileUtil
{
public:
	FileUtil(const std::string &filename);
	FileUtil(const char *filename);

	~FileUtil();
	void append(const char *msg, size_t len);
	size_t write(const char *msg, size_t len);
	void flush()
	{
		fflush(fp_);
	}

private:
	FILE *fp_;
};