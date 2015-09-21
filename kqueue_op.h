//
//  kqueue_op.h
//  agile_io_xc
//
//  Created by huanghaifeng on 15/9/19.
//  Copyright (c) 2015 wonghoifung. All rights reserved.
//

#ifndef __agile_io_xc__kqueue_op__
#define __agile_io_xc__kqueue_op__

#include "event_defs.h"

typedef void (*event_callback)(void* args);

int add_fd_event(int fd, event_t et, event_callback event_cb, void* args);

void del_fd_event(int fd, event_t et);

void event_loop(int millisecs);

void event_loop_init(int maxconn);

#endif /* defined(__agile_io_xc__kqueue_op__) */
