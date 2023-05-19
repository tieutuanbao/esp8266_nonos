#include "firework.h"
#include "ezform.h"
#include "color.h"



/**
 * @brief Các chế độ chạy trong chương trình 
 */
typedef enum {
    app_run_mode,
    app_setup_mode
} app_mode_t;

/**
 * @brief Danh sách LED hỗ trợ trong chương trình
 */
const char *led_type_string[] = {
#define XX(id, name) #name,
    LED_MAP(XX)
#undef XX
};

rgb_color_t color_buf[MAX_CHIP_PARA][MAX_CHIP_SERI];

/**
 * @brief Danh sách control trong chương trình
 */
application_t app_firework;
form_t form_run;
form_t form_set_type_led;
form_t form_set_cot;
form_t form_set_tia;
form_t form_set_td_cot;
form_t form_set_td_tia;
single_led_t led_tia;
single_led_t led_cot;
single_led_t led_run;
single_led_t led_tia;
single_led_t led_cot;
io_key_t key_set;
io_key_t key_up;
io_key_t key_down;
stimer_t stimer1;


firework_t firework = {.buf_color = (&color_buf[0][0])};
firework_param_t firework_param;
/**
 * @brief ----------> Function prototype <----------
 * 
 */
ICACHE_FLASH_ATTR void firework_form_run_init();
ICACHE_FLASH_ATTR void firework_form_run_run();
ICACHE_FLASH_ATTR void firework_form_set_type_led_run();
ICACHE_FLASH_ATTR void firework_form_set_cot_run();
ICACHE_FLASH_ATTR void firework_form_set_tia_run();
ICACHE_FLASH_ATTR void firework_form_set_td_cot_run();
ICACHE_FLASH_ATTR void firework_form_set_td_tia_run();

/**
 * @brief ----------> Các hàm cho người dùng gọi <----------
 * 
 */
ICACHE_FLASH_ATTR void firework_init(
    void (*firework_fire_callback)(), void (*firework_expl_callback)(),
    bool (*key_set_get)(), bool (*key_up_get)(), bool (*key_down_get)(),  \
    void (*led_run_out)(bool), void (*led_tia_out)(bool), void (*led_cot_out)(bool)
) {
    firework.fire.init_cb = firework_fire_callback;
    firework.explode.init_cb = firework_expl_callback;
    /**
     * @brief Khởi tạo các control dùng trong chương trình 
     */
    form_run = (form_t){firework_form_run_init, firework_form_run_run, 0};
    form_set_type_led = (form_t){0, firework_form_run_run, 0};
    form_set_cot = (form_t){0, firework_form_run_run, 0};
    form_set_tia = (form_t){0, firework_form_run_run, 0};
    form_set_td_cot = (form_t){0, firework_form_run_run, 0};
    form_set_td_tia = (form_t){0, firework_form_run_run, 0};
    key_set.get_signal_input = key_set_get;
    key_up.get_signal_input = key_up_get;
    key_down.get_signal_input = key_down_get;
    led_run.drv_output = led_run_out;
    led_tia.drv_output = led_tia_out;
    led_cot.drv_output = led_cot_out;
    stimer1 = (stimer_t){.counter = 0, .compare = 0, .enable = false, .repeat = false, .callback = 0, .arg = 0};
    /**
     * @brief Thêm các control dùng trong chương trình 
     */
    app_firework.form_list = application_add_control(app_firework.form_list, &form_run);
    app_firework.form_list = application_add_control(app_firework.form_list, &form_set_type_led);
    app_firework.form_list = application_add_control(app_firework.form_list, &form_set_cot);
    app_firework.form_list = application_add_control(app_firework.form_list, &form_set_tia);
    app_firework.form_list = application_add_control(app_firework.form_list, &form_set_td_cot);
    app_firework.form_list = application_add_control(app_firework.form_list, &form_set_td_tia);
    app_firework.sled_list = application_add_control(app_firework.sled_list, &led_run);
    app_firework.sled_list = application_add_control(app_firework.sled_list, &led_tia);
    app_firework.sled_list = application_add_control(app_firework.sled_list, &led_cot);
    app_firework.key_list = application_add_control(app_firework.key_list, &key_set);
    app_firework.key_list = application_add_control(app_firework.key_list, &key_up);
    app_firework.key_list = application_add_control(app_firework.key_list, &key_down);
    app_firework.stimer_list = application_add_control(app_firework.stimer_list, &stimer1);

    app_firework.form_next = &form_run;      /* Cài đặt form chạy đầu tiên */
}

ICACHE_FLASH_ATTR void firework_run(void) {
    application_run(&app_firework);
}


/**
 * @brief ----------> Các callback timer trong chương trình <----------
 */
ICACHE_FLASH_ATTR void firework_change_mode_cb(void *arg) {
    
}
/**
 * @brief ----------> Các callback key trong chương trình <----------
 */
ICACHE_FLASH_ATTR void firework_key_set_press(void *arg) {
    stimer_start(&stimer1, 3000, firework_change_mode_cb, 0, false);        /* Khởi tạo timer thay đổi chế độ nếu giữ nút hơn 3 Giây */
}
ICACHE_FLASH_ATTR void firework_key_set_release(void *arg) {
    stimer_stop(&stimer1);        /* Dừng timer thay đổi chế độ */    
}
ICACHE_FLASH_ATTR void firework_key_up_press(void *arg) {
}
ICACHE_FLASH_ATTR void firework_key_up_release(void *arg) {
    if(app_firework.form_running == &form_set_type_led) {
        switch(firework_param.led_type) {
            case LED_ID_1903: {
                firework_param.led_type = LED_ID_6803;
                break;
            }
            case LED_ID_6803: {
                firework_param.led_type = LED_ID_1903;
                break;
            }
            default:{
                break;
            }
        }
    }
    else if(app_firework.form_running == &form_set_cot) {
        if(firework_param.so_cot < FIREWORK_MAX_LED_COT) {
            firework_param.so_cot++;
        }
    }
    else if(app_firework.form_running == &form_set_tia) {
        if(firework_param.so_tia < FIREWORK_MAX_LED_TIA) {
            firework_param.so_tia++;
        }
    }
    else if(app_firework.form_running == &form_set_td_cot) {
        if(firework_param.td_cot < FIREWORK_MAX_SPD_COT) {
            firework_param.td_cot++;
        }
    }
    else if(app_firework.form_running == &form_set_td_tia) {
        if(firework_param.td_tia < FIREWORK_MAX_SPD_TIA) {
            firework_param.td_tia++;
        }
    }
}
ICACHE_FLASH_ATTR void firework_key_down_press(void *arg) {
}
ICACHE_FLASH_ATTR void firework_key_down_release(void *arg) {
    if(app_firework.form_running == &form_set_type_led) {
        switch(firework_param.led_type) {
            case LED_ID_1903: {
                firework_param.led_type = LED_ID_6803;
                break;
            }
            case LED_ID_6803: {
                firework_param.led_type = LED_ID_1903;
                break;
            }
            default:{
                break;
            }
        }
    }
    else if(app_firework.form_running == &form_set_cot) {
        if(firework_param.so_cot > FIREWORK_MIN_LED_COT) {
            firework_param.so_cot--;
        }
    }
    else if(app_firework.form_running == &form_set_tia) {
        if(firework_param.so_tia > FIREWORK_MIN_LED_TIA) {
            firework_param.so_tia--;
        }
    }
    else if(app_firework.form_running == &form_set_td_cot) {
        if(firework_param.td_cot > FIREWORK_MIN_SPD_COT) {
            firework_param.td_cot--;
        }
    }
    else if(app_firework.form_running == &form_set_td_tia) {
        if(firework_param.td_tia > FIREWORK_MIN_SPD_TIA) {
            firework_param.td_tia--;
        }
    }
}

/**
 * @brief ----------> Các form chạy trong chương trình <----------
 */
ICACHE_FLASH_ATTR void firework_form_run_init() {
    firework.run_state = FIREWORK_STT_SETUP;
}

ICACHE_FLASH_ATTR void firework_form_run_run() {
    /**
     * @brief -------------> Setup hiệu ứng phóng
     */
    if(firework.run_state & FIREWORK_STT_SETUP) {
        firework.fire.strip = (firework_strip_t *) malloc(sizeof(firework_strip_t));        /* 1 - Tạo hiệu dải led cho ứng phóng */
        if(firework.fire.strip) {
            rgb_color_sample_t rand_color = (rand() %  COLOR_IDX_MAX);                      /* 2 - Random chọn màu cho hiệu ứng phóng */
            /* 3 - Màu và độ dài đuôi */
            firework.fire.strip[0].color[0].value.r = sample_color[rand_color].r * 2 / 10;
            firework.fire.strip[0].color[0].value.g = sample_color[rand_color].g * 2 / 10;
            firework.fire.strip[0].color[0].value.b = sample_color[rand_color].b * 2 / 10;
            firework.fire.strip[0].color[0].len = 2;
            /* 4 - Màu và độ dài thân-đuôi */
            firework.fire.strip[0].color[1].value.r = sample_color[rand_color].r * 4 / 10;
            firework.fire.strip[0].color[1].value.g = sample_color[rand_color].g * 4 / 10;
            firework.fire.strip[0].color[1].value.b = sample_color[rand_color].b * 4 / 10;
            firework.fire.strip[0].color[1].len = (rand() % 5) + 3;
            /* 5 - Màu và độ dài đầu-thân */
            firework.fire.strip[0].color[2].value.r = sample_color[rand_color].r * 6 / 10;
            firework.fire.strip[0].color[2].value.g = sample_color[rand_color].g * 6 / 10;
            firework.fire.strip[0].color[2].value.b = sample_color[rand_color].b * 6 / 10;
            firework.fire.strip[0].color[2].len = 1;
            firework.fire.strip[0].color[3].len = 0;            
            /* 6 - Random chọn điểm nổ */
            if(firework_param.so_cot >= (firework.fire.strip[0].color[0].len + firework.fire.strip[0].color[1].len + firework.fire.strip[0].color[2].len)) {
                firework.fire.threshold = firework_param.so_cot - (rand() % (firework.fire.strip[0].color[0].len + firework.fire.strip[0].color[1].len + firework.fire.strip[0].color[2].len));
            }
            else {
                firework.fire.threshold = 0;
            }
            /* 7 - Offset điểm bắn */
            firework.fire.strip[0].pos_draw = 0 - (firework.fire.strip[0].color[0].len + firework.fire.strip[0].color[1].len + firework.fire.strip[0].color[2].len + firework.fire.strip[0].color[3].len);
            /* 8 - Cài đặt tốc độ phóng */
            firework.fire.strip[0].ntick_update = firework_param.td_cot;
            /* 9 - Chuyển sang trạng thái phóng */
            firework.run_state = FIREWORK_STT_RUN_FIRE;
            /* 10 - Cài đặt âm thanh phóng */
            if(firework.fire.init_cb) {
                firework.fire.init_cb();
            }
        }
    }
    /**
     * @brief -------------> Chạy hiệu ứng phóng
     */        
    else if(firework.run_state & FIREWORK_STT_RUN_FIRE) {
        // BITS_LOG(__FILE__":%d BKPT\r\n", __LINE__);
        if((millis() - firework.__time_cot_interval) > firework_param.td_cot) {
            firework.__time_cot_interval = millis();
            /**
             * @brief Thay vì xóa toàn bộ điểm ảnh để in hiệu ứng mới thì xóa điểm ảnh trước đó khi vị trí in > 0 
             */
            if(firework.fire.strip[0].pos_draw > 0) {
                firework.buf_color[firework.fire.strip[0].pos_draw - 1] = (rgb_color_t){0, 0, 0};
            }
            meteor_draw(firework.buf_color, firework_param.so_cot, firework.fire.strip[0].pos_draw, firework.fire.strip[0].color, 3);   /* Vẽ hiệu ứng sao băng */
            firework.fire.strip[0].pos_draw++;                      /* Tăng con trỏ vẽ hiệu ứng */

            if(firework.fire.strip[0].pos_draw > firework_param.so_cot) {  /* Hết điểm ảnh hiển thị */
                if(firework.fire.strip != 0) {
                    free(firework.fire.strip);
                    firework.fire.strip = 0;
                }
                firework.run_state &= ~FIREWORK_STT_RUN_FIRE;     /* Xóa chế độ phóng */
                goto goto_firework_end_fire;
            }
            if( (firework.fire.strip[0].pos_draw == firework.fire.threshold)) {     /* Đến ngưỡng nổ */
            goto_firework_end_fire:
                if((firework.run_state != FIREWORK_STT_SETUP_EXPLODE) && (firework.run_state != FIREWORK_STT_RUN_EXPLODE)) {
                    firework.run_state |= FIREWORK_STT_SETUP_EXPLODE;       /* Kích hoạt trạng thái setup nổ */
                }
            }
        }
    }
    /**
     * @brief -------------> Trạng thái Setup Nổ, tạo kịch b     ản nổ
     * 
     */
    if(firework.run_state & FIREWORK_STT_SETUP_EXPLODE) {
        // BITS_LOG(__FILE__":%d BKPT\r\n", __LINE__);
        if(firework.explode.strip == 0) {     /* Chỉ tại hiệu ứng mới khi số lượng hiệu ứng không có */
            // firework.explode.effect = (rand() % firework_ef_max);   /* 1 - Random chọn hiệu ứng nổ */
            firework.explode.effect = FIREWORK_EFF_METEOR_SLOW_DOWN_NO_DECAY;   /* Chọn hiệu ứng test */
            /**
             * @brief 2 - Setup số lần nổ theo hiệu ứng 
             */
            // switch(dev->explode_eff) {                       
            //     case firework_ef_serial_explode_strobe:{
            //         dev->n_explode = (rand() % 8) + 4;
            //         break;
            //     }
            //     case firework_ef_serial_explode_spread:{
            //         dev->n_explode = (rand() % 8) + 4;
            //         break;
            //     }
            //     default:{
            //         dev->n_explode = (rand() % 3);
            //         break;
            //     }
            // }
            firework.idx_explode = 0;     /* Reset đếm số lần nổ */
            firework.explode.num = 1;     /* Test set số lần nổ */
            // firework.explode.num = (rand() % FIREWORK_EFF_MAX) + 0;     /* Random số lần nổ, tối thiểu 1 lần nổ, tối đa 3 lần nổ */
            firework.explode.strip = (firework_strip_t *)malloc(firework.explode.num * sizeof(firework_strip_t));     /* 3 - Tạo danh sách hiệu ứng nổ mới */
        }
        for(uint8_t idx_eff = 0; idx_eff < firework.explode.num; idx_eff++) {
            if(firework_param.td_tia == 0) {
                firework.explode.strip[idx_eff].ntick_update = FIREWORK_MIN_SPD_TIA;        /* Cài đặt tốc độ nổ */
            }
            else {
                firework.explode.strip[idx_eff].ntick_update = firework_param.td_tia;       /* Cài đặt tốc độ nổ */
            }
            /**
             * @brief Cài đặt kịch bản cho từng hiệu ứng 
             */
            switch(firework.explode.effect) {
                case FIREWORK_EFF_METEOR_SLOW_DOWN_NO_DECAY: {
                    rgb_color_sample_t rand_color = (rand() %  COLOR_IDX_MAX);      /* Random màu hiệu ứng nổ  */
                    /**
                     * @brief Hiệu ứng này không có đuôi phân rã nên color[0].len = 0 
                     */
                    firework.explode.strip[idx_eff].color[0].value.r = sample_color[rand_color].r * 5 / 100;
                    firework.explode.strip[idx_eff].color[0].value.g = sample_color[rand_color].g * 5 / 100;
                    firework.explode.strip[idx_eff].color[0].value.b = sample_color[rand_color].b * 5 / 100;
                    firework.explode.strip[idx_eff].color[0].len = 0;
                    /* Màu và độ dài thân-đuôi */
                    firework.explode.strip[idx_eff].color[1].value.r = sample_color[rand_color].r * 10 / 100;
                    firework.explode.strip[idx_eff].color[1].value.g = sample_color[rand_color].g * 10 / 100;
                    firework.explode.strip[idx_eff].color[1].value.b = sample_color[rand_color].b * 10 / 100;
                    firework.explode.strip[idx_eff].color[1].len = 3;
                    /* Màu và độ dài đầu-thân */
                    firework.explode.strip[idx_eff].color[2].value.r = sample_color[rand_color].r;
                    firework.explode.strip[idx_eff].color[2].value.g = sample_color[rand_color].g;
                    firework.explode.strip[idx_eff].color[2].value.b = sample_color[rand_color].b;
                    firework.explode.strip[idx_eff].color[2].len = 1;

                    firework.explode.strip[idx_eff].color[3].len = 0;
                    break;
                }
                case FIREWORK_EFF_METEOR_SLOW_DOWN:{
                    break;
                }
                case FIREWORK_EFF_METEOR_NO_DECAY_NO_CONTINUES:{
                    break;
                }
                case FIREWORK_EFF_METEOR_NO_DECAY_CONTINUES:{
                    break;
                }
                case FIREWORK_EFF_SERIAL_EXPLODE_STROBE:{
                    break;
                }
                case FIREWORK_EFF_SERIAL_EXPLODE_SPREAD:{
                    break;
                }
                default:{
                    break;
                }
            }
            firework.explode.strip[idx_eff].pos_draw = 0 - (firework.explode.strip[idx_eff].color[0].len + firework.explode.strip[idx_eff].color[1].len + firework.explode.strip[idx_eff].color[2].len + firework.explode.strip[idx_eff].color[3].len);
        }
        /**
         * @brief Chuyển trạng thái nổ
         */
        firework.idx_explode = 0;
        firework.run_state &= ~FIREWORK_STT_SETUP_EXPLODE;
        firework.run_state |= FIREWORK_STT_RUN_EXPLODE;
        /* 10 - Cài đặt âm thanh phóng */
        if(firework.explode.init_cb) {
            firework.explode.init_cb();
        }
    }

    /**
     * @brief -------------> Chạy hiệu ứng trạng thái Nổ nếu có
     */
    if(firework.run_state & FIREWORK_STT_RUN_EXPLODE) {
        if(firework.explode.strip) {
            if((millis() - firework.explode.strip[firework.idx_explode].tick_interval) >= firework.explode.strip[firework.idx_explode].ntick_update) {
                firework.explode.strip[firework.idx_explode].tick_interval = millis();
                switch(firework.explode.effect) {
                    case FIREWORK_EFF_METEOR_SLOW_DOWN_NO_DECAY: {
                        if(firework.explode.strip[firework.idx_explode].pos_draw > 0) {     /* Xóa điểm ảnh tàn dư */
                            firework.buf_color[firework_param.so_cot + firework.explode.strip[firework.idx_explode].pos_draw - 1] = (rgb_color_t){0, 0, 0};
                        }
                        meteor_draw(firework.buf_color + firework_param.so_cot, firework_param.so_tia, firework.explode.strip[firework.idx_explode].pos_draw, firework.explode.strip[firework.idx_explode].color, 4);   /* Vẽ hiệu ứng ra Led */
                        if(firework.explode.strip[firework.idx_explode].pos_draw < firework_param.so_tia) {     /* Còn LED Hiển thị */
                            firework.explode.strip[firework.idx_explode].pos_draw++;
                        }
                        else {      /* Hết LED hiển thị */
                            if(firework.idx_explode < firework.explode.num) {       /* Lấy hiệu ứng khác */
                                firework.idx_explode++;
                            }
                            if(firework.idx_explode >= firework.explode.num) {      /* Hết hiệu ứng */
                                goto firework_end_explode;
                            }
                        }
                        break;
                    }
                    case FIREWORK_EFF_METEOR_SLOW_DOWN: {
                        goto firework_end_explode;
                        break;
                    }
                    default:{
                        goto firework_end_explode;
                        break;
                    }
                }
                firework.explode.strip[firework.idx_explode].ntick_update = firework.explode.strip[firework.idx_explode].ntick_update * 90 / 100;
            }
        }
        else {
        firework_end_explode:
            if(firework.explode.strip) {
                free(firework.explode.strip);
                firework.explode.strip = 0;
                firework.explode.num = 0;
            }
            if((firework.run_state & FIREWORK_STT_RUN_FIRE) == 0) {     /* Nếu đã hết hiệu ứng phóng thì bắt đầu lại từ đầu*/
                firework.run_state = FIREWORK_STT_SETUP;
            }
        }
    }
}

// ICACHE_FLASH_ATTR void firework_form_set_type_led_run();
// ICACHE_FLASH_ATTR void firework_form_set_cot_run();
// ICACHE_FLASH_ATTR void firework_form_set_tia_run();
// ICACHE_FLASH_ATTR void firework_form_set_td_cot_run();
// ICACHE_FLASH_ATTR void firework_form_set_td_tia_run();

ICACHE_FLASH_ATTR void firework_set_num_led_fire(uint16_t n_led) {
    if(n_led < FIREWORK_MIN_LED_COT) {
        n_led = FIREWORK_MIN_LED_COT;
    }
    if(n_led > FIREWORK_MAX_LED_COT) {
        n_led = FIREWORK_MAX_LED_COT;        
    }
    firework_param.so_cot = n_led;
}
ICACHE_FLASH_ATTR void firework_set_num_led_expl(uint16_t n_led) {
    if(n_led < FIREWORK_MIN_LED_TIA) {
        n_led = FIREWORK_MIN_LED_TIA;
    }
    if(n_led > FIREWORK_MAX_LED_TIA) {
        n_led = FIREWORK_MAX_LED_TIA;        
    }
    firework_param.so_tia = n_led;
}
ICACHE_FLASH_ATTR void firework_set_spd_led_fire(uint16_t spd) {
    if(spd < FIREWORK_MIN_SPD_COT) {
        spd = FIREWORK_MIN_SPD_COT;
    }
    if(spd > FIREWORK_MAX_SPD_COT) {
        spd = FIREWORK_MAX_SPD_COT;        
    }
    firework_param.td_cot = spd;
}
ICACHE_FLASH_ATTR void firework_set_spd_led_expl(uint16_t spd) {
    if(spd < FIREWORK_MIN_SPD_TIA) {
        spd = FIREWORK_MIN_SPD_TIA;
    }
    if(spd > FIREWORK_MAX_SPD_TIA) {
        spd = FIREWORK_MAX_SPD_TIA;        
    }
    firework_param.td_tia = spd;
}