//
//  decoder.cpp
//  console
//
//  Created by huanghaifeng on 14-10-1.
//  Copyright (c) 2014 wonghoifung. All rights reserved.
//

#include "decoder.h"
        
int decoder::read_int()
{
    int val(-1);
    codec::read_b((char*)&val, sizeof(int));
    return endian_op(val);
}
        
int decoder::read_int_and_repack()
{
    int val(-1);
    codec::read_del_b((char*)&val, sizeof(int));
    return endian_op(val);
}
        
unsigned int decoder::read_uint()
{
    unsigned int val(0);
    codec::read_b((char*)&val, sizeof(unsigned int));
    return endian_op(val);
}
        
int64_t decoder::read_int64()
{
	int64_t val(0);
	codec::read_b((char*)&val, sizeof(int64_t));
    return endian_op(val);
}

uint64_t decoder::read_uint64()
{
	uint64_t val(0);
	codec::read_b((char*)&val, sizeof(uint64_t));
    return endian_op(val);
}

long decoder::read_long()
{
    long val(0);
    codec::read_b((char*)&val, sizeof(long));
    return endian_op(val);
}

unsigned long decoder::read_ulong()
{
    unsigned long val(0);
    codec::read_b((char*)&val, sizeof(unsigned long));
    return endian_op(val);
}
        
short decoder::read_short()
{
    short val(-1);
    codec::read_b((char*)&val, sizeof(short));
    return endian_op(val);
}
        
unsigned short decoder::read_ushort()
{
	unsigned short val(0);
	codec::read_b((char*)&val, sizeof(unsigned short));
    return endian_op(val);
}

char decoder::read_char()
{
	char val(0);
    codec::read_b((char*)&val, sizeof(char));
    return endian_op(val);
}

unsigned char decoder::read_uchar()
{
    unsigned char val(0);
    codec::read_b((char*)&val, sizeof(unsigned char));
    return endian_op(val);
}
        
char* decoder::read_cstring()
{
    int len = read_int();
    if(len <= 0 || len >= c_buffer_size)
        return NULL;
    return codec::read_bytes_b(len);
}
        
int decoder::read_binary(char* outbuf, int maxlen)
{
    int len = read_int();
    if(len <= 0)
        return -1;
    if (len > maxlen)
    {
        codec::read_undo_b(sizeof(int));
        return -1;
    }
    if (codec::read_b(outbuf, len))
        return len;
    return 0;
}

uint32_t decoder::read_body(char* outbuf, uint32_t maxlen)
{
	uint32_t bodylen = codec::length();
	if (bodylen > maxlen)
	{
		return 0;
	}
	if (codec::read_b(outbuf, bodylen))
	{
		return bodylen;
	}
	return 0;
}

bool decoder::copy(const void* inbuf, int len)
{
    return codec::copy_b(inbuf, len);
}
        
void decoder::begin(uint16_t cmd)
{
    codec::begin_b(cmd);
}

void decoder::begin(uint16_t cmd, uint32_t streamid)
{
	codec::begin_b(cmd, streamid);
}

void decoder::end()
{
    codec::end_b();
}

