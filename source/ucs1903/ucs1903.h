#ifndef __UCS1903_H
#define __UCS1903_H

#include <stdint.h>
#include "color.h"
#include "port_macros.h"

__attribute__((section(".text"))) void ucs1903_update(Color_RGB_t *color_buf, uint16_t len, uint16_t full_len);

#endif
