#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <strings.h>
#include "header.pb-c.h"
#include <errno.h>

#define BUFSIZE 4096
#define SA  struct sockaddr 

ssize_t saferead(int fd, char *buf,size_t len)
{
	size_t nread;
	size_t nleft;
	char *ptr;

	ptr = buf;
	nleft = len;
	while(nleft > 0)
	{
		if(nread = read(fd,ptr,nleft) < 0)
		{
			if(errno == EINTR)
			  	nread = 0;
			else
				return(-1);
		}else if(nread == 0)
		{
			break;
		}
		nleft -= nread;
		ptr += nread;
	}
	return (len - nleft);
}

int main(int argc, const char *argv[])
{
	int fd,clifd;
	struct sockaddr_in sockaddr;
	char buf[BUFSIZE];
	int len,ilen;
	char header[100];
	char flag[100];
	char rinfo[100];


	fd = socket(AF_INET,SOCK_STREAM,0);
	bzero(&sockaddr,sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	sockaddr.sin_port = htons(7000);

	bind(fd,(SA *)&sockaddr,sizeof(sockaddr));

	listen(fd,10);

	Header *mh;
	
	clifd = accept(fd,NULL,NULL);
	while(1)
	{
		saferead(clifd,buf,sizeof(buf));
		printf("%s\n",buf);
		sscanf(buf,"%s %s",flag,header);
		//printf("%s %d\n",flag,len);
		printf("Header: %d\n", strlen(header));
		//printf("info size:%d\n",strlen(rinfo));
		mh = header__unpack(NULL,4,header);
		if(mh == NULL)
		{ 
			printf("unpack error\n");
			exit(1);
	 	 }  
	
		printf("%d %d \n",mh->message_id,mh->message_flag);
	 }  

	message_header__free_unpacked(mh,NULL);
	return 0;
}

