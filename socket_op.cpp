//
//  socket_op.cpp
//  agile_io_xc
//
//  Created by huanghaifeng on 15/9/19.
//  Copyright (c) 2015 wonghoifung. All rights reserved.
//

#include "socket_op.h"
#include "coroutine.h"
#include "event_defs.h"
#include "event_op.h"
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/types.h>
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

int set_keep_alive(int fd, int interval)
{
#ifndef __APPLE__
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
#endif
    return 0;
}

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

unsigned ip_to_nl(const char* ip)
{
    struct in_addr s;
    
    if (!ip)
        return htonl(INADDR_ANY);
    
    if (1 != inet_pton(AF_INET, ip, &s))
        return htonl(INADDR_ANY);
    
    return s.s_addr;
}

int create_tcp_server(const char* ip, int port)
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

static void on_readwrite(void* coid)
{
    schedule::ref().coroutine_ready(ud2fd(coid));
}

static void on_connect(void* coid)
{
    schedule::ref().urgent_coroutine_ready(ud2fd(coid));
}

#define CONN_TIMEOUT (10 * 1000)

int CONNECT(int sockfd, const struct sockaddr* addr, socklen_t addrlen)
{
    int ret;
    int flags;
    socklen_t len;
    
    set_nonblock(sockfd);
    
    ret = connect(sockfd, addr, addrlen);
    if (0 == ret)
        return 0;
    
    if (ret < 0 && errno != EINPROGRESS)
        return -1;
    
    if (add_fd_event(sockfd, EVENT_WRITE, on_connect, fd2ud(schedule::ref().currentco_)))
        return -2;
    
    schedule::ref().wait(CONN_TIMEOUT);
    del_fd_event(sockfd, EVENT_WRITE);
    if (schedule::ref().istimeout())
    {
        errno = ETIMEDOUT;
        return -3;
    }
    
    ret = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &flags, &len);
    if (ret == -1 || flags || !len)
    {
        if (flags)
            errno = flags;
        
        return -4;
    }
    
    return 0;
}

#define blocking() ((EAGAIN == errno) || (EWOULDBLOCK == errno))

#define ACCEPT_TIMEOUT (3 * 1000)

int ACCEPT(int sockfd, struct sockaddr* addr, socklen_t* addrlen)
{
    int connfd = 0;
    
    while ((connfd = accept(sockfd, addr, addrlen)) < 0)
    {
        if (EINTR == errno)
            continue;
        
        if (!blocking())
            return -1;
        
        if (add_fd_event(sockfd, EVENT_READ, on_connect, fd2ud(schedule::ref().currentco_)))
            return -2;
        
        //schedule::ref().wait(ACCEPT_TIMEOUT);
        //del_fd_event(sockfd, EVENT_READ);
        //if (schedule::ref().istimeout())
        //{
        //    errno = ETIME;
        //    return -3;
        //}
		schedule::ref().yield();
		del_fd_event(sockfd, EVENT_READ);
    }
    
    if (set_nonblock(connfd))
    {
        close(connfd);
        return -4;
    }
    
    if (enable_tcp_no_delay(connfd))
    {
        close(connfd);
        return -5;
    }
    
    //if (set_keep_alive(connfd, KEEP_ALIVE))
    //{
    //    close(connfd);
    //    return -6;
    //}
    
    return connfd;
}

#define READ_TIMEOUT (10 * 1000)

ssize_t READ(int fd, void* buf, size_t count)
{
    ssize_t n;
    
    while ((n = read(fd, buf, count)) < 0)
    {
        if (EINTR == errno)
            continue;
        
        if (!blocking())
            return -1;
        
        if (add_fd_event(fd, EVENT_READ, on_readwrite, fd2ud(schedule::ref().currentco_)))
            return -2;
        
        //schedule::ref().wait(READ_TIMEOUT);
        //del_fd_event(fd, EVENT_READ);
        //if (schedule::ref().istimeout())
        //{
        //    errno = ETIME;
        //    return -3;
        //}
		schedule::ref().yield();
		del_fd_event(fd, EVENT_READ);
    }
    
    return n;
}

#define RECV_TIMEOUT (10 * 1000)

ssize_t RECV(int sockfd, void* buf, size_t len, int flags)
{
    ssize_t n;
    
    while ((n = recv(sockfd, buf, len, flags)) < 0)
    {
        if (EINTR == errno)
            continue;
        
        if (!blocking())
            return -1;
        
        if (add_fd_event(sockfd, EVENT_READ, on_readwrite, fd2ud(schedule::ref().currentco_)))
            return -2;
        
        //schedule::ref().wait(RECV_TIMEOUT);
        //del_fd_event(sockfd, EVENT_READ);
        //if (schedule::ref().istimeout())
        //{
        //    errno = ETIME;
        //    return -3;
        //}
		schedule::ref().yield();
		del_fd_event(sockfd, EVENT_READ);
    }
    
    return n;
}

#define WRITE_TIMEOUT (10 * 1000)

ssize_t WRITE(int fd, const void* buf, size_t count)
{
    ssize_t n;
    
    while ((n = write(fd, buf, count)) < 0)
    {
        if (EINTR == errno)
            continue;
        
        if (!blocking())
            return -1;
        
        if (add_fd_event(fd, EVENT_WRITE, on_readwrite, fd2ud(schedule::ref().currentco_)))
            return -2;
        
        //schedule::ref().wait(WRITE_TIMEOUT);
        //del_fd_event(fd, EVENT_WRITE);
        //if (schedule::ref().istimeout())
        //{
        //    errno = ETIME;
        //    return -3;
        //}
		schedule::ref().yield();
		del_fd_event(fd, EVENT_WRITE);
    }
    
    return n;
}

#define SEND_TIMEOUT (10 * 1000)

ssize_t SEND(int sockfd, const void* buf, size_t len, int flags)
{
    ssize_t n;
    
    while ((n = send(sockfd, buf, len, flags)) < 0)
    {
        if (EINTR == errno)
            continue;
        
		if (!blocking())
		{
			printf("send error, %s\n", strerror(errno));
			return -1;
		}
            
        
        if (add_fd_event(sockfd, EVENT_WRITE, on_readwrite, fd2ud(schedule::ref().currentco_)))
            return -2;
        
        //schedule::ref().wait(SEND_TIMEOUT);
        //del_fd_event(sockfd, EVENT_WRITE);
        //if (schedule::ref().istimeout())
        //{
        //    errno = ETIME;
        //    return -3;
        //}
		schedule::ref().yield();
		del_fd_event(sockfd, EVENT_WRITE);
    }
    
    return n;
}


