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

#endif

