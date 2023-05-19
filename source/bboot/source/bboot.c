/**
 * @file bboot.c
 * @author Tieu Tuan Bao (tieutuanbao@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2022-10-13
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "bboot.h"
#include "rom_private.h"
#include "loader.h"

/**
 * @brief Kiểm tra rom mới nếu có
 * Copy Rom mới vào phân vùng Rom chính 
 * 
 * @return usercode_t*  
 */
__attribute__ ((noinline)) uint32_t check_new_rom(uint32_t readpos) {
    uint8_t buffer[4096];
    uint32_t flash_size = 0;
    rom_header_t *header = (rom_header_t *)buffer;
    rom_header_new_t *new_header = (rom_header_new_t *)buffer;
    bboot_data_t *boot_header = (bboot_data_t *)buffer;
    uint32_t load_addr = 0;

    ets_printf("BBoot V1.0 - tieutuanbao@gmail.com\r\n");
    /* Lấy header rom boot */
	if (SPIRead(0x0000, buffer, sizeof(rom_header_t)) != 0) {
		return 0;
	}
    /* kiểm tra kích thước flash */
    if(header->flags2.flash_size == 0) {
        ets_printf("Flash size:   4Mbit\r\n");
        flash_size = 0x80000;
    }
    else if(header->flags2.flash_size == 1) {
        ets_printf("Flash size:   2Mbit\r\n");
        flash_size = 0x40000;
    }
    else if(header->flags2.flash_size == 2) {
        ets_printf("Flash size:   8Mbit\r\n");
        flash_size = 0x100000;
    }
    else if((header->flags2.flash_size == 3) || (header->flags2.flash_size == 5)) {
        ets_printf("Flash size:   16Mbit\r\n");
        flash_size = 0x200000 ;
    }
    else if((header->flags2.flash_size == 4) || (header->flags2.flash_size == 6)) {
        ets_printf("Flash size:   32Mbit\r\n");
        flash_size = 0x400000;
    }
    else if(header->flags2.flash_size == 8) {
        ets_printf("Flash size:   64Mbit\r\n");
        flash_size = 0x800000;
    }
    else if(header->flags2.flash_size == 9) {
        ets_printf("Flash size:   128Mbit\r\n");
        flash_size = 0x1000000;
    }
    /* Lấy thông tin Boot param */
	if (SPIRead(BOOT_PARAM_PARTITION, buffer, sizeof(bboot_data_t)) != 0) {
		return 0;
	}
    ets_printf("BBoot param: magic=%02X vers=%d new=%d size=%d\r\n", boot_header->magic, boot_header->version, boot_header->new_rom, boot_header->rom_size);
    /* Tạo thông tin Boot mới nếu chưa có sẵn */
    if(boot_header->magic != BBOOT_MAGIC) {
        ets_printf("---> Create new BBoot param ...\r\n");
        boot_header->magic = BBOOT_MAGIC;
        boot_header->version = 1;
        boot_header->new_rom = 0;
        boot_header->rom_size = 0;
        /* Xóa phân vùng ROM cũ */
        if (SPIEraseSector(BOOT_PARAM_PARTITION >> 12) != 0) {
            return 0;
        }
        /* Copy Rom mới vào phân vùng ROM cũ */
        if (SPIWrite(BOOT_PARAM_PARTITION, buffer, sizeof(bboot_data_t)) != 0) {
            return 0;
        }
    }
    else if((boot_header->new_rom == 1) && (boot_header->rom_size != 0)) {
        /* Lấy header new ROM */
        if (SPIRead(APP_NEW_PARTITION, buffer, sizeof(rom_header_t)) != 0) {
            return 0;
        }
        /* Kiểm tra new ROM */
        if((header->magic == ROM_MAGIC) || (header->magic == ROM_MAGIC_NEW1) || (header->magic == ROM_MAGIC_NEW2)) {
            ets_printf("---> Updating new ROM ...\r\n");
            // uint32_t byte_remainning = bboot_data->rom_size;
            uint32_t byte_remainning = (512 * 1024) - APP_MAIN_PARTITION;
            uint32_t offset = 0;
            uint32_t size_can_write = 0;
            uint32_t read_addr = 0;
            uint32_t write_addr = 0;
            while(byte_remainning > 0) {
                size_can_write = byte_remainning;
                if(size_can_write > 0x1000) {
                    size_can_write = 0x1000;
                }
                /* Lấy địa chỉ đọc ghi */
                read_addr = APP_NEW_PARTITION + offset;
                write_addr = APP_MAIN_PARTITION + offset;
                /* Đọc ROM mới */
                if (SPIRead(read_addr, buffer, size_can_write) != 0) {
                    return 0;
                }
                if((write_addr & 0xFFF) == 0) {
                    /* Xóa phân vùng ROM cũ */
                    if (SPIEraseSector(write_addr >> 12) != 0) {
                        return 0;
                    }
                }
                /* Copy Rom mới vào phân vùng ROM cũ */
                if (SPIWrite(write_addr, buffer, size_can_write) != 0) {
                    return 0;
                }
                offset += size_can_write;
                byte_remainning -= size_can_write;
            }
            /* Lấy thông tin Boot param */
            if (SPIRead(BOOT_PARAM_PARTITION, buffer, sizeof(bboot_data_t)) != 0) {
                return 0;
            }
            /* Thay đổi thông tin Boot */
            boot_header->new_rom = 0;
            boot_header->rom_size = 0;
            /* Xóa phân vùng ROM cũ */
            if (SPIEraseSector(BOOT_PARAM_PARTITION >> 12) != 0) {
                return 0;
            }
            /* Ghi boot param */
            if (SPIWrite(BOOT_PARAM_PARTITION, buffer, sizeof(bboot_data_t)) != 0) {
                return 0;
            }
        }
    }

    /* Lấy header main rom */
	if (SPIRead(APP_MAIN_PARTITION, buffer, sizeof(rom_header_new_t)) != 0) {
		return 0;
	}
    /* Kiểm tra Rom và lấy địa chỉ Rom cho loader */
    if(header->magic == ROM_MAGIC) {
        load_addr = 0x2000;
    }
    else if((header->magic == ROM_MAGIC_NEW1) || (header->magic == ROM_MAGIC_NEW2)) {
        load_addr = 0x2000 + new_header->len + sizeof(rom_header_new_t);
    }
    else {
        ets_printf("Main ROM error!!!\r\n");
        while(1);
    }
    /* Copy code loader vào Ram */
	ets_memcpy((void*)_text_addr, _text_data, _text_len);
    return load_addr;
}

void call_user_start(void) {
	__asm volatile (
		"mov a15, a0\n"          // Lưu địa chỉ return
		"call0 check_new_rom\n"  // Kiểm tra rom mới
		"mov a0, a15\n"          // Lấy lại địa chỉ return
		"bnez a2, 1f\n"          // ?success
		"ret\n"                  // no, return
		"1:\n"                   // yes...
		"movi a3, entry_addr\n"  // get pointer to entry_addr
		"l32i a3, a3, 0\n"       // get value of entry_addr
		"jx a3\n"                // now jump to it
	);
}
