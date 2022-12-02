#include<iostream>
#include"tinyweb.h"


void doit(int fd);
void read_requesthdrs(rio_t* rp);
int parse_uri(char* uri, char* filename, char* cgiargs);

void get_filetype(char* filename, char* filetype);
void serve_static(int fd, char* filename, int filesize);
void serve_dynamic(int fd, char* filename, char* cigargs);
void clienterror(int fd, const char* cause, const char* errnum,
	               const char* shortmsg, const char* longmsg);
void handlerSIGCHLD(int sig);


int main(int argc,char *argv[])
{
	int listenfd, connfd;
	char hostname[MAXLINE], port[MAXLINE];
	socklen_t clientlen;
    struct sockaddr_storage clientaddr;

     if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(1);
    }

	/*struct sockaddr_in 16字节， 
	{sa_family_t    sin_family;
	 uint16_t       sin_port;
	 struct in_addr sin_addr;
	 char   sin_zero[8]     //8个字节暂时未使用
	 };
	 in_addr{
	 In_addr_t s_addr;
	 };

	 //sockaddr也是16字节
	 struct sockaddr{
	 sa_family_t    sin_family;
	 char sa_data[14]
	 };
     
    truct sockaddr_storage
     {
    __SOCKADDR_COMMON (ss_);	 Address family, etc.  
    char __ss_padding[_SS_PADSIZE];
    __ss_aligntype __ss_align;	 Force desired alignment.  
     };
    sockaddr_storage够大，足够容下所有类型的ip地址  
   */
	listenfd = Open_listenfd(argv[1]);
	std::cout << listenfd<<std::endl;
	while (1)
	{
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (sockaddr*)&clientaddr, &clientlen);
		
		Getnameinfo((sockaddr*)&clientaddr,clientlen,hostname,MAXLINE,port,MAXLINE,0); 
        printf("Accepted connection from (%s, %s)\n", hostname, port);

		doit(connfd);
        Close(connfd);
	}

}

void doit(int fd)
{
	
	int  is_static;
	struct stat sbuf;
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	char filename[MAXLINE], cgiargs[MAXLINE];
	rio_t rio;

	/*读取请求行和头*/
	Rio_readinitb(&rio, fd);
	if(!Rio_readlineb(&rio, buf, MAXLINE))
    return;
	//std::cout << "Request headers:\n";
	std::cout << buf;
	sscanf(buf, "%s %s %s", method, uri, version);

	if (strcasecmp(method, "GET"))  //忽略大小写比较字符串，相等返回0
	{
		clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
		return;
	}

	read_requesthdrs(&rio);

	//分析来自GET请求的uri

	is_static = parse_uri(uri, filename, cgiargs); //1表示静态，0表示动态

	if (stat(filename, &sbuf) < 0)
	{
		clienterror(fd, filename, "404", "Not found",
			"Tiny couldn't find this file");
		return;
	}
	
	if (is_static)         //服务静态内容
	{
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))//S_ISREG(m)表示是否普通文件
																 //S_IRUSR&sbuf.st_mode表示是否有读取权限
		{
			clienterror(fd, filename, "403", "Forbidden", "tiny couldn't read the file");
			return;
		}
		serve_static(fd, filename, sbuf.st_size);

	}
	else                    //服务动态内容
	{
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) 
		{
			clienterror(fd, filename, "403", "Forbidden", "tiny couldn't run the file");
			return;
		}

		serve_dynamic(fd, filename, cgiargs);



	}


}

