//
//  Created by huanghaifeng on 15/9/21.
//  Copyright (c) 2015 wonghoifung. All rights reserved.
//

#ifndef EVENT_DEFS_HEADER
#define EVENT_DEFS_HEADER

enum event_t
{
    EVENT_NONE = 0x0,
    EVENT_READ = 0x1,
    EVENT_WRITE = 0x2,
    EVENT_ALL = 0x3,
};

inline void* fd2ud(int fd)
{
    return (void*)(long)fd;
}

inline int ud2fd(void* ud)
{
    return (int)(long)ud;
}

#endif

