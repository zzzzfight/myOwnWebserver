// @Author Z
// @Email 3161349290@qq.com
#include "HttpData.h"
#include "Channel.h"
#include "EventLoop.h"
#include "./base/Util.h"

#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>

pthread_once_t mimeType::once_control = PTHREAD_ONCE_INIT;

std::unordered_map<std::string, std::string> mimeType::mime_;
void mimeType::init()
{
	mime_[".html"] = "text/html";
	mime_[".avi"] = "video/x-msvideo";
	mime_[".bmp"] = "image/bmp";
	mime_[".c"] = "text/plain";
	mime_[".doc"] = "application/msword";
	mime_[".gif"] = "image/gif";
	mime_[".gz"] = "application/x-gzip";
	mime_[".htm"] = "text/html";
	mime_[".ico"] = "image/x-icon";
	mime_[".jpg"] = "image/jpeg";
	mime_[".png"] = "image/png";
	mime_[".txt"] = "text/plain";
	mime_[".mp3"] = "audio/mp3";
	mime_["default"] = "text/html";
}

std::string mimeType::getMime(const std::string &suffix)
{
	pthread_once(&once_control, mimeType::init);
	if (mime_.find(suffix) == mime_.end())
	{
		return mime_["default"];
	}
	else
	{
		return mime_[suffix];
	}
}

// const __uint32_t DEFAULT_EVENT = EPOLLIN | EPOLLET | EPOLLONESHOT;
const int DEFAULT_EXPIRED_TIME = 2000; // ms
// const int DEFAULT_KEEP_ALIVE_TIME = 5 * 60 * 1000; // ms
const int DEFAULT_KEEP_ALIVE_TIME = 5000;

HttpData::HttpData(EventLoop *loop, int fd)
	: ownloop_(loop),
	  fd_(fd),
	  ownchannel_(new Channel(loop, fd)),
	  connectionState_(H_CONNECTED),
	  error_(false),
	  //   method_(GET),
	  state_(STATE_PARSE_URL),
	  isKeepAlive_(false)
{
	//后续看有没办法改成shared_enable_from()
	ownchannel_->setReadCallback(std::bind(&HttpData::handleRead, this));
	ownchannel_->setWriteCallback(std::bind(&HttpData::handleRead, this));
	ownchannel_->setConnCallback(std::bind(&HttpData::handleConn, this));
}

void HttpData::handleRead()
{
	this->seperaterTimer();
	{
		std::cout << "httpdata use count:" << shared_from_this().use_count() << std::endl;
		std::cout << "httpdata fd_:" << fd_ << std::endl;
	}
	do
	{
		bool eof = false;
		int readNum = readn(fd_, inBuffer_, eof);

		if (readNum < 0)
		{
			error_ = true;
			handleError(fd_, 400, "Bad Request");
			break;
		}

		if (state_ == STATE_PARSE_URL)
		{
			URL_STATE flag = this->parseURL();
			if (flag == PARSE_URL_ERROR)
			{
				inBuffer_.clear();
				error_ = true;
				handleError(fd_, 400, "Bad Request");
				break;
			}
			else
			{
				state_ = STATE_PARSE_HEADERS;
			}
		}
		if (state_ == STATE_PARSE_HEADERS)
		{
			HEADER_STATE flag = this->parseHeaders();
			if (flag == PARSE_HEADER_AGAIN)
			{
				break;
			}
			else if (flag == PARSE_HEADER_ERROR)
			{
				error_ = true;
				handleError(fd_, 400, "Bad Request");
				break;
			}
			if (method_ == POST)
			{
				state_ = STATE_RECV_BODY;
			}
			else
			{
				state_ = STATE_ANALYSIS;
			}
		}
		if (state_ == STATE_RECV_BODY)
		{
		}
		if (state_ == STATE_ANALYSIS)
		{
			ANALYSIS_STATE flag = this->analysisRequest();
			if (flag == ANALYSIS_SUCESS)
			{
				state_ = STATE_FINISH;
				break;
			}
			else
			{
				error_ = true;
				break;
			}
		}
	} while (false);

	if (!error_ && connectionState_ == H_CONNECTED)
	{
		if (outBuffer_.size() > 0)
		{
			handleWrite();
		}
		if (!error_)
		{
			if (state_ == STATE_FINISH)
			{
				this->reset();
				if (inBuffer_.size() > 0)
				{
					handleRead();
				}
			}
			else
			{
				uint32_t events = ownchannel_->events();
				ownchannel_->set_events(events | EPOLLIN);
			}
		}
	}
}

