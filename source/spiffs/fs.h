#ifndef __FS_H
#define __FS_H

#include "spiffs.h"
#include <fcntl.h>

#include "c_types.h"
#include "esp8266_spi.h"

typedef spiffs_file File;
extern spiffs fs;

#if SPIFFS_SINGLETON == 1
int32_t fbegin();
#else
int32_t fbegin(uint32_t addr, uint32_t size);
#endif
int16_t fopen(char *path, const char *type);
int32_t fclose(int16_t fd);
int32_t fseek(int16_t fd, int32_t offs, int whence);
uint32_t fsize(int16_t fd);
uint32_t fposition(int16_t fd);
uint32_t favailable(int16_t fd);
int32_t fread(int16_t fd, char *buf, size_t len);
int32_t fwrite(int16_t fd, const void *buf, size_t len);

#endif