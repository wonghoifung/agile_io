//
//  Created by huanghaifeng on 15/9/24.
//  Copyright (c) 2015 wonghoifung. All rights reserved.
//

#ifndef HIREDIS_ADAPTER_HEADER
#define HIREDIS_ADAPTER_HEADER

#include "hiredis.h"
#include "async.h"
#include "event_op.h"
#include <stdlib.h>

typedef struct adapterEvents 
{
	redisAsyncContext* context;
} adapterEvents;

static void adapterReadEvent(int fd, short event, void* arg) 
{
	((void)fd); ((void)event);
	adapterEvents* e = (adapterEvents*)arg;
	redisAsyncHandleRead(e->context);
}

static void adapterWriteEvent(int fd, short event, void* arg) 
{
	((void)fd); ((void)event);
	adapterEvents *e = (adapterEvents*)arg;
	redisAsyncHandleWrite(e->context);
}

static void event_read_callback(void* args)
{
	adapterReadEvent(0, 0, args);
}

static void event_write_callback(void* args)
{
	adapterWriteEvent(0, 0, args);
}

static void adapterAddRead(void* privdata) 
{
	adapterEvents* e = (adapterEvents*)privdata;
	add_fd_event(e->context->c.fd, EVENT_READ, event_read_callback, e);
}

static void adapterDelRead(void* privdata) 
{
	adapterEvents* e = (adapterEvents*)privdata;
	del_fd_event(e->context->c.fd, EVENT_READ);
}

static void adapterAddWrite(void* privdata) 
{
	adapterEvents* e = (adapterEvents*)privdata;
	add_fd_event(e->context->c.fd, EVENT_WRITE, event_write_callback, e);
}

static void adapterDelWrite(void* privdata) 
{
	adapterEvents* e = (adapterEvents*)privdata;
	del_fd_event(e->context->c.fd, EVENT_WRITE);
}

static void adapterCleanup(void* privdata) 
{
	adapterEvents* e = (adapterEvents*)privdata;
	del_fd_event(e->context->c.fd, EVENT_ALL);
	free(e);
}

inline int adapterAttach(redisAsyncContext* ac)
{
	//redisContext* c = &(ac->c);
	adapterEvents* e;

	/* Nothing should be attached when something is already attached */
	if (ac->ev.data != NULL)
		return REDIS_ERR;

	/* Create container for context and r/w events */
	e = (adapterEvents*)malloc(sizeof(*e));
	e->context = ac;

	/* Register functions to start/stop listening for events */
	ac->ev.addRead = adapterAddRead;
	ac->ev.delRead = adapterDelRead;
	ac->ev.addWrite = adapterAddWrite;
	ac->ev.delWrite = adapterDelWrite;
	ac->ev.cleanup = adapterCleanup;
	ac->ev.data = e;

	/* Initialize and install read/write events */
	// ...
	return REDIS_OK;
}

#endif