void HttpData::handleWrite()
{
	this->seperaterTimer();
	//出错 error_ 改变
	//不出错 未写完 掩码置为EPOLLOUT
	//不出错 写完 掩码置为0
	if (outBuffer_.size() > 0)
	{
		if (!error_ && connectionState_ != H_DISCONNECTED)
		{
			if (writen(fd_, outBuffer_) < 0)
			{
				this->ownchannel_->set_events(0);
				error_ = true;
				return;
			}
			if (outBuffer_.size() > 0)
			{
				uint32_t events = this->ownchannel_->events();
				this->ownchannel_->set_events(events | EPOLLOUT);
			}
			else
			{
				this->ownchannel_->set_events(0);
			}
		}
	}
}

void HttpData::handleConn()
{
	//
	uint32_t events = ownchannel_->events();
	// seperaterTimer(); //删除原有的计时器节点
	if (!error_ && connectionState_ == H_CONNECTED)
	{
		if (events != 0)
		{
			int timeout = DEFAULT_EXPIRED_TIME;
			if (isKeepAlive_)
				timeout = DEFAULT_KEEP_ALIVE_TIME;
			if ((events & EPOLLIN) && (events & EPOLLOUT))
			{
				events = uint32_t(0);
				events |= EPOLLOUT;
				ownchannel_->set_events(events);
			}
			events |= EPOLLET;
			ownchannel_->set_events(events);
			ownloop_->updateChannel(ownchannel_, timeout);
		}
		else if (isKeepAlive_)
		{
			events |= (EPOLLIN | EPOLLET);
			ownchannel_->set_events(events);
			int timeout = DEFAULT_KEEP_ALIVE_TIME;
			ownloop_->updateChannel(ownchannel_, timeout);
		}
		else
		{
			events |= (EPOLLIN | EPOLLET);
			ownchannel_->set_events(events);
			int timeout = (DEFAULT_EXPIRED_TIME);
			ownloop_->updateChannel(ownchannel_, timeout);
		}
	}
	// else if (!error_ && connectionState_ == H_DISCONNECTING && (event & EPOLLOUT))
	// { //默认对端关闭 直接关闭连接
	// 	events_ = (EPOLLOUT | EPOLLET);
	// 	int timeout = (DEFAULT_KEEP_ALIVE_TIME >> 1);
	// 	ownchannel_->set_events(events);
	// 	ownloop_->updateChannel(ownchannel_, timeout);
	// }
	else
	{
		ownloop_->runInLoop(std::bind(&HttpData::handleClose, shared_from_this()));
	}
}

void HttpData::handleError(int fd, int errnum, std::string shortmsg)
{
	shortmsg = " " + shortmsg;
	char send_buff[4096];
	string body_buff, header_buff;
	body_buff += "<html><title>哎~出错了</title>";
	body_buff += "<body bgcolor=\"ffffff\">";
	body_buff += to_string(errnum) + shortmsg;
	body_buff += "<hr><em> Z's Web Server</em>\n</body></html>";

	header_buff += "HTTP/1.1 " + to_string(errnum) + shortmsg + "\r\n";
	header_buff += "Content-Type: text/html\r\n";
	header_buff += "Connection: Close\r\n";
	header_buff += "Content-Length: " + to_string(body_buff.size()) + "\r\n";
	header_buff += "Server: Z's Web Server\r\n";
	;
	header_buff += "\r\n";
	// 错误处理不考虑writen不完的情况
	sprintf(send_buff, "%s", header_buff.c_str());
	writen(fd, send_buff, strlen(send_buff));
	sprintf(send_buff, "%s", body_buff.c_str());
	writen(fd, send_buff, strlen(send_buff));
}

void HttpData::reset()
{
	/*没有发生错误且完成了一次协议的协议
		此时可能写缓存区仍有数据，读缓存区仍有数据
		将http协议相关状态重置
		不改变inBuffer/outBuffer
	 */
	method_ = GET;
	url_.clear();
	version_.clear();
	headtToValue_.clear();
	error_ = false;
	state_ = STATE_PARSE_URL;
}

void HttpData::handleClose()
{
	// seperaterTimer();
	connectionState_ = H_DISCONNECTED;
	shared_ptr<HttpData> guard(shared_from_this());
	ownloop_->removeChannel(ownchannel_);
}

void HttpData::newEvent()
{
	ownchannel_->set_events(DEFAULT_EVENT);
	ownloop_->addChannel(ownchannel_, DEFAULT_EXPIRED_TIME);
}
void HttpData::seperaterTimer() //将HTTP存活的管理权由计时器转交给处理程序
{
	std::shared_ptr<TimerNode> timerHolder = timer_.lock();
	if (timerHolder)
	{
		timerHolder->clearHttp();
		timer_.reset();
	}
	return;
}

