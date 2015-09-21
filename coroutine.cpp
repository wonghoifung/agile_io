//
//  Created by huanghaifeng on 15/9/21.
//  Copyright (c) 2015 wonghoifung. All rights reserved.
//

#include "coroutine.h"
#include <assert.h>

static void coroutine_delegate()
{
	schedule& s = schedule::ref();
	coroutine* co = s.coroutines_[s.currentco_];
	co->cb_(&s, co->ud_);
	s.coroutines_[s.currentco_]->status_ = COROUTINE_DEAD;
	s.currentco_ = -1;
}

coroutine::coroutine(schedule* s, coroutine_cb cb, void* ud, int coid)
	:cb_(cb), sch_(s), ud_(ud), status_(COROUTINE_READY), 
	stack_((char*)malloc(COROUTINE_STACK_SIZE)), coid_(coid)
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

int schedule::new_coroutine(coroutine_cb cb, void* ud)
{
	coroutine* co = new coroutine(this, cb, ud, next_coid_);
	coroutines_.insert(std::make_pair(co->coid_, co));
	next_coid_ += 1;
	return co->coid_;
}

void schedule::del_coroutine(int coid)
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

