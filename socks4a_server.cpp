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

uint32_t ip_str2int(const char* ip)
{
	struct in_addr addr;
	inet_aton(ip, &addr);
	return endian_op<uint32_t>(addr.s_addr);
}

void ip_int2str(uint32_t ip_num, char* ip)
{
	struct in_addr addr;
	addr.s_addr = endian_op<uint32_t>(ip_num);
	strcpy(ip, (char*)inet_ntoa(addr));
}

int sendnbytes(int sockfd, size_t N, char* buf)
{
	ssize_t offset = 0;
	ssize_t left = N;

	while (true)
	{
		ssize_t n = SEND(sockfd, buf + offset, left, 0);
		if (n < 0)
		{
			return n;
		}
		left -= n;
		offset += n;
		if (0 == left)
		{
			break;
		}
	}
	return N;
}

int readnbytes(int fd, size_t N, char* buf)
{
	size_t left = N;
	while (left)
	{
		int offset = N - left;
		int n = RECV(fd, buf + offset, left, 0);
		if (n <= 0)
		{
			return n;
		}
		left -= n;
		if (0 == left)
		{
			break;
		}
	}
	return N;
}

std::pair<int,int> readuntil(int fd, char* buf, size_t buflen, char e)
{
	int readcnt = 0;
	int n = 0;
	while (buflen - readcnt > 0)
	{
		n = RECV(fd, buf + readcnt, buflen - readcnt, 0);
		if (n <= 0)
		{
			return std::make_pair(-1, readcnt); // error 
		}
		readcnt += n;
		char* pos = strchr(buf, e);
		if (pos)
		{
			return std::make_pair(pos - buf, readcnt);
		}
	}
	return std::make_pair(-2, readcnt); // not enough buf
}

struct sock_pair
{
	int src;
	int dst;
	bool dst_done;
	bool src_done;
};

#define RECVBUFLEN 1024

void dest_connection_handler(schedule* s, void* args)
{
	sock_pair* sp = (sock_pair*)args;
	char buf[RECVBUFLEN] = { 0 };
	int n = 0;
	while ((n = RECV(sp->dst, buf, RECVBUFLEN, 0)) > 0)
	{
		sendnbytes(sp->src, n, buf);
	}
	// TODO
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
	printf("--- ver:%d, cmd:%d\n", ver, cmd);
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
	printf("--- port:%u, ip:%u(%s)\n", port, ip, ipbuf);

	std::pair<int,int> ret = readuntil(connfd, buf, RECVBUFLEN, '\0');
	if (ret.first < 0)
	{
		printf("--- read userid error, ret:%d\n", ret.first);
		close(connfd);
		return;
	}
	printf("--- idx:%d, readcnt:%d, userid:%s\n", ret.first, ret.second, buf);
	
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
	printf("--- server connected\n");

	char* wp = buf;
	write_int8(0, wp);
	write_int8(90, wp);
	std::pair<uint32_t, uint16_t> netpeer = get_peer_net_pair(relaysock);
	write_uint16(endian_op<uint16_t>(netpeer.second), wp);
	write_uint32(endian_op<uint32_t>(netpeer.first), wp);
	sendnbytes(connfd, 8, buf);

	sock_pair* sp = new sock_pair;
	sp->src = connfd;
	sp->dst = relaysock;
	sp->src_done = false;
	sp->dst_done = false;
	schedule::ref().new_coroutine(dest_connection_handler, (void*)sp);

	while ((n = RECV(connfd, buf, RECVBUFLEN, 0)) > 0)
	{
		sendnbytes(relaysock, n, buf);
	}

	// TODO
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

