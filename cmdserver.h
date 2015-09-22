//
//  Created by huanghaifeng on 15/9/22.
//  Copyright (c) 2015 wonghoifung. All rights reserved.
//

#ifndef CMDSERVER_HEADER
#define CMDSERVER_HEADER

#include <string>
#include "commons.h"

class cmdserver : private noncopyable
{
public:
	cmdserver(const char* host, short port);
	~cmdserver();
	void start();

	const int& listenfd() const { return listenfd_; }

private:
	std::string host_;
	short port_;
	int listenfd_;
};

#endif


