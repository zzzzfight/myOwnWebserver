// @Author Z
// @Email 3161349290@qq.com

#include "Util.h"
#include <sys/socket.h>
#include <arpa/inet.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <errno.h>
#include <string.h>
#include <iostream>
// #include <string>
// #include<String>
using namespace std;
// sigaction()
void handle_for_sigpipe()
{
	struct sigaction sa;
	memset(&sa, '\0', sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;
	if (sigaction(SIGPIPE, &sa, NULL))
		return;
}

int setSocketNonBlocking(int fd)
{
	int flag = fcntl(fd, F_GETFL, 0);
	if (flag == -1)
		return -1;

	flag |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flag) == -1)
		return -1;
	return 0;
}
void setSocketNodelay(int fd)
{
	int enable = 1;
	setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&enable, sizeof(enable));
}
void setSocketNoLinger(int fd)
{
}
void shutDownWR(int fd)
{
	shutdown(fd, SHUT_WR);
}
int socket_bind_listen(int port)
{
	assert(port > 0 && port < 65536);
	int listenfd = socket(PF_INET, SOCK_STREAM, 0);

	assert(listenfd > 0);

	sockaddr_in listenaddr;
	bzero((char *)&listenaddr, sizeof(listenaddr));
	listenaddr.sin_port = htons(port);
	listenaddr.sin_family = AF_INET;
	listenaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	int optval = 1;
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
	{
		close(listenfd);
		return -1;
	}

	int ret = bind(listenfd, (sockaddr *)&listenaddr, sizeof(listenaddr));
	std::cout << "bind\n";
	char *msg = strerror(errno);
	perror(msg);
	// assert(ret == 0);

	if (listen(listenfd, 1024) == -1)
	{
		close(listenfd);
		return -1;
	}

	// 无效监听描述符
	if (listenfd == -1)
	{
		close(listenfd);
		return -1;
	}
	return listenfd;

	// setSocketNonBlocking()
}

size_t readn(int fd, string &buffer)
{
	ssize_t nread = 0;
	ssize_t readSum = 0;
	const int MAX_BUFF = 1024;
	while (true)
	{
		char buff[MAX_BUFF];
		if ((nread = read(fd, buff, MAX_BUFF)) < 0)
		{
			if (errno == EINTR)
				continue;
			else if (errno == EAGAIN)
			{
				// std::cout << buff << std::endl;
				break;
			}
			else
			{
				perror("read error");
				break;
			}
		}
		else if (nread == 0)
		{
			// printf("redsum = %d\n", readSum);
			// std::cout << buff << std::endl;
			break;
		}
		readSum += nread;
		buffer += std::string(buff, buff + nread);
	}
	// buffer += std::string(buff, buff + nread);
	// char *ptr = buff;
	// buffer.append(buff);
	return readSum;
}

size_t readn(int fd, void *buff, size_t n)
{
	size_t nleft = n;
	ssize_t nread = 0;
	ssize_t readSum = 0;
	// const int MAX_BUFF = 1024;
	char *ptr = (char *)buff;
	while (nleft > 0)
	{
		// char buff[MAX_BUFF];
		if ((nread = read(fd, buff, nleft)) < 0)
		{
			if (errno == EINTR)
				nread = 0;
			else if (errno == EAGAIN)
			{
				// std::cout << buff << std::endl;
				return readSum;
				// break;
			}
			else
			{
				perror("read error");
				// break;
				return -1;
			}
		}
		else if (nread == 0)
		{
			// printf("redsum = %d\n", readSum);
			// std::cout << buff << std::endl;
			break;
		}
		readSum += nread;
		nleft -= nread;
		ptr += nread;
		// buffer += std::string(buff, buff + nread);
	}
	// buffer += std::string(buff, buff + nread);
	// char *ptr = buff;
	// buffer.append(buff);
	return readSum;
}

size_t readn(int fd, std::string &buffer, bool &eof)
{
	// read return 0 对面关闭 return -1
	//			  -1 判断errno
	//            >0 正常
	// readn return  0 EPOLLIN 未就绪
	// 				-1 错误
	// 			  	>0 正常
	ssize_t nread = 0;
	ssize_t readSum = 0;
	const int MAX_BUFF = 1024;
	while (true)
	{
		char buff[MAX_BUFF];
		if ((nread = read(fd, buff, MAX_BUFF)) < 0)
		{
			if (errno == EINTR)
				continue;
			else if (errno == EAGAIN)
			{
				// std::cout << buff << std::endl;
				break;
			}
			else
			{
				perror("read error");
				// break;
				return -1;
			}
		}
		else if (nread == 0)
		{
			// printf("redsum = %d\n", readSum);
			// std::cout << buff << std::endl;
			// eof = true;
			// break;
			return -1;
		}
		readSum += nread;
		buffer += std::string(buff, buff + nread);
	}
	// buffer += std::string(buff, buff + nread);
	// char *ptr = buff;
	// buffer.append(buff);
	return readSum;
}

size_t writen(int fd, std::string &buffer)
{
	size_t nleft = buffer.size();
	ssize_t nwritten = 0;
	ssize_t writeSum = 0;
	const char *ptr = buffer.c_str();
	while (nleft > 0)
	{
		if ((nwritten = write(fd, ptr, nleft)) <= 0)
		{
			if (nwritten < 0)
			{
				if (errno == EINTR)
				{
					nwritten = 0;
					continue;
				}
				else if (errno == EAGAIN)
					break;
				else
					return -1;
			}
			else
			{
				return -1;
			}
		}
		writeSum += nwritten;
		nleft -= nwritten;
		ptr += nwritten;
	}
	if (writeSum == static_cast<int>(buffer.size()))
		buffer.clear();
	else
		buffer = buffer.substr(writeSum);
	return writeSum;
}

size_t writen(int fd, void *buffer, int n)
{
	size_t nleft = n;
	ssize_t nwritten = 0;
	ssize_t writeSum = 0;
	char *ptr = (char *)buffer;
	while (nleft > 0)
	{
		if ((nwritten = write(fd, ptr, nleft)) <= 0)
		{
			if (nwritten < 0)
			{
				if (errno == EINTR)
				{
					nwritten = 0;
					continue;
				}
				else if (errno == EAGAIN)
					return writeSum;
				else
					return -1;
			}
			else
			{
				return -1;
			}
		}
		writeSum += nwritten;
		nleft -= nwritten;
		ptr += nwritten;
	}
	return writeSum;
}