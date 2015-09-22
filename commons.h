//
//  Created by huanghaifeng on 15/9/22.
//  Copyright (c) 2015 wonghoifung. All rights reserved.
//

#ifndef COMMONS_HEADER
#define COMMONS_HEADER

#include <time.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

class noncopyable
{
protected:
	noncopyable() {}
	~noncopyable() {}
private: 
	noncopyable(const noncopyable&);
	const noncopyable& operator=(const noncopyable&);
};

inline std::string get_string_time(time_t* time_)
{
	char szTime[20] = { 0 };
	struct tm* t = localtime(time_);
	sprintf(szTime, "%d-%02d-%02d %02d:%02d:%02d",
		1900 + t->tm_year, t->tm_mon + 1, t->tm_mday,
		t->tm_hour, t->tm_min, t->tm_sec);
	return szTime;
}

#endif

