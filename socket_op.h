//
//  socket_op.h
//  agile_io_xc
//
//  Created by huanghaifeng on 15/9/19.
//  Copyright (c) 2015 wonghoifung. All rights reserved.
//

#ifndef __agile_io_xc__socket_op__
#define __agile_io_xc__socket_op__

#include <string>

int set_nonblock(int fd);
int enable_tcp_no_delay(int fd);
int disable_tcp_no_delay(int fd);
//int set_keep_alive(int fd, int interval);
int set_reuse_addr(int fd);

unsigned fd_to_nl(int fd);
char *get_peer_ip(int fd);
unsigned ip_to_nl(const char *ip);

int create_tcp_server(const char *ip, int port);

std::string get_peer_addr(int fd);

#endif /* defined(__agile_io_xc__socket_op__) */