URL_STATE HttpData::parseURL()
{
	if (state_ == STATE_PARSE_URL)
	{
		string &str = inBuffer_;
		size_t pos = inBuffer_.find('\r');
		if (pos == string::npos)
		{
			return PARSE_URL_AGAIN;
		}
		string requestline = inBuffer_.substr(0, pos);
		if (str.size() > pos + 2)
		{
			str = str.substr(pos + 2);
		}
		else
		{
			str.clear();
		}
		pos = requestline.find(' ');
		string method = requestline.substr(0, pos);
		if (!method.compare("GET"))
		{
			method_ = GET;
		}
		else if (!method.compare("POST"))
		{
			method_ = POST;
		}
		else if (!method.compare("HEAD"))
		{
			method_ = HEAD;
		}
		requestline = requestline.substr(pos + 1);
		pos = requestline.find(' ');
		if (pos == string::npos)
		{
			return PARSE_URL_ERROR;
		}
		url_ = requestline.substr(0, pos);
		if (0 == url_.compare("/"))
		{
			url_ = "root" + url_ + "log.html";
		}
		else
		{
			url_ = "root" + url_;
		}
		requestline = requestline.substr(pos + 1);
		version_ = requestline;
		cout << "-------" << endl;
		cout << method << endl;
		cout << url_ << endl;
		cout << version_ << endl;
		cout << "-------" << endl;
	}
	return PARSE_URL_SUCCESS;
}

HEADER_STATE HttpData::parseHeaders()
{
	string &str = inBuffer_;
	while (true)
	{
		size_t pos = str.find('\n');
		if (pos == string::npos)
		{
			if (str.size())
			{
				return PARSE_HEADER_AGAIN;
			}
		}
		if (str[0] == '\r' || str.size() == 0)
		{
			if (str.size() > 2)
			{
				str = str.substr(2);
			}
			else
			{
				str.clear();
			}
			break;
		}
		string headline = str.substr(0, pos);
		if (str.size() > pos + 1)
		{
			str = str.substr(pos + 1);
		}
		else
		{
			str.clear();
		}
		pos = headline.find_first_of(':');
		if (pos == string::npos)
		{
			return PARSE_HEADER_ERROR;
		}
		string head(headline.begin(), headline.begin() + pos);
		string value(headline.begin() + pos + 2, headline.end() - 1);
		headtToValue_[head] = value;
	}
	return PARSE_HEADER_SUCCESS;
}

ANALYSIS_STATE HttpData::analysisRequest()
{
	if (method_ == POST)
	{
	}
	else if (method_ == GET || method_ == HEAD)
	{
		string header;
		header += "HTTP/1.1 200 OK\r\n";
		if (headtToValue_.find("Connection") != headtToValue_.end() && ((headtToValue_["Connection"] == "keep-alive") || (headtToValue_["Connection"] == "Keep-Alive")))
		{
			isKeepAlive_ = true;
			header += string("Connection: Keep-Alive\r\n") + "Keep-Alive: timeout=" + to_string(DEFAULT_KEEP_ALIVE_TIME) + "\r\n";
		}
		int dot_pos = url_.find('.');
		string filetype;
		if (dot_pos == string::npos)
		{
			filetype = mimeType::getMime("default");
		}
		else
		{
			filetype = mimeType::getMime(url_.substr(dot_pos));
		}
		cout << "filetype\n";
		if (url_ == "hello" || url_ == "Hello")
		{
			outBuffer_ = "HTTP/1.1 200 OK\r\nContent-type: text/plain\r\n\r\nHello World";
			return ANALYSIS_SUCESS;
		}

		struct stat sbuf;
		if (stat(url_.c_str(), &sbuf) < 0)
		{
			header.clear();
			handleError(fd_, 404, "Not Fount!");
			return ANALYSIS_ERROR;
		}
		header += "Content-Type: " + filetype + "\r\n";
		header += "Content-Length: " + to_string(sbuf.st_size) + "\r\n";
		header += "Server: ZHF's web server\r\n";
		header += "\r\n";
		outBuffer_ += header;
		if (method_ == HEAD)
			return ANALYSIS_SUCESS;
		int src_fd = open(url_.c_str(), O_RDONLY);
		if (src_fd < 0)
		{
			outBuffer_.clear();
			handleError(fd_, 404, "Not Fount!");
			return ANALYSIS_ERROR;
		}
		void *mmapret = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
		close(src_fd);
		if (mmapret == (void *)-1)
		{
			munmap(mmapret, sbuf.st_size);
			outBuffer_.clear();
			// handleError;
			handleError(fd_, 404, "Not Fount!");
			return ANALYSIS_ERROR;
		}
		char *src_addr = static_cast<char *>(mmapret);
		// outbuffer.append()
		outBuffer_ += string(src_addr, src_addr + sbuf.st_size);
		munmap(mmapret, sbuf.st_size);
		return ANALYSIS_SUCESS;
	}
	return ANALYSIS_ERROR;
}