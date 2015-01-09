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
#include "message_header.pb-c.h"

#define BUFSIZE 4096
#define SA  struct sockaddr 

int main(int argc, const char *argv[])
{
	int fd,clifd;
	struct sockaddr_in sockaddr;
	char buf[BUFSIZE];
	int len;
	char header[BUFSIZE];
	char flag[100];
	char rinfo[100];


	fd = socket(AF_INET,SOCK_STREAM,0);
	bzero(&sockaddr,sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	sockaddr.sin_port = htons(7000);

	bind(fd,(SA *)&sockaddr,sizeof(sockaddr));

	listen(fd,10);

	MessageHeader *mh;
	
	clifd = accept(fd,NULL,NULL);
	while(1)
	{
		recv(clifd,buf,sizeof(buf),0);
	
		sscanf(buf,"%s %d %s %s",flag, &len,header,rinfo);
		printf("%s %d\n",flag,len);
		mh = message_header__unpack(NULL,strlen(header),header);
		if(mh == NULL)
		{
			printf("unpack error\n");
			exit(1);
		}
	
		printf("%d %d %d\n",mh->message_id,mh->room_tag,mh->room_tag);
	 }

	message_header__free_unpacked(mh,NULL);
	return 0;
}

