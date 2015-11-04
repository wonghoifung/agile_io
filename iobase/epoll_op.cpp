//
//  epoll_op.cpp
//  agile_io_xc
//
//  Created by huanghaifeng on 15/9/19.
//  Copyright (c) 2015 wonghoifung. All rights reserved.
//
#ifdef __linux__
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include "epoll_op.h"

struct fd_event
{
    int mask;
    event_callback cb;
    void *args;
};

struct event_loop
{
    int max_conn;               
    struct fd_event *array;     
    int epfd;                   
    struct epoll_event *ee; 
};

static struct event_loop g_eventloop;

int add_fd_event(int fd, event_t et, event_callback event_cb, void* args)
{
    struct fd_event *fe;
    int op;
    struct epoll_event ee;
    int mask;

    if (fd >= g_eventloop.max_conn)
    {
        errno = ERANGE;
        return -1;
    }

    fe = &g_eventloop.array[fd];
    op = (EVENT_NONE == fe->mask) ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;
    ee.events = 0;
    ee.data.fd = fd;
    mask = et | fe->mask; //merge old event

    if (mask & EVENT_READ)  ee.events |= EPOLLIN;
    if (mask & EVENT_WRITE)  ee.events |= EPOLLOUT;

    if (-1 == epoll_ctl(g_eventloop.epfd, op, fd, &ee))
        return -2;

    fe->mask |= et;
    fe->cb = event_cb;
    fe->args = args;

    return 0;
}

void del_fd_event(int fd, event_t et)
{
    struct fd_event *fe;
    struct epoll_event ee;
    int mask;

    if (fd >= g_eventloop.max_conn)
        return;

    fe = &g_eventloop.array[fd];
    if (EVENT_NONE == fe->mask)
        return;

    fe->mask &= ~(et);    
    mask = fe->mask;
    ee.events = 0;
    ee.data.fd = fd;

    if (mask & EVENT_READ)  ee.events |= EPOLLIN;
    if (mask & EVENT_WRITE)  ee.events |= EPOLLOUT;

    if (mask == EVENT_NONE)
        epoll_ctl(g_eventloop.epfd, EPOLL_CTL_DEL, fd, &ee);
    else
        epoll_ctl(g_eventloop.epfd, EPOLL_CTL_MOD, fd, &ee);
}

void event_loop(int millisecs)
{
    int i;
    int value;

    value = epoll_wait(g_eventloop.epfd, g_eventloop.ee, g_eventloop.max_conn, millisecs);
    if (value <= 0)
        return;

    for (i = 0; i < value; i++)
    {
        struct epoll_event *ee = &g_eventloop.ee[i];
        struct fd_event *fe = &g_eventloop.array[ee->data.fd];
		printf("[event_loop] fd:%d\n",ee->data.fd);
        fe->cb(fe->args);
    }
}

void event_loop_init(int maxconn)
{
    g_eventloop.max_conn = maxconn + 128;    

    g_eventloop.array = (struct fd_event *)calloc(g_eventloop.max_conn, sizeof(struct fd_event));
    if (NULL == g_eventloop.array)
    {
        printf("Failed to malloc fd g_eventloop\n");
        exit(0);
    }

    g_eventloop.epfd = epoll_create(1024);
    if (-1 == g_eventloop.epfd)
    {
        printf("Failed to create epoll. err:%s\n", strerror(errno));
        exit(0);
    }

    g_eventloop.ee = (struct epoll_event *)malloc(g_eventloop.max_conn * sizeof(struct epoll_event));
    if (NULL == g_eventloop.ee)
    {
        printf("Failed to malloc epoll g_eventloop\n");
        exit(0);
    }
}
#endif

