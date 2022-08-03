// @Author Z
// @Email 3161349290@qq.com
#pragma once
#include <algorithm>
#include <string>
#include <string.h>

// #include"Log"

const int kSmallBuffer = 4000;
const int kLargerBuffer = 4000 * 1000;

template <int SIZE>
class FixedBuffer
{
public:
	FixedBuffer() : cur_(data_) {}
	~FixedBuffer() = default;

	//缓存区添加数据
	void append(const char *buf, size_t len)
	{
		if (avail() > static_cast<int>(len))
		{
			memcpy(cur_, buf, len);
			cur_ += len;
		}
	}
	void add(size_t len) { cur_ += len; }
	char *current() { return cur_; }
	int length() const { return static_cast<int>(cur_ - data_); }
	int avail() const { return static_cast<int>(end() - cur_); }
	const char *data() const { return data_; }
	void reset() { cur_ = data_; }
	void bzero() { memset(data_, 0, sizeof data_); }
	// const char *foo() const { return data_; }

private:
	const char *end() const { return data_ + sizeof data_; }
	char data_[SIZE];
	char *cur_;

	// test
public:
	const char *foo() const
	{
		return data_;
	}
};

class LogStream
{
public:
	using Buffer = FixedBuffer<kSmallBuffer>;
	LogStream() {}
	~LogStream() {}

	template <typename T>
	void formatInterger(T v)
	{
		if (buffer_.avail() >= kMaxNumericSize)
		{
			std::string temp = std::to_string(v);
			*this << temp;
		}
	}
	// C++重载操作符
	LogStream &operator<<(bool v);
	LogStream &operator<<(char);
	LogStream &operator<<(short);
	LogStream &operator<<(unsigned short);
	LogStream &operator<<(int);
	LogStream &operator<<(unsigned int);
	LogStream &operator<<(long);
	LogStream &operator<<(unsigned long);
	LogStream &operator<<(long long);
	LogStream &operator<<(unsigned long long);
	LogStream &operator<<(float);
	LogStream &operator<<(double);
	LogStream &operator<<(long double);
	LogStream &operator<<(const char *);
	LogStream &operator<<(const unsigned char *);
	LogStream &operator<<(const std::string &);

	// FixedBuffer操作的封装
	void append(const char *data, int len)
	{
		buffer_.append(data, len);
	}
	const Buffer &buffer() const { return buffer_; }
	void resetBuffer();

private:
	Buffer buffer_;
	const int kMaxNumericSize = 32;
};