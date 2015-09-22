//
//  encoder.cpp
//  console
//
//  Created by huanghaifeng on 14-10-1.
//  Copyright (c) 2014 wonghoifung. All rights reserved.
//

#include "encoder.h"
   
bool encoder::write_int(int val)
{
	val = endian_op(val);
    return codec::write_b((char*)&val, sizeof(int));
}
        
bool encoder::write_uint(unsigned int val)
{
	val = endian_op(val);
    return codec::write_b((char*)&val, sizeof(unsigned int));
}
        
bool encoder::write_int64(int64_t val)
{
	val = endian_op(val);
	return codec::write_b((char*)&val, sizeof(int64_t));
}

bool encoder::write_uint64(uint64_t val)
{
	val = endian_op(val);
	return codec::write_b((char*)&val, sizeof(uint64_t));
}

bool encoder::write_long(long val)
{
	val = endian_op(val);
    return codec::write_b((char*)&val, sizeof(long));
}

bool encoder::write_ulong(unsigned long val)
{
	val = endian_op(val);
    return codec::write_b((char*)&val, sizeof(unsigned long));
}
        
bool encoder::write_char(char val)
{
	val = endian_op(val);
    return codec::write_b((char*)&val, sizeof(char));
}

bool encoder::write_uchar(unsigned char val)
{
	val = endian_op(val);
    return codec::write_b((char*)&val, sizeof(unsigned char));
}
        
bool encoder::write_short(short val)
{
	val = endian_op(val);
    return codec::write_b((char*)&val, sizeof(short));
}
        
bool encoder::write_ushort(unsigned short val)
{
	val = endian_op(val);
    return codec::write_b((char*)&val, sizeof(unsigned short));
}

bool encoder::write_front_int(int val)
{
	val = endian_op(val);
    return codec::write_front_b((char*)&val, sizeof(int));
}
        
bool encoder::write_front_uint(unsigned int val)
{
	val = endian_op(val);
    return codec::write_front_b((char*)&val, sizeof(unsigned int));
}

bool encoder::write_front_char(char val)
{
	val = endian_op(val);
    return codec::write_front_b((char*)&val, sizeof(char));
}

bool encoder::write_front_uchar(unsigned char val)
{
	val = endian_op(val);
    return codec::write_front_b((char*)&val, sizeof(unsigned char));
}
        
bool encoder::write_cstring(const char* cstr)
{
    int len = (int)strlen(cstr) ;
    write_int(len + 1) ;
    return codec::write_b(cstr, len) && codec::write_zero_b();
}
        
bool encoder::write_string(const std::string& str)
{
    /* same way with c-string */
    return write_cstring(str.c_str());
}
        
bool encoder::write_binary(const char* inbuf, int len)
{
    write_int(len) ;
    return codec::write_b(inbuf, len) ;
}

bool encoder::write_body(const char* inbuf, uint32_t len)
{
	return codec::write_b(inbuf, len);
}

bool encoder::copy(const void* inbuf, int len)
{
    return codec::copy_b(inbuf, len);
}
        
void encoder::begin(uint16_t cmd)
{
    codec::begin_b(cmd);
}

void encoder::begin(uint16_t cmd, uint32_t streamid)
{
	codec::begin_b(cmd, streamid);
}

void encoder::end()
{
    codec::end_b();
}


