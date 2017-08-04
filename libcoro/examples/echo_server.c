#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "coro.h"

void* co_server(void* arg);
void* co_connection(void* arg);

int main()
{
	init_coroutine_env();
	crt_create(NULL,NULL,co_server,NULL);
	pause();
}

void* co_server(void* arg)
{
	int listenfd = crt_tcp_prepare_sock(inet_addr("127.0.0.1"),htons(8888));
	(void)arg;
	assert(listenfd>=0);
	while (1)
	{
		int connfd = crt_tcp_accept(listenfd,NULL,NULL);
		crt_create(NULL,NULL,co_connection,(void*)(intptr_t)connfd);
	}
	crt_exit(NULL);
}

void* co_connection(void* arg)
{
	int fd = (int)(intptr_t)arg;
	char buf[1024] = {0};
	ssize_t nread,nwrite;
	for (size_t i=0; i<sizeof buf; ++i)
	{
		nread = crt_tcp_read(fd, &buf[i], 1);
		if (nread != 1) printf("errno = %d, strerror(errno) = %s\n", errno, strerror(errno));
		if (buf[i]=='\n')
		{
			buf[i+1]='\0';
			break;
		}
	}
	printf("[%d] (nread = %zd) %s\n", fd, nread, buf);
	nwrite = crt_tcp_write(fd, &buf[0], strlen(buf));
	printf("[%d] (nwrite = %zd) %s\n", fd, nwrite, buf);
	crt_sock_close(fd);
	crt_exit(NULL);
}
