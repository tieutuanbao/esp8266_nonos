#include "osapi.h"
#include "mem.h"
#include "eagle_soc.h"
#include "ets_sys.h"
#include "user_interface.h"
#include "common_macros.h"

#include "main.h"
#include "hw_timer.h"

#include "esp8266_gpio.h"
#include "esp8266_spi.h"

#include "rboot/appcode/rboot-api.h"
#include "rboot/rboot.h"
#include "fs.h"
#include "ucs1903.h"
#include "meteor.h"
#include "firework.h"
#include "ezform.h"
#include "http_server.h"
// #include "dns_server.h"

/**
 * @brief -----------------------------------> DEFINE <----------------------------------- 
 */
#define WIFI_AP_SSID            "ESP_FIREWORK"
#define SERVER_LOCAL_PORT       80

/**
 * @brief -----------------------------------> TYPEDEF <----------------------------------- 
 */

/**
 * @brief -----------------------------------> FUNCTION PROTOTYPE <----------------------------------- 
 */
uint32_t get_tick(void);
void hw_timer_tick_ms(void);

void gpio_init(void);
bool user_set_softap_config(char *ssid, char *password, struct ip_info *sv_ip);
static void wifi_scan_done(void *arg, STATUS status);

bool key_set_get_input(void);
bool key_up_get_input(void);
bool key_down_get_input(void);
void led_run_set_output(bool state);
void led_cot_set_output(bool state);
void led_tia_set_output(bool state);

bool index_GET_callback(http_session_t *sess);
bool style_css_GET_callback(http_session_t *sess);
bool logo_png_GET_callback(http_session_t *sess);
bool bg_jpg_GET_callback(http_session_t *sess);
bool param_GET_callback(http_session_t *sess);
bool param_POST_callback(http_session_t *sess);
bool update_GET_callback(http_session_t *sess);
bool update_POST_callback(http_session_t *sess);

void cb_before_fire();
void cb_before_expl();

/**
 * @brief -----------------------------------> VARIABLE <----------------------------------- 
 */
storage_flash_t storage_flash;

uint32_t effect_update_interval = 0;
uint32_t change_color_interval = 0;
uint32_t delay_interval = 0;

struct{
    char ssid[32];
    uint8_t ssid_len;
} ap_scanned[20];

struct softap_config sta;
http_server_t http_server;

// dns_server_t *dns_server;
os_timer_t wifi_connect_network_timeout_timer;

const http_uri_t index_get_handler = {
    .handler = (http_handle_t)index_GET_callback,
    .method = HTTP_GET,
    .uri = "/",
    .file = "/webpanel/index.html"
};

const http_uri_t index_1_get_handler = {
    .handler = (http_handle_t)index_GET_callback,
    .method = HTTP_GET,
    .uri = "/hotspot-detect.html",
    .file = "/webpanel/index.html"
};

const http_uri_t param_get_handler = {
    .handler = (http_handle_t)param_GET_callback,
    .method = HTTP_GET,
    .uri = "/param",
    .file = 0
};
const http_uri_t param_post_handler = {
    .handler = (http_handle_t)param_POST_callback,
    .method = HTTP_POST,
    .uri = "/param",
    .file = 0
};

const http_uri_t style_css_handler = {
    .handler = (http_handle_t)style_css_GET_callback,
    .method = HTTP_GET,
    .uri = "/style.css",
    .file = "/webpanel/style.css"
};

const http_uri_t logo_png_handler = {
    .handler = (http_handle_t)logo_png_GET_callback,
    .method = HTTP_GET,
    .uri = "/logo.png",
    .file = "/webpanel/logo.png"
};

const http_uri_t bg_jpg_handler = {
    .handler = (http_handle_t)bg_jpg_GET_callback,
    .method = HTTP_GET,
    .uri = "/bg.jpg",
    .file = "/webpanel/bg.jpg"
};

const http_uri_t update_get_handler = {
    .handler = (http_handle_t)update_GET_callback,
    .method = HTTP_GET,
    .uri = "/update",
    .file = "/webpanel/update.html"
};

const http_uri_t update_post_handler = {
    .handler = (http_handle_t)update_POST_callback,
    .method = HTTP_POST,
    .uri = "/update",
    .file = 0
};

/**
 * @brief -----------------------------------> MAIN FUNCTION <----------------------------------- 
 */
void setup() {
    system_print_meminfo();     /* In thông tin hệ thống */

    /**
     * @brief Khởi tạo timer  
     */
    hw_timer_init(FRC1_SOURCE, 1);
    hw_timer_set_func(hw_timer_tick_ms);
    hw_timer_arm(1000);

    /**
     * @brief Khởi tạo GPIO  
     */
    gpio_init();
    /**
     * @brief Khởi tạo file system 
     */
    if (fbegin() != SPIFFS_OK) {
        BITS_LOGE("Error mount SPIFFS\r\n");
    }
    else {
        File fd = fopen("/param.txt", "r");
        if (fd < 0) {
            BITS_LOGE("Error opening file\r\n");
            system_restart();
            return;
        }
        char *temp_buff = (char *)malloc(sizeof(storage_flash_t) + 1), *temp_p = 0;
        if(temp_buff) {
            temp_p = temp_buff;
            fseek(fd, 0, SPIFFS_SEEK_SET);                     // Về đầu file
            fread(fd, temp_p, sizeof(storage_flash_t) + 1);    // Lấy toàn bộ dữ liệu
            /* Lấy SSID */
            strcpy(storage_flash.ssid, temp_p);
            temp_p += strlen(storage_flash.ssid) + 1;
            /* Lấy Password */
            strcpy(storage_flash.password, temp_p);
            temp_p += strlen(storage_flash.password) + 1;
            /*  */
            DataSaveRam.LedType = (led_type_t)Bits_String_ToInt(temp_p);
            temp_p += strlen(temp_p) + 1;
            DataSaveRam.SizeCot = Bits_String_ToInt(temp_p);
            temp_p += strlen(temp_p) + 1;
            DataSaveRam.SizeTia = Bits_String_ToInt(temp_p);
            temp_p += strlen(temp_p) + 1;
            DataSaveRam.SpeedCot = Bits_String_ToInt(temp_p);
            temp_p += strlen(temp_p) + 1;
            DataSaveRam.SpeedTia = Bits_String_ToInt(temp_p);
            temp_p += strlen(temp_p) + 1;
            
            BITS_LOG("ssid: %s\r\n", storage_flash.ssid);
            BITS_LOG("pass: %s\r\n", storage_flash.password);
            BITS_LOG("led_type: %d\r\n", DataSaveRam.LedType);
            BITS_LOG("so_cot: %d\r\n", DataSaveRam.SizeCot);
            BITS_LOG("so_tia: %d\r\n", DataSaveRam.SizeTia);
            BITS_LOG("td_cot: %d\r\n", DataSaveRam.SpeedCot);
            BITS_LOG("td_tia: %d\r\n", DataSaveRam.SpeedTia);
            free(temp_buff);
        }
        fclose(fd);
    }    
    /**
     * @brief Cấu hình Access Point
     */    
    static struct ip_info sv_ip;
    IP4_ADDR(&sv_ip.gw, 8,8,8,8);
    IP4_ADDR(&sv_ip.ip, 8,8,8,8);
    IP4_ADDR(&sv_ip.netmask, 255,255,255,0);
    if(user_set_softap_config("SOLANTECH: FIREWORK_ESP\0", 0, &sv_ip)) {
        /* Cấu hình DNS server cổng 53 */
        // dns_server = dns_server_init(53, "*", sv_ip.ip);
    }

    /* Scan wifi */
    wifi_set_opmode(STATIONAP_MODE);
    wifi_station_scan(NULL, wifi_scan_done);

    /* Check ip local */
    struct ip_info ipconfig;
    wifi_get_ip_info(SOFTAP_IF, &ipconfig);
    /**
     * @brief Cấu hình pháo hoa 
     */
    FIREWORK_Init(
        key_set_get_input, key_up_get_input, key_down_get_input, 
        led_run_set_output, led_cot_set_output, led_tia_set_output
    );
    /**
     * @brief Tạo server HTTP 
     */
    http_server_init(&http_server, ipconfig.ip, 80, 15);
    /**
     * @brief Đăng ký response cho server
     */
    http_server_register_uri(&http_server, (http_uri_t *)&index_get_handler);
    http_server_register_uri(&http_server, (http_uri_t *)&index_1_get_handler);
    http_server_register_uri(&http_server, (http_uri_t *)&param_get_handler);
    http_server_register_uri(&http_server, (http_uri_t *)&param_post_handler);
    http_server_register_uri(&http_server, (http_uri_t *)&update_get_handler);
    http_server_register_uri(&http_server, (http_uri_t *)&update_post_handler);
    http_server_register_uri(&http_server, (http_uri_t *)&style_css_handler);
    http_server_register_uri(&http_server, (http_uri_t *)&logo_png_handler);
    http_server_register_uri(&http_server, (http_uri_t *)&bg_jpg_handler);
}

uint8_t state = 0;
void loop(os_event_t *events) {
    /* BEGIN IMPORTANT */
    system_os_post(LOOP_TASK_PRIORITY, 0, 0);
    /* END IMPORTANT */
    firework_run();
}

/**
 * @brief -----------------------------------> USER CODE <----------------------------------- 
 */
uint32_t get_tick(void) {
    uint32_t tick;
    asm("rsr %0, ccount" : "=r" (tick));
    return tick;
}

__attribute__((section(".text"))) void hw_timer_tick_ms(void) {
    __tick_ms_counter++;
    single_led_handler(app_firework.sled_list);
}

/**
 * @brief Cấu hình softAP
 * 
 * @param ssid 
 * @param password 
 */
bool user_set_softap_config(char *ssid, char *password, struct ip_info *sv_ip) {
    struct softap_config stationConf;
    if((wifi_get_opmode() & SOFTAP_MODE) == 0){
        if(wifi_set_opmode(SOFTAP_MODE) == false){
            return false;
        }
    }

    /**
     * @brief Set SSID và Password 
     */
    wifi_softap_get_config(&stationConf);
    memset(stationConf.ssid, 0, sizeof(stationConf.ssid));
    memset(stationConf.password, 0, sizeof(stationConf.password));
    memcpy(stationConf.ssid, ssid, strlen(ssid));
    stationConf.ssid_len = strlen(ssid);
    stationConf.ssid_hidden = 0;
    stationConf.max_connection = 4;
    stationConf.authmode = AUTH_OPEN;
    stationConf.channel = 2;
    if(password) {
        memcpy(stationConf.password, password, strlen(password));
        stationConf.authmode = AUTH_WPA2_PSK;
        printf("ap pass: %s\r\n", stationConf.password);
    }
    printf("ap ssid: %s\r\n", stationConf.ssid);
    wifi_softap_set_config(&stationConf);

    wifi_softap_dhcps_stop();       /* Stop DHCP */

    /**
     * @brief Set IP và Gateway
     */
    wifi_set_ip_info(SOFTAP_IF, sv_ip);

    wifi_softap_set_dhcps_lease_time(720);

    wifi_softap_dhcps_start();      /* Start DHCP */

    return true;
}

/**
 * @brief Khởi tạo GPIO
 * 
 * @return void 
 */
void gpio_init(void) {
    gpio_config_t ucs_io_cfg;
    ucs_io_cfg.dir = GPIO_DIR_OUTPUT; 
    ucs_io_cfg.intr_type = GPIO_INT_DISABLE; 
    ucs_io_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    ucs_io_cfg.pull_up_en = GPIO_PULLUP_DISABLE;
    ucs_io_cfg.pin_bit_mask = USC_ALL_MASK|GPIO_LED_SET_BIT_MASK|GPIO_LED_COT_BIT_MASK|GPIO_LED_TIA_BIT_MASK;
    gpio_config(&ucs_io_cfg);

    ucs_io_cfg.dir = GPIO_DIR_INPUT; 
    ucs_io_cfg.intr_type = GPIO_INT_DISABLE; 
    ucs_io_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    ucs_io_cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    ucs_io_cfg.pin_bit_mask = GPIO_BUTTON_SET_BIT_MASK|GPIO_BUTTON_UP_BIT_MASK;
    gpio_config(&ucs_io_cfg);

    ucs_io_cfg.dir = GPIO_DIR_INPUT; 
    ucs_io_cfg.intr_type = GPIO_INT_DISABLE; 
    ucs_io_cfg.pull_down_en = GPIO_PULLDOWN_ENABLE;
    ucs_io_cfg.pull_up_en = GPIO_PULLUP_DISABLE;
    ucs_io_cfg.pin_bit_mask = GPIO_BUTTON_DOWN_BIT_MASK;
    gpio_config(&ucs_io_cfg);
}
static void wifi_scan_done(void *arg, STATUS status) {
    int ret, idx_ap = 0;
    if (status == OK) {
        struct bss_info *bss_link = (struct bss_info *)arg;
        while (bss_link != NULL) {
            memcpy(ap_scanned[idx_ap].ssid, bss_link->ssid, bss_link->ssid_len);
            ap_scanned[idx_ap].ssid[bss_link->ssid_len] = 0;
            ap_scanned[idx_ap].ssid_len = bss_link->ssid_len;
            idx_ap++;
            if(idx_ap >= 20) break;
            else {
                ap_scanned[idx_ap].ssid_len = 0;
            }
            bss_link = bss_link->next.stqe_next;
        }
    } else {
        BITS_LOGE("err, scan status %d\n", status);
    }

}

/**
 * @brief -----------------------------------> Các hàm callback dùng trong App firework
 */
bool key_set_get_input(void) {
    if(GPIO->in & GPIO_BUTTON_SET_BIT_MASK){
        return false;
    }
    else {
        return true;
    }
}
bool key_up_get_input(void) {
    if(GPIO->in & GPIO_BUTTON_UP_BIT_MASK) return false;
    else return true;
}
bool key_down_get_input(void) {
    if(GPIO->in & GPIO_BUTTON_DOWN_BIT_MASK) return true;
    else return false;    
}
void led_run_set_output(bool state) {
    if(state) {
        GPIO->out |= GPIO_LED_SET_BIT_MASK;
        return;
    }
    GPIO->out &= ~GPIO_LED_SET_BIT_MASK;
}
void led_cot_set_output(bool state) {
    if(state) {
        GPIO->out |= GPIO_LED_COT_BIT_MASK;
        return;
    }
    GPIO->out &= ~GPIO_LED_COT_BIT_MASK;
}
void led_tia_set_output(bool state) {
    if(state) {
        GPIO->out |= GPIO_LED_TIA_BIT_MASK;
        return;
    }
    GPIO->out &= ~GPIO_LED_TIA_BIT_MASK;
}

