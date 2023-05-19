/**
 * @file i2s.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-06-14
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef __I2S_H
#define __I2S_H

#include "esp8266_i2s.h"
#include "i2s_dma.h"

#define i2s_write_buffer(p, s)  i2s_dma_write(p, s)


#endif