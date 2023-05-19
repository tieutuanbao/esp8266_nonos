#ifndef __METEOR_H
#define __METEOR_H

#include "strip_led_misc.h"
#include "c_types.h"
#include "common_macros.h"

ICACHE_FLASH_ATTR void meteor_draw(Color_RGB_t *color_buf, uint16_t color_buf_len, int32_t offset_pos, strip_color_t *colors, uint8_t colors_size);

#endif