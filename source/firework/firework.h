#ifndef __FIRE_WORK_H
#define __FIRE_WORK_H

#include "string.h"
#include "bits_math.h"

#ifndef bool
#include "c_types.h"
#endif

#include "ezform.h"
#include "color.h"

#include "ucs1903.h"
#include "lpd6803.h"
#include "audio_generator_wav.h"
#include "audio_output_i2s_noDAC.h"

//Nhấp nháy tàn dư rồi mới thu vào nhấp nháy rồi tỏa ra
//Có điểm rơi ở cột -> 
//Hiệu ứng 2, 3 phát nổ to. nổ to: sao băng toàn bộ led tia, rồi tắt dần từ tâm đi ra
//Hiệu ứng nổ nhỏ: nổ rồi giữ 1/3 ở tâm, 1 điểm sáng phụt ra
//Hiệu ứng nổ nhỏ: 1 điểm sáng phụt ra
//Hiệu ứng nổ phụt: chỉ 3 điểm sáng nổ từ tâm ra
//Hiệu ứng phụt điền đầy

/* -----> Option người dùng tự set <----- */
#define FIREWORK_MAX_LED_COT    256
#define FIREWORK_MIN_LED_COT    1
#define FIREWORK_MAX_SPD_COT    100
#define FIREWORK_MIN_SPD_COT    1
#define FIREWORK_MAX_LED_TIA    128
#define FIREWORK_MIN_LED_TIA    1
#define FIREWORK_MAX_SPD_TIA    FIREWORK_MAX_SPD_COT
#define FIREWORK_MIN_SPD_TIA    FIREWORK_MIN_SPD_COT

/* Define số cổng - chạy song song */
#define MAX_CHIP_PARA        2
/* Define số lượng chip nối tiếp mỗi cổng */
#define MAX_CHIP_SERI        (FIREWORK_MAX_LED_COT + FIREWORK_MAX_LED_TIA)

#define MAX_LEDS 1024
#define TIME_RUN 20

#define LED_MAP(XX)       \
        XX(0, 6803)       \
        XX(1, 1904)       \
        XX(2, 2811)       \
        XX(3, 2812)       \
        XX(4, 8206)       \
        XX(5, 16703)      \
        XX(6, 9883)       \
        XX(7, 1903)       \
        XX(8, 1914)       \
        XX(9, 1916)       \
        XX(10, 9813)      \
        XX(11, 8903)      \
        XX(12, 8806)      \
        XX(13, IDX_MAX)

typedef enum {
#define XX(id, name) LED_##name = id,
    LED_MAP(XX)
#undef XX
} led_type_t;

typedef enum
{
  NOMAL,//Nổ rồi tỏa đường nổ,
}FIRE_WORK_EFFECT;
#if   defined ( __CC_ARM )
#pragma pack(1)
#elif defined ( __ICCARM__ ) /* IAR Compiler */ 
//Với IAR thì mỗi struct lại phải viết lại cấu trúc này
#pragma data_alignment=1
__packed
#endif /* defined ( __ICCARM__ ) */
typedef struct
{
  uint8_t     SizePut0;
  uint8_t     SizePut1;
  uint8_t     SizePut2;
  Color_RGB_t   DataPut[50];
}DATA_PUT;
typedef enum
{
  RUN_EFFECT,
  SET_LED_TYPE,
  SET_SIZE_COT,
  SET_SIZE_TIA,
  SET_SPEED_COT,
  SET_SPEED_TIA,
}MODE_RUN;
#if   defined ( __CC_ARM )
#pragma pack(1)
#elif defined ( __ICCARM__ ) /* IAR Compiler */ 
//Với IAR thì mỗi struct lại phải viết lại cấu trúc này
#pragma data_alignment=1
__packed
#endif /* defined ( __ICCARM__ ) */
typedef struct
{
  //uint32_t  IAP_ID;
  uint16_t  SpeedCot;
  uint16_t  SpeedTia;
  uint16_t  SizeCot;
  uint16_t  SizeTia;
  led_type_t   LedType;
}DATA_SAVE;
#if   defined ( __CC_ARM )
#pragma pack(1)
#elif defined ( __ICCARM__ ) /* IAR Compiler */ 
//Với IAR thì mỗi struct lại phải viết lại cấu trúc này
#pragma data_alignment=1
__packed
#endif /* defined ( __ICCARM__ ) */

typedef enum
{
  STATE_COT_OFF,
  STATE_COT_BAN1,
  STATE_COT_BAN2,
  STATE_COT_BAN3,
  STATE_COT_BAN4,
  STATE_COT_BAN5
}STATE_COT;
typedef enum
{
  STATE_TIA_OFF,
  STATE_TIA_NO,
  STATE_TIA_GIAT
}STATE_TIA;
typedef enum
{
  GIAT_DON_DIEM,
  GIAT_DA_DIEM,
  GIAT_CO_TAM
}TYPE_GIAT;
typedef enum
{
  TAT_NGAY,
  KHONG_TAT,
  TAT_DAN,
  DICH_TRAI_DAN
}TYPE_TAT_TIA;
typedef enum
{
  NoTo,
  NoTo1DiemDau,
  No3PhanVung1Diem,
  NoToCoTam,
  No1Diem,
  No1DiemCoTam,
  No1DiemDienDay,
  No4Diem,
  No4DiemCoTam,
  No2PhanVung,
  No3PhanVung,
  NoToCoTam2,
  No1DiemCoTam2,
  No4DiemCoTam2
}TIA_TYPE_EFFECT;
typedef enum
{
  TYPE_PUT_COT,
  TYPE_PUT_TIA_COT,
  TYPE_PUT_TIA_RAND,
  TYPE_PUT_TIA_TAM,
  TYPE_PUT_TIA_DUOI,
}TYPE_PUT;

extern const char *led_type_string[];
extern DATA_SAVE DataSaveRam;
extern i2s_no_dac_t i2s_wav;
extern audio_gen_wav_t wav_file;

extern void FIREWORK_Init(void);

void FIREWORK_Loop();
void FIREWORK_Test(void);
void FIREWORK_Test_FullLED();
void FIREWORK_Test_nCot();
void FIREWORK_Test_nTia();
void FIREWORK_Test_Speed_Cot();
void FIREWORK_Test_Speed_Tia();

void firework_set_num_led_fire(uint16_t n_led);
void firework_set_num_led_expl(uint16_t n_led);
void firework_set_spd_led_fire(uint16_t spd);
void firework_set_spd_led_expl(uint16_t spd);
void firework_set_type_led(uint8_t idx_type);
void firework_set_state_effect(bool stt);
void firework_set_state_sound(bool stt);
bool firework_get_state_effect();
bool firework_get_state_sound();

#endif