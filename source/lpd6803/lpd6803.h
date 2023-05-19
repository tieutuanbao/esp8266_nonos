#ifndef __LPD6803_H__
#define __LPD6803_H__

#include <stdint.h>
#include "color.h"
#include "port_macros.h"

__attribute__((section(".text"))) void lpd6803_update(Color_RGB_t *color_buf, uint16_t len, uint16_t full_len);

#endif