void clienterror(int fd, const char* cause, const char* errnum,const char* shortmsg, const char* longmsg)
{
	char buf[MAXLINE], body[MAXBUF];
	/*构建响应体*/

	sprintf(body, "<html><title>Tiny Error</title>");
	sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
	sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
	sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
	sprintf(body, "%s<hr><em>The Tiny Web srver</em>\r\n\r\n", body);

	/*打印HTTP响应*/
	sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-type: text/html\r\n");
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
	Rio_writen(fd, buf, strlen(buf));
	Rio_writen(fd, body, strlen(body));

}
void read_requesthdrs(rio_t* rp) //不使用请求报头的任何信息，用这个函数来读取并忽略
{
	char buf[MAXLINE];
	
	Rio_readlineb(rp, buf, MAXLINE);
     printf("%s", buf);

	while (strcmp(buf, "\r\n"))  //终止请求报头的空白行由回车和换行符对组成
	{

		Rio_readlineb(rp, buf, MAXLINE);
		printf("%s", buf);
	}
	return;
}
int parse_uri(char* uri, char* filename, char* cigargs)
{
	char* ptr;

	if (!strstr(uri, "cgi-bin")) //没有cgi-bin,说明是静态内容，strstr return val char*类型，	                         //返回uri中第一次出现cgi-bin的指针，不是uri的一部分则返回空指针
	{
		strcpy(cigargs, "");     //字符串复制函数
		strcpy(filename,".");
		strcat(filename, uri);  //字符串追加，连接函数
        if (uri[strlen(uri)-1] == '/')                   
	        strcat(filename, "home.html");  
		return 1;
	}
	else                        //动态内容
	{
		ptr = index(uri, '?');
		if (ptr) {
			strcpy(cigargs, ptr + 1);
			*ptr = '\0';
		}
		else
			strcpy(cigargs, "");
		strcpy(filename, ".");
		strcat(filename, uri);
		return 0;

	}
}

void serve_static(int fd, char* filename, int filesize)
{
	int srcfd;
	char* srcp, filetype[MAXLINE], buf[MAXLINE];
	
	/*发送响应头到客户端*/
	get_filetype(filename, filetype);

	sprintf(buf,"HTTP/1.0 200 OK\r\n");
	sprintf(buf, "%sServer:Tiny Web Server\r\n", buf);
	sprintf(buf, "%sConnection: close\r\n", buf);
	sprintf(buf, "%sContent-length:%d\r\n", buf, filesize);
	sprintf(buf, "%sContent-type:%s\r\n\r\n", buf, filetype);
	Rio_writen(fd, buf, strlen(buf));
	printf("Response headers:\n");
	printf("%s", buf);

	/*发送响应体给客户端*/
	srcfd = Open(filename, O_RDONLY, 0);
	srcp =(char*) Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
	
	Rio_writen(fd, srcp, filesize);
    Close(srcfd);
	Munmap(srcp, filesize);

}
void get_filetype(char* filename, char* filetype)
{
	if (strstr(filename, ".html"))
		strcpy(filetype, "text/html");
	else if (strstr(filename, ".gif"))
		strcpy(filetype, "image/gif");
	else if (strstr(filename, ".png"))
		strcpy(filetype, "image/png");
	else if (strstr(filename, ".jpg"))
		strcpy(filetype, "image/jpg");
	else if(strstr(filename,"mp4"))
       strcpy(filetype,"video/mp4");
    else
		strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char* filename, char* cgiargs)
{
	char buf[MAXLINE], * emptylist[] = { nullptr };
     /* Return first part of HTTP response */
    sprintf(buf, "HTTP/1.0 200 OK\r\n"); 
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));
    if(signal(SIGCHLD,handlerSIGCHLD)==SIG_ERR)
    unix_error("signal error");

	if (Fork() == 0)               //子进程
	{
		/*真实的服务器会在这设置所有的CGI变量*/
		setenv("QUERY_STRING", cgiargs, 1);

		Dup2(fd, STDOUT_FILENO);            //重定向stdout到客户端

		Execve(filename, emptylist, environ);  //运行CGI程序

	}
	//Wait(NULL);                        //父进程等待回收子进程
                                        //修改利用SIGCHLD信号处理程序回收子进程
}

void handlerSIGCHLD(int sig)
{
    int olderrno=errno;
    while(Waitpid(-1,NULL,0)>0);

    errno=olderrno;

}