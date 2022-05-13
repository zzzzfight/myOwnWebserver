// @Author Z
// @Email 3161349290@qq.com
#include "Timer.h"
#include "HttpData.h"

#include <sys/time.h>
#include <unistd.h>

#include<iostream>
#include <queue>

TimerNode::TimerNode(std::shared_ptr<HttpData> &http, size_t timeout)
	: deleted_(false),
	  ownHttp_(http)
{
	struct timeval now;
	gettimeofday(&now, NULL);
	expiredTime_ = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

TimerNode::~TimerNode()
{
	if (ownHttp_)
		ownHttp_->handleClose();
}

bool TimerNode::isVaild()
{
	struct timeval now;
	gettimeofday(&now, NULL);
	size_t temp = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000));
	if (temp < expiredTime_)
	{
		return true;
	}
	else
	{
		this->setDeleted();
		return false;
	}
}
//这个API的作用：将对httpdata强指针指针的删除权从计时器中释放
void TimerNode::clearHttp()
{
	ownHttp_.reset();
	this->setDeleted();
}

//这个API的作用：向计时器注册删除httpdata的强指针的删除权限
std::shared_ptr<TimerNode> Timer::addTimer(std::shared_ptr<HttpData> http, size_t timeout)
{
	std::shared_ptr<TimerNode> newNode(new TimerNode(http, timeout));
	timerNodeQueue.push(newNode);
	return newNode;
}

//删除节点超时的httpdata
void Timer::handleExpiredEvent()
{
	while (!timerNodeQueue.empty())
	{
		std::shared_ptr<TimerNode> topNode = timerNodeQueue.top();
		if (topNode->isDeleted())
		{
			// std::cout << "无效删除\n";
			timerNodeQueue.pop();
		}
		else if (!topNode->isVaild())
		{
			// std::cout << "有效删除\n";
			timerNodeQueue.pop();
		}
		else
		{
			break;
		}
	}
}