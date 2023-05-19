#include "osapi.h"
#include "mem.h"
#include "eagle_soc.h"
#include "ets_sys.h"
#include "user_interface.h"
#include "common_macros.h"

#include "main.h"
#include "hw_timer.h"
#include "lwip/mdns.h"

#include "esp8266_gpio.h"
#include "esp8266_spi.h"

#include "fs.h"
#include "ucs1903.h"
#include "meteor.h"
#include "firework.h"
#include "ezform.h"
#include "http_server.h"
#include "jsmn.h"
#include "realtime.h"
#include "bboot.h"
#include "web_bin.h"
// #include "dns_server.h"

/**
 * @brief -----------------------------------> DEFINE <----------------------------------- 
 */
#define WIFI_AP_SSID            "ESP_FIREWORK"
#define SERVER_LOCAL_PORT       80

#define APP1_BOOT_ADDR      0x82000
#define APP1_ADDR           0x82000
/**
 * @brief -----------------------------------> TYPEDEF <----------------------------------- 
 */
typedef enum {
    TimeRun_Event_None,
    TimeRun_Event_Sound,
    TimeRun_Event_Effect,
    TimeRun_Event_Both
} TimeRun_Events;

/**
 * @brief State firmware update
 * 
 */
typedef enum {
    FW_UPDATE_START,
    FW_UPDATE_WRITE,
    FW_UPDATE_DONE,
    FW_UPDATE_ERROR
} fw_update_state_t;

typedef struct {
	volatile fw_update_state_t state;
    uint32_t address;
	uint32_t file_size;
    uint32_t byte_written;
} upload_state_t;

typedef struct {
    uint8_t magic;
    uint32_t address;
	uint32_t file_size;
} upload_struct_t;

/**
 * @brief -----------------------------------> FUNCTION PROTOTYPE <----------------------------------- 
 */
uint32_t get_tick(void);
void hw_timer_tick_ms(void);

void gpio_init(void);
void timer_close_AP_callback();
bool user_set_softap_config(char *ssid, char *password, struct ip_info *sv_ip);
static void wifi_scan_done(void *arg, STATUS status);
static void save_state_effect(bool state);
static void save_state_sound(bool state);

bool key_set_get_input(void);
bool key_up_get_input(void);
bool key_down_get_input(void);
void led_run_set_output(bool state);
void led_cot_set_output(bool state);
void led_tia_set_output(bool state);

bool api_GET_callback(http_session_t *sess);
bool api_POST_callback(http_session_t *sess);
bool update_fw_callback(http_session_t *sess);
bool update_fw_init_callback(http_session_t *sess);

void firework_form_run_init(void *arg);
void firework_form_run_run(void *arg);
void firework_form_set_typeled_run(void *arg);
void firework_form_set_tia_init(void *arg);
void firework_form_set_cot_init(void *arg);
void firework_form_set_nled_cot(void *arg);
void firework_form_set_nled_tia(void *arg);
void firework_form_set_sled_cot(void *arg);
void firework_form_set_sled_tia(void *arg);

void firework_key_set_press(void *arg);
void firework_key_set_release(void *arg);
void firework_key_up_press(void *arg);
void firework_key_up_release(void *arg);
void firework_key_down_press(void *arg);
void firework_key_down_release(void *arg);
/**
 * @brief -----------------------------------> VARIABLE <----------------------------------- 
 */
Application_t app_firework;
Form_t form_run;
Form_t form_set_type_led;
Form_t form_set_cot;
Form_t form_set_tia;
Form_t form_set_td_cot;
Form_t form_set_td_tia;
Led_t led_run;
Led_t led_tia;
Led_t led_cot;
IO_Key_t key_set;
IO_Key_t key_up;
IO_Key_t key_down;
STimer_t stimer1;
STimer_t stimer_clear_param;

struct{
    char ssid[32];
    uint8_t ssid_len;
} ap_scanned[20];

struct softap_config ap_conf;
struct station_config station_conf;
static struct ip_info sv_ip;

http_server_t http_server = {
    .default_header = {
        {.key = "User-Agent", .value = "Solantech_BitsCat"},
        {.key = 0, .value = 0},
    }
};
const http_uri_t index_get_handler = {
    .handler = (http_handle_t)0,
    .method = HTTP_GET,
    .uri = "/",
    .file = "/webpanel/control.html",
    .context = 0,
    .content_type = "text/html; charset=UTF-8"
};
const http_uri_t setup_get_handler = {
    .handler = (http_handle_t)0,
    .method = HTTP_GET,
    .uri = "/setup",
    .file = "/webpanel/setup.html",
    .context = 0,
    .content_type = "text/html; charset=UTF-8"
};
const http_uri_t wifi_get_handler = {
    .handler = (http_handle_t)0,
    .method = HTTP_GET,
    .uri = "/wifi",
    .file = "/webpanel/wifi.html",
    .context = 0,
    .content_type = "text/html; charset=UTF-8"
};
const http_uri_t alarm_get_handler = {
    .handler = (http_handle_t)0,
    .method = HTTP_GET,
    .uri = "/alarm",
    .file = "/webpanel/alarm.html",
    .context = 0,
    .content_type = "text/html; charset=UTF-8"
};
const http_uri_t update_get_handler = {
    .handler = (http_handle_t)0,
    .method = HTTP_GET,
    .uri = "/info",
    .file = 0,
    .context = update_html,
    .content_type = "text/html; charset=UTF-8"
};
const http_uri_t style_css_handler = {
    .handler = (http_handle_t)0,
    .method = HTTP_GET,
    .uri = "/style.css",
    .file = "/webpanel/style.css",
    .context = 0,
    .content_type = "text/css"
};
const http_uri_t firework_bg_js_handler = {
    .handler = (http_handle_t)0,
    .method = HTTP_GET,
    .uri = "/firework_bg.js",
    .file = 0,
    .context = firework_bg_js,
    .content_type = "text/javascript"
};
const http_uri_t logo_png_handler = {
    .handler = (http_handle_t)0,
    .method = HTTP_GET,
    .uri = "/logo.png",
    .file = 0,
    .context = logo_png,
    .content_type = "image/png"
};
const http_uri_t setup_svg_handler = {
    .handler = (http_handle_t)0,
    .method = HTTP_GET,
    .uri = "/setup.svg",
    .file = "/webpanel/setup.svg",
    .context = 0,
    .content_type = "image/svg+xml"
};
const http_uri_t firework_svg_handler = {
    .handler = (http_handle_t)0,
    .method = HTTP_GET,
    .uri = "/firework.svg",
    .file = "/webpanel/firework.svg",
    .context = 0,
    .content_type = "image/svg+xml"
};
const http_uri_t sound_svg_handler = {
    .handler = (http_handle_t)0,
    .method = HTTP_GET,
    .uri = "/sound.svg",
    .file = "/webpanel/sound.svg",
    .context = 0,
    .content_type = "image/svg+xml"
};
const http_uri_t close_svg_handler = {
    .handler = (http_handle_t)0,
    .method = HTTP_GET,
    .uri = "/close.svg",
    .file = "/webpanel/close.svg",
    .context = 0,
    .content_type = "image/svg+xml"
};
const http_uri_t alarm_svg_handler = {
    .handler = (http_handle_t)0,
    .method = HTTP_GET,
    .uri = "/alarm.svg",
    .file = "/webpanel/alarm.svg",
    .context = 0,
    .content_type = "image/svg+xml"
};
const http_uri_t power_svg_handler = {
    .handler = (http_handle_t)0,
    .method = HTTP_GET,
    .uri = "/power.svg",
    .file = "/webpanel/power.svg",
    .context = 0,
    .content_type = "image/svg+xml"
};
const http_uri_t wifi_svg_handler = {
    .handler = (http_handle_t)0,
    .method = HTTP_GET,
    .uri = "/wifi.svg",
    .file = "/webpanel/wifi.svg",
    .context = 0,
    .content_type = "image/svg+xml"
};
const http_uri_t info_svg_handler = {
    .handler = (http_handle_t)0,
    .method = HTTP_GET,
    .uri = "/info.svg",
    .file = "/webpanel/info.svg",
    .context = 0,
    .content_type = "image/svg+xml"
};
const http_uri_t api_get_handler = {
    .handler = (http_handle_t)api_GET_callback,
    .method = HTTP_GET,
    .uri = "/api",
    .file = 0,
    .context = 0,
    .content_type = "text/html; charset=UTF-8"
};
const http_uri_t api_post_handler = {
    .handler = (http_handle_t)api_POST_callback,
    .method = HTTP_POST,
    .uri = "/api",
    .file = 0,
    .context = 0,
    .content_type = "text/html; charset=UTF-8"
};
const http_uri_t fw_stream_ws_handler = {
    .handler = (http_handle_t)update_fw_callback,
    .ws_init_handler = (http_handle_t)update_fw_init_callback,
    .method = HTTP_GET,
    .uri = "/fw_stream",
    .file = 0,
    .context = 0,
    .content_type = "text/html; charset=UTF-8"
};

realtime_t getRT;

struct {
    struct {
        struct {
            uint8_t hours;
            uint8_t min;
            uint8_t sec;
        } On;
        struct {
            uint8_t hours;
            uint8_t min;
            uint8_t sec;
        } Off;
        TimeRun_Events Event;
    } Alarm;
    struct {
        char *ssid;
        char *password;
    } Wifi;
} VarSaveFlash;

struct {
    bool Effect;
    bool Sound;
} FireworkState = {.Effect = true, .Sound = true};

uint32_t print_free_heap_interval = 0;
uint32_t check_wifi_station_interval = 0;
uint32_t get_real_time_interval = 0;
bool wifi_status = false;

upload_state_t upload_state = {
    .state = FW_UPDATE_START,
    .address = 0,
    .file_size = 0,
    .byte_written = 0
};

static os_timer_t timer_close_AP;
static os_timer_t timer_connect_AP;
static os_timer_t timer_restart_after_update;

/**
 * @brief -----------------------------------> MAIN FUNCTION <----------------------------------- 
 */
void setup() {
    system_print_meminfo();     /* In thông tin hệ thống */
    /**
     * @brief <-----> Khởi tạo ngoại vi, phần cứng  <----->
     */
    gpio_init();
    hw_timer_init(FRC1_SOURCE, 1);
    hw_timer_set_func(hw_timer_tick_ms);
    hw_timer_arm(1000);
    /**
     * @brief <-----> Khởi tạo file system <----->
     */
    VarSaveFlash.Wifi.ssid = station_conf.ssid;
    VarSaveFlash.Wifi.password = station_conf.password;
    if (fbegin() != SPIFFS_OK) {
        BITS_LOGE("Error mount SPIFFS\r\n");
        system_restart();
    }
    else {
        /* Lấy thông tin wifi */
        File fd = fopen("/wifi.txt", "r+");
        if (fd < 0) {
            BITS_LOGE("File not found\r\n");
            system_restart();
        }
        char *read_file_buf = (char *)malloc(256), *temp_p = 0;
        if(read_file_buf) {
            memset(read_file_buf, 0, 256);
            temp_p = read_file_buf;
            if(fsize(fd) > 0) {
                fseek(fd, 0, SPIFFS_SEEK_SET);
                fread(fd, temp_p, fsize(fd));
                /* Lấy SSID */
                strcpy(VarSaveFlash.Wifi.ssid, temp_p);
                temp_p += strlen(VarSaveFlash.Wifi.ssid) + 1;
                /* Lấy Password */
                strcpy(VarSaveFlash.Wifi.password, temp_p);
                temp_p += strlen(VarSaveFlash.Wifi.password) + 1;
            }
            free(read_file_buf);
        }
        fclose(fd);
        /* Lấy thông tin tham số chạy firework */
        read_file_buf = (char *)malloc(20);
        fd = fopen("/param.txt", "r+");
        if (fd < 0) {
            BITS_LOGE("File not found\r\n");
            system_restart();
        }
        if(read_file_buf) {
            memset(read_file_buf, 0, 20);
            temp_p = read_file_buf;
            if(fsize(fd)) {
                fseek(fd, 0, SPIFFS_SEEK_SET);
                fread(fd, temp_p, fsize(fd));
            }
            else {
                fseek(fd, 0, SPIFFS_SEEK_SET);
                read_file_buf[12] = 1;
                read_file_buf[13] = 1;
                fwrite(fd, read_file_buf, 14);
            }
            firework_set_type_led(temp_p[0]);
            firework_set_num_led_fire((((uint16_t)temp_p[1]) << 8) | temp_p[2]);
            firework_set_num_led_expl((((uint16_t)temp_p[3]) << 8) | temp_p[4]);
            firework_set_spd_led_fire(temp_p[5]);
            firework_set_spd_led_expl(temp_p[6]);
            VarSaveFlash.Alarm.On.hours = temp_p[7];
            VarSaveFlash.Alarm.On.min = temp_p[8];
            VarSaveFlash.Alarm.Off.hours = temp_p[9];
            VarSaveFlash.Alarm.Off.min = temp_p[10];
            VarSaveFlash.Alarm.Event = temp_p[11];
            firework_set_state_effect(temp_p[12]);
            firework_set_state_sound(temp_p[13]);
            free(read_file_buf);
        }
        fclose(fd);
    }
    /**
     * @brief <-----> Cấu hình Access Point <----->
     */
    wifi_set_opmode(STATIONAP_MODE);
    IP4_ADDR(&sv_ip.gw, 8,8,8,8);
    IP4_ADDR(&sv_ip.ip, 8,8,8,8);
    IP4_ADDR(&sv_ip.netmask, 255,255,255,0);
    if(user_set_softap_config("SOLANTECH: FIREWORK_ESP\0", 0, &sv_ip)) {
        // dns_server = dns_server_init(53, "*", sv_ip.ip);
    }
    sv_ip.ip.addr = 0;
    /**
     * @brief Cấu hình Station
     */
    if(strlen(VarSaveFlash.Wifi.ssid)) {
        wifi_station_set_config_current(&station_conf);
        wifi_station_connect();
    }
    /**
     * @brief Đăng ký callback scan wifi
     */
    wifi_station_scan(NULL, wifi_scan_done);
    /**
     * @brief <-----> Khởi tạo các control <----->
     */
    form_run = (Form_t){firework_form_run_init, firework_form_run_run, 0, .Arg = 0};
    form_set_type_led = (Form_t){0, firework_form_set_typeled_run, 0, .Arg = 0};
    form_set_cot = (Form_t){firework_form_set_cot_init, firework_form_set_nled_cot, 0, .Arg = 0};
    form_set_tia = (Form_t){firework_form_set_tia_init, firework_form_set_nled_tia, 0, .Arg = 0};
    form_set_td_cot = (Form_t){firework_form_set_cot_init, firework_form_set_sled_cot, 0, .Arg = 0};
    form_set_td_tia = (Form_t){firework_form_set_tia_init, firework_form_set_sled_tia, 0, .Arg = 0};

    key_set = (IO_Key_t){.SignalInput_Get = key_set_get_input, .PressCallback = firework_key_set_press, .ReleaseCallback = firework_key_set_release, .Debounce.TimeMs = 20, .Enable = true};
    key_up = (IO_Key_t){.SignalInput_Get = key_up_get_input, .PressCallback = firework_key_up_press, .ReleaseCallback = firework_key_up_release, .Debounce.TimeMs = 20, .Enable = true};
    key_down = (IO_Key_t){.SignalInput_Get = key_down_get_input, .PressCallback = firework_key_down_press, .ReleaseCallback = firework_key_down_release, .Debounce.TimeMs = 20, .Enable = true};

    led_run.Output_Driver = led_run_set_output;
    led_tia.Output_Driver = led_tia_set_output;
    led_cot.Output_Driver = led_cot_set_output;

    stimer1 = (STimer_t){.Counter = 0, .Compare = 0, .Enable = false, .Repeat = false, .Callback = 0, .Arg = 0};
    stimer_clear_param = (STimer_t){.Counter = 0, .Compare = 0, .Enable = false, .Repeat = false, .Callback = 0, .Arg = 0};
    /**
     * @brief Thêm các control vào chương trình 
     */
    Application_Add_Led(&app_firework, &led_run);
    Application_Add_Led(&app_firework, &led_tia);
    Application_Add_Led(&app_firework, &led_cot);
    Application_Add_Key(&app_firework, &key_set);
    Application_Add_Key(&app_firework, &key_up);
    Application_Add_Key(&app_firework, &key_down);
    Application_Add_STimer(&app_firework, &stimer1);
    Application_Add_STimer(&app_firework, &stimer_clear_param);

    Application_Form_Switch(&app_firework, &form_run);
    FIREWORK_Init();
    /**
     * @brief <-----> Đăng ký response cho server <----->
     */
    http_server_register_uri(&http_server, (http_uri_t *)&index_get_handler);
    http_server_register_uri(&http_server, (http_uri_t *)&setup_get_handler);
    http_server_register_uri(&http_server, (http_uri_t *)&wifi_get_handler);
    http_server_register_uri(&http_server, (http_uri_t *)&alarm_get_handler);
    http_server_register_uri(&http_server, (http_uri_t *)&update_get_handler);
    http_server_register_uri(&http_server, (http_uri_t *)&fw_stream_ws_handler);
    http_server_register_uri(&http_server, (http_uri_t *)&style_css_handler);
    http_server_register_uri(&http_server, (http_uri_t *)&firework_bg_js_handler);
    http_server_register_uri(&http_server, (http_uri_t *)&logo_png_handler);
    http_server_register_uri(&http_server, (http_uri_t *)&firework_svg_handler);
    http_server_register_uri(&http_server, (http_uri_t *)&sound_svg_handler);
    http_server_register_uri(&http_server, (http_uri_t *)&close_svg_handler);
    http_server_register_uri(&http_server, (http_uri_t *)&power_svg_handler);
    http_server_register_uri(&http_server, (http_uri_t *)&setup_svg_handler);
    http_server_register_uri(&http_server, (http_uri_t *)&alarm_svg_handler);
    http_server_register_uri(&http_server, (http_uri_t *)&wifi_svg_handler);
    http_server_register_uri(&http_server, (http_uri_t *)&info_svg_handler);
    http_server_register_uri(&http_server, (http_uri_t *)&api_get_handler);
    http_server_register_uri(&http_server, (http_uri_t *)&api_post_handler);
    /**
     * @brief Tạo server HTTP 
     */
    http_server_init(&http_server, (ip_addr_t){.addr = INADDR_ANY}, 80, 15);
}

uint32_t refresh_sound = 0;

void loop(os_event_t *events) {
    /* BEGIN IMPORTANT */
    system_os_post(LOOP_TASK_PRIORITY, 0, 0);
    /* END IMPORTANT */
    // if(audio_gen_wav_is_running(&wav_file) == audio_gen_wav_running){
    //     audio_gen_wav_loop(&wav_file);
    // }
    // if((user_millis() - refresh_sound) > 3000) {
    //     refresh_sound = user_millis();
    //     audio_gen_wav_file(&wav_file, "/whistle.wav");
    //     audio_gen_wav_regist_drv_output(&wav_file, &i2s_wav);
    // }
    if(upload_state.state == FW_UPDATE_START) {
        Application_Run(&app_firework);
    }
    if((user_millis() - check_wifi_station_interval) > 5000) {
        check_wifi_station_interval = user_millis();
        wifi_get_ip_info(STATION_IF, &sv_ip);   
        if ((wifi_station_get_connect_status() == STATION_GOT_IP) && (sv_ip.ip.addr != 0)) {
            if(wifi_status == false) {
                wifi_status = true;
                realtime_init();
                /* Đóng AP sau 1 phút */
                os_timer_disarm(&timer_close_AP);
                os_timer_setfn(&timer_close_AP, (os_timer_func_t *)timer_close_AP_callback, NULL);
                os_timer_arm(&timer_close_AP, 60000, 0);
            }
            // getRT = realtime_get();
            // BITS_LOGD("current time : hour:%d min:%d sec:%d\r\n", getRT.tm_hour, getRT.tm_min, getRT.tm_sec);
        } else {
            if ((wifi_station_get_connect_status() == STATION_WRONG_PASSWORD ||
                    wifi_station_get_connect_status() == STATION_NO_AP_FOUND ||
                    wifi_station_get_connect_status() == STATION_CONNECT_FAIL)) {
                sv_ip.ip.addr = 0;
                wifi_status = false;
            }
            else if(wifi_station_get_connect_status() == STATION_IDLE) {
                wifi_status = false;
            }
        }
    }
    if(wifi_status == true) {
        if((user_millis() - get_real_time_interval) > 500) {
            get_real_time_interval = user_millis();
            getRT = realtime_get();
            if((getRT.tm_hour == VarSaveFlash.Alarm.On.hours) && (getRT.tm_min == VarSaveFlash.Alarm.On.min)) {
                switch(VarSaveFlash.Alarm.Event) {
                    case 0: {
                        break;
                    }
                    case 1: {
                        firework_set_state_sound(true);
                        save_state_sound(firework_get_state_sound());
                    }
                    case 2: {
                        firework_set_state_effect(true);
                        save_state_effect(firework_get_state_effect());
                    }
                    case 3: {
                        firework_set_state_effect(true);
                        firework_set_state_sound(true);
                        save_state_effect(firework_get_state_effect());
                        save_state_sound(firework_get_state_sound());
                    }
                }
            }
            else if((getRT.tm_hour == VarSaveFlash.Alarm.Off.hours) && (getRT.tm_min == VarSaveFlash.Alarm.Off.min)) {
                switch(VarSaveFlash.Alarm.Event) {
                    case 0: {
                        break;
                    }
                    case 1: {
                        firework_set_state_sound(false);
                    }
                    case 2: {
                        firework_set_state_effect(false);
                    }
                    case 3: {
                        firework_set_state_effect(false);
                        firework_set_state_sound(false);
                    }
                }
            }
        }
    }
    // if((user_millis() - print_free_heap_interval) > 1000) {
    //     print_free_heap_interval = user_millis();
    //     BITS_LOGD("Free heap: %d\r\n", system_get_free_heap_size());
    // }
}

/**
 * @brief Get the tick object
 * 
 * @return uint32_t 
 */
uint32_t get_tick(void) {
    uint32_t tick;
    asm("rsr %0, ccount" : "=r" (tick));
    return tick;
}
/**
 * @brief Callback hardware timer
 * 
 */
__attribute__((section(".text"))) void hw_timer_tick_ms(void) {
    __tick_ms_counter++;
    Application_Led_Handler(&app_firework);
}
/**
 * @brief Callback Software timer 
 */
void timer_connect_AP_callback() {
    wifi_station_set_config_current(&station_conf);
    wifi_station_connect();
}
void timer_close_AP_callback() {
    wifi_set_opmode(STATION_MODE);
}
void timer_restart_after_update_callback() {
    system_restart();
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
    ucs_io_cfg.pin_bit_mask = GPIO_BUTTON_DOWN_BIT_MASK|GPIO_BUTTON_UP_BIT_MASK;
    gpio_config(&ucs_io_cfg);

    ucs_io_cfg.dir = GPIO_DIR_INPUT; 
    ucs_io_cfg.intr_type = GPIO_INT_DISABLE; 
    ucs_io_cfg.pull_down_en = GPIO_PULLDOWN_ENABLE;
    ucs_io_cfg.pull_up_en = GPIO_PULLUP_DISABLE;
    ucs_io_cfg.pin_bit_mask = GPIO_BUTTON_SET_BIT_MASK;
    gpio_config(&ucs_io_cfg);
}
/**
 * @brief Callback khi wifi scan xong
 * 
 * @param arg 
 * @param status 
 */
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

static void save_state_effect(bool state) {
    uint8_t temp_data = 0;  
    File fd = fopen("/param.txt", "r+");
    if (fd < 0) {
        BITS_LOGE("Fire not found\r\n");
        system_restart();
    }
    fseek(fd, 12, SPIFFS_SEEK_SET);
    fread(fd, &temp_data, 1);
    if(temp_data != state) {
        fseek(fd, 12, SPIFFS_SEEK_SET);
        fwrite(fd, &state, 1);
    }
    fclose(fd);
}
static void save_state_sound(bool state) {
    uint8_t temp_data = 0;  
    File fd = fopen("/param.txt", "r+");
    if (fd < 0) {
        BITS_LOGE("File not found\r\n");
        system_restart();
    }
    fseek(fd, 13, SPIFFS_SEEK_SET);
    fread(fd, &temp_data, 1);
    if(temp_data != state) {
        fseek(fd, 13, SPIFFS_SEEK_SET);
        fwrite(fd, &state, 1);
    }
    fclose(fd);
}
/**
 * @brief -----------------------------------> Các hàm Driver hardware trong app <-----------------------------------
 */
bool key_set_get_input(void) {
    if(GPIO->in & GPIO_BUTTON_SET_BIT_MASK) return true;
    else return false;
}
bool key_up_get_input(void) {
    if(GPIO->in & GPIO_BUTTON_UP_BIT_MASK) return false;
    else return true;
}
bool key_down_get_input(void) {
    if(GPIO->in & GPIO_BUTTON_DOWN_BIT_MASK) return false;
    else return true;    
}
__attribute__((section(".text"))) void led_run_set_output(bool state) {
    if(state) {
        GPIO->out &= ~GPIO_LED_SET_BIT_MASK;
        return;
    }
    GPIO->out |= GPIO_LED_SET_BIT_MASK;
}
__attribute__((section(".text"))) void led_cot_set_output(bool state) {
    if(state) {
        GPIO->out &= ~GPIO_LED_COT_BIT_MASK;
        return;
    }
    GPIO->out |= GPIO_LED_COT_BIT_MASK;
}
__attribute__((section(".text"))) void led_tia_set_output(bool state) {
    if(state) {
        GPIO->out &= ~GPIO_LED_TIA_BIT_MASK;
        return;
    }
    GPIO->out |= GPIO_LED_TIA_BIT_MASK;
}
/**
 * @brief -----------------------------------> Các callback software timer trong App <-----------------------------------
 */
static inline void firework_config_up_value() {
    if(app_firework.Form.Running == &form_set_type_led) {
        switch(DataSaveRam.LedType) {
            case LED_6803: {
                firework_set_type_led(LED_1903);
                break;
            }
            default : {
                firework_set_type_led(LED_6803);
                break;
            }
        }
    }
    else if(app_firework.Form.Running == &form_set_cot) {
        if(DataSaveRam.SizeCot < FIREWORK_MAX_LED_COT) {
            DataSaveRam.SizeCot++;
        }
    }
    else if(app_firework.Form.Running == &form_set_tia) {
        if(DataSaveRam.SizeTia < FIREWORK_MAX_LED_TIA) {
            DataSaveRam.SizeTia++;
        }
    }
    else if(app_firework.Form.Running == &form_set_td_cot) {
        if(DataSaveRam.SpeedCot < FIREWORK_MAX_SPD_COT) {
            DataSaveRam.SpeedCot++;
        }
    }
    else if(app_firework.Form.Running == &form_set_td_tia) {
        if(DataSaveRam.SpeedTia < FIREWORK_MAX_SPD_TIA) {
            DataSaveRam.SpeedTia++;
        }
    }
}
static inline void firework_config_down_value() {
    if(app_firework.Form.Running == &form_set_type_led) {
        switch(DataSaveRam.LedType) {
            case LED_6803: {
                firework_set_type_led(LED_1903);
                break;
            }
            default : {
                firework_set_type_led(LED_6803);
                break;
            }
        }
    }
    else if(app_firework.Form.Running == &form_set_cot) {
        if(DataSaveRam.SizeCot > FIREWORK_MIN_LED_COT) {
            DataSaveRam.SizeCot--;
        }
    }
    else if(app_firework.Form.Running == &form_set_tia) {
        if(DataSaveRam.SizeTia > FIREWORK_MIN_LED_TIA) {
            DataSaveRam.SizeTia--;
        }
    }
    else if(app_firework.Form.Running == &form_set_td_cot) {
        if(DataSaveRam.SpeedCot > FIREWORK_MIN_SPD_COT) {
            DataSaveRam.SpeedCot--;
        }
    }
    else if(app_firework.Form.Running == &form_set_td_tia) {
        if(DataSaveRam.SpeedTia > FIREWORK_MIN_SPD_TIA) {
            DataSaveRam.SpeedTia--;
        }
    }
}
void firework_change_mode_cb(void *arg) {
    if(app_firework.Form.Running == &form_run) {
        Application_Form_Switch(&app_firework, &form_set_type_led);
    }
}
void firework_up_value_cb(void *arg) {
    stimer1.Compare = 100;
    firework_config_up_value();
}
void firework_down_value_cb(void *arg) {
    stimer1.Compare = 100;
    firework_config_down_value();
}
void firework_clear_param_cb(void *arg) {
    static uint8_t state_clear = 0;
    if(state_clear == 1) {
        memset(VarSaveFlash.Wifi.password, 0, 32);
        /* Xóa dữ liệu wifi */
        File fd = fopen("/wifi.txt", "w");
        if (fd < 0) {
            BITS_LOGE("Error opening file\r\n");
            system_restart();
            return false;
        }
        fseek(fd, 0, SPIFFS_SEEK_SET);                     // Về đầu file
        fwrite(fd, "\0", 1);
        fwrite(fd, VarSaveFlash.Wifi.password, strlen(VarSaveFlash.Wifi.password));
        fwrite(fd, "\0", 1);
        fclose(fd);
        /* Disconnect wifi */
        wifi_station_disconnect();
        memset(VarSaveFlash.Wifi.ssid, 0, 32);
        /* Bật lại AP */
        wifi_set_opmode(STATIONAP_MODE);
        IP4_ADDR(&sv_ip.gw, 8,8,8,8);
        IP4_ADDR(&sv_ip.ip, 8,8,8,8);
        IP4_ADDR(&sv_ip.netmask, 255,255,255,0);
        if(user_set_softap_config("SOLANTECH: FIREWORK_ESP\0", 0, &sv_ip)) {
            // dns_server = dns_server_init(53, "*", sv_ip.ip);
        }
        stimer_clear_param.Compare = 0;
        stimer_clear_param.Repeat = false;
        state_clear = 0;
        Application_Form_Switch(&app_firework, &form_run);
    }
    else {
        Application_Led_Config(&led_run, LED_DUTY_MAX, 300, 0, -1);
        Application_Led_Config(&led_cot, LED_DUTY_MAX, 300, 0, -1);
        Application_Led_Config(&led_tia, LED_DUTY_MAX, 300, 0, -1);
        stimer_clear_param.Compare = 2000;
        state_clear = 1;
    }
}
/**
 * @brief -----------------------------------> Các callback key event <-----------------------------------
 */
void firework_key_set_press(void *arg) {
    if(app_firework.Form.Running == &form_run) {
        STimer_Start(&stimer1, 3000, firework_change_mode_cb, 0, false);    /* Khởi tạo timer thay đổi chế độ nếu giữ nút hơn 3 Giây */
    }
    else if(app_firework.Form.Running == &form_set_type_led) {
        Application_Form_Switch(&app_firework, &form_set_cot);
    }
    else if(app_firework.Form.Running == &form_set_cot) {
        Application_Form_Switch(&app_firework, &form_set_tia);
    }
    else if(app_firework.Form.Running == &form_set_tia) {
        Application_Form_Switch(&app_firework, &form_set_td_cot);
    }
    else if(app_firework.Form.Running == &form_set_td_cot) {
        Application_Form_Switch(&app_firework, &form_set_td_tia);
    }
    else if(app_firework.Form.Running == &form_set_td_tia) {
        File fd = fopen("/param.txt", "w");
        if (fd < 0) {
            BITS_LOGE("Error opening file\r\n");
            system_restart();
            return false;
        }
        uint8_t *param_buf = (char *)malloc(10);
        if(param_buf) {
            fseek(fd, 0, SPIFFS_SEEK_SET);                     // Về đầu file
            param_buf[0] = DataSaveRam.LedType;
            param_buf[1] = (DataSaveRam.SizeCot >> 8) & 0xFF;
            param_buf[2] = (DataSaveRam.SizeCot & 0xFF);
            param_buf[3] = (DataSaveRam.SizeTia >> 8) & 0xFF;
            param_buf[4] = (DataSaveRam.SizeTia & 0xFF);
            param_buf[5] = (DataSaveRam.SpeedCot & 0xFF);
            param_buf[6] = (DataSaveRam.SpeedTia & 0xFF);
            fwrite(fd, param_buf, 7);
            free(param_buf);
        }
        fclose(fd);
        Application_Form_Switch(&app_firework, &form_run);
    }
}
void firework_key_set_release(void *arg) {
    STimer_Stop(&stimer1);        /* Dừng timer thay đổi chế độ */    
}
void firework_key_up_press(void *arg) {
    STimer_Start(&stimer1, 1500, firework_up_value_cb, 0, true);        /* Ấn giữ nút trong 1.5s thì giá trị sẽ thay đổi liên tục */
}
void firework_key_up_release(void *arg) {  
    STimer_Stop(&stimer1);        /* Dừng timer thay đổi giá trị */
    firework_config_up_value();
}
void firework_key_down_press(void *arg) {
    if(app_firework.Form.Running == &form_run) {
        STimer_Start(&stimer_clear_param, 5000, firework_clear_param_cb, 0, true);        /* Ấn giữ nút trong 5s thì xóa wifi đang dùng và bật lại AP */
    }
    STimer_Start(&stimer1, 1500, firework_down_value_cb, 0, true);        /* Ấn giữ nút trong 1.5s thì giá trị sẽ thay đổi liên tục */
}
void firework_key_down_release(void *arg) {
    STimer_Stop(&stimer1);        /* Dừng timer thay đổi giá trị */
    if(app_firework.Form.Running == &form_run) {
        STimer_Stop(&stimer_clear_param);
    }
    firework_config_down_value();
}
/**
 * @brief -----------------------------------> Các form trong App <-----------------------------------
 */
void firework_form_run_init(void *arg) {
    Application_Led_Config(&led_run, LED_DUTY_MAX, 300, 300, -1);
    Application_Led_Config(&led_cot, LED_DUTY_MAX, 0, 5, -1);
    Application_Led_Config(&led_tia, LED_DUTY_MAX, 0, 5, -1);
}
void firework_form_run_run(void *arg) {
    FIREWORK_Loop();
}
void firework_form_set_typeled_run(void *arg) {
    switch(DataSaveRam.LedType) {
        case LED_1903:
        case LED_1904: 
        case LED_1914:
        case LED_1916:
        case LED_9883:
        case LED_8903:
        case LED_2811:
        case LED_2812:
        case LED_8806:
        case LED_16703: {
            Application_Led_Config(&led_run, LED_DUTY_MAX, 500, 0, -1);
            Application_Led_Config(&led_cot, LED_DUTY_MAX, 500, 0, -1);
            Application_Led_Config(&led_tia, LED_DUTY_MAX, 500, 0, -1);
            break;
        }
        case LED_6803 : {
            Application_Led_Config(&led_run, LED_DUTY_MAX, 500, 0, -1);
            Application_Led_Config(&led_cot, LED_DUTY_MAX, 500, 0, -1);
            Application_Led_Config(&led_tia, LED_DUTY_MAX, 0, 500, -1);
            break;
        }
        default : {
            break;
        }
    }
    /* Show LED */
    FIREWORK_Test_FullLED();
}
void firework_form_set_cot_init(void *arg) {
    Application_Led_Config(&led_run, LED_DUTY_MAX, 0, 500, -1);
    Application_Led_Config(&led_cot, LED_DUTY_MAX, 500, 0, -1);
    Application_Led_Config(&led_tia, LED_DUTY_MAX, 0, 500, -1);
}
void firework_form_set_tia_init(void *arg) {
    Application_Led_Config(&led_run, LED_DUTY_MAX, 0, 500, -1);
    Application_Led_Config(&led_cot, LED_DUTY_MAX, 0, 500, -1);
    Application_Led_Config(&led_tia, LED_DUTY_MAX, 500, 0, -1);
}
void firework_form_set_nled_cot(void *arg) {
    FIREWORK_Test_nCot();
}
void firework_form_set_nled_tia(void *arg) {
    FIREWORK_Test_nTia();
}
void firework_form_set_sled_cot(void *arg) {
    FIREWORK_Test_Speed_Cot();
}
void firework_form_set_sled_tia(void *arg) {
    FIREWORK_Test_Speed_Tia();
}

/**
 * @brief -----------------------------------> Các callback handler HTTP <-----------------------------------
 */
bool api_GET_callback(http_session_t *sess) {
    char *uri_p = sess->request.uri_param;
    char *buf_json = malloc(1024);
    if(buf_json == 0) return false;
    /* Xóa buffer response json */
    memset(buf_json, 0, 1024);
    /* Lấy param trong URI */
    if((memcmp(uri_p, "id", 2) == 0) && (uri_p[2] == '=')) {
        uri_p += 3;
        sess->request.uri_param_len -= 3;
        if((memcmp(uri_p, "wifi", sess->request.uri_param_len) == 0)) {
            wifi_station_scan(NULL, wifi_scan_done);
            /* --- Mở đầu json --- */
            strcpy(buf_json, "{");
            /* Wifi list */
            strcat(buf_json, "\"wifiap\":[");
            for(uint8_t idx_ap = 0; (idx_ap < 20) && (ap_scanned[idx_ap].ssid_len != 0); idx_ap++) {
                if(idx_ap > 0) {
                    strcat(buf_json, ",");
                }
                strcat(buf_json, "\"");
                strcat(buf_json, ap_scanned[idx_ap].ssid);
                strcat(buf_json, "\"");
            }
            strcat(buf_json, "],");
            /* Password đang dùng */
            strcat(buf_json, "\"pass\":\"");
            strcat(buf_json, VarSaveFlash.Wifi.password);
            strcat(buf_json, "\",");
            /* AP đang chọn */
            strcat(buf_json, "\"ap_select\":\"");
            strcat(buf_json, VarSaveFlash.Wifi.ssid);
            strcat(buf_json, "\",");
            /* AP đang chọn */
            strcat(buf_json, "\"ip\":\"");
            if(sv_ip.ip.addr != 0) {
                char *ip_convert = malloc(20);
                if(ip_convert) {
                    memset(ip_convert, 0, 20);
                    sprintf(ip_convert, IPSTR, IP2STR(&sv_ip.ip));
                    if(strlen(ip_convert) <= 15) {
                        strcat(buf_json, ip_convert);
                    }
                    free(ip_convert);
                }
            }
            strcat(buf_json, "\"");
            /* --- Kết thúc json --- */
            strcat(buf_json, "}");
            /* Gửi json */
            http_server_write(sess, buf_json, strlen(buf_json));
        }
        else if((memcmp(uri_p, "ud_stt", sess->request.uri_param_len) == 0)) {
            /* --- Mở đầu json --- */
            strcpy(buf_json, "{");
            /* AP đang chọn */
            strcat(buf_json, "\"ip\":\"");
            if(sv_ip.ip.addr != 0) {
                char *ip_convert = malloc(20);
                if(ip_convert) {
                    memset(ip_convert, 0, 20);
                    sprintf(ip_convert, IPSTR, IP2STR(&sv_ip.ip));
                    if(strlen(ip_convert) <= 15) {
                        strcat(buf_json, ip_convert);
                    }
                    free(ip_convert);
                }
            }
            strcat(buf_json, "\",");
            /* FW Version hiện tại */
            strcat(buf_json, "\"fvers\":\"");
            strcat(buf_json, FW_VERSION);
            strcat(buf_json, "\",");
            /* Version hiện tại */
            strcat(buf_json, "\"hvers\":\"");
            strcat(buf_json, HW_VERSION);
            strcat(buf_json, "\"");
            /* --- Kết thúc json --- */
            strcat(buf_json, "}");
            /* Gửi json */
            http_server_write(sess, buf_json, strlen(buf_json));            
        }
        else if(memcmp(uri_p, "param", sess->request.uri_param_len) == 0) {
            /* --- Mở đầu json --- */
            strcpy(buf_json, "{");
            /* Danh sách LED đang hỗ trợ */
            strcat(buf_json, "\"led_t\":[");
            for(uint8_t idx_led = 0; idx_led < LED_IDX_MAX; idx_led++) {
                if(idx_led > 0) {
                    strcat(buf_json, ",");
                }
                strcat(buf_json, "\"");
                strcat(buf_json, led_type_string[idx_led]);
                strcat(buf_json, "\"");
            }
            strcat(buf_json, "],");
            /* Loại LED đang chọn */
            strcat(buf_json, "\"led_select\":\"");
            strcat(buf_json, led_type_string[DataSaveRam.LedType]);    
            strcat(buf_json, "\",");
            /* Max số LED cột */
            strcat(buf_json, "\"maxncot\":");
            sprintf(buf_json + strlen(buf_json), "%d", FIREWORK_MAX_LED_COT);
            strcat(buf_json, ",");
            /* Max số LED tia */
            strcat(buf_json, "\"maxntia\":");
            sprintf(buf_json + strlen(buf_json), "%d", FIREWORK_MAX_LED_TIA);
            strcat(buf_json, ",");
            /* Max tốc độ LED cột */
            strcat(buf_json, "\"maxscot\":");
            sprintf(buf_json + strlen(buf_json), "%d", FIREWORK_MAX_SPD_COT);
            strcat(buf_json, ",");
            /* Max tốc độ LED tia */
            strcat(buf_json, "\"maxstia\":");
            sprintf(buf_json + strlen(buf_json), "%d", FIREWORK_MAX_SPD_TIA);
            strcat(buf_json, ",");
            /* số LED cột */
            strcat(buf_json, "\"ncot\":");
            sprintf(buf_json + strlen(buf_json), "%d", DataSaveRam.SizeCot);
            strcat(buf_json, ",");
            /* số LED tia */
            strcat(buf_json, "\"ntia\":");
            sprintf(buf_json + strlen(buf_json), "%d", DataSaveRam.SizeTia);
            strcat(buf_json, ",");
            /* tốc độ LED cột */
            strcat(buf_json, "\"scot\":");
            sprintf(buf_json + strlen(buf_json), "%d", DataSaveRam.SpeedCot);
            strcat(buf_json, ",");
            /* tốc độ LED tia */
            strcat(buf_json, "\"stia\":");
            sprintf(buf_json + strlen(buf_json), "%d", DataSaveRam.SpeedTia);
            /* --- Kết thúc json --- */
            strcat(buf_json, "}");
            /* Gửi json */
            http_server_write(sess, buf_json, strlen(buf_json));
        }
        else if(memcmp(uri_p, "time_cur", sess->request.uri_param_len) == 0) {
            /* --- Mở đầu json --- */
            strcpy(buf_json, "{");
            /* Thời gian thực */
            strcat(buf_json, "\"timecur\":\"");
            char *time_convert = malloc(8);
            if(time_convert) {
                memset(time_convert, 0, 8);
                sprintf(time_convert, "%02d:%02d", getRT.tm_hour, getRT.tm_min);
                strcat(buf_json, time_convert);
                free(time_convert);
            }
            strcat(buf_json, "\"");
            /* --- Kết thúc json --- */
            strcat(buf_json, "}");
            /* Gửi json */
            http_server_write(sess, buf_json, strlen(buf_json));
        }
        else if(memcmp(uri_p, "time_set", sess->request.uri_param_len) == 0) {
            /* --- Mở đầu json --- */
            strcpy(buf_json, "{");
            /* Giờ bật */
            strcat(buf_json, "\"thon\":\"");
            char *time_convert = malloc(8);
            if(time_convert) {
                memset(time_convert, 0, 8);
                sprintf(time_convert, "%02d", VarSaveFlash.Alarm.On.hours);
                strcat(buf_json, time_convert);
                free(time_convert);
            }
            strcat(buf_json, "\",");
            /* Phút bật */
            strcat(buf_json, "\"tmon\":\"");
            time_convert = malloc(8);
            if(time_convert) {
                memset(time_convert, 0, 8);
                sprintf(time_convert, "%02d", VarSaveFlash.Alarm.On.min);
                strcat(buf_json, time_convert);
                free(time_convert);
            }
            strcat(buf_json, "\",");
            /* Giờ bật */
            strcat(buf_json, "\"thoff\":\"");
            time_convert = malloc(8);
            if(time_convert) {
                memset(time_convert, 0, 8);
                sprintf(time_convert, "%02d", VarSaveFlash.Alarm.Off.hours);
                strcat(buf_json, time_convert);
                free(time_convert);
            }
            strcat(buf_json, "\",");
            /* Phút tắt */
            strcat(buf_json, "\"tmoff\":\"");
            time_convert = malloc(8);
            if(time_convert) {
                memset(time_convert, 0, 8);
                sprintf(time_convert, "%02d", VarSaveFlash.Alarm.Off.min);
                strcat(buf_json, time_convert);
                free(time_convert);
            }
            strcat(buf_json, "\",");
            /* Sự kiện */
            strcat(buf_json, "\"event\":");
            switch(VarSaveFlash.Alarm.Event) {
                case TimeRun_Event_None : {
                    strcat(buf_json, "0");
                    break;
                }
                case TimeRun_Event_Sound : {
                    strcat(buf_json, "1");
                    break;
                }
                case TimeRun_Event_Effect : {
                    strcat(buf_json, "2");
                    break;
                }
                case TimeRun_Event_Both : {
                    strcat(buf_json, "3");
                    break;
                }
            }
            /* --- Kết thúc json --- */
            strcat(buf_json, "}");
            /* Gửi json */
            http_server_write(sess, buf_json, strlen(buf_json));
        }
        else if(memcmp(uri_p, "fwork_stt", sess->request.uri_param_len) == 0) {
            /* --- Mở đầu json --- */
            strcpy(buf_json, "{");
            /* Trạng thái hiệu ứng */
            strcat(buf_json, "\"eff\":");
            Bits_Int_ToString(firework_get_state_effect(), buf_json + strlen(buf_json), 10);
            strcat(buf_json, ",");
            /* Trạng thái âm thanh */
            strcat(buf_json, "\"sound\":");
            Bits_Int_ToString(firework_get_state_sound(), buf_json + strlen(buf_json), 10);
            /* --- Kết thúc json --- */
            strcat(buf_json, "}");
            /* Gửi json */
            http_server_write(sess, buf_json, strlen(buf_json));            
        }
    }
    free(buf_json);
    return false;
}
bool api_POST_callback(http_session_t *sess) {
    uint8_t ret = 0;
    /* Lấy dữ liệu request nếu có */
    if(sess->request.content) {
        char *json_string = sess->request.content;
        // BITS_LOGD("param request: ");
        // for(uint16_t idx_char = 0; idx_char < sess->request.content_len; idx_char++) {
        //     printf("%c", json_string[idx_char]); 
        // }
        // printf("\r\n");
        /* Cấp phát động bộ nhớ cho buffer chuyển đổi dữ liệu */
        char *buf_convert = malloc(20);
        if(buf_convert != 0) {
            memset(buf_convert, 0, 20);
            int num_token = 0;
            jsmn_parser p;
            jsmntok_t json_token[128]; /* We expect no more than 128 tokens */            
            jsmn_init(&p);
            num_token = jsmn_parse(&p, json_string, sess->request.content_len, json_token, sizeof(json_token)/sizeof(json_token[0]));
            if(num_token < 0) {
                /* Lỗi Parse chuỗi Json */
            }
            else {
                // BITS_LOGD("json parser: num=%d type=%d\r\n", num_token, json_token[0].type);
                if((num_token < 1) || (json_token[0].type != JSMN_OBJECT)) {
                    // Json Không phải là một Object
                    if(num_token == 1) {
                        if(memcmp(sess->request.content, "save_param", 10) == 0) {
                            File fd = fopen("/param.txt", "w");
                            if (fd < 0) {
                                BITS_LOGE("Error opening file\r\n");
                                system_restart();
                                return false;
                            }
                            uint8_t *param_buf = (char *)malloc(10);
                            if(param_buf) {
                                fseek(fd, 0, SPIFFS_SEEK_SET);                     // Về đầu file
                                param_buf[0] = DataSaveRam.LedType;
                                param_buf[1] = (DataSaveRam.SizeCot >> 8) & 0xFF;
                                param_buf[2] = (DataSaveRam.SizeCot & 0xFF);
                                param_buf[3] = (DataSaveRam.SizeTia >> 8) & 0xFF;
                                param_buf[4] = (DataSaveRam.SizeTia & 0xFF);
                                param_buf[5] = (DataSaveRam.SpeedCot & 0xFF);
                                param_buf[6] = (DataSaveRam.SpeedTia & 0xFF);
                                fwrite(fd, param_buf, 7);
                                free(param_buf);
                                http_server_write(sess, "SAVE_OK", 7);
                            }
                            fclose(fd);
                        }
                    }
                }
                else {
                    if( (json_token[2].type == JSMN_STRING) && ((json_token[1].end - json_token[1].start) == strlen("api_type")) &&
                        (strncmp(json_string + json_token[1].start, "api_type", json_token[1].end - json_token[1].start) == 0))
                    {
                        if((strncmp(json_string + json_token[2].start, "wifi", json_token[2].end - json_token[2].start) == 0)) {
                            for(uint8_t idx_token = 3; idx_token < num_token; idx_token++) {
                                if( (json_token[idx_token + 1].type == JSMN_STRING) && ((json_token[idx_token].end - json_token[idx_token].start) == strlen("ssid")) &&
                                    (strncmp(json_string + json_token[idx_token].start, "ssid", json_token[idx_token].end - json_token[idx_token].start) == 0))
                                {
                                    memset(VarSaveFlash.Wifi.ssid, 0, 32);
                                    memcpy(VarSaveFlash.Wifi.ssid, json_string + json_token[idx_token + 1].start, json_token[idx_token + 1].end - json_token[idx_token + 1].start);
                                    idx_token++;
                                }
                                else if( (json_token[idx_token + 1].type == JSMN_STRING) && ((json_token[idx_token].end - json_token[idx_token].start) == strlen("pass")) &&
                                    (strncmp(json_string + json_token[idx_token].start, "pass", json_token[idx_token].end - json_token[idx_token].start) == 0))
                                {
                                    memset(VarSaveFlash.Wifi.password, 0, 32);
                                    memcpy(VarSaveFlash.Wifi.password, json_string + json_token[idx_token + 1].start, json_token[idx_token + 1].end - json_token[idx_token + 1].start);
                                    idx_token++;
                                    /* Lưu lại cấu hình wifi sau khi đã lấy được password */
                                    File fd = fopen("/wifi.txt", "w");
                                    if (fd < 0) {
                                        BITS_LOGE("Error opening file\r\n");
                                        system_restart();
                                        return false;
                                    }
                                    fseek(fd, 0, SPIFFS_SEEK_SET);                     // Về đầu file
                                    fwrite(fd, VarSaveFlash.Wifi.ssid, strlen(VarSaveFlash.Wifi.ssid));
                                    fwrite(fd, "\0", 1);
                                    fwrite(fd, VarSaveFlash.Wifi.password, strlen(VarSaveFlash.Wifi.password));
                                    fwrite(fd, "\0", 1);
                                    fclose(fd);
                                    /* Kết nối đến Access Point */
                                    os_timer_disarm(&timer_connect_AP);
                                    os_timer_setfn(&timer_connect_AP, (os_timer_func_t *)timer_connect_AP_callback, NULL);
                                    os_timer_arm(&timer_connect_AP, 2000, 0);
                                    http_server_write(sess, "SAVE_OK", 7);
                                    break;
                                }
                            }
                        }
                        else if(strncmp(json_string + json_token[2].start, "param", json_token[2].end - json_token[2].start) == 0) {
                            for(uint8_t idx_token = 3; idx_token < num_token; idx_token++) {
                                if( (json_token[idx_token + 1].type == JSMN_PRIMITIVE) && ((json_token[idx_token].end - json_token[idx_token].start) == strlen("type")) &&
                                    (strncmp(json_string + json_token[idx_token].start, "type", json_token[idx_token].end - json_token[idx_token].start) == 0))
                                {
                                    memcpy(buf_convert, json_string + json_token[idx_token + 1].start, json_token[idx_token + 1].end - json_token[idx_token + 1].start);
                                    buf_convert[json_token[idx_token + 1].end - json_token[idx_token + 1].start] = 0;
                                    firework_set_type_led((led_type_t)Bits_String_ToInt(buf_convert));
                                    idx_token++;
                                }
                                else if( (json_token[idx_token + 1].type == JSMN_PRIMITIVE) && ((json_token[idx_token].end - json_token[idx_token].start) == strlen("ncot")) &&
                                    (strncmp(json_string + json_token[idx_token].start, "ncot", json_token[idx_token].end - json_token[idx_token].start) == 0))
                                {
                                    memcpy(buf_convert, json_string + json_token[idx_token + 1].start, json_token[idx_token + 1].end - json_token[idx_token + 1].start);
                                    buf_convert[json_token[idx_token + 1].end - json_token[idx_token + 1].start] = 0;
                                    firework_set_num_led_fire(Bits_String_ToInt(buf_convert));
                                    idx_token++;
                                }
                                else if( (json_token[idx_token + 1].type == JSMN_PRIMITIVE) && ((json_token[idx_token].end - json_token[idx_token].start) == strlen("ntia")) &&
                                    (strncmp(json_string + json_token[idx_token].start, "ntia", json_token[idx_token].end - json_token[idx_token].start) == 0))
                                {
                                    memcpy(buf_convert, json_string + json_token[idx_token + 1].start, json_token[idx_token + 1].end - json_token[idx_token + 1].start);
                                    buf_convert[json_token[idx_token + 1].end - json_token[idx_token + 1].start] = 0;
                                    firework_set_num_led_expl(Bits_String_ToInt(buf_convert));
                                    idx_token++;
                                }
                                else if( (json_token[idx_token + 1].type == JSMN_PRIMITIVE) && ((json_token[idx_token].end - json_token[idx_token].start) == strlen("scot")) &&
                                    (strncmp(json_string + json_token[idx_token].start, "scot", json_token[idx_token].end - json_token[idx_token].start) == 0))
                                {
                                    memcpy(buf_convert, json_string + json_token[idx_token + 1].start, json_token[idx_token + 1].end - json_token[idx_token + 1].start);
                                    buf_convert[json_token[idx_token + 1].end - json_token[idx_token + 1].start] = 0;
                                    firework_set_spd_led_fire(Bits_String_ToInt(buf_convert));
                                    idx_token++;
                                }
                                else if( (json_token[idx_token + 1].type == JSMN_PRIMITIVE) && ((json_token[idx_token].end - json_token[idx_token].start) == strlen("stia")) &&
                                    (strncmp(json_string + json_token[idx_token].start, "stia", json_token[idx_token].end - json_token[idx_token].start) == 0))
                                {
                                    memcpy(buf_convert, json_string + json_token[idx_token + 1].start, json_token[idx_token + 1].end - json_token[idx_token + 1].start);
                                    buf_convert[json_token[idx_token + 1].end - json_token[idx_token + 1].start] = 0;
                                    firework_set_spd_led_expl(Bits_String_ToInt(buf_convert));
                                    idx_token++;
                                }
                            }
                        }
                        else if(strncmp(json_string + json_token[2].start, "alarm", json_token[2].end - json_token[2].start) == 0) {
                            for(uint8_t idx_token = 3; idx_token < num_token; idx_token++) {
                                if( (json_token[idx_token + 1].type == JSMN_PRIMITIVE) && ((json_token[idx_token].end - json_token[idx_token].start) == strlen("thon")) &&
                                    (strncmp(json_string + json_token[idx_token].start, "thon", json_token[idx_token].end - json_token[idx_token].start) == 0))
                                {
                                    memcpy(buf_convert, json_string + json_token[idx_token + 1].start, json_token[idx_token + 1].end - json_token[idx_token + 1].start);
                                    buf_convert[json_token[idx_token + 1].end - json_token[idx_token + 1].start] = 0;
                                    VarSaveFlash.Alarm.On.hours = Bits_String_ToInt(buf_convert);
                                    idx_token++;
                                }
                                else if( (json_token[idx_token + 1].type == JSMN_PRIMITIVE) && ((json_token[idx_token].end - json_token[idx_token].start) == strlen("tmon")) &&
                                    (strncmp(json_string + json_token[idx_token].start, "tmon", json_token[idx_token].end - json_token[idx_token].start) == 0))
                                {
                                    memcpy(buf_convert, json_string + json_token[idx_token + 1].start, json_token[idx_token + 1].end - json_token[idx_token + 1].start);
                                    buf_convert[json_token[idx_token + 1].end - json_token[idx_token + 1].start] = 0;
                                    VarSaveFlash.Alarm.On.min = Bits_String_ToInt(buf_convert);
                                    idx_token++;
                                }
                                else if( (json_token[idx_token + 1].type == JSMN_PRIMITIVE) && ((json_token[idx_token].end - json_token[idx_token].start) == strlen("thoff")) &&
                                    (strncmp(json_string + json_token[idx_token].start, "thoff", json_token[idx_token].end - json_token[idx_token].start) == 0))
                                {
                                    memcpy(buf_convert, json_string + json_token[idx_token + 1].start, json_token[idx_token + 1].end - json_token[idx_token + 1].start);
                                    buf_convert[json_token[idx_token + 1].end - json_token[idx_token + 1].start] = 0;
                                    VarSaveFlash.Alarm.Off.hours = Bits_String_ToInt(buf_convert);
                                    idx_token++;
                                }
                                else if( (json_token[idx_token + 1].type == JSMN_PRIMITIVE) && ((json_token[idx_token].end - json_token[idx_token].start) == strlen("tmoff")) &&
                                    (strncmp(json_string + json_token[idx_token].start, "tmoff", json_token[idx_token].end - json_token[idx_token].start) == 0))
                                {
                                    memcpy(buf_convert, json_string + json_token[idx_token + 1].start, json_token[idx_token + 1].end - json_token[idx_token + 1].start);
                                    buf_convert[json_token[idx_token + 1].end - json_token[idx_token + 1].start] = 0;
                                    VarSaveFlash.Alarm.Off.min = Bits_String_ToInt(buf_convert);
                                    idx_token++;
                                }
                                else if( (json_token[idx_token + 1].type == JSMN_PRIMITIVE) && ((json_token[idx_token].end - json_token[idx_token].start) == strlen("event")) &&
                                    (strncmp(json_string + json_token[idx_token].start, "event", json_token[idx_token].end - json_token[idx_token].start) == 0))
                                {
                                    memcpy(buf_convert, json_string + json_token[idx_token + 1].start, json_token[idx_token + 1].end - json_token[idx_token + 1].start);
                                    buf_convert[json_token[idx_token + 1].end - json_token[idx_token + 1].start] = 0;
                                    VarSaveFlash.Alarm.Event = (TimeRun_Events)(Bits_String_ToInt(buf_convert));
                                    /* Lưu flash sau khi nhận đủ param */
                                    File fd = fopen("/param.txt", "w");
                                    if (fd < 0) {
                                        BITS_LOGE("Error opening file\r\n");
                                        system_restart();
                                        return false;
                                    }
                                    uint8_t *param_buf = (char *)malloc(10);
                                    if(param_buf) {
                                        fseek(fd, 7, SPIFFS_SEEK_SET);
                                        param_buf[0] = VarSaveFlash.Alarm.On.hours;
                                        param_buf[1] = VarSaveFlash.Alarm.On.min;
                                        param_buf[2] = VarSaveFlash.Alarm.Off.hours;
                                        param_buf[3] = VarSaveFlash.Alarm.Off.min;
                                        param_buf[4] = VarSaveFlash.Alarm.Event;
                                        fwrite(fd, param_buf, 5);
                                        free(param_buf);
                                        http_server_write(sess, "SAVE_OK", 7);
                                    }
                                    fclose(fd);
                                }
                            }
                        }
                        else if(strncmp(json_string + json_token[2].start, "cled", json_token[2].end - json_token[2].start) == 0) {
                            uint8_t idx_token = 3;
                            if( (json_token[idx_token + 1].type == JSMN_PRIMITIVE) && ((json_token[idx_token].end - json_token[idx_token].start) == strlen("stt")) &&
                                (strncmp(json_string + json_token[idx_token].start, "stt", json_token[idx_token].end - json_token[idx_token].start) == 0))
                            {
                                memcpy(buf_convert, json_string + json_token[idx_token + 1].start, json_token[idx_token + 1].end - json_token[idx_token + 1].start);
                                buf_convert[json_token[idx_token + 1].end - json_token[idx_token + 1].start] = 0;
                                FireworkState.Effect = Bits_String_ToInt(buf_convert) ? true: false;
                                firework_set_state_effect(FireworkState.Effect);
                                save_state_effect(firework_get_state_effect());
                                sprintf(buf_convert, "{\"eff\":%d}", firework_get_state_effect());
                                http_server_write(sess, buf_convert, strlen(buf_convert));
                            }
                        }
                        else if(strncmp(json_string + json_token[2].start, "csound", json_token[2].end - json_token[2].start) == 0) {
                            uint8_t idx_token = 3;
                            if( (json_token[idx_token + 1].type == JSMN_PRIMITIVE) && ((json_token[idx_token].end - json_token[idx_token].start) == strlen("stt")) &&
                                (strncmp(json_string + json_token[idx_token].start, "stt", json_token[idx_token].end - json_token[idx_token].start) == 0))
                            {
                                memcpy(buf_convert, json_string + json_token[idx_token + 1].start, json_token[idx_token + 1].end - json_token[idx_token + 1].start);
                                buf_convert[json_token[idx_token + 1].end - json_token[idx_token + 1].start] = 0;
                                FireworkState.Sound = Bits_String_ToInt(buf_convert) ? true: false;
                                firework_set_state_sound(FireworkState.Sound);
                                save_state_sound(firework_get_state_sound());
                                sprintf(buf_convert, "{\"sound\":%d}", firework_get_state_sound());
                                http_server_write(sess, buf_convert, strlen(buf_convert));
                            }
                        }
                    }
                }
            }
            free(buf_convert);
        }
    }
    return true;
}
bool update_fw_callback(http_session_t *sess) {
    uint16_t sector_size_remainning = 0x1000;
    char *frame = 0;
    if(upload_state.state == FW_UPDATE_START) {
        if((sess->request.content[0] == 0x7E) && (sess->request.content_len == 9)) {
            /* Lấy địa chỉ Ghi File */
            upload_state.address = sess->request.content[1];
            upload_state.address <<= 8;
            upload_state.address |= sess->request.content[2];
            upload_state.address <<= 8;
            upload_state.address |= sess->request.content[3];
            upload_state.address <<= 8;
            upload_state.address |= sess->request.content[4];
            /* Lấy kích thước File */
            upload_state.file_size = sess->request.content[5];
            upload_state.file_size <<= 8;
            upload_state.file_size |= sess->request.content[6];
            upload_state.file_size <<= 8;
            upload_state.file_size |= sess->request.content[7];
            upload_state.file_size <<= 8;
            upload_state.file_size |= sess->request.content[8];
            if((upload_state.address == 0) || (upload_state.file_size == 0) || (upload_state.address < 0x082000) || (upload_state.file_size > 0x07E000)) {
                return true;
            }            
            upload_state.byte_written = 0;
            upload_state.state = FW_UPDATE_WRITE;
            // BITS_LOGD("UPLOAD: Start upload file, addr: 0x%08X, size: 0x%08X\r\n", upload_state.address, upload_state.file_size);
            size_t frame_len = websocket_calc_frame_size(WS_OP_TEXT | WS_FINAL_FRAME, strlen("START_UPLOAD"));
            frame = malloc(sizeof(char) * frame_len);
            websocket_build_frame(frame, WS_OP_TEXT | WS_FINAL_FRAME, 0, "START_UPLOAD", strlen("START_UPLOAD"));
            http_server_write(sess, frame, frame_len);
            free(frame);
        }
    }
    else if(upload_state.state == FW_UPDATE_WRITE) {
        // BITS_LOG("UPLOAD: Write %d Byte\r\n", sess->request.content_len);
        while(sess->request.content_pos < sess->request.content_len) {
            sector_size_remainning = 0x1000;
            /* Xóa sector nếu địa chỉ hiện tại ở đầu sector */
            if(((upload_state.address + upload_state.byte_written) & 0xfff) == 0) {
                // BITS_LOG("- UPLOAD: erase sector %d --- ", ((upload_state.address + upload_state.byte_written) >> 12));
                SpiFlashOpResult err = spi_flash_erase_sector((upload_state.address + upload_state.byte_written) >> 12);
                if(err != SPI_FLASH_RESULT_OK) {
                    // printf("FAIL\r\n");
                }
                else {
                    // printf("OK\r\n");
                }
            }
            else {
                /* Kích thước bộ nhớ còn lại trong sector nếu địa chỉ ghi hiện tại không ở đầu sector */
                sector_size_remainning -= ((upload_state.address + upload_state.byte_written) & 0xfff);
            }
            if(sector_size_remainning > (sess->request.content_len - sess->request.content_pos)) {
                sector_size_remainning = (sess->request.content_len - sess->request.content_pos);
            }
            /* Ghi dữ liệu */
            // BITS_LOG("- UPLOAD: write %d Byte at 0x%08X\r\n", sector_size_remainning, upload_state.address + upload_state.byte_written);
            spi_write_align_byte(upload_state.address + upload_state.byte_written, sess->request.content + sess->request.content_pos, sector_size_remainning);
            // BITS_LOG("- UPLOAD: start pos: %d\r\n", sess->request.content_pos);
            sess->request.content_pos += sector_size_remainning;
            upload_state.byte_written += sector_size_remainning;
        }
        /* Thông báo tiến trình */
        char * string_send = malloc(20);
        memset(string_send, 0, 20);
        sprintf(string_send, "{\"proc\":%d}", upload_state.byte_written);
        size_t frame_len = websocket_calc_frame_size(WS_OP_TEXT | WS_FINAL_FRAME, strlen(string_send));
        frame = malloc(frame_len);
        websocket_build_frame(frame, WS_OP_TEXT | WS_FINAL_FRAME, 0, string_send, strlen(string_send));
        free(string_send);
        http_server_write(sess, frame, frame_len);
        free(frame);
        /* Kết thúc nhận file */
        if(upload_state.byte_written >= upload_state.file_size) {
            /* Thông báo dừng Upload cho Client */
            size_t frame_len = websocket_calc_frame_size(WS_OP_TEXT | WS_FINAL_FRAME, strlen("STOP_UPLOAD"));
            frame = malloc(sizeof(char) * frame_len);
            websocket_build_frame(frame, WS_OP_TEXT | WS_FINAL_FRAME, 0, "STOP_UPLOAD", strlen("STOP_UPLOAD"));
            http_server_write(sess, frame, frame_len);
            free(frame);
            /* Báo cho bootloader có Rom mới */
            bboot_data_t boot_data;
            SPIRead(BOOT_PARAM_PARTITION, &boot_data, sizeof(bboot_data_t));
            boot_data.magic = BBOOT_MAGIC;
            boot_data.new_rom = 1;
            boot_data.rom_size = upload_state.file_size;
            /* Reset trạng thái Upload */
            upload_state.state = FW_UPDATE_START;
            upload_state.address = 0;
            upload_state.file_size = 0;
            upload_state.byte_written = 0;
            /* Xóa phân vùng ROM cũ */
            if (SPIEraseSector(BOOT_PARAM_PARTITION >> 12) != 0) {
                return true;
            }
            /* Copy Rom mới vào phân vùng ROM cũ */
            if (SPIWrite(BOOT_PARAM_PARTITION, &boot_data, sizeof(bboot_data_t)) != 0) {
                return true;
            }
            /* Restart mạch sau 5 giây */
            os_timer_disarm(&timer_restart_after_update);
            os_timer_setfn(&timer_restart_after_update, (os_timer_func_t *)timer_restart_after_update_callback, NULL);
            os_timer_arm(&timer_restart_after_update, 3000, 0);
            BITS_LOG("UPLOAD Done!\r\n");
            return true;
        }
    }
    return true;
}
bool update_fw_init_callback(http_session_t *sess) {
    BITS_LOG("UPLOAD Init!\r\n");
    upload_state.state = FW_UPDATE_START;
    upload_state.file_size = 0;
    upload_state.address = 0;
    upload_state.byte_written = 0;
    return true;
}