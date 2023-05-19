#include "fs.h"
/**
 * Number of file descriptors opened at the same time
 */
#define ESP_SPIFFS_FD_NUMBER       10
#define ESP_SPIFFS_CACHE_PAGES     10

typedef struct {
    void *buf;
    uint32_t size;
} fs_buf_t;

spiffs fs;

static spiffs_config config = {0};
static fs_buf_t work_buf = {0};
static fs_buf_t fds_buf = {0};
static fs_buf_t cache_buf = {0};
spiffs_DIR fs_dir;


static s32_t esp_spiffs_read(u32_t addr, u32_t size, u8_t *dst) {
    if (spi_read_align_byte(addr, dst, size) != true) {
        BITS_LOGE("%s:%d Read spi flash err!!\r\n", __FILE__, __LINE__);
        return SPIFFS_ERR_INTERNAL;
    }

    return SPIFFS_OK;
}

static s32_t esp_spiffs_write(u32_t addr, u32_t size, u8_t *src) {
    if (spi_write_align_byte(addr, src, size) != true) {
        BITS_LOGE("%s:%d Write spi flash err!!\r\n", __FILE__, __LINE__);
        return SPIFFS_ERR_INTERNAL;
    }

    return SPIFFS_OK;
}

static s32_t esp_spiffs_erase(u32_t addr, u32_t size)
{
    uint32_t sectors = size / SPI_FLASH_SEC_SIZE;

    for (uint32_t i = 0; i < sectors; i++) {
        if (spi_erase_sector(addr + (SPI_FLASH_SEC_SIZE * i)) != true) {
            BITS_LOGE("%s:%d Erase spi flash err!!\r\n", __FILE__, __LINE__);
            return SPIFFS_ERR_INTERNAL;
        }
    }

    return SPIFFS_OK;
}


/**
 * @brief Khởi tạo Filesystem
 * 
 */
#if SPIFFS_SINGLETON == 1
int32_t fbegin() {
#else
/**
 * @brief Khởi tạo Filesystem
 * 
 * @param addr Địa chỉ trong flash
 * @param size Kích thước FileSystem
 */
int32_t fbegin(uint32_t addr, uint32_t size) {
    config.phys_addr = addr;
    config.phys_size = size;

    config.phys_erase_block = SPIFFS_ESP_ERASE_SIZE;
    config.log_page_size = SPIFFS_LOG_PAGE_SIZE;
    config.log_block_size = SPIFFS_LOG_BLOCK_SIZE;

    // Initialize fs.cfg so the following helper functions work correctly
    memcpy(&fs.cfg, &config, sizeof(spiffs_config));
#endif
    /**
     * @brief 1 - Config FS
     */
    work_buf.size = 2 * SPIFFS_LOG_PAGE_SIZE;
    fds_buf.size = SPIFFS_buffer_bytes_for_filedescs(&fs, ESP_SPIFFS_FD_NUMBER);
    cache_buf.size= SPIFFS_buffer_bytes_for_cache(&fs, ESP_SPIFFS_CACHE_PAGES);

    work_buf.buf = malloc(work_buf.size);
    fds_buf.buf = malloc(fds_buf.size);
    cache_buf.buf = malloc(cache_buf.size);

    config.hal_read_f = esp_spiffs_read;
    config.hal_write_f = esp_spiffs_write;
    config.hal_erase_f = esp_spiffs_erase;

    config.fh_ix_offset = SPIFFS_FILEHDL_OFFSET;
    /**
     * @brief 2 - Mount FS 
     */
    printf("SPIFFS memory, work_buf_size=%d, fds_buf_size=%d, cache_buf_size=%d\n",
            work_buf.size, fds_buf.size, cache_buf.size);

    int32_t err = SPIFFS_mount(&fs, &config, (uint8_t*)work_buf.buf,
            (uint8_t*)fds_buf.buf, fds_buf.size,
            cache_buf.buf, cache_buf.size, 0);

    if (err != SPIFFS_OK) {
        printf("Error spiffs mount: %d\n", err);
    }
    /**
     * @brief 2.1 - Nếu mount lỗi thì unmount và format SPIFFS sau đó remount
     */
    if(err == SPIFFS_ERR_NOT_A_FS) {
        printf("Not a SPIFFS\r\n");
        SPIFFS_unmount(&fs);
        printf("Formatting SPIFFS ...\r\n");
        SPIFFS_format(&fs);
        printf("Re-mount SPIFFS ...\r\n");
        err = SPIFFS_mount(&fs, &config, (uint8_t*)work_buf.buf,
                            (uint8_t*)fds_buf.buf, fds_buf.size,
                            cache_buf.buf, cache_buf.size, 0);
        if (err != SPIFFS_OK) {
            printf("Error spiffs re-mount: %d\n", err);
            return err;
        }
        return err;
    }
    // SPIFFS_opendir(&fs, "/", &fs_dir);
    return err;
}

int16_t fopen(char *path, const char *type) {
    u16_t flag_fs = SPIFFS_RDONLY;
    if(strcmp(type, "r") == 0) {
        flag_fs = SPIFFS_RDONLY;
    }
    else if(strcmp(type, "w") == 0) {
        flag_fs = SPIFFS_WRONLY;
    }
    if(strcmp(type, "rw") == 0) {
        flag_fs = SPIFFS_RDWR;
    }
    else if(strcmp(type, "a") == 0) {
        flag_fs = SPIFFS_APPEND;
    }
    else if(strcmp(type, "r+") == 0) {
        flag_fs = SPIFFS_RDWR;
    }
    else if(strcmp(type, "w+") == 0) {
        flag_fs = SPIFFS_RDWR;
    }
    else if(strcmp(type, "a+") == 0) {
        flag_fs = SPIFFS_APPEND;
    }
    return SPIFFS_open(&fs, path, flag_fs, 0);
}

/**
 * @brief Đóng FileSystem
 * 
 * @param fd FileDescriptor
 * @return 
 */
int32_t fclose(int16_t fd) {
    return SPIFFS_close(&fs, fd);
}

int32_t fseek(int16_t fd, int32_t offs, int whence) {
    return SPIFFS_lseek(&fs, fd, offs, whence);
}

uint32_t fsize(int16_t fd) {
    spiffs_stat f_stat;
    int32_t rc = SPIFFS_fstat(&fs, fd, &f_stat);
    if (rc != SPIFFS_OK) {
        BITS_LOGE("fsize rc=%d\r\n", rc);
        memset(&f_stat, 0, sizeof(f_stat));
    }
    return f_stat.size;
}

uint32_t fposition(int16_t fd) {
    return SPIFFS_lseek(&fs, fd, 0, SPIFFS_SEEK_CUR);
}

uint32_t favailable(int16_t fd) {
    spiffs_stat f_stat;
    int32_t rc = SPIFFS_fstat(&fs, fd, &f_stat);
    if (rc != SPIFFS_OK) {
        BITS_LOGE("favailable rc=%d\r\n", rc);
        memset(&f_stat, 0, sizeof(f_stat));
    }
    int32_t curr_pos = SPIFFS_lseek(&fs, fd, 0, SPIFFS_SEEK_CUR);
    return f_stat.size - curr_pos;
}

int32_t fwrite(int16_t fd, const void *buf, size_t len) {
    return SPIFFS_write(&fs, fd, buf, len);
}

int32_t fread(int16_t fd, char *buf, size_t len) {
    return SPIFFS_read(&fs, fd, buf, len);
}

int32_t fread_until(int16_t fd, char terminator, char *buf, size_t len) {
    int32_t len_read = SPIFFS_read(&fs, fd, buf, len);
    char *detect_term = strchr(buf, terminator);
    if(detect_term) {
        fseek(fd, 0 - (uint32_t)((buf + len_read) - detect_term), SPIFFS_SEEK_CUR);
        return (int32_t)(detect_term - buf);
    }
    return -1;
}