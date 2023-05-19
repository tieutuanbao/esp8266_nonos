#include <string.h>
#include <stdio.h>
#include "port.h"
#include "strip_led.h"

#define LED_LENGTH  100
#define TAG     "test.c"

rainbow_t rainbow;
meteor_t meteor;
strip_led_t strip_led;

rgb_color_t led_buf[LED_LENGTH];

void main(){
    strip_led = new_strip_led(led_buf, LED_LENGTH);

    meteor = new_meteor(115, 5, 5, effect_dir_right);
    rgb_color_t *meteor_color_1 = meteor_add_new_color(&meteor, (rgb_color_t){255,0,0});
    rgb_color_t *meteor_color_2 = meteor_add_new_color(&meteor, (rgb_color_t){0,255,0});
    rgb_color_t *meteor_color_3 = meteor_add_new_color(&meteor, (rgb_color_t){0,0,255});

    strip_led_add_effect(&strip_led, (void *)&meteor);

    /* Preview */
    {
        for(uint32_t update_interval = 0; update_interval < 115 * 5; update_interval+=5){
            strip_led_update(&strip_led, update_interval);
            for(uint16_t idx_led = 0; idx_led < LED_LENGTH; idx_led++){
                printf("\x1b[38;2;%d;%d;%dm#", led_buf[idx_led].r, led_buf[idx_led].g, led_buf[idx_led].b,' '*21);
            }
            usleep(20000);
            printf("\r");
        }
        printf("\033[0m");
    }
}