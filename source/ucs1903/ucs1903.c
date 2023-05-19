#include "ucs1903.h"
#include <stdio.h>
#include <ets_sys.h>
#include "main.h"

#include "esp8266_gpio.h"
#include "esp8266_interrupt.h"

#define UCS_PORT_SET(mask)      (GPIO_1903_PORT_SET = mask)
#define UCS_PORT_CLR(mask)      (GPIO_1903_PORT_CLR = mask)

#define UCS_PIN_MASK_0      GPIO1903_DI_CH0_BIT_MASK
#define UCS_PIN_MASK_1      GPIO1903_DI_CH1_BIT_MASK

#define UCS_TIMMING_0   asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); \
                        asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
#define UCS_TIMMING_1   asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); \
                        asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); \
                        asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); \
                        asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); \
                        asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); \
                        asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); \
                        asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); \
                        asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); \
                        asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
#define UCS_TIMMING_2   asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); \
                        asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); \
                        asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");


__attribute__((section(".text"))) void ucs1903_update(Color_RGB_t *color_buf, uint16_t len, uint16_t full_len) {
    uint8_t idx_bit = 0;
    uint16_t idx_dev_serial = 0;
    uint32_t color_to_io = 0;

    /**
     * @brief Vô hiệu hóa tất cả các ngắt và task 
     */
    disable_all_interrupts();
    for(idx_dev_serial = 0; idx_dev_serial < len; idx_dev_serial++) {
        for(idx_bit = 0x80; idx_bit != 0; idx_bit>>=1){
            /* Set cả bus lên 1 */
            UCS_PORT_SET(USC_ALL_MASK);
            if((color_buf[idx_dev_serial].R & idx_bit) == 0) color_to_io |= UCS_PIN_MASK_0;
            if((color_buf[1 * full_len + idx_dev_serial].R & idx_bit) == 0) color_to_io |= UCS_PIN_MASK_1;
            UCS_TIMMING_0;            
            /* Đưa các chân bit màu = 0 xuống 0 */
            UCS_PORT_CLR(color_to_io);
            color_to_io = 0;
            UCS_TIMMING_1;
            /* Đưa cả bus xuống 0 */
            UCS_PORT_CLR(USC_ALL_MASK);
            UCS_TIMMING_2;
        }
        for(idx_bit = 0x80; idx_bit != 0; idx_bit>>=1){
            /* Set cả bus lên 1 */
            UCS_PORT_SET(USC_ALL_MASK);
            if((color_buf[idx_dev_serial].G & idx_bit) == 0) color_to_io |= UCS_PIN_MASK_0;
            if((color_buf[1 * full_len + idx_dev_serial].G & idx_bit) == 0) color_to_io |= UCS_PIN_MASK_1;
            UCS_TIMMING_0;
            /* Đưa các chân bit màu = 0 xuống 0 */
            UCS_PORT_CLR(color_to_io);
            color_to_io = 0;
            UCS_TIMMING_1;
            /* Đưa cả bus xuống 0 */
            UCS_PORT_CLR(USC_ALL_MASK);
            UCS_TIMMING_2;
        }
        for(idx_bit = 0x80; idx_bit != 0; idx_bit>>=1){
            /* Set cả bus lên 1 */
            UCS_PORT_SET(USC_ALL_MASK);
           if((color_buf[idx_dev_serial].B & idx_bit) == 0) color_to_io |= UCS_PIN_MASK_0;
            if((color_buf[1 * full_len + idx_dev_serial].B & idx_bit) == 0) color_to_io |= UCS_PIN_MASK_1;
            UCS_TIMMING_0;
            /* Đưa các chân bit màu = 0 xuống 0 */
            UCS_PORT_CLR(color_to_io);
            color_to_io = 0;
            UCS_TIMMING_1;
            /* Đưa cả bus xuống 0 */
            UCS_PORT_CLR(USC_ALL_MASK);
            UCS_TIMMING_2;
        }
    }
    /**
     * @brief Cho các ngắt và task hoạt động 
     */
    allow_interrupts();
}

