#ifndef __FIRE_WORK_H
#define __FIRE_WORK_H

#include <stdint.h>
#include <math.h>
#include <string.h>

#include "main.h"
#include "meteor.h"

/* -----> Option người dùng tự set <----- */
#define FIREWORK_MAX_LED_COT    256
#define FIREWORK_MIN_LED_COT    1
#define FIREWORK_MAX_SPD_COT    50
#define FIREWORK_MIN_SPD_COT    1
#define FIREWORK_MAX_LED_TIA    128
#define FIREWORK_MIN_LED_TIA    1
#define FIREWORK_MAX_SPD_TIA    FIREWORK_MAX_SPD_COT
#define FIREWORK_MIN_SPD_TIA    FIREWORK_MIN_SPD_COT


#define LED_MAP(XX)     \
        XX(0, 1903)     \
        XX(1, 1904)     \
        XX(2, 6803)     \
        XX(3, MAX_ID)

typedef enum {
#define XX(id, name) LED_##name = id,
    LED_MAP(XX)
#undef XX
} led_type_t;

/**
 * @brief Hiệu ứng nổ
 */
typedef enum {
    FIREWORK_EFF_METEOR_SLOW_DOWN_NO_DECAY   = 0,        // 1 Sao băng dài, không đuôi, chậm dần
    FIREWORK_EFF_METEOR_SLOW_DOWN,                       // 1 Sao băng dài, có đuôi, chậm dần
    FIREWORK_EFF_METEOR_NO_DECAY_NO_CONTINUES,           // nhiều sao băng trên một tia, không đuôi, không đuổi nhau
    FIREWORK_EFF_METEOR_NO_DECAY_CONTINUES,              // nhiều sao băng trên một tia, không đuôi, không đuổi nhau
    FIREWORK_EFF_SERIAL_EXPLODE_STROBE,                  // Nổ liên tục nhấp nháy điểm cuối và giữa
    FIREWORK_EFF_SERIAL_EXPLODE_SPREAD,                  // Nổ liên tục tỏa điểm
    FIREWORK_EFF_MAX
} firework_effect_explode_t;

typedef enum {
    FIREWORK_STT_STOP           = 0x00,
    FIREWORK_STT_SETUP          = 0x01,
    FIREWORK_STT_RUN_FIRE       = 0x02,
    FIREWORK_STT_SETUP_EXPLODE  = 0x04,
    FIREWORK_STT_RUN_EXPLODE    = 0x08
} firework_state_t;

typedef struct {
    int16_t pos_draw;           /* Vị trí điểm ảnh vẽ hiệu ứng */
    strip_color_t color[4];     /* Dải màu */
    uint8_t ntick_update;       /* Số lượng tick xảy ra sự kiện update */
    uint32_t tick_interval;     /* Dấu thời gian */
} firework_strip_t;

typedef struct {
    led_type_t led_type;
    uint16_t so_cot;
    uint16_t so_tia;
    uint16_t td_cot;
    uint16_t td_tia;
} firework_param_t;


typedef struct {
    /* ***** PUBLIC ***** */
    rgb_color_t *buf_color;
    /* ***** PRIVATE ***** */
    firework_state_t run_state;
    uint32_t __time_cot_interval;
    uint32_t __time_tia_interval;
    uint8_t idx_explode;
    struct {
        void (*init_cb)();
        firework_strip_t *strip;        /* Dải màu hiệu ứng phóng */
        uint16_t threshold;             /* Ngưỡng kích hoạt nổ */
    } fire;
    struct {
        void (*init_cb)();
        /**
         *  Dải màu hiệu ứng nổ, có thể có nhiều hiệu ứng nổ trong 1 lần nổ, 
         *  Ví dụ: Hiệu ứng nổ liên tục tỏa điểm thì mỗi điểm ở tia là một hiệu ứng có tốc độ và màu sắckhác nhau
         */
        firework_strip_t *strip;                /* Dải màu hiệu ứng nổ */
        firework_effect_explode_t effect;       /* Hiệu ứng đang chọn */
        uint8_t num;                            /* Số lần nổ */
    } explode;    
} firework_t;

extern const char *led_type_string[];
extern firework_param_t firework_param;
extern firework_t firework;
extern rgb_color_t color_buf[MAX_CHIP_PARA][MAX_CHIP_SERI];


ICACHE_FLASH_ATTR void firework_init(
    void (*firework_fire_callback)(), void (*firework_expl_callback)(),
    bool (*key_set_get)(), bool (*key_up_get)(), bool (*key_down_get)(),  \
    void (*led_run_out)(bool), void (*led_tia_out)(bool), void (*led_cot_out)(bool)
);
ICACHE_FLASH_ATTR void firework_run(void);

ICACHE_FLASH_ATTR void firework_set_num_led_fire(uint16_t n_led);
ICACHE_FLASH_ATTR void firework_set_num_led_expl(uint16_t n_led);
ICACHE_FLASH_ATTR void firework_set_spd_led_fire(uint16_t spd);
ICACHE_FLASH_ATTR void firework_set_spd_led_expl(uint16_t spd);

#endif /* __FIRE_WORK_H */