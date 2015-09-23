//
//  Created by huanghaifeng on 15/9/21.
//  Copyright (c) 2015 wonghoifung. All rights reserved.
//

#ifndef EVENT_OP_HEADER
#define EVENT_OP_HEADER

#ifdef __linux__
#include "epoll_op.h"
#endif

#ifdef __APPLE__
#include "kqueue_op.h"
#endif

#ifdef _WIN32 // just make visual studio happy
#include "event_defs.h"
typedef void(*event_callback)(void* args);
inline int add_fd_event(int fd, event_t et, event_callback event_cb, void* args) { return 0; }
inline void del_fd_event(int fd, event_t et) { }
inline void event_loop(int millisecs) { }
inline void event_loop_init(int maxconn) { }
#endif

#endif

