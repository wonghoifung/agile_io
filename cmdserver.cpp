//
//  Created by huanghaifeng on 15/9/22.
//  Copyright (c) 2015 wonghoifung. All rights reserved.
//

#include "cmdserver.h"
#include "streamblock.h"
#include "socket_op.h"
#include "coroutine.h"
#include "command.h"
#include "redisdao.h"
#include <json/json.h>
#include <sstream>

inline std::string get_connectionid(const std::string& devid, const std::string& addr)
{
	std::stringstream ss;
	ss << devid << "(" << addr << ")";
	return ss.str();
}

void cmd_connection_handler(schedule* s, void* args)
{
	//cmdserver* cmdsrv = (cmdserver*)args;
	int connfd = (int)(intptr_t)args;
	decoder dpack;

	int ret = recv_block(connfd, dpack);
	if (ret <= 0)
	{
		printf("recv_block return: %d\n", ret);
		close(connfd);
		return;
	}

	if (dpack.command() != cmd_frombox_greeting_request)
	{
		printf("should send greeting request first\n");
		close(connfd);
		return;
	}

	char body[c_buffer_size] = { 0 };
	dpack.read_body(body, c_buffer_size);
	printf("box info: %s\n", body);

	Json::Reader reader;
	Json::Value root;
	if (!reader.parse(body, root, false))
	{
		printf("invalid greeting request json\n");
		close(connfd);
		return;
	}
	std::string devid = root["deviceid"].asString();
	std::string connid = get_connectionid(devid, get_peer_addr(connfd));
	set_device_online(devid, connid, 0);

	encoder out;
	out.begin(cmd_tobox_greeting_response);
	out.end();
	ret = send_block(connfd, out);

	if (ret < 0)
	{
		printf("send_block ret: %d\n", ret);
		close(connfd);
		set_device_offline(devid, connid);
		return;
	}

	std::string peer = get_peer_addr(connfd);
	printf("%s register successfully\n", peer.c_str());

	while (ret > 0)
	{
		ret = recv_block(connfd, dpack);
		printf("ret:%d, cmd:%d\n", ret, dpack.command());
	}
	
	// TODO
	close(connfd);
	set_device_offline(devid, connid);
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

