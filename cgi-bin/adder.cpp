#include"tinyweb.h"


int main(int argc,char* argv[])
{
    char*args,*p;
    char arg1[MAXLINE];
    char arg2[MAXLINE];
    char content[MAXNAMLEN];
    int n1=0,n2=0;


    if((args=getenv("QUERY_STRING"))!=nullptr)
    {
        //构建响应头
    p=strstr(args,"&");
    *p='\0';
    strcpy(arg1,args);
    strcpy(arg2,p+1);
    n1=atoi(arg1);
    n2=atoi(arg2);
    }
    
    //生成响应主体
    sprintf(content,"<head>welcome to add.com</head>\r\n");
    sprintf(content,"%s <p>The internet addition portal\r\n</p>",content);
    sprintf(content,"%s <p>The anawser :%d + %d=%d\r\n</p>",content,n1,n2,n1+n2);
    sprintf(content,"%s thank you visiting!\r\n<p>",content);


    //生成响应报头
    printf("Connect : closed\r\n ");
    printf("Content length : %d\r\n",(int )strlen(content));
    printf("Content type : text/html\r\n\r\n");

    printf("%s",content);
    fflush(stdout);
    exit(0);








}