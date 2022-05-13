// @Author Z
// @Email 3161349290@qq.com
#pragma once
#include "Timer.h"
class Channel;

#include <string.h>
#include <pthread.h>
#include <sys/epoll.h>

#include <memory>
#include <functional>
#include <iostream>
#include <unordered_map>
// #include <iostream>

class mimeType
{
private:
	static void init();
	static std::unordered_map<std::string, std::string> mime_;
	mimeType();
	mimeType(const mimeType &m);

public:
	static std::string getMime(const std::string &suffix);

private:
	static pthread_once_t once_control;
};

class Channel;
class EventLoop;
class TimerNode;
enum HTTP_METHOD
{
	GET = 0,
	POST,
	HEAD
};

enum URL_STATE
{
	PARSE_URL_AGAIN = 0,
	PARSE_URL_ERROR,
	PARSE_URL_SUCCESS,
};
//主状态机
enum PROCESS_STATE
{
	STATE_PARSE_URL = 0,
	STATE_PARSE_HEADERS,
	STATE_RECV_BODY,
	STATE_ANALYSIS,
	STATE_FINISH
};

//连接状态
enum CONNECTION_STATE
{
	H_CONNECTED = 0,
	H_DISCONNECTING,
	H_DISCONNECTED
};
enum HEADER_STATE
{
	PARSE_HEADER_SUCCESS = 0,
	PARSE_HEADER_AGAIN,
	PARSE_HEADER_ERROR
};
enum ANALYSIS_STATE
{
	ANALYSIS_SUCESS = 0,
	ANALYSIS_ERROR
};

const unsigned int DEFAULT_EVENT = EPOLLIN | EPOLLET | EPOLLONESHOT;
const size_t DEFAULTALIVETIME = 2000;

class HttpData : public std::enable_shared_from_this<HttpData>
{
public:
	HttpData(EventLoop *loop, int fd);
	~HttpData()
	{
		std::cout << "析构httpdata" << std::endl;
		std::cout << "fd_: " << fd_ << std::endl;
		close(fd_);
	}

	void handleRead();
	void handleWrite();
	void handleConn();
	void handleError(int fd, int errnum, std::string shortmsg);
	void reset();
	void handleClose();
	void newEvent();

	void setTimerHolder(std::shared_ptr<TimerNode> http)
	{
		timer_ = http;
	}

	void seperaterTimer(); //从计时器中注销httpdata强指针的删除权

private:
	// epoll相关
	int fd_;

public:
	EventLoop *ownloop_;
	std::shared_ptr<Channel> ownchannel_;

private:
	//计时器
	std::weak_ptr<TimerNode> timer_;

private:
	// http协议层
	std::string inBuffer_;	//输入缓存
	std::string outBuffer_; //输出缓存
	HTTP_METHOD method_;

	std::string url_;
	// std::
	std::string version_;										// http版本
	std::unordered_map<std::string, std::string> headtToValue_; //字段与值的映射

	bool error_;	   //错误flag
	bool isKeepAlive_; //是否保活

private:
	URL_STATE parseURL();			   //请求行解析
	HEADER_STATE parseHeaders();	   //首部字段解析
	ANALYSIS_STATE analysisRequest();  //响应报文封装
	PROCESS_STATE state_;			   //主状态
	CONNECTION_STATE connectionState_; //连接状态
};