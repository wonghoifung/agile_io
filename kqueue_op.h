//
//  kqueue_op.h
//  agile_io_xc
//
//  Created by huanghaifeng on 15/9/19.
//  Copyright (c) 2015 wonghoifung. All rights reserved.
//

#ifndef __agile_io_xc__kqueue_op__
#define __agile_io_xc__kqueue_op__

enum event_t
{
    EVENT_NONE = 0x0,
    EVENT_READ = 0x1,
    EVENT_WRITE = 0x2,
    EVENT_ALL = 0x3,
};

typedef void (*event_callback)(void* args);

int add_fd_event(int fd, event_t et, event_callback event_cb, void* args);

void del_fd_event(int fd, event_t et);

void event_loop(int millisecs);

void event_loop_init(int maxconn);

inline void* fd2ud(int fd)
{
    return (void*)(long)fd;
}

inline int ud2fd(void* ud)
{
    return (int)(long)ud;
}

#endif /* defined(__agile_io_xc__kqueue_op__) */
