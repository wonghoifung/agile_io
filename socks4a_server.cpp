//
//  Created by huanghaifeng on 15/11/2.
//  Copyright (c) 2015 wonghoifung. All rights reserved.
//
#include "socks4a_server.h"
#include "socket_op.h"
#include "coroutine.h"
#include <strings.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <algorithm>

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

int readhostname(int fd, std::string& hostname)
{
	const size_t blen = 1024 * 1024;
	char buf[blen] = { 0 };

	size_t left = blen;
	size_t read = 0;
	char* firstpos = NULL;
	char* secondpos = NULL;
	while (left)
	{
		int offset = blen - left;
		int n = RECV(fd, buf + offset, left, 0);
		if (n <= 0)
		{
			return -1;
		}

		read += n;
		if (firstpos == NULL)
		{
			const char* where = std::find(buf, buf + read, '\0');
			if (where != buf + n)
			{
				firstpos = (char*)where;
			}
		}
		else
		{
			const char* where = std::find(firstpos + 1, buf + read, '\0');
			if (where != buf + n)
			{
				secondpos = (char*)where;
				hostname = secondpos + 1;
				return 1;
			}
		}

		left -= n;
		if (0 == left)
		{
			printf("not enough buffer\n");
			break;
		}
	}
	return -1;
}

static char t_resolveBuffer[64 * 1024];
bool resolve(std::string& hostname, sockaddr_in& addr)
{
	struct hostent hent;
	struct hostent* he = NULL;
	int herrno = 0;
	bzero(&hent, sizeof(hent));

	int ret = gethostbyname_r(hostname.c_str(), &hent, t_resolveBuffer, sizeof t_resolveBuffer, &he, &herrno);
	if (ret == 0 && he != NULL)
	{
		addr.sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
		return true;
	}
	else
	{
		return false;
	}
}

struct sockpair
{
	int src;
	int dst;
};

class sockpair_pool
{
	// TODO
};

void dest_connection_handler(schedule* s, void* args)
{
	sockpair* pair = (sockpair*)args;
	while (true)
	{
		char buf[1024] = { 0 };
		int n = RECV(pair->dst, buf, 1024, 0);
		if (n <= 0)
		{
			close(pair->dst);
			close(pair->src);
			return;
		}

		sendnbytes(pair->src, n, buf);
	}
}

void socks4a_connection_handler(schedule* s, void* args)
{
	int connfd = (int)(intptr_t)args;
	char buf[8] = { 0 };
	int ret = readnbytes(connfd, 8, buf);
	if (ret <= 0)
	{
		close(connfd);
		return;
	}
	char ver = buf[0];
	char cmd = buf[1];
	const void* port = buf + 2;
	const void* ip = buf + 4;

	sockaddr_in addr;
	bzero(&addr, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = *static_cast<const in_port_t*>(port);
	addr.sin_addr.s_addr = *static_cast<const uint32_t*>(ip);

	bool socks4a = be32toh(addr.sin_addr.s_addr) < 256;
	bool okay = false;
	if (socks4a)
	{
		std::string hostname;
		int r = readhostname(connfd, hostname);
		if (r <= 0)
		{
			close(connfd);
			return;
		}
		printf("socks4a host name: %s\n", hostname.c_str());
		if (resolve(hostname, addr))
		{
			okay = true;
		}
	}
	else
	{
		okay = true;
	}

	if (ver == 4 && cmd == 1 && okay)
	{
		int socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (CONNECT(socket, (struct sockaddr *)&addr, sizeof(addr)))
		{
			close(connfd);
			return;
		}
		char response[] = "\000\x5aUVWXYZ";
		memcpy(response + 2, &addr.sin_port, 2);
		memcpy(response + 4, &addr.sin_addr.s_addr, 4);
		sendnbytes(connfd, 8, response);
		sockpair* ud = new sockpair;
		ud->src = connfd;
		ud->dst = socket;
		schedule::ref().new_coroutine(dest_connection_handler, (void*)ud);
	}
	else
	{
		char response[] = "\000\x5bUVWXYZ";
		sendnbytes(connfd, 8, response);
		shutdown(connfd, SHUT_WR);
		return;
	}

	while (true)
	{
		char b[1024] = { 0 };
		int n = RECV(connfd, b, 1024, 0);
		if (n <= 0)
		{
			close(connfd);
			return;
		}
		sendnbytes(connfd, n, b);
	}	
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

