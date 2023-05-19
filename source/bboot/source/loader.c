#include "bboot.h"
#include "rom_private.h"

/**
 * @brief Hàm chạy trong phần IRam từ địa chỉ 0x40110000,
 * nhiệm vụ là load section trong Rom vào địa chỉ được chỉ định trong Ram
 * 
 */
__attribute__ ((noinline)) usercode_t* load_rom(uint32_t readpos) {	
	uint8_t sectcount;
	uint8_t *writepos;
	uint32_t remaining;
	usercode_t* usercode;
	
	rom_header_t header;
	section_header_t section;

	/* Lấy header Rom */
	SPIRead(readpos, &header, sizeof(rom_header_t));
	readpos += sizeof(rom_header_t);
	/* Lấy entry */
	usercode = header.entry;	
	/* Copy */
	for (sectcount = header.count; sectcount > 0; sectcount--) {
        /* Đọc header section */
		SPIRead(readpos, &section, sizeof(section_header_t));
		readpos += sizeof(section_header_t);
        /* Lấy địa chỉ và kích thước section */
		writepos = section.address;
		remaining = section.length;
        /* Ghi section vào Ram */		
		while (remaining > 0) {
			uint32_t readlen = (remaining < 0x1000) ? remaining : 0x1000;
			SPIRead(readpos, writepos, readlen);
			readpos += readlen;
			writepos += readlen;
			remaining -= readlen;
		}
	}
	return usercode;
}

void call_user_start(uint32_t readpos) {
	__asm volatile (
		"mov a15, a0\n"     // store return addr, we already splatted a15!
		"call0 load_rom\n"  // load the rom
		"mov a0, a15\n"     // restore return addr
		"jx a2\n"           // now jump to the rom code
	);
}
