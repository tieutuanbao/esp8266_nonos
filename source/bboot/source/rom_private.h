#ifndef __ROM_PRIVATE_H__
#define __ROM_PRIVATE_H__

#include <stdint.h>
#include <stdbool.h>

#define ROM_MAGIC	   0xe9
#define ROM_MAGIC_NEW1 0xea
#define ROM_MAGIC_NEW2 0x04

typedef void usercode_t(void);
typedef void loader_t(uint32_t);

/**
 * @brief Cấu trúc flags1
 * flags1 :
 *      qio     = 0
 *      qout    = 1
 *      dio     = 2
 *      dout    = 0x0F
 * flash_speed :
 *      40Mhz   = 0
 *      26.7Mhz = 1
 *      20Mhz   = 2
 *      80Mhz   = 0x0F
 * flash_size :
 *      4Mb     = 0
 *      2Mb     = 1
 *      8Mb     = 2
 *      16Mb    = (3 || 5)
 *      32Mb    = (4 || 6)
 *      64Mb    = 8
 *      128Mb   = 9
 */

typedef struct {
	uint8_t magic;
	uint8_t count;
	uint8_t flags1;
	struct {
        uint8_t flash_speed : 4;
        uint8_t flash_size  : 4;
    } flags2;
	usercode_t* entry;
} rom_header_t;

typedef struct {
	uint8_t* address;
	uint32_t length;
} section_header_t;

typedef struct {
	uint8_t magic;
	uint8_t count; // second magic for new header
	uint8_t flags1;
	uint8_t flags2;
	uint32_t entry;
	// new type rom, lib header
	uint32_t add; // zero
	uint32_t len; // length of irom section
} rom_header_new_t;

/* Các hàm hỗ trợ trong BROM ESP */
extern uint32_t SPIRead(uint32_t addr, void *outptr, uint32_t len);
extern uint32_t SPIEraseSector(int);
extern uint32_t SPIWrite(uint32_t addr, void *inptr, uint32_t len);
extern void ets_printf(char*, ...);
extern void ets_delay_us(int);
extern void ets_memset(void*, uint8_t, uint32_t);
extern void ets_memcpy(void*, const void*, uint32_t);

#endif