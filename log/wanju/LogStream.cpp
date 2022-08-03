#include "LogStream.h"
#include <string>
#include <string.h>

// const int kSmallBuffer = 4000;
const char digits[] = "9876543210123456789";
const char *zero = digits + 9;

template <typename T>
size_t convert(char buf[], T value)
{
	T i = value;
	char *p = buf;
	do
	{
		int lsd = static_cast<int>(i % 10);
		i /= 10;
		*p++ = zero[lsd];
	} while (i != 0);

	if (value < 0)
	{
		*p++ = '-';
	}

	*p = '\0';
	std::reverse(buf, p);
	return p - buf;
}

LogStream &LogStream::operator<<(bool v)
{
	buffer_.append(v ? "1" : "0", 1);
	return *this;
}
LogStream &LogStream::operator<<(char v)
{
	buffer_.append(&v, 1);
	return *this;
}
LogStream &LogStream::operator<<(short v)
{
	*this << static_cast<int>(v);
	return *this;
}
LogStream &LogStream::operator<<(unsigned short v)
{
	*this << static_cast<int>(v);
	return *this;
}
LogStream &LogStream::operator<<(int v)
{
	formatInterger(v);
	return *this;
}
LogStream &LogStream::operator<<(unsigned int v)
{
	formatInterger(v);
	return *this;
}
LogStream &LogStream::operator<<(long v)
{
	formatInterger(v);
	return *this;
}
LogStream &LogStream::operator<<(unsigned long v)
{
	formatInterger(v);
	return *this;
}
LogStream &LogStream::operator<<(long long v)
{
	formatInterger(v);
	return *this;
}
LogStream &LogStream::operator<<(unsigned long long v)
{
	formatInterger(v);
	return *this;
}
LogStream &LogStream::operator<<(float v)
{
	*this << static_cast<double>(v);
	return *this;
}
LogStream &LogStream::operator<<(double v)
{
	if (buffer_.avail() >= kMaxNumericSize)
	{
		int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", v);
		buffer_.add(len);
	}
	return *this;
}
LogStream &LogStream::operator<<(long double v)
{
	if (buffer_.avail() >= kMaxNumericSize)
	{
		int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12Lg", v);
		buffer_.add(len);
	}
	return *this;
}
LogStream &LogStream::operator<<(const char *v)
{
	if (v)
	{
		buffer_.append(v, strlen(v));
	}
	else
	{
		buffer_.append("(null)", 6);
	}
	return *this;
}
LogStream &LogStream::operator<<(const unsigned char *v)
{
	return operator<<(reinterpret_cast<const char *>(v));
}

LogStream &LogStream::operator<<(const std::string &v)
{
	buffer_.append(v.c_str(), v.size());
	return *this;
}