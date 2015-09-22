//
//  command.h
//  console
//
//  Created by huanghaifeng on 14-10-3.
//  Copyright (c) 2014 wonghoifung. All rights reserved.
//

#ifndef command_header
#define command_header

enum
{
    cmd_reserved = 0x00,
    cmd_frombox_data = 0x01,
	cmd_frombox_heartbeat = 0x02,
	cmd_frombox_greeting_request = 0x03,
	cmd_tobox_close_stream = 0x04,
	cmd_tobox_greeting_response = 0x05,
	cmd_tobox_close_connection = 0x06,
	cmd_cmd2box_create_dataconn = 0x07,

	// internal messages
	cmd_web2cmd_register = 0xF0,
	cmd_web2data_register = 0xF1,
	cmd_data2cmd_register = 0xF2,
	cmd_web2cmd_create_dataconn = 0xF3,
	cmd_cmd2data_close_conns = 0xF4,
	cmd_web2data_new_stream_data = 0xF5,
	cmd_web2data_close_stream = 0xF6,
	cmd_data2web_stream_response = 0xF7,
	cmd_data2web_close_streamsocks = 0xF8,
};
        
enum
{
	flag_normal = 0x00,
	flag_high = 0x01,
	flag_new = 0x02,
	flag_fin = 0x04,
};

#define MAXDATASIZE (8192-16)

#endif
