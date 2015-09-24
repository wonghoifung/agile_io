//
//  Created by huanghaifeng on 15/9/22.
//  Copyright (c) 2015 wonghoifung. All rights reserved.
//

#include "echoserver.h"
#include "socket_op.h"
#include "coroutine.h"
#include "redisdao.h"
#include <string.h>

int cnt = 0;

void echo_connection_handler(schedule* s, void* args)
{
	int connfd = (int)(intptr_t)args;

	const size_t len = 1024;
	char buf[len] = { 0 };
	while (true)
	{
		memset(buf, 0, len);
		ssize_t n = RECV(connfd, buf, len, 0);
		if (n <= 0) {
			printf("RECV return %ld\n", n);
			break;
		}

		cnt += 1;
		if (cnt % 2)
		{
			set_device_online("abc", "xyz", 0);
		}
		else
		{
			set_device_offline("abc", "xyz");
		}
		
		printf("receive: %s\n", buf);
		SEND(connfd, buf, n, 0);
	}
	close(connfd);
}

void echo_accept_handler(schedule* s, void* args)
{
	echoserver* echo = (echoserver*)args;
	int listenfd = echo->listenfd();

	for (;;)
	{
		struct sockaddr addr;
		socklen_t addrlen;
		int connfd = ACCEPT(listenfd, &addr, &addrlen);
		if (connfd > 0)
		{
			schedule::ref().new_coroutine(echo_connection_handler, (void*)(intptr_t)connfd);
		}
		else if (connfd == 0)
		{
			printf("connfd: %d\n", connfd);
			schedule::ref().wait(200);
			continue;
		}
		else
		{
			printf("connfd: %d\n", connfd);
		}
	}
}

echoserver::echoserver(const char* host, short port)
	:host_(host), port_(port), listenfd_(-1)
{

}

echoserver::~echoserver()
{

}

void echoserver::start()
{
	listenfd_ = create_tcp_server(host_.c_str(), port_);

	schedule::ref().new_coroutine(echo_accept_handler, this);
}