bool index_GET_callback(http_session_t *sess) {
    wifi_station_scan(NULL, wifi_scan_done);

    size_t size_available = http_server_get_available_mem(sess);
    
    BITS_LOGD(__FILE__":%d BKPT\r\n", __LINE__);
    if(size_available >= 118) {
        char *http_header_buf = malloc(130);
        memset(http_header_buf, 0, 130);
        if(http_header_buf) {
            sprintf(http_header_buf, "HTTP/1.1 200 OK\r\n"
                                    "Content-Type: text/html; charset=UTF-8\r\n"
                                    "User-Agent: BitsCat\r\n"
                                    "Connection: close\r\n"
                                    "Content-Length: %d\r\n\r\n", fsize(sess->response.fd));

            http_server_send(sess,  http_header_buf, strlen(http_header_buf));
            free(http_header_buf);
        }
    }    
}
bool param_GET_callback(http_session_t *sess) {
    if(sess->response.content == 0) {
        /* Cấp phát động nội dung Response */
        sess->response.content = malloc(1024);
        sess->response.content_len = 0;
        sess->response.content_pos = 0;        
        /* Tạo nội dung Response */
        if(sess->response.content != 0) {
            memset(sess->response.content, 0, 1024);
            /* --- Ghi 180 Byte đầu để chỗ cho Header --- */
            memset(sess->response.content, '0', 180);
            /* --- Mở đầu json --- */
            strncat(sess->response.content, "{", 1);
            /* Wifi list */
            strncat(sess->response.content, "\"wifiap\":[", 10);
            for(uint8_t idx_ap = 0; (idx_ap < 20) && (ap_scanned[idx_ap].ssid_len != 0); idx_ap++) {
                if(idx_ap > 0) {
                    strncat(sess->response.content, ",", 1);
                }
                strncat(sess->response.content, "\"", 1);
                strncat(sess->response.content, ap_scanned[idx_ap].ssid, strlen(ap_scanned[idx_ap].ssid));
                strncat(sess->response.content, "\"", 1);
            }
            strncat(sess->response.content, "],", 2);
            /* Password đang dùng */
            strncat(sess->response.content, "\"pass\":\"", 8);
            strncat(sess->response.content, storage_flash.password, strlen(storage_flash.password));
            strncat(sess->response.content, "\",", 2);
            /* Danh sách LED đang hỗ trợ */
            strncat(sess->response.content, "\"led_t\":[", 9);
            for(uint8_t idx_led = 0; idx_led < LED_IDX_MAX; idx_led++) {
                if(idx_led > 0) {
                    strncat(sess->response.content, ",", 1);
                }
                strncat(sess->response.content, "\"", 1);
                strncat(sess->response.content, led_type_string[idx_led], strlen(led_type_string[idx_led]));
                strncat(sess->response.content, "\"", 1);
            }
            strncat(sess->response.content, "],", 2);
            /* Max số LED cột */
            strncat(sess->response.content, "\"maxncot\":", 10);
            sprintf(sess->response.content + strlen(sess->response.content), "%d", FIREWORK_MAX_LED_COT);
            strncat(sess->response.content, ",", 1);
            /* Max số LED tia */
            strncat(sess->response.content, "\"maxntia\":", 10);
            sprintf(sess->response.content + strlen(sess->response.content), "%d", FIREWORK_MAX_LED_TIA);
            strncat(sess->response.content, ",", 1);
            /* Max tốc độ LED cột */
            strncat(sess->response.content, "\"maxscot\":", 10);
            sprintf(sess->response.content + strlen(sess->response.content), "%d", FIREWORK_MAX_SPD_COT);
            strncat(sess->response.content, ",", 1);
            /* Max tốc độ LED tia */
            strncat(sess->response.content, "\"maxstia\":", 10);
            sprintf(sess->response.content + strlen(sess->response.content), "%d", FIREWORK_MAX_SPD_TIA);
            strncat(sess->response.content, ",", 1);
            /* số LED cột */
            strncat(sess->response.content, "\"ncot\":", 7);
            sprintf(sess->response.content + strlen(sess->response.content), "%d", DataSaveRam.SizeCot);
            strncat(sess->response.content, ",", 1);
            /* số LED tia */
            strncat(sess->response.content, "\"ntia\":", 7);
            sprintf(sess->response.content + strlen(sess->response.content), "%d", DataSaveRam.SizeTia);
            strncat(sess->response.content, ",", 1);
            /* tốc độ LED cột */
            strncat(sess->response.content, "\"scot\":", 7);
            sprintf(sess->response.content + strlen(sess->response.content), "%d", DataSaveRam.SpeedCot);
            strncat(sess->response.content, ",", 1);
            /* tốc độ LED tia */
            strncat(sess->response.content, "\"stia\":", 7);
            sprintf(sess->response.content + strlen(sess->response.content), "%d", DataSaveRam.SpeedTia);
            /* --- Kết thúc json --- */
            strncat(sess->response.content, "}", 1);
            
            /* Lấy độ dài nội dung Response */
            sess->response.content_len = strlen(sess->response.content)  - 180;
            uint16_t http_header_len = sprintf(sess->response.content,  "HTTP/1.1 200 OK\r\n"
                                                                        "Content-Type: text/html; charset=UTF-8\r\n"
                                                                        "User-Agent: BitsCat\r\n"
                                                                        "Connection: keep-alive\r\n"
                                                                        "Content-Length: %d\r\n"
                                                                        "Keep-Alive: timeout=5, max=10\r\n\r\n"
                        , sess->response.content_len);
            for(uint16_t idx_char_content = 0; idx_char_content < sess->response.content_len; idx_char_content++) {
                sess->response.content[http_header_len + idx_char_content] = sess->response.content[180 + idx_char_content];
            }
            sess->response.content[http_header_len + sess->response.content_len] = 0;
            /* Lấy lại content length */
            sess->response.content_len = strlen(sess->response.content);
            sess->response.content_pos = 0;
        }
    }
    /* --- Gửi dữ liệu HTTP --- */
    if(sess->response.content_pos >= sess->response.content_len) {
        free(sess->response.content);
        sess->response.content = 0;
        sess->response.content_len = 0;
        sess->response.content_pos = 0;
        return true;
    }
    /* Tính số Byte có thể gửi qua TCP */
    size_t size_available = http_server_get_available_mem(sess);
    if(size_available > (sess->response.content_len - sess->response.content_pos)) {
        size_available = (sess->response.content_len - sess->response.content_pos);
        http_server_send(sess, sess->response.content + sess->response.content_pos, size_available);
        sess->response.content_pos += size_available;
    }
    return false;
}

