//
//  main.cpp
//  agile_io_xc
//
//  Created by huanghaifeng on 15/9/19.
//  Copyright (c) 2015 wonghoifung. All rights reserved.
//

#include "event_op.h"
#include "socket_op.h"
#include "coroutine.h"
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
#include <assert.h>
#include <vector>

static void connection_coroutine(schedule* s, void* args)
{
    int connfd = (int)(intptr_t)args;
    
    const size_t len = 1024;
    char buf[len] = {0};
    while (true)
    {
        ssize_t n = RECV(connfd, buf, len, 0);
        if (n <= 0) {
            break;
        }
        SEND(connfd, buf, len, 0);
    }
    close(connfd);
}

static void accept_coroutine(schedule* s, void* args)
{
    int listenfd = ud2fd(args);

    for (;;)
    {
        struct sockaddr addr;
        socklen_t addrlen;
        int connfd = accept(listenfd, &addr, &addrlen);
        if (connfd > 0)
        {
            if (schedule::ref().new_coroutine(connection_coroutine, (void *)(intptr_t)connfd))
            {
                printf("system busy to handle request.\n");
                close(connfd);
                continue;
            }
            //increase_conn();
        }
        else if (connfd == 0)
        {
            schedule::ref().wait(200);
            continue;
        }
    }
}

std::deque<int>& ready_cos = schedule::ref().ready_cos_;
std::set<int>& suspend_cos = schedule::ref().suspend_cos_;

void run_ready_coroutines()
{
    schedule& s = schedule::ref();
    
    while (ready_cos.size())
    {
        int coid = ready_cos.front();
        ready_cos.pop_front();
        if (s.status(coid))
        {
            s.resume(coid);
        }
        
        if (s.status(coid) == COROUTINE_READY)
        {
            ready_cos.push_back(coid);
        }
        else if (s.status(coid) == COROUTINE_SUSPEND)
        {
            suspend_cos.insert(coid);
        }
        else if (s.status(coid) == COROUTINE_DEAD)
        {
            s.del_coroutine(coid);
        }
        else
        {
            assert(0);
        }
        
    }
}

int main(int argc, char** argv)
{
    event_loop_init(1024);
    int listenfd = create_tcp_server("0.0.0.0", 9797);
    schedule::ref().new_coroutine(accept_coroutine, fd2ud(listenfd));
    
    for (;;) {
        long long now = current_miliseconds();
        std::vector<cotimeout> tos;
        cotimeout_queue::iterator it(schedule::ref().timers_.begin());
        for (; it != schedule::ref().timers_.end(); ++it)
        {
            if (now < it->timeout) {
                break;
            }
            int coid = it->coid;
            coroutine* co = schedule::ref().coroutines_[coid];
            assert(co);
            co->istimeout_ = true;
            schedule::ref().suspend_cos_.erase(coid);
            schedule::ref().ready_cos_.push_back(coid);
            tos.push_back(*it);
        }
        for (size_t i=0; i<tos.size(); ++i) {
            schedule::ref().timers_.erase(tos[i]);
        }
        
        run_ready_coroutines();
        
        int timeout_milliseconds = 0;
        if (schedule::ref().timers_.empty()) {
            timeout_milliseconds = 10 * 1000;
        }
        else {
            timeout_milliseconds = (schedule::ref().timers_.begin())->timeout -current_miliseconds();
            if (timeout_milliseconds < 0) {
                timeout_milliseconds = 0;
            }
        }
        event_loop(timeout_milliseconds);
        run_ready_coroutines();
    }
    
    return 0;
}

#if 0
static void func1(schedule* s, void* ud)
{
	int i = 0;
	while (i < 5)
	{
		printf("[%s] %d\n", __FUNCTION__, i);
		i += 1;
		s->yield();
	}
}

static void func2(schedule* s, void* ud)
{
	int i = 0;
	while (i < 5)
	{
		printf("[%s] %d\n", __FUNCTION__, i);
		i += 1;
		s->yield();
	}
}

std::deque<int>& ready_cos = schedule::ref().ready_cos_;
std::set<int>& suspend_cos = schedule::ref().suspend_cos_;

void run_ready_coroutines()
{
	schedule& s = schedule::ref();

	while (ready_cos.size())
	{
		int coid = ready_cos.front();
		ready_cos.pop_front();
		if (s.status(coid))
		{
			s.resume(coid);
		}

		if (s.status(coid) == COROUTINE_READY)
		{
			ready_cos.push_back(coid);
		}
		else if (s.status(coid) == COROUTINE_SUSPEND)
		{
			suspend_cos.insert(coid);
		}
		else if (s.status(coid) == COROUTINE_DEAD)
		{
			s.del_coroutine(coid);
		}
		else
		{
			assert(0);
		}
	}
}

int main(int argc, char** argv)
{
	schedule& s = schedule::ref();

	int co1 = s.new_coroutine(func1, NULL);
	int co2 = s.new_coroutine(func2, NULL);

	ready_cos.push_back(co1);
	ready_cos.push_back(co2);

	while (ready_cos.size())
	{
		run_ready_coroutines();
		std::set<int>::iterator it(suspend_cos.begin());
		for (; it != suspend_cos.end(); ++it) ready_cos.push_back(*it);
		suspend_cos.clear();
	}

	printf("...\n");
	return 0;
}
#endif

#if 0
void handle_close(int fd)
{
    del_fd_event(fd, EVENT_ALL);
    std::string peeraddr = get_peer_addr(fd);
    printf("%s closed, fd:%d\n", peeraddr.c_str(), fd);
    close(fd);
}

void handle_write(void* args)
{
    //int fd = ud2fd(args);
}

void handle_read(void* args)
{
    int fd = ud2fd(args);
    const size_t len = 1024;
    char buf[len] = {0};
    ssize_t n = recv(fd, buf, len, 0);
    if (n < 0)
    {
        if ((EAGAIN == errno) || (EWOULDBLOCK == errno))
        {
            printf("fd:%d not ready to read\n", fd);
            return;
        }
        else
        {
            printf("fd:%d error happens, close\n", fd);
            handle_close(fd);
        }
    }
    else if (n == 0)
    {
        printf("fd:%d client close\n", fd);
        handle_close(fd);
    }
    else
    {
        printf("fd:%d recv: %s\n", fd, buf);
        n = write(fd, buf, n);
        if (n < 0)
        {
            printf("fd:%d cannot write\n", fd);
        }
    }
}

void handle_accept(void* args)
{
    int listenfd = ud2fd(args);
    struct sockaddr addr;
    socklen_t addrlen;
    int connfd = accept(listenfd, &addr, &addrlen);
    if (connfd < 0)
    {
        printf("connfd < 0\n");
        return;
    }
    int ret = set_nonblock(connfd);
    if (ret != 0)
    {
        printf("fd:%d cannot set nonblock\n", connfd);
        close(connfd);
        return;
    }
    ret = enable_tcp_no_delay(connfd);
    if (ret != 0)
    {
        printf("fd:%d cannot set tcp no delay\n", connfd);
        close(connfd);
        return;
    }

    std::string peeraddr = get_peer_addr(connfd);
    printf("%s connected, fd:%d\n", peeraddr.c_str(), connfd);
    
    ret = add_fd_event(connfd, EVENT_READ, handle_read, fd2ud(connfd));
    if (ret != 0)
    {
        printf("cannot add conn fd %d to eventloop\n", connfd);
        close(connfd);
    }
}

int main(int argc, char** argv)
{
    event_loop_init(1024);
    int listenfd = create_tcp_server("0.0.0.0", 9797);
    if (add_fd_event(listenfd, EVENT_READ, handle_accept, fd2ud(listenfd)) != 0)
    {
        printf("cannot add listen fd:%d to eventloop\n", listenfd);
        return -1;
    }
    while (true)
    {
        event_loop(500);
    }
    return 0;
}

#endif

