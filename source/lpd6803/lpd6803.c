#include "lpd6803.h"
#include <stdio.h>
#include <ets_sys.h>
#include "main.h"

#include "esp8266_gpio.h"
#include "esp8266_interrupt.h"

#define LPD6803_DATA_SET()      (GPIO_6803_DAT_PORT_SET |= GPIO_6803_DAT_PIN_BIT_MASK)
#define LPD6803_DATA_CLR()      (GPIO_6803_DAT_PORT_CLR |= GPIO_6803_DAT_PIN_BIT_MASK)
#define LPD6803_CLK_SET()       (GPIO_6803_CLK_PORT_SET |= GPIO_6803_CLK_PIN_BIT_MASK)
#define LPD6803_CLK_CLR()       (GPIO_6803_CLK_PORT_CLR |= GPIO_6803_CLK_PIN_BIT_MASK)

#define LPD6803_TSETUP_TIMMING      
#define LPD6803_TCLK_HIGH_TIMMING   
#define LPD6803_TCLK_LOW_TIMMING    


__attribute__((section(".text"))) void lpd6803_update(Color_RGB_t *color_buf, uint16_t len, uint16_t full_len) {
    uint8_t idx_bit = 0;
    uint16_t idx_dev_serial = 0;

    /**
     * @brief Vô hiệu hóa tất cả các ngắt và task 
     */
    disable_all_interrupts();
    /* Gửi 32bit start */
    LPD6803_DATA_CLR();
    LPD6803_TSETUP_TIMMING;
    for(idx_bit = 0; idx_bit < 32; idx_bit++) {
        LPD6803_CLK_SET();
        LPD6803_TCLK_HIGH_TIMMING;
        LPD6803_CLK_CLR();
        LPD6803_TCLK_LOW_TIMMING;
    }
    for(idx_dev_serial = 0; idx_dev_serial < len; idx_dev_serial++) {
        /* Gửi 1 bit HIGH */
        LPD6803_DATA_SET();
        LPD6803_TSETUP_TIMMING;
        LPD6803_CLK_SET();
        LPD6803_TCLK_HIGH_TIMMING;
        LPD6803_CLK_CLR();
        LPD6803_TCLK_LOW_TIMMING;
        /* Gửi 5bit màu RED */
        for(idx_bit = 0x10; idx_bit != 0; idx_bit >>= 1) {
            if(color_buf[idx_dev_serial].R & idx_bit) LPD6803_DATA_SET();
            else LPD6803_DATA_CLR();
            LPD6803_TSETUP_TIMMING;
            LPD6803_CLK_SET();
            LPD6803_TCLK_HIGH_TIMMING;
            LPD6803_CLK_CLR();
            LPD6803_TCLK_LOW_TIMMING;
        }
        /* Gửi 5bit màu GREEN */
        for(idx_bit = 0x10; idx_bit != 0; idx_bit >>= 1) {
            if(color_buf[idx_dev_serial].G & idx_bit) LPD6803_DATA_SET();
            else LPD6803_DATA_CLR();
            LPD6803_TSETUP_TIMMING;
            LPD6803_CLK_SET();
            LPD6803_TCLK_HIGH_TIMMING;
            LPD6803_CLK_CLR();
            LPD6803_TCLK_LOW_TIMMING;
        }
        /* Gửi 5bit màu BLUE */
        for(idx_bit = 0x10; idx_bit != 0; idx_bit >>= 1) {
            if(color_buf[idx_dev_serial].B & idx_bit) LPD6803_DATA_SET();
            else LPD6803_DATA_CLR();
            LPD6803_TSETUP_TIMMING;
            LPD6803_CLK_SET();
            LPD6803_TCLK_HIGH_TIMMING;
            LPD6803_CLK_CLR();
            LPD6803_TCLK_LOW_TIMMING;
        }
    }
    LPD6803_DATA_CLR();
    LPD6803_TSETUP_TIMMING;
    /* Gửi n bit tương ứng số led */
    for(idx_dev_serial = 0; idx_dev_serial < len; idx_dev_serial++) {
        LPD6803_CLK_SET();
        LPD6803_TCLK_HIGH_TIMMING;
        LPD6803_CLK_CLR();
        LPD6803_TCLK_LOW_TIMMING;
    }
    /**
     * @brief Cho các ngắt và task hoạt động 
     */
    allow_interrupts();
}



