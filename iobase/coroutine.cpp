//
//  Created by huanghaifeng on 15/9/21.
//  Copyright (c) 2015 wonghoifung. All rights reserved.
//

#include "coroutine.h"
#include <assert.h>
#include <sys/time.h>
#include <vector>

long long current_miliseconds()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)(tv.tv_sec) * 1000 + tv.tv_usec / 1000;
}

static void coroutine_delegate()
{
	schedule& s = schedule::ref();
	coroutine* co = s.coroutines_[s.currentco_];
	co->cb_(&s, co->ud_);
	s.coroutines_[s.currentco_]->status_ = COROUTINE_DEAD;
	s.currentco_ = -1;
}

coroutine::coroutine(schedule* s, coroutine_cb cb, void* ud, int coid)
	:sch_(s), cb_(cb), ud_(ud), status_(COROUTINE_READY),
	stack_((char*)malloc(COROUTINE_STACK_SIZE)), coid_(coid),
    istimeout_(false), timeout_(0)
{
	assert(stack_);
	int ret = getcontext(&uctx_);
	assert(ret != -1);
	uctx_.uc_stack.ss_sp = stack_;
	uctx_.uc_stack.ss_size = COROUTINE_STACK_SIZE;
	uctx_.uc_link = &s->mainctx_;
	makecontext(&uctx_, coroutine_delegate, 0);
}

coroutine::~coroutine()
{
	if (stack_)
	{
		free(stack_);
		stack_ = NULL;
	}
}

schedule::schedule() :currentco_(-1), next_coid_(0)
{

}

schedule::~schedule()
{
	std::map<int, coroutine*>::iterator it(coroutines_.begin());
	for (; it != coroutines_.end(); ++it)
	{
		if (it->second)
		{
			delete it->second;
		}
	}
	coroutines_.clear();
}

schedule& schedule::ref()
{
	static schedule s;
	return s;
}

void schedule::init()
{
	event_loop_init(100000);
}

void schedule::run()
{
	for (;;) {
		schedule::ref().check_timers();

		//printf("after check_timers, readycount: %lu, suspendcount: %lu\n", ready_cos_.size(), suspend_cos_.size());

		run_ready_coroutines();

		int timeout_milliseconds = next_evloop_timeout();

		//printf("call event_loop, timeout: %d milliseconds\n", timeout_milliseconds);

		event_loop(timeout_milliseconds);

		//printf("after event_loop, readycount: %lu, suspendcount: %lu\n", ready_cos_.size(), suspend_cos_.size());

		run_ready_coroutines();
	}
}

int schedule::new_coroutine(coroutine_cb cb, void* ud) // called by main routine
{
	coroutine* co = new coroutine(this, cb, ud, next_coid_);
	bool ret = coroutines_.insert(std::make_pair(co->coid_, co)).second;
	if (ret == false)
	{
		printf("[new_coroutine] !!!! cannot insert coid:%d\n", co->coid_);
	}
	else
	{
		printf("[new_coroutine] ++++ insert coid:%d\n", co->coid_);
	}
	next_coid_ += 1;
    if (next_coid_ == -1) {
        next_coid_ += 1;
    }
	ready_cos_.push_back(co->coid_);
	return co->coid_;
}

void schedule::del_coroutine(int coid) // called by main routine
{
	coroutine* currco = coroutines_[coid];
	if (currco)
	{
		delete currco;
		coroutines_.erase(coid);
	}
}

void schedule::resume(int coid)
{
	assert(currentco_ == -1);
	coroutine* co = coroutines_[coid];
	assert(co);
	assert(co->coid_ == coid);
	switch (co->status_)
	{
	case COROUTINE_READY:
	case COROUTINE_SUSPEND:
		currentco_ = coid;
		co->status_ = COROUTINE_RUNNING;
		swapcontext(&mainctx_, &co->uctx_);
		break;
	default:
		assert(0);
	}
}

void schedule::yield()
{
	coroutine* currco = coroutines_[currentco_];
	assert(currco);
	currco->status_ = COROUTINE_SUSPEND;
	suspend_cos_.insert(currentco_);
	currentco_ = -1;
	swapcontext(&currco->uctx_, &mainctx_);
}

