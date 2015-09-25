//
//  Created by huanghaifeng on 15/9/24.
//  Copyright (c) 2015 wonghoifung. All rights reserved.
//

#ifndef REDISDAO_HEADER
#define REDISDAO_HEADER

#include "commons.h"
#include "hiredis_adapter.h"
#include <map>

class redisdao :private noncopyable
{
private:
	redisdao();
	~redisdao();

public:
	static redisdao& ref();
	void start(const char* host, short port);
	void stop();

	redisAsyncContext* context_;
	bool connected_;
	std::string host_;
	short port_;
	int connect_coid_;
	bool stop_;
	std::map<int, redisReply*> replys_;
};

bool set_device_online(const std::string& devid, const std::string& connid, int expires);
bool set_device_offline(const std::string& devid, const std::string& connid);

#endif

