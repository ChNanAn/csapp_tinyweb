#include<iostream>
#include"tinyweb.h"
/***********************************************************/
/*Rio包函数*/
ssize_t rio_writen(int fd, void* usrbuf, size_t n)
{
	size_t nleft = n;
	ssize_t nwritten;
	char* bufp = (char*)usrbuf;

	while (nleft > 0)
	{
		if ((nwritten = write(fd, bufp, nleft)) <= 0)
		{
			if (errno == EINTR)
				nwritten = 0;
			else
				return -1;
		}
		nleft -= nwritten;
		bufp += nwritten;
	}
	return n;
}


static ssize_t rio_read(rio_t* rp, char* userbuf, size_t n)
{
	size_t cnt;

	while (rp->rio_cnt <= 0)       //buf为空，填充rio中的buf
	{                          
		rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, sizeof(rp->rio_buf));

		if (rp->rio_cnt < 0)
		{
			if (errno != EINTR)
				return -1;
		}
		else if (rp->rio_cnt == 0) //EOF
			return 0;
		else
			rp->rio_bufptr = rp->rio_buf;
	}

	cnt = n;
	if (rp->rio_cnt < n)
		cnt = rp->rio_cnt;
	memcpy(userbuf, rp->rio_bufptr, cnt);

	rp->rio_bufptr += cnt;
	rp->rio_cnt -= cnt;

	return cnt;
}

ssize_t rio_readlineb(rio_t* rp, void* usrbuf, size_t maxlen)
{
	int  rc;
    size_t n;
	char c, * bufp = (char*)usrbuf;
	
	for (n = 1; n < maxlen; n++)
	{
		if ((rc = rio_read(rp, &c, 1)) == 1)
		{
			*bufp++ = c;
			if (c == '\n')
			{
				n++;
				break;
			}
		}
		else if (rc == 0)
		{
			if (n == 1)
				return 0;      //EOF ,没数据可读
			else
				break;         //读了一些数据

		}
		else
			return -1;         // 错误
	}
	*bufp = 0;
	return n - 1;

}

ssize_t rio_readnb(rio_t* rp, void* usrbuf, size_t n)
{
	size_t nleft = n;
	ssize_t nread;
	char* bufp = (char*)usrbuf;

	while (nleft > 0)
	{
		if ((nread = rio_readnb(rp,bufp, n) < 0))
			return -1;     //read() 设置了errno

		else if (nread == 0)
			break;         //无数据可读，EOF
		
		nleft -= nread;
		bufp += nread;
	}
	return n - nleft;      //return >=0

}

void rio_readinitb(rio_t* rp, int fd)
{
	rp->rio_fd = fd;
	rp->rio_bufptr = rp->rio_buf;
	rp->rio_cnt = 0;
}


/***********************************************************/
/*Rio包函数的包装函数*/
void Rio_writen(int fd, void* usrbuf, size_t n)
{
	if ((size_t(rio_writen(fd, usrbuf, n)))!= n)
		unix_error("Rio_writen error");
}
void Rio_readinitb(rio_t* rp,int fd)
{
	rio_readinitb(rp, fd);
}

ssize_t Rio_readnb(rio_t* rp, void* usrbuf, size_t n)
{
	ssize_t rc;
	if ((rc = rio_readnb(rp, usrbuf, n)) < 0)
		unix_error("Rio_readnb error");
	return rc;
}

ssize_t Rio_readlineb(rio_t* rp, void* usrbuf, size_t maxlen)
{
	ssize_t rc;
	if ((rc = rio_readlineb(rp, usrbuf, maxlen) )< 0)
		unix_error("Rio_readlineb error");
	return rc;
}

/***********************************************************/
/*服务端，客户端的辅助函数*/
int open_listenfd(char* port)
{

	int listenfd;
	/*用于设置套接字属性，SO_REUSEADDR这个套接字选项通知内核，如果端口忙，但TCP状态位于 TIME_WAIT ，可以重用端口。
	如果端口忙，而TCP状态位于其他状态，重用端口时依旧得到一个错误信息，
	指明"地址已经使用中"*/
	int optval = 1;
	addrinfo hints, * listp, * p;

	//get a list of potential server address
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;      //在任意ip地址
	hints.ai_flags|= AI_NUMERICSERV;     //参数service默认是服务名或端口号，这个标志强制service为端口号

	//本地ip至少有一个ipv4,才会返回一个ipv4地址，
	//至少有一个ipv6,才会返回一个ipv6地址
	hints.ai_flags |= AI_ADDRCONFIG; 

	Getaddrinfo(nullptr, port, &hints, &listp);

	for (p = listp; p; p = p->ai_next)
	{
		
		if ((listenfd=Socket(p->ai_family, p->ai_socktype, p->ai_protocol))< 0)
			continue;
		Setsockopt(listenfd, SOL_SOCKET,SO_REUSEADDR ,&optval, sizeof(optval));

		if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
			break;     //绑定成功

		close(listenfd);

	}
	
	Freeaddrinfo(listp);   //释放堆上内存
	if (!p)     //
		return -1;
	Listen(listenfd, LISTENQ);
	return listenfd;
}
int open_clientfd(char* hostname, char* port)
{
	int clientfd;
	addrinfo hints, * p, * plist;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_NUMERICSERV;
	hints.ai_flags |= AI_ADDRCONFIG;
	Getaddrinfo(hostname, port, &hints, &plist);



	for(p=plist; p; p = p->ai_next)
	{
		if ((clientfd=socket(p->ai_family, p->ai_socktype, p->ai_protocol) )< 0)
			continue;
		if (connect(clientfd, p->ai_addr, p->ai_addrlen) > 0)
			break;



		close(clientfd);
	}
	Freeaddrinfo(plist); //释放内存

	if (!p)         //所有connect 都失败了
		return -1;
	else
		return clientfd;

}
int Open_clientfd(char* host, char* port)
{
	int rc;
	rc = open_clientfd(host, port);
	if (rc < 0)
		unix_error("Open_clienfd error");
	return rc;
}
int Open_listenfd(char* port)
{
	int rc;
	rc = open_listenfd(port);
	if (rc < 0)
		unix_error("Open_listenfd error");
	return rc;
}
/*end*/



/**************************************************************/
/*套接字接口包装函数*/

int Socket(int domain, int type, int protocol)
{
	int rc;
	rc = socket(domain, type, protocol);
	if (rc < 0)
		unix_error("Socket() error");
	return rc;
}

void Bind(int sockfd, struct sockaddr* my_addr, int addrlen)
{
	int rc;
	rc=bind(sockfd, my_addr, addrlen);
	if (rc < 0)
		unix_error("Bind error");
}

void Listen(int fd, int backlog)
{
	int rc;
	rc = listen(fd, backlog);
	if (rc < 0)
		unix_error("Listen error");
}


int Accept(int fd, sockaddr* client_addr, socklen_t* addrlen)
{
	int connectfd;
	connectfd = accept(fd, client_addr, addrlen);
	if (connectfd <0)
		unix_error("Accept error");
	return connectfd;
}

void Connect(int sockfd, struct sockaddr* serv_addr, int addrlen)
{
	int rc;
	rc = connect(sockfd, serv_addr, addrlen);
	if (rc < 0)
		unix_error("Connect error");
}

void Setsockopt(int s, int level, int optname, const void* optval, int optlen)
{
	int rc;
	rc = setsockopt(s, level, optname, optval, optlen);
	if (rc < 0)
		unix_error("Setsockopt error");
}
/*end*/


/****************************************************************/
/*与协议无关的包装函数*/

void Getaddrinfo(const char* node, const char* service,
	const struct addrinfo* hints,struct addrinfo** res)
{
	int rc;
	rc=getaddrinfo(node, service, hints, res);
	if (rc != 0)
		gai_error(rc,"Getaddrinfo error");
}

void Getnameinfo(const struct sockaddr* sa, socklen_t salen, char* host,
	size_t hostlen, char* serv, size_t servlen, int flags)
{
	int rc;
	rc=getnameinfo(sa, salen, host, hostlen, serv, servlen, flags);
	if (rc != 0)
		gai_error(rc, "Getnameinfo error");

}
void Freeaddrinfo(struct addrinfo* res)
{
	freeaddrinfo(res);
}

//inet_pton,和inet_ntop是随着ipv6出现的函数，对于ipv4,ipv6都适用，p表示presention，n网络
//ipv4的时候用inet_aton跟inet_ntoa
void Inet_ntop(int af, const void* src, char* dst, socklen_t size)
{
	if (!inet_ntop(af, src, dst, size))
		unix_error("Inet_ntop error");

}
void Inet_pton(int af, const char* src, void* dst)
{
	int rc;
	rc=inet_pton(af, src, dst);
	if (rc == 0)
		app_error("inet_pton error : invalid dotted - decimal address");
	else if (rc <= 0)
		unix_error("Inet_pton error");

}
/*********************************************************/
/*错误处理包装函数*/
void unix_error(const char* msg)
{
	fprintf(stderr, "%s:%s\n", msg, strerror(errno));
	exit(-1);
}
/*
有很多socket相关的函数的错误号和错误信息是无法通过errno，
strerror(errno)函数去获取的。
其原因在于很多函数并没有将errno.h作为错误码
*/

void gai_error(int code, const char* msg) /* Getaddrinfo1-style error */
{
	fprintf(stderr, "%s: %s\n", msg, gai_strerror(code));
	exit(0);
}
void app_error(const char* msg) /* Application error */
{
	fprintf(stderr, "%s\n", msg);
	exit(0);
}

/**************************************************/
/*标准的unix I/O 的包装函数*/
int Open(const char* pathname, int flags, mode_t mode)
{
	int rc;
	if ((rc = open(pathname, flags, mode)) < 0)
		unix_error("Open error");
	return rc;
}
void Close(int fd)
{
	int rc;
	if ((rc = close(fd) )< 0)
		unix_error("Close error");

}
ssize_t Read(int fd, void* buf, size_t count)
{
	int rc;
	if ((rc = read(fd, buf, count)) < 0)
		unix_error("Read error");
	return rc;
}

int Dup2(int fd1, int fd2)
{
	int rc;
	if ((rc = dup2(fd1, fd2) )< 0)
		unix_error("Dup2 error");
	return rc;
}


/********************************************************/
/**mmap()和munmap()的包装函数*/
void* Mmap(void* addr, size_t len, int prot, int flags, int fd, off_t offset)
{
	void* ptr;
	if ((ptr = mmap(addr, len, prot, flags, fd, offset))==((void*)-1))
		unix_error("Mmap error");
	return ptr;
}
void Munmap(void* start, size_t length)
{
	int rc;
	if ((rc = munmap(start, length) )< 0)
		unix_error("Munmap error");
}

/*******************************************************************
*****************进程控制函数的包装函数****************************/

int Fork()
{
	int rc;
	if ((rc = fork()) < 0)
		unix_error("Fork error");
    return rc;
}

void Execve(char const* filename, char* const argv[], char* const envp[])
{
	if (execve(filename, argv, envp) < 0)
		unix_error("Execve error");
}

pid_t Wait(int* status)
{
	int pid;
	if((pid=wait(status) )< 0)
		unix_error("Wait error");
	return pid;
}

pid_t Waitpid(pid_t pid, int* iptr, int options)
{
	pid_t repid;
	if ((repid = waitpid(pid, iptr, options) )< 0)
    {
        if(errno!=ECHILD)
        unix_error("Waitpid error");
    }
	return repid;
}