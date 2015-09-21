//
//  Created by huanghaifeng on 15/9/21.
//  Copyright (c) 2015 wonghoifung. All rights reserved.
//
#ifndef COROUTINE_HEADER
#define COROUTINE_HEADER

#ifdef __APPLE__
#define _XOPEN_SOURCE 600
#endif

#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <queue>
#include <set>

#define COROUTINE_STACK_SIZE (1024*1024)

enum coroutine_status_t
{
	COROUTINE_DEAD,
	COROUTINE_READY,
	COROUTINE_RUNNING,
	COROUTINE_SUSPEND,
};


class schedule;

typedef void(*coroutine_cb)(schedule*, void* ud);

class coroutine
{
public:
	coroutine(schedule* s, coroutine_cb cb, void* ud, int coid);
	~coroutine();

	schedule* sch_;
	coroutine_cb cb_;
	void* ud_;
	ucontext_t uctx_;
	int status_;
	char* stack_;
	int coid_;
};

class schedule
{
private:
	schedule();
	~schedule();

public:
	static schedule& ref();

	int new_coroutine(coroutine_cb cb, void* ud);
	void del_coroutine(int coid);
	void resume(int coid);
	void yield();
	int status(int coid);

	ucontext_t mainctx_;
	int currentco_;
	std::map<int, coroutine*> coroutines_;
	int next_coid_;
    
    std::queue<int> ready_cos_;
    std::set<int> suspend_cos_;
};

#endif

