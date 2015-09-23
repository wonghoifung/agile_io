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
#include <deque>
#include <set>
#include "event_op.h"
#include "commons.h"

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

class coroutine : private noncopyable
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
    bool istimeout_;
    long long timeout_;
};

struct cotimeout
{
    int coid;
    int timeout;
};

struct cotimeout_comp
{
    bool operator()(const cotimeout& left, const cotimeout& right)
    {
        if (left.timeout != right.timeout)
        {
            return left.timeout < right.timeout;
        }
        return left.coid < right.coid;
    }
};

typedef std::set<cotimeout, cotimeout_comp> cotimeout_queue;

class schedule : private noncopyable
{
private:
	schedule();
	~schedule();

public:
	static schedule& ref();

	void init();
	void run();

	int new_coroutine(coroutine_cb cb, void* ud);
	void del_coroutine(int coid);

	void resume(int coid);
	void yield();
	int status(int coid);

    void coroutine_ready(int coid);
    void urgent_coroutine_ready(int coid);

    void wait(int milliseconds);
    bool istimeout();
	void check_timers();
	int next_evloop_timeout();

	void run_ready_coroutines();

	ucontext_t mainctx_;
	int currentco_;
	std::map<int, coroutine*> coroutines_;
	int next_coid_;
    
    std::deque<int> ready_cos_;
    std::set<int> suspend_cos_;
    
    cotimeout_queue timers_;
};

long long current_miliseconds();

#endif

