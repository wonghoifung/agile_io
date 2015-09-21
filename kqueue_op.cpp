//
//  kqueue_op.cpp
//  agile_io_xc
//
//  Created by huanghaifeng on 15/9/19.
//  Copyright (c) 2015 wonghoifung. All rights reserved.
//
#ifdef __APPLE__
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/event.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "kqueue_op.h"

struct fd_event
{
    int mask;
    event_callback cb;
    void* args;
};

struct event_loop
{
    int maxconn;
    struct fd_event* fe;
    int loopfd;
    struct kevent* ke;
};

static struct event_loop g_eventloop;

int add_fd_event(int fd, event_t et, event_callback event_cb, void* args)
{
    if (fd >= g_eventloop.maxconn)
    {
        errno = ERANGE;
        printf("fd too large\n");
        return -1;
    }
    struct fd_event* fe = &g_eventloop.fe[fd];
    if (EVENT_NONE == fe->mask)
    {
        struct kevent e;
        EV_SET(&e, fd, EVFILT_READ, EV_ADD, 0, 0, fd2ud(fd));
        if (kevent(g_eventloop.loopfd, &e, 1, NULL, 0, NULL) == -1)
        {
            printf("cannot add read, %s\n", strerror(errno));
            return -1;
        }
        EV_SET(&e, fd, EVFILT_WRITE, EV_ADD, 0, 0, fd2ud(fd));
        if (kevent(g_eventloop.loopfd, &e, 1, NULL, 0, NULL) == -1)
        {
            printf("cannot add write, %s\n", strerror(errno));
            EV_SET(&e, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
            kevent(g_eventloop.loopfd, &e, 1, NULL, 0, NULL);
            return -1;
        }
        EV_SET(&e, fd, EVFILT_READ, EV_DISABLE, 0, 0, fd2ud(fd));
        if (kevent(g_eventloop.loopfd, &e, 1, NULL, 0, NULL) == -1)
        {
            printf("cannot disable read, %s\n", strerror(errno));
            EV_SET(&e, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
            kevent(g_eventloop.loopfd, &e, 1, NULL, 0, NULL);
            EV_SET(&e, fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
            kevent(g_eventloop.loopfd, &e, 1, NULL, 0, NULL);
            return -1;
        }
        EV_SET(&e, fd, EVFILT_WRITE, EV_DISABLE, 0, 0, fd2ud(fd));
        if (kevent(g_eventloop.loopfd, &e, 1, NULL, 0, NULL) == -1)
        {
            printf("cannot disable write, %s\n", strerror(errno));
            EV_SET(&e, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
            kevent(g_eventloop.loopfd, &e, 1, NULL, 0, NULL);
            EV_SET(&e, fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
            kevent(g_eventloop.loopfd, &e, 1, NULL, 0, NULL);
            return -1;
        }
    }
    int mask = et | fe->mask;
    if (mask & EVENT_READ)
    {
        struct kevent e;
        EV_SET(&e, fd, EVFILT_READ, EV_ENABLE, 0, 0, fd2ud(fd));
        if (kevent(g_eventloop.loopfd, &e, 1, NULL, 0, NULL) == -1)
        {
            printf("fd:%d, cannot enable read, %s\n", fd, strerror(errno));
            return -1;
        }
    }
    if (mask & EVENT_WRITE)
    {
        struct kevent e;
        EV_SET(&e, fd, EVFILT_WRITE, EV_ENABLE, 0, 0, fd2ud(fd));
        if (kevent(g_eventloop.loopfd, &e, 1, NULL, 0, NULL) == -1)
        {
            printf("cannot enable write, %s\n", strerror(errno));
            return -1;
        }
    }
    fe->mask |= et;
    fe->cb = event_cb;
    fe->args = args;
    return 0;
}

void del_fd_event(int fd, event_t et)
{
    if (fd >= g_eventloop.maxconn)
    {
        return;
    }
    struct fd_event* fe = &g_eventloop.fe[fd];
    if (EVENT_NONE == fe->mask)
    {
        return;
    }
    fe->mask &= ~(et);
    int mask = fe->mask;
    struct kevent e;
    if (mask == EVENT_NONE)
    {
        EV_SET(&e, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
        kevent(g_eventloop.loopfd, &e, 1, NULL, 0, NULL);
        EV_SET(&e, fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
        kevent(g_eventloop.loopfd, &e, 1, NULL, 0, NULL);
        return;
    }
    if (et & EVENT_READ)
    {
        EV_SET(&e, fd, EVFILT_READ, EV_DISABLE, 0, 0, fd2ud(fd));
        kevent(g_eventloop.loopfd, &e, 1, NULL, 0, NULL);
    }
    if (et & EVENT_WRITE)
    {
        EV_SET(&e, fd, EVFILT_WRITE, EV_DISABLE, 0, 0, fd2ud(fd));
        kevent(g_eventloop.loopfd, &e, 1, NULL, 0, NULL);
    }
}

void event_loop(int millisecs)
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = millisecs * 1000 * 1000;
    int n = kevent(g_eventloop.loopfd, NULL, 0, g_eventloop.ke, g_eventloop.maxconn, &t);
    if (n <= 0)
    {
        return;
    }
    for (int i=0; i<n; ++i)
    {
        struct kevent* e = &g_eventloop.ke[i];
        struct fd_event* fe = &g_eventloop.fe[(long)(e->udata)];
        fe->cb(fe->args);
    }
}

void event_loop_init(int maxconn)
{
    g_eventloop.maxconn = maxconn + 128;
    g_eventloop.fe = (struct fd_event*)calloc(g_eventloop.maxconn, sizeof(struct fd_event));
    if (NULL == g_eventloop.fe)
    {
        printf("calloc failure for all fd_events in event loop\n");
        exit(0);
    }
    g_eventloop.loopfd = kqueue();
    if (-1 == g_eventloop.loopfd)
    {
        printf("kqueue failed\n");
        exit(0);
    }
    g_eventloop.ke = (struct kevent*)malloc(g_eventloop.maxconn * sizeof(struct kevent));
    if (NULL == g_eventloop.ke)
    {
        printf("malloc failure for all kevents in event loop\n");
        exit(0);
    }
}
#endif
