#include "FileUtil.h"
#include <string>

FileUtil::FileUtil(const std::string &filename)
	: fp_(fopen(filename.c_str(), "ae"))
{
}

FileUtil::FileUtil(const char *filename)
	: fp_(fopen(filename, "ae"))
{
}

FileUtil::~FileUtil()
{
	fflush(fp_);
	fclose(fp_);
}

// void append(const char *msg, int len)
// {
// 	int x = fwrite_unlocked()
// }
void FileUtil::append(const char *msg, size_t len)
{
	size_t n = this->write(msg, len);
	size_t remain = len - n;
	while (remain > 0)
	{
		size_t x = this->write(msg + n, remain);
		if (x == 0)
		{
			int err = ferror(fp_);
			if (err)
				fprintf(stderr, "AppendFile::append() failed !\n");
			break;
		}
		n += x;
		remain -= x;
	}
}

// void FileUtil::flush()
// {
// 	fflush(fp_);
// }

size_t FileUtil::write(const char *msg, size_t len)
{
	return fwrite_unlocked(msg, 1, len, fp_);
}