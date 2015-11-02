//
//  Created by huanghaifeng on 15/11/2.
//  Copyright (c) 2015 wonghoifung. All rights reserved.
//

#ifndef SOCKS4A_SERVER_HEADER
#define SOCKS4A_SERVER_HEADER

#include <string>
#include "commons.h"

class socks4a_server : private noncopyable
{
public:
	socks4a_server(const char* host, short port);
	~socks4a_server();
	void start();

	const int& listenfd() const { return listenfd_; }

private:
	std::string host_;
	short port_;
	int listenfd_;
};

#endif


