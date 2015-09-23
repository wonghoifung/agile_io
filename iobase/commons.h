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
#include <sys/resource.h>

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

inline void set_rlimit()
{
	struct rlimit rlim, rlim_new;

	if (getrlimit(RLIMIT_NOFILE, &rlim) == 0)
	{
		rlim_new.rlim_cur = rlim_new.rlim_max = 100000;
		if (setrlimit(RLIMIT_NOFILE, &rlim_new) != 0)
		{
			printf("set rlimit file failure\n");
			exit(0);
		}
	}

	if (getrlimit(RLIMIT_CORE, &rlim) == 0)
	{
		rlim_new.rlim_cur = rlim_new.rlim_max = RLIM_INFINITY;
		if (setrlimit(RLIMIT_CORE, &rlim_new) != 0)
		{
			printf("set rlimit core failure\n");
			exit(0);
		}
	}
}

inline int cpu_num()
{
	static int num = sysconf(_SC_NPROCESSORS_CONF);
	return num;
}

inline void set_daemon()
{
	if (fork() > 0)
	{
		exit(0);
	}
}

#endif