int schedule::status(int coid)
{
	std::map<int, coroutine*>::iterator it = coroutines_.find(coid);
	if (it != coroutines_.end())
	{
		return it->second->status_;
	}
	return COROUTINE_DEAD;
}

void schedule::coroutine_ready(int coid) // called by main routine
{
    coroutine* co = coroutines_[coid];
	if (co == NULL)
	{
		printf("[coroutine_ready] gone coid:%d\n", coid);
		//return;
		assert(co);
	}
    
    co->istimeout_ = false;
    suspend_cos_.erase(coid);
    ready_cos_.push_back(coid);
    co->status_ = COROUTINE_READY;
}

void schedule::urgent_coroutine_ready(int coid) // called by main routine
{
    coroutine* co = coroutines_[coid];
    assert(co);
    co->istimeout_ = false;
    suspend_cos_.erase(coid);
    ready_cos_.push_front(coid);
    co->status_ = COROUTINE_READY;
	//printf("[urgent_coroutine_ready] coid:%d\n", coid);
}

void schedule::wait(int milliseconds) 
{
    coroutine* currco = coroutines_[currentco_];
    assert(currco);
    currco->timeout_ = current_miliseconds() + milliseconds;

	//time_t to = currco->timeout_ / 1000;
	//printf("[wait co:%d] timeout:%s\n", currentco_, get_string_time(&to).c_str());

    cotimeout ct;
    ct.coid = currentco_;
    ct.timeout = currco->timeout_;
    timers_.insert(ct);
    yield();
}

void schedule::cancel_wait()
{
	// now one coroutine can only have one timer
	cotimeout_queue::iterator it(timers_.begin());
	for (; it != timers_.end(); ++it)
	{
		if (it->coid == currentco_)
		{
			timers_.erase(it);
			return;
		}
	}
}

bool schedule::istimeout()
{
    coroutine* currco = coroutines_[currentco_];
    assert(currco);
	bool ret = currco->istimeout_;
	currco->istimeout_ = false;
    return ret;
}

void schedule::check_timers() // called by main routine
{
	long long now = current_miliseconds();
	std::vector<cotimeout> tos;
	cotimeout_queue::iterator it(timers_.begin());
	//printf("timer count: %lu\n", timers_.size());

	//time_t t = now / 1000;
	//printf("[check_timers co:%d] now: %s\n", currentco_, get_string_time(&t).c_str());
	for (; it != timers_.end(); ++it)
	{
		//time_t t = it->timeout / 1000;
		//printf("[check_timers co:%d] cco: %d, timeout: %s\n", currentco_, it->coid, get_string_time(&t).c_str());
		if (now <= it->timeout) {
			//printf("[check_timers] not timeout, break cco:%d\n", it->coid);
			break;
		}
		//printf("[check_timers] timeout, cco:%d\n", it->coid);
		int coid = it->coid;
		coroutine* co = coroutines_[coid];
		assert(co);
		co->istimeout_ = true;
		suspend_cos_.erase(coid);
		ready_cos_.push_back(coid);
		tos.push_back(*it);
	}
	for (size_t i = 0; i<tos.size(); ++i) {
		timers_.erase(tos[i]);
	}
	//printf("timeout timer count: %lu, left:%lu\n", tos.size(), timers_.size());
	tos.clear();
}

int schedule::next_evloop_timeout()
{
	int timeout_milliseconds = 0;
	if (timers_.empty()) {
		timeout_milliseconds = 10 * 1000;
	}
	else {
		timeout_milliseconds = (timers_.begin())->timeout - current_miliseconds();
		if (timeout_milliseconds < 0) {
			timeout_milliseconds = 0;
		}
	}
	return timeout_milliseconds;
}

void schedule::run_ready_coroutines() // called by main routine
{
	while (ready_cos_.size())
	{
		int coid = ready_cos_.front();
		ready_cos_.pop_front();

		if (status(coid))
		{
			resume(coid);
		}

		if (status(coid) == COROUTINE_DEAD)
		{
			printf("[run_ready_coroutines] del coroutine: %d\n", coid);
			del_coroutine(coid);
		}
	}
}