bool param_POST_callback(http_session_t *sess) {
    BITS_LOG("Body content [%d]: %s\r\n", sess->request.content_len, sess->request.content);

    char *buf_content = malloc(sess->request.content_len + 1);
    if(buf_content) {
        memcpy(buf_content, sess->request.content, sess->request.content_len);
        buf_content[sess->request.content_len] = 0;

        int16_t temp_val = 0;
        char *pos_cont = strstr(buf_content, "so_cot");
        if(pos_cont != 0) {
            buf_content[6] = 0;
            temp_val = Bits_String_ToInt(pos_cont + 7);
            printf("POST value: %s, %d\r\n", buf_content, temp_val);
            firework_set_num_led_fire(temp_val);
        }
        else if((pos_cont = strstr(buf_content, "so_tia")) != 0){
            buf_content[6] = 0;
            temp_val = Bits_String_ToInt(pos_cont + 7);
            printf("POST value: %s, %d\r\n", buf_content, temp_val);
            firework_set_num_led_expl(temp_val);
        }
        else if((pos_cont = strstr(buf_content, "td_cot")) != 0){
            buf_content[6] = 0;
            temp_val = Bits_String_ToInt(pos_cont + 7);
            printf("POST value: %s, %d\r\n", buf_content, temp_val);
            firework_set_spd_led_fire(temp_val);
        }
        else if((pos_cont = strstr(buf_content, "td_tia")) != 0){
            buf_content[6] = 0;
            temp_val = Bits_String_ToInt(pos_cont + 7);
            printf("POST value: %s, %d\r\n", buf_content, temp_val);
            firework_set_spd_led_expl(temp_val);
        }
        free(buf_content);
    }
    /* Gửi response */
    size_t size_available = http_server_get_available_mem(sess);    
    if(size_available >= 140) {
        char *http_header_buf = malloc(140);
        memset(http_header_buf, 0, 140);
        if(http_header_buf) {
            strcpy(http_header_buf, "HTTP/1.1 200 OK\r\n"
                                    "Content-Type: text/html\r\n"
                                    "User-Agent: BitsCat\r\n"
                                    "Connection: keep-alive\r\n"
                                    "Content-Length: 0\r\n"
                                    "Keep-Alive: timeout=5, max=10\r\n\r\n");

            http_server_send(sess,  http_header_buf, strlen(http_header_buf));
            free(http_header_buf);
            return true;
        }
    }
    return false;
}
bool style_css_GET_callback(http_session_t *sess) {
    size_t size_available = http_server_get_available_mem(sess);
    
    if(size_available >= 118) {
        char *http_header_buf = malloc(130);
        memset(http_header_buf, 0, 130);
        if(http_header_buf) {
            sprintf(http_header_buf, "HTTP/1.1 200 OK\r\n"
                                    "Content-Type: text/css\r\n"
                                    "User-Agent: BitsCat\r\n"
                                    "Connection: close\r\n"
                                    "Content-Length: %d\r\n\r\n", fsize(sess->response.fd));

            http_server_send(sess,  http_header_buf, strlen(http_header_buf));
            free(http_header_buf);
        }
    }
}
bool logo_png_GET_callback(http_session_t *sess) {
    size_t size_available = http_server_get_available_mem(sess);
    
    if(size_available >= 118) {
        char *http_header_buf = malloc(130);
        memset(http_header_buf, 0, 130);
        if(http_header_buf) {
            sprintf(http_header_buf, "HTTP/1.1 200 OK\r\n"
                                    "Content-Type: image/png\r\n"
                                    "User-Agent: BitsCat\r\n"
                                    "Connection: close\r\n"
                                    "Content-Length: %d\r\n\r\n", fsize(sess->response.fd));

            http_server_send(sess,  http_header_buf, strlen(http_header_buf));
            free(http_header_buf);
        }
    }
}
bool bg_jpg_GET_callback(http_session_t *sess) {
    size_t size_available = http_server_get_available_mem(sess);
    
    if(size_available >= 118) {
        char *http_header_buf = malloc(130);
        memset(http_header_buf, 0, 130);
        if(http_header_buf) {
            sprintf(http_header_buf, "HTTP/1.1 200 OK\r\n"
                                    "Content-Type: image/jpg\r\n"
                                    "User-Agent: BitsCat\r\n"
                                    "Connection: close\r\n"
                                    "Content-Length: %d\r\n\r\n", fsize(sess->response.fd));

            http_server_send(sess,  http_header_buf, strlen(http_header_buf));
            free(http_header_buf);
        }
    }
}
bool update_GET_callback(http_session_t *sess) {
    size_t size_available = http_server_get_available_mem(sess);
    
    BITS_LOGD(__FILE__":%d BKPT\r\n", __LINE__);
    if(size_available >= 118) {
        char *http_header_buf = malloc(130);
        memset(http_header_buf, 0, 130);
        if(http_header_buf) {
            sprintf(http_header_buf, "HTTP/1.1 200 OK\r\n"
                                    "Content-Type: text/html; charset=UTF-8\r\n"
                                    "User-Agent: BitsCat\r\n"
                                    "Connection: close\r\n"
                                    "Content-Length: %d\r\n\r\n", fsize(sess->response.fd));

            http_server_send(sess,  http_header_buf, strlen(http_header_buf));
            free(http_header_buf);
        }
    }   
}
bool update_POST_callback(http_session_t *sess) {
    printf("Current rom: %d\r\n", rboot_get_current_rom());
    printf("--start--\r\n%s\r\n--end--", sess->request.content);
    /* Gửi response */
    size_t size_available = http_server_get_available_mem(sess);    
    if(size_available >= 140) {
        char *http_header_buf = malloc(140);
        memset(http_header_buf, 0, 140);
        if(http_header_buf) {
            strcpy(http_header_buf, "HTTP/1.1 200 OK\r\n"
                                    "Content-Type: text/html\r\n"
                                    "User-Agent: BitsCat\r\n"
                                    "Connection: keep-alive\r\n"
                                    "Content-Length: 0\r\n"
                                    "Keep-Alive: timeout=5, max=10\r\n\r\n");

            http_server_send(sess,  http_header_buf, strlen(http_header_buf));
            free(http_header_buf);
            return true;
        }
    }
}