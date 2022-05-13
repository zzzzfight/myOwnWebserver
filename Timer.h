// @Author Z
// @Email 3161349290@qq.com
#pragma once
#include <queue>
#include <memory>
#include <deque>

#include <unistd.h>

class HttpData;
class TimerNode
{
public:
	TimerNode(std::shared_ptr<HttpData> &http, size_t timeout);
	~TimerNode();
	size_t getExpiredTime() { return expiredTime_; }
	bool isVaild();
	void clearHttp();

	bool isDeleted() const { return deleted_; }
	void setDeleted() { deleted_ = true; }

private:
	size_t expiredTime_;
	std::shared_ptr<HttpData> ownHttp_;
	bool deleted_;
};

//函数对象，用作小根堆的比较
class TimerCmp
{
public:
	bool operator()(std::shared_ptr<TimerNode> &a, std::shared_ptr<TimerNode> &b)
	{
		return a->getExpiredTime() > b->getExpiredTime();
	}
};

class Timer
{
public:
	Timer() {}
	~Timer() {}

	std::shared_ptr<TimerNode> addTimer(std::shared_ptr<HttpData> http, size_t timeout);
	void handleExpiredEvent();

private:
	std::priority_queue<std::shared_ptr<TimerNode>, std::deque<std::shared_ptr<TimerNode>>, TimerCmp> timerNodeQueue;
};
