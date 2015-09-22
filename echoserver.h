//
//  Created by huanghaifeng on 15/9/22.
//  Copyright (c) 2015 wonghoifung. All rights reserved.
//

#ifndef ECHOSERVER_HEADER
#define ECHOSERVER_HEADER

#include <string>
#include "commons.h"

class echoserver : private noncopyable
{
public:
	echoserver(const char* host, short port);
	~echoserver();
	void start();

	const int& listenfd() const { return listenfd_; }

private:
	std::string host_;
	short port_;
	int listenfd_;
};

#endif


