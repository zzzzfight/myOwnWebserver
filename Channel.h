// @Author Z
// @Email 3161349290@qq.com
#pragma once

#include <functional>
#include <memory>
#include <iostream>
class HttpData;

class EventLoop;
class Channel
{
public:
	typedef std::function<void()> EventCallback;
	Channel(EventLoop *loop, int fd);
	~Channel()
	{
		std::cout << "析构channel" << std::endl;
	}

public:
	EventLoop *ownerloop() { return ownerLoop_; }

public:
	//设置回调函数
	void setReadCallback(EventCallback cb)
	{
		readCallback_ = std::move(cb);
	}

	void setWriteCallback(EventCallback cb)
	{
		writeCallback_ = std::move(cb);
	}
	void setConnCallback(EventCallback cb)
	{
		connCallback_ = std::move(cb);
	}

public:
	void handleEvents();

public:
	// setFd
	int fd() const { return fd_; }				//返回文件描述符
	uint32_t events() const { return events_; } //返回
	uint32_t revents() const { return revents_; }

	void set_events(uint32_t evt) { events_ = evt; }
	void set_revents(uint32_t revt) { revents_ = revt; }

	// uint32_t get_events(){return events_;}
	std::shared_ptr<HttpData> getHolder()
	{
		std::shared_ptr<HttpData> ret(http_.lock());
		return ret;
	}

	void setHolder(std::shared_ptr<HttpData> http) { http_ = http; }

private:
	uint32_t events_;			 //需要监听的事件状态
	uint32_t revents_;			 // epoll返回的事件状态
	bool eventHandling_ = false; //

private:
	//回调函数
	EventCallback readCallback_;
	EventCallback writeCallback_;
	EventCallback closeCallback_;
	EventCallback connCallback_;

	EventCallback errorCallback_;

private:
	//持有事件池和持有fd
	EventLoop *ownerLoop_;
	const int fd_;
	std::weak_ptr<HttpData> http_;
};