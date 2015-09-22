//
//  Created by huanghaifeng on 15/9/22.
//  Copyright (c) 2015 wonghoifung. All rights reserved.
//

#include "streamblock.h"
#include "socket_op.h"
#include <stdio.h>

int recv_block(int sockfd, decoder& dpack)
{
	char* buf = dpack.buffer();

	size_t left = c_header_size;
	while (left)
	{
		int offset = c_header_size - left;
		int n = RECV(sockfd, buf + offset, left, 0);
		if (n <= 0)
		{
			return n;
		}
		left -= n;
		if (0 == left)
		{
			break;
		}
	}
	
	uint8_t version = dpack.version();
	uint8_t headsize = dpack.headsize();
	if (version != c_default_version || headsize != c_header_size)
	{
		printf("packet format error, version:%d, headsize:%d\n", version, headsize);
		return 0;
	}

	size_t bodyleft = dpack.length();
	if (bodyleft > (c_buffer_size - c_header_size))
	{
		printf("packet format error, bodylen:%d\n", bodyleft);
		return 0;
	}

	size_t bodyoffset = c_header_size;
	while (bodyleft)
	{
		int n = RECV(sockfd, buf + bodyoffset, bodyleft, 0);
		if (n <= 0)
		{
			return n;
		}
		bodyleft -= n;
		bodyoffset += n;
		if (0 == bodyleft)
		{
			break;
		}
	}

	return dpack.size();
}

int send_block(int sockfd, encoder& epack)
{
	while (true)
	{
		ssize_t offset = 0;
		ssize_t left = epack.size();
		ssize_t n = SEND(sockfd, epack.buffer() + offset, left, 0);
		if (n < 0)
		{
			return n;
		}
		left -= n;
		offset += n;
		if (0 == left)
		{
			break;
		}
	}
	return epack.size();
}

