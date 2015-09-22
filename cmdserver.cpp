//
//  Created by huanghaifeng on 15/9/22.
//  Copyright (c) 2015 wonghoifung. All rights reserved.
//

#include "cmdserver.h"
#include "streamblock.h"
#include "socket_op.h"
#include "coroutine.h"
#include "command.h"

void cmd_connection_handler(schedule* s, void* args)
{
	//cmdserver* cmdsrv = (cmdserver*)args;
	int connfd = (int)(intptr_t)args;
	decoder dpack;

	int ret = recv_block(connfd, dpack);
	if (ret <= 0)
	{
		close(connfd);
	}

	if (dpack.command() != cmd_frombox_greeting_request)
	{
		printf("should send greeting request first\n");
		close(connfd);
	}

	char body[c_buffer_size] = { 0 };
	dpack.read_body(body, c_buffer_size);
	printf("box info: %s\n", body);

	encoder out;
	out.begin(cmd_tobox_greeting_response);
	out.end();
	send_block(connfd, out);

	std::string peer = get_peer_addr(connfd);
	printf("%s register successfully\n", peer.c_str());

	s->wait(20 * 1000);

	close(connfd);
}

void cmd_accept_handler(schedule* s, void* args)
{
	cmdserver* cmdsrv = (cmdserver*)args;
	int listenfd = cmdsrv->listenfd();

	for (;;)
	{
		struct sockaddr addr;
		socklen_t addrlen;
		int connfd = ACCEPT(listenfd, &addr, &addrlen);
		if (connfd > 0)
		{
			schedule::ref().new_coroutine(cmd_connection_handler, (void*)(intptr_t)connfd);
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

cmdserver::cmdserver(const char* host, short port) 
	:host_(host), port_(port), listenfd_(-1)
{

}

cmdserver::~cmdserver()
{

}

void cmdserver::start()
{
	listenfd_ = create_tcp_server(host_.c_str(), port_);

	schedule::ref().new_coroutine(cmd_accept_handler, this);
}

