//
//  Created by huanghaifeng on 15/9/24.
//  Copyright (c) 2015 wonghoifung. All rights reserved.
//

#include "redisdao.h"
#include "coroutine.h"
#include <assert.h>
#include <sstream>

void connectCallback(const redisAsyncContext* c, int status)
{
	assert(redisdao::ref().connect_coid_ != -1);
	if (status != REDIS_OK)
	{
		printf("connect error: %s\n", c->errstr);
		redisdao::ref().stop_ = true;
	}
	redisdao::ref().connected_ = true;
	schedule::ref().coroutine_ready(redisdao::ref().connect_coid_);
}

void disconnectCallback(const redisAsyncContext* c, int status)
{
	assert(redisdao::ref().connect_coid_ != -1);
	if (status != REDIS_OK)
	{
		printf("disconnect error: %s\n", c->errstr);
		redisdao::ref().stop_ = true;
	}
	redisdao::ref().connected_ = false;
	schedule::ref().coroutine_ready(redisdao::ref().connect_coid_);
}

void redis_connect_handler(schedule* s, void* args)
{
	redisdao* dao = (redisdao*)args;
	while (!dao->stop_)
	{
		dao->context_ = redisAsyncConnect(dao->host_.c_str(), dao->port_);

		if (dao->context_->err)
		{
			printf("init connect error: %s\n", dao->context_->errstr);
			return;
		}

		adapterAttach(dao->context_);
		redisAsyncSetConnectCallback(dao->context_, connectCallback);
		redisAsyncSetDisconnectCallback(dao->context_, disconnectCallback);

		schedule::ref().yield();

		if (redisdao::ref().connected_)
		{
			schedule::ref().yield();
		}
	}
}

redisdao::redisdao() 
	:context_(NULL), connected_(false), host_(""),
	port_(-1), connect_coid_(-1), stop_(false)
{

}

redisdao::~redisdao()
{

}

redisdao& redisdao::ref()
{
	static redisdao dao; 
	return dao;
}

void redisdao::start(const char* host, short port)
{
	host_ = host;
	port_ = port;
	connect_coid_ = schedule::ref().new_coroutine(redis_connect_handler, this);
}

void redisdao::stop()
{
	redisAsyncDisconnect(context_);
	// TODO yield?

	stop_ = true;
	if (connect_coid_ != -1)
	{
		schedule::ref().coroutine_ready(connect_coid_);
	}
}

void replyCallback(redisAsyncContext* c, void* r, void* privdata)
{
	int coid = ud2fd(privdata);
	redisReply* reply = (redisReply*)r;
	redisdao::ref().replys_[coid] = reply;
	schedule::ref().coroutine_ready(coid);
}

#define STATUS_KEY_PREFIX "XL:TIMECLOUD"

inline std::string status_key(const std::string& devid)
{
	std::stringstream sskey;
	sskey << STATUS_KEY_PREFIX << "DEVICE_STATUS:" << devid;
	return sskey.str();
}

#define DATACONN_HISTORY_KEY_PREFIX "DATACONN_HISTORY_"

inline std::string dataconn_history_key()
{
	std::stringstream sskey;
	sskey << DATACONN_HISTORY_KEY_PREFIX << 1 << ":" << 1; // TODO pid nid
	return sskey.str();
}

bool set_device_online(const std::string& devid, const std::string& connid, int expires)
{
	if (!redisdao::ref().connected_)
	{
		printf("redis not connected\n");
		return false;
	}

	std::string key = status_key(devid);
	int expire_at = time(NULL) + expires;
	redisAsyncCommand(redisdao::ref().context_, replyCallback, fd2ud(schedule::ref().currentco_), "HSET %s %s %d", key.c_str(), connid.c_str(), expire_at);
	
	schedule::ref().yield();
	redisReply* reply = redisdao::ref().replys_[schedule::ref().currentco_]; 
	redisdao::ref().replys_.erase(schedule::ref().currentco_);
	if (reply == NULL)
	{
		printf("error: %s\n", redisdao::ref().context_->errstr);
		return false;
	}

	std::string dkey = dataconn_history_key();
	redisAsyncCommand(redisdao::ref().context_, replyCallback, fd2ud(schedule::ref().currentco_), "LPUSH %s %s", dkey.c_str(), connid.c_str());
	
	schedule::ref().yield();
	reply = redisdao::ref().replys_[schedule::ref().currentco_];
	redisdao::ref().replys_.erase(schedule::ref().currentco_);
	if (reply == NULL)
	{
		printf("error: %s\n", redisdao::ref().context_->errstr);
		return false;
	}

	return true;
}

bool set_device_offline(const std::string& devid, const std::string& connid)
{
	if (!redisdao::ref().connected_)
	{
		printf("redis not connected\n");
		return false;
	}

	std::string key = status_key(devid);
	redisAsyncCommand(redisdao::ref().context_, replyCallback, fd2ud(schedule::ref().currentco_), "HDEL %s %s", key.c_str(), connid.c_str());
	schedule::ref().yield();
	redisReply* reply = redisdao::ref().replys_[schedule::ref().currentco_];
	redisdao::ref().replys_.erase(schedule::ref().currentco_);
	if (reply == NULL)
	{
		printf("error: %s\n", redisdao::ref().context_->errstr);
		return false;
	}

	std::string dkey = dataconn_history_key();
	redisAsyncCommand(redisdao::ref().context_, replyCallback, fd2ud(schedule::ref().currentco_), "LREM %s 0 %s", dkey.c_str(), connid.c_str());
	schedule::ref().yield();
	reply = redisdao::ref().replys_[schedule::ref().currentco_];
	redisdao::ref().replys_.erase(schedule::ref().currentco_);
	if (reply == NULL)
	{
		printf("error: %s\n", redisdao::ref().context_->errstr);
		return false;
	}

	return true;
}
