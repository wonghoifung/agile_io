//
//  decoder.h
//  console
//
//  Created by huanghaifeng on 14-10-1.
//  Copyright (c) 2014 wonghoifung. All rights reserved.
//

#ifndef decoder_header
#define decoder_header

#include "codec.h"
#include <vector>

class decoder : public codec
{
public:
    int            read_int();
    int            read_int_and_repack();
    unsigned int   read_uint();
	int64_t        read_int64();
	uint64_t       read_uint64();
	long           read_long();
    unsigned long  read_ulong();
    short          read_short();
	unsigned short read_ushort();
	char           read_char();
    unsigned char  read_uchar();
    char*          read_cstring();
    int            read_binary(char* outbuf, int maxlen);
	uint32_t       read_body(char* outbuf, uint32_t maxlen);
            
    bool copy(const void* inbuf, int len);
	void begin(uint16_t cmd);
	void begin(uint16_t cmd, uint32_t streamid);
    void end();
            
    /* for parser to build packet */
    bool append(const char* inbuf, int len)
    { return codec::write_b(inbuf, len); }
};

#endif
