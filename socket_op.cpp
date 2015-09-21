//
//  socket_op.cpp
//  agile_io_xc
//
//  Created by huanghaifeng on 15/9/19.
//  Copyright (c) 2015 wonghoifung. All rights reserved.
//

#include "socket_op.h"
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int set_nonblock(int fd)
{
    int flags;
    
    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
        return -1;
    
    if (flags & O_NONBLOCK)
        return 0;
    
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        return -1;
    
    return 0;
}

static inline int set_tcp_no_delay(int fd, int val)
{
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) == -1)
        return -1;
    
    return 0;
}

int enable_tcp_no_delay(int fd)
{
    return set_tcp_no_delay(fd, 1);
}

int disable_tcp_no_delay(int fd)
{
    return set_tcp_no_delay(fd, 0);
}
/*
int set_keep_alive(int fd, int interval)
{
    int val = 1;
    
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) == -1)
        return -1;
    
    val = interval;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &val, sizeof(val)) < 0)
        return -1;
    
    val = interval / 3;
    if (val == 0)
        val = 1;
    
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &val, sizeof(val)) < 0)
        return -1;
    
    val = 3;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &val, sizeof(val)) < 0)
        return -1;
    
    return 0;
}
*/
int set_reuse_addr(int fd)
{
    int yes = 1;
    
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
        return -1;
    
    return 0;
}

unsigned fd_to_nl(int fd)
{
    struct sockaddr_in sa;
    socklen_t addrlen = sizeof(sa);
    
    if (getpeername(fd, (struct sockaddr *)&sa, &addrlen))
        return htonl(INADDR_ANY);
    
    return sa.sin_addr.s_addr;
}

char *get_peer_ip(int fd)
{
    struct in_addr addr;
    
    addr.s_addr = fd_to_nl(fd);
    return inet_ntoa(addr);
}

unsigned ip_to_nl(const char *ip)
{
    struct in_addr s;
    
    if (!ip)
        return htonl(INADDR_ANY);
    
    if (1 != inet_pton(AF_INET, ip, &s))
        return htonl(INADDR_ANY);
    
    return s.s_addr;
}

int create_tcp_server(const char *ip, int port)
{
    int listenfd;
    struct sockaddr_in svraddr;
    
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1)
    {
        printf("socket failed. %d %s\n", errno, strerror(errno));
        exit(0);
    }
    
    if (set_reuse_addr(listenfd))
    {
        printf("set reuse listen socket failed. %d %s\n", errno, strerror(errno));
        exit(0);
    }
    
    if (set_nonblock(listenfd))
    {
        printf("set listen socket non-bloack failed. %d %s\n", errno, strerror(errno));
        exit(0);
    }
    
    memset(&svraddr, 0, sizeof(svraddr));
    svraddr.sin_family = AF_INET;
    svraddr.sin_port = htons(port);
    svraddr.sin_addr.s_addr = ip_to_nl(ip);
    
    if (0 != bind(listenfd, (struct sockaddr *)&svraddr, sizeof(svraddr)))
    {
        printf("bind failed. %d %s\n", errno, strerror(errno));
        exit(0);
    }
    
    if (0 != listen(listenfd, 1000))
    {
        printf("listen failed. %d %s\n", errno, strerror(errno));
        exit(0);
    }
    
    return listenfd;
}

std::string get_peer_addr(int fd)
{
    struct sockaddr_in peeraddr;
    bzero(&peeraddr, sizeof peeraddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
    if (::getpeername(fd, (struct sockaddr*)(&peeraddr), &addrlen) < 0)
    {
        printf("cannot getpeername of fd:%d\n", fd);
        return "";
    }
    else
    {
        char buf[64] = {0};
        sprintf(buf, "%s:%d", inet_ntoa(peeraddr.sin_addr),
                ntohs(peeraddr.sin_port));
        return buf;
    }
}
