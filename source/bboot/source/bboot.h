#ifndef __BBOOT_H__
#define __BBOOT_H__

#include <stdint.h>
#include <stdbool.h>

#define BBOOT_MAGIC		0xef

/* Bỏ comment này để cho phép kích thước file rom > 512 */
// #define BIG_FLASH_ROM

#define BOOT_PARAM_PARTITION  	0x001000
#define APP_MAIN_PARTITION  	0x002000
#define APP_NEW_PARTITION   	0x082000

typedef struct {
	uint8_t magic;
	uint8_t version;
	uint8_t reserve;
	bool new_rom;
	uint32_t rom_size;
} bboot_data_t;

#endif