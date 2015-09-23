//
//  Created by huanghaifeng on 15/9/22.
//  Copyright (c) 2015 wonghoifung. All rights reserved.
//

#ifndef STREAMBLOCK_HEADER
#define STREAMBLOCK_HEADER

#include "decoder.h"
#include "encoder.h"

int recv_block(int sockfd, decoder& dpack);
int send_block(int sockfd, encoder& epack);

#endif


