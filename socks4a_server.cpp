//
//  Created by huanghaifeng on 15/11/2.
//  Copyright (c) 2015 wonghoifung. All rights reserved.
//
#include "socks4a_server.h"
#include "socket_op.h"
#include "coroutine.h"
#include "codec/seri.h"
#include "codec/endian_op.h"
#include <strings.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <algorithm>
#include <errno.h>
#include <assert.h>

std::map<int, int> g_socks_pairs;

#define RECVBUFLEN 1024

void dest_connection_handler(schedule* s, void* args)
{
	int connfd = (int)args;

	std::map<int, int>::iterator it = g_socks_pairs.find(connfd);
	if (it == g_socks_pairs.end())
	{
		printf("[dest_connection_handler] cannot find relay sock, coid:%d\n", schedule::ref().currentco_);
		return;
	}
	int relaysock = it->second;

	char buf[RECVBUFLEN] = { 0 };
	int n = 0;
	while ((n = RECV(relaysock, buf, RECVBUFLEN, 0)) > 0)
	{
		sendnbytes(connfd, n, buf);
	}
	del_fd_event(relaysock, EVENT_ALL);
	close(relaysock);

	printf("[dest_connection_handler] ------> over, coid:%d(fd:%d)\n", schedule::ref().currentco_, relaysock);
}

void socks4a_connection_handler(schedule* s, void* args)
{
	int connfd = (int)(intptr_t)args;
	char buf[RECVBUFLEN] = { 0 };
	
	int n = readnbytes(connfd, 2, buf);
	if (n <= 0)
	{
		printf("--- error read ver and cmd\n");
		close(connfd);
		return;
	}
	
	char ver = buf[0];
	char cmd = buf[1];
	//printf("--- ver:%d, cmd:%d\n", ver, cmd);
	if (ver != 4 || cmd != 1)
	{
		printf("--- invalid ver and cmd\n");
		close(connfd);
		return;
	}

	n = readnbytes(connfd, 6, buf);
	if (n <= 0)
	{
		printf("--- error read port and host\n");
		close(connfd);
		return;
	}

	char* p = buf;
	uint16_t port = read_uint16(p);
	uint32_t ip = read_uint32(p);
	char ipbuf[32] = { 0 };
	ip_int2str(ip, ipbuf);
	//printf("--- port:%u, ip:%u(%s)\n", port, ip, ipbuf);

	std::pair<int,int> ret = readuntil(connfd, buf, RECVBUFLEN, '\0');
	if (ret.first < 0)
	{
		printf("--- read userid error, ret:%d\n", ret.first);
		close(connfd);
		return;
	}
	//printf("--- idx:%d, readcnt:%d, userid:%s\n", ret.first, ret.second, buf);
	
	int relaysock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	struct sockaddr_in sai;
	sai.sin_family = PF_INET;
	sai.sin_port = endian_op<uint16_t>(port);
	sai.sin_addr.s_addr = endian_op<uint32_t>(ip);
	n = CONNECT(relaysock, (struct sockaddr*)&sai, sizeof(struct sockaddr_in));
	if (n != 0)
	{
		printf("--- cannot connect server, ret:%d, %s:%d\n", n, ipbuf, port);
		char* wp = buf;
		write_int8(4, wp);
		write_int8(92, wp);
		write_uint16(0x00, wp);
		write_uint32(0x00, wp);
		sendnbytes(connfd, 8, buf);
		close(connfd);
		return;
	}
	//printf("--- server connected\n");

	char* wp = buf;
	write_int8(0, wp);
	write_int8(90, wp);
	std::pair<uint32_t, uint16_t> netpeer = get_peer_net_pair(relaysock);
	write_uint16(endian_op<uint16_t>(netpeer.second), wp);
	write_uint32(endian_op<uint32_t>(netpeer.first), wp);
	sendnbytes(connfd, 8, buf);

	assert(g_socks_pairs.find(connfd) == g_socks_pairs.end());
	g_socks_pairs[connfd] = relaysock;
	
	int coi = schedule::ref().new_coroutine(dest_connection_handler, (void*)connfd);
	printf("[______________] src_coid:%d(fd:%d), dst_coid:%d(fd:%d)\n", schedule::ref().currentco_, connfd, coi, relaysock);

	while ((n = RECV(connfd, buf, RECVBUFLEN, 0)) > 0)
	{
		sendnbytes(relaysock, n, buf);
	}
	del_fd_event(connfd, EVENT_ALL);
	close(connfd);
	
	g_socks_pairs.erase(connfd);

	printf("[socks4a_connection_handler] ==========> over, coid:%d(fd:%d)\n", schedule::ref().currentco_, connfd);
}

void socks4a_accept_handler(schedule* s, void* args)
{
	socks4a_server* s4asrv = (socks4a_server*)args;
	int listenfd = s4asrv->listenfd();

	for (;;)
	{
		struct sockaddr addr;
		socklen_t addrlen;
		int connfd = ACCEPT(listenfd, &addr, &addrlen);
		if (connfd > 0)
		{
			schedule::ref().new_coroutine(socks4a_connection_handler, (void*)(intptr_t)connfd);
		}
		else if (connfd == 0)
		{
			printf("--- connfd: %d\n", connfd);
			schedule::ref().wait(200);
			continue;
		}
		else
		{
			printf("--- connfd: %d\n", connfd);
		}
	}
}

socks4a_server::socks4a_server(const char* host, short port) :host_(host), port_(port), listenfd_(-1)
{

}

socks4a_server::~socks4a_server()
{

}

void socks4a_server::start()
{
	listenfd_ = create_tcp_server(host_.c_str(), port_);
	schedule::ref().new_coroutine(socks4a_accept_handler, this);
}

