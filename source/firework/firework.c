#include "firework.h"

/**
 * @brief Danh sách LED hỗ trợ trong chương trình
 */
const char *led_type_string[] = {
#define XX(id, name) #name,
    LED_MAP(XX)
#undef XX
};

DATA_SAVE DataSaveRam;
Color_RGB_t color_buffer[MAX_CHIP_SERI];
Color_RGB_t *DataLeds = &color_buffer[0];
DATA_PUT ColorTia; // Lưu dữ liệu để puts vào tia.
DATA_PUT ColorCot; // Lưu dữ liệu để put vào cột. Tối đa 5 cột bắn.

i2s_no_dac_t i2s_wav;
audio_gen_wav_t wav_file;

struct {
    bool Effect;
    bool Sound;
} Firework_State = {0, 0};

void (*led_driver) (Color_RGB_t * buffer, uint16_t led_len, uint16_t max_len);

#define GET_MIN(X, Y) (X > Y ? Y : X)
const uint8_t TypeTiaTable[] = {
        NoTo, No3PhanVung1Diem,
        NoTo, NoTo,
        NoToCoTam, NoTo1DiemDau,
        No1Diem, NoTo,
        No3PhanVung1Diem, No1DiemCoTam,
        No1Diem, No1DiemDienDay,
        NoTo1DiemDau, NoTo,
        No1DiemCoTam2, NoTo,
        NoToCoTam2, No4DiemCoTam2,
        No4Diem, No2PhanVung,
        No4DiemCoTam, No3PhanVung,
        No2PhanVung, No1DiemCoTam,
        No3PhanVung, NoToCoTam,
        NoToCoTam2, No1DiemCoTam2,
        NoTo1DiemDau, NoTo,
        No4Diem, No3PhanVung1Diem,
        NoTo, No2PhanVung
};
const uint8_t StateCotTabe[] = {
        STATE_COT_BAN1, STATE_COT_BAN1,
        STATE_COT_BAN2, STATE_COT_BAN1,
        STATE_COT_BAN1, STATE_COT_BAN2,
        STATE_COT_BAN1, STATE_COT_BAN3,
        STATE_COT_BAN3, STATE_COT_BAN1,
        STATE_COT_BAN2, STATE_COT_BAN1,
        STATE_COT_BAN4, STATE_COT_BAN1,
        STATE_COT_BAN1, STATE_COT_BAN1,
        STATE_COT_BAN5, STATE_COT_BAN1
};        
const uint8_t SolanNoTabe[] = {
    1, 1, 1, 1, 1, 1, 1, 4,
    1, 1, 2, 1, 1, 1, 1, 1,
    3, 1, 2, 2, 2, 1, 1, 1,
    1, 1, 1, 1, 2, 3, 1, 1};
TIA_TYPE_EFFECT TypeTiaEffect = NoTo;
TYPE_GIAT TypeGiat = GIAT_DON_DIEM;
STATE_COT CotState = STATE_COT_OFF;
STATE_TIA TiaState = STATE_TIA_OFF;
TYPE_TAT_TIA TatTia = TAT_NGAY;
MODE_RUN ModeRun = RUN_EFFECT;
uint32_t TimeCot = 0, TimeTia = 0;
uint8_t SoLanNo = 0, SoLanGiat = 0, SystemTick = 0, IndexGiat = 0;
uint32_t CountToUpDate = 0;
int IndexLedCot, IndexLedTia = 0, IndexNo;
uint8_t IndexRepeat = 0;


/******************************* DISPLAY FUNCTION ***********************************/
void FIREWORK_FillColor(Color_RGB_t Color, uint32_t Size) {
    for(uint32_t idx_data = 0; idx_data < Size; idx_data++) {
        DataLeds[idx_data] = Color;
    }
    if(led_driver) {
        led_driver((Color_RGB_t *)DataLeds, Size, MAX_CHIP_SERI);
    }
}
void FIREWORK_DisplayData(Color_RGB_t *Color, uint16_t MaxLeds) {
    if(MaxLeds > MAX_CHIP_SERI)
        MaxLeds = MAX_CHIP_SERI;

    if(led_driver) {
        led_driver((Color_RGB_t *)DataLeds, MaxLeds, DataSaveRam.SizeCot);
    }
}
/************************************** - BASIC FUNCTION - **************************************/
void firework_set_num_led_fire(uint16_t n_led) {
    if(n_led < FIREWORK_MIN_LED_COT) {
        n_led = FIREWORK_MIN_LED_COT;
    }
    if(n_led > FIREWORK_MAX_LED_COT) {
        n_led = FIREWORK_MAX_LED_COT;        
    }
    DataSaveRam.SizeCot = n_led;
}
void firework_set_num_led_expl(uint16_t n_led) {
    if(n_led < FIREWORK_MIN_LED_TIA) {
        n_led = FIREWORK_MIN_LED_TIA;
    }
    if(n_led > FIREWORK_MAX_LED_TIA) {
        n_led = FIREWORK_MAX_LED_TIA;        
    }
    DataSaveRam.SizeTia = n_led;
}
void firework_set_spd_led_fire(uint16_t spd) {
    if(spd < FIREWORK_MIN_SPD_COT) {
        spd = FIREWORK_MIN_SPD_COT;
    }
    if(spd > FIREWORK_MAX_SPD_COT) {
        spd = FIREWORK_MAX_SPD_COT;        
    }
    DataSaveRam.SpeedCot = spd;
}
void firework_set_spd_led_expl(uint16_t spd) {
    if(spd < FIREWORK_MIN_SPD_TIA) {
        spd = FIREWORK_MIN_SPD_TIA;
    }
    if(spd > FIREWORK_MAX_SPD_TIA) {
        spd = FIREWORK_MAX_SPD_TIA;        
    }
    DataSaveRam.SpeedTia = spd;
}
void firework_set_type_led(uint8_t idx_type) {
    idx_type %= LED_IDX_MAX;
    DataSaveRam.LedType = idx_type;
    if(DataSaveRam.LedType == LED_6803) {
        led_driver = lpd6803_update;
    }
    else {
        led_driver = ucs1903_update;
    }
}
void firework_set_state_effect(bool stt) {
    Firework_State.Effect = stt;
}
void firework_set_state_sound(bool stt) {
    Firework_State.Sound = stt;
}
bool firework_get_state_effect() {
    return Firework_State.Effect;
}
bool firework_get_state_sound() {
    return Firework_State.Sound;
}
void FIREWORK_GetRandomBangSound() {
    uint8_t Value = (rand() % 3);
    switch(Value) {
        case 0: {
            audio_gen_wav_file(&wav_file, "/bang4.wav");
            break;
        }
        case 1: {
            audio_gen_wav_file(&wav_file, "/bang5.wav");
            break;
        }
        case 2: {
            audio_gen_wav_file(&wav_file, "/bang8.wav");
            break;
        }
    }
}
uint32_t FIREWORK_GetRandomMinMax(uint32_t Min, uint32_t Max)
{
    uint32_t Value = Min + rand() % (Max - Min);
    return Value;
}
void FIREWORK_UpdatePixel(void *Data, Color_RGB_t Color, uint16_t IndexPosition)
{
    Color_RGB_t *Pointer = (Color_RGB_t *)Data;
    Pointer += IndexPosition;
    *Pointer = Color;
}
void FIREWORK_PutLed(void *Data, void *DataPut, uint16_t SizePut, uint16_t MaxSize, int IndexPosition)
{
    uint16_t Index;
    Color_RGB_t *Pointer = (Color_RGB_t *)Data;
    Color_RGB_t *PointColor = (Color_RGB_t *)DataPut;
    for (Index = 0; Index < SizePut; Index++)
    {
        if ((IndexPosition >= 0) & (IndexPosition < MaxSize))
        {
            Pointer[IndexPosition] = *PointColor;
        }
        IndexPosition++;
        PointColor++;
    }
}
void FIREWORK_FillBuffer(void *Data, Color_RGB_t Color, uint16_t Size)
{
    Color_RGB_t *PointColor = (Color_RGB_t *)Data;
    while (Size--)
    {
        *PointColor++ = Color;
    }
}
void FIREWORK_ShiftLed(void *Data, uint16_t Size, Color_RGB_t Color, uint8_t DirectShift)
{
    Color_RGB_t *PointColor = (Color_RGB_t *)Data;
    Size--;
    if (DirectShift)
    {
        PointColor += Size;
        while (Size--)
        {
            *PointColor = *(PointColor - 1);
            PointColor--;
        }
    }
    else
    {
        while (Size--)
        {
            *PointColor = *(PointColor + 1);
            PointColor++;
        }
    }
    *PointColor = Color;
}
void FIREWORK_ReduceLight(void *Data, uint16_t Size, float BrightRatio)
{
    Color_RGB_t *PointColor = (Color_RGB_t *)Data;
    while (Size--)
    {
        PointColor->R = (uint8_t)(PointColor->R * BrightRatio);
        PointColor->G = (uint8_t)(PointColor->G * BrightRatio);
        PointColor->B = (uint8_t)(PointColor->B * BrightRatio);
        PointColor++;
    }
}
/***************************************** PROCESS FUNCTION **************************************/
void FIREWORK_RandomDataPut(void *DataPut, uint16_t SizePut0, Color_RGB_t *ColorFix, uint8_t Ratio)
{
    uint16_t i;
    float Abc = 1.1f;
    Color_RGB_t ColorTemp;
    if (ColorFix == NULL)
        ColorTemp = COLOR_BY_ID(FIREWORK_GetRandomMinMax(COLOR_RED_ID, COLOR_WHITE_ID));
    else
        ColorTemp = *ColorFix;
    Color_RGB_t *PointColor = (Color_RGB_t *)DataPut;
    Abc += (float)(DataSaveRam.SizeTia + 1 - SizePut0) / DataSaveRam.SizeTia;
    for (i = 0; i < SizePut0; i++)
    {
        //     PointColor->R = (uint8_t)pow(ColorTemp.R/Ratio, (float)(i+1)/SizePut0);
        //     PointColor->G = (uint8_t)pow(ColorTemp.G/Ratio, (float)(i+1)/SizePut0);
        //     PointColor->B = (uint8_t)pow(ColorTemp.B/Ratio, (float)(i+1)/SizePut0);
        //     PointColor++;
        PointColor->R = (uint8_t)(ColorTemp.R / (Ratio * Bits_Int_Pow(Abc, SizePut0 - 1 - i)));
        PointColor->G = (uint8_t)(ColorTemp.G / (Ratio * Bits_Int_Pow(Abc, SizePut0 - 1 - i)));
        PointColor->B = (uint8_t)(ColorTemp.B / (Ratio * Bits_Int_Pow(Abc, SizePut0 - 1 - i)));
        PointColor++;
    }
}
void FIREWROK_NewTia(TIA_TYPE_EFFECT TiaEffect, uint8_t ResetLanNo, Color_RGB_t *ColorFix)
{
    memset(&ColorTia, 0, sizeof(DATA_PUT));
    switch (TiaEffect)
    {
    case NoTo:
        ColorTia.SizePut0 = DataSaveRam.SizeTia;
        FIREWORK_RandomDataPut(ColorTia.DataPut, ColorTia.SizePut0, ColorFix, 1);
        if (ResetLanNo)
            SoLanNo = SolanNoTabe[FIREWORK_GetRandomMinMax(0, sizeof(SolanNoTabe))];
        IndexLedTia = -DataSaveRam.SizeTia;
        IndexRepeat = DataSaveRam.SizeTia - 2;
        FIREWORK_GetRandomBangSound();
        break;
    case No3PhanVung1Diem:
        FIREWORK_RandomDataPut(&ColorTia.DataPut[DataSaveRam.SizeTia / 3], 1, NULL, 1);
        FIREWORK_RandomDataPut(&ColorTia.DataPut[DataSaveRam.SizeTia * 2 / 3], 1, NULL, 1);
        FIREWORK_RandomDataPut(&ColorTia.DataPut[DataSaveRam.SizeTia - 1], 1, ColorFix, 1);
        ColorTia.SizePut0 = DataSaveRam.SizeTia;
        if (ResetLanNo)
            SoLanNo = FIREWORK_GetRandomMinMax(1, 3);
        IndexLedTia = -DataSaveRam.SizeTia;
        IndexRepeat = 1;
        FIREWORK_GetRandomBangSound();
        break;
    case NoTo1DiemDau:
        FIREWORK_RandomDataPut(ColorTia.DataPut, DataSaveRam.SizeTia - 1, ColorFix, 1);
        FIREWORK_RandomDataPut(&ColorTia.DataPut[DataSaveRam.SizeTia - 1], 1, NULL, 1);
        ColorTia.SizePut0 = DataSaveRam.SizeTia;
        if (ResetLanNo)
            SoLanNo = SolanNoTabe[FIREWORK_GetRandomMinMax(0, sizeof(SolanNoTabe))];
        IndexLedTia = -DataSaveRam.SizeTia;
        IndexRepeat = DataSaveRam.SizeTia - 2;
        FIREWORK_GetRandomBangSound();
        break;
    case NoToCoTam:
    case NoToCoTam2:
        ColorTia.SizePut0 = FIREWORK_GetRandomMinMax(1, 4);
        FIREWORK_RandomDataPut(ColorTia.DataPut, ColorTia.SizePut0, ColorFix, 2);
        ColorTia.SizePut1 = DataSaveRam.SizeTia - ColorTia.SizePut0;
        FIREWORK_RandomDataPut(&ColorTia.DataPut[ColorTia.SizePut0], ColorTia.SizePut1, NULL, 1);
        if (ResetLanNo)
            SoLanNo = SolanNoTabe[FIREWORK_GetRandomMinMax(0, sizeof(SolanNoTabe))];
        if (TiaEffect == NoToCoTam2)
        {
            IndexLedTia = -ColorTia.SizePut0;
            IndexRepeat = DataSaveRam.SizeTia + ColorTia.SizePut0;
        }
        else
        {
            IndexLedTia = -DataSaveRam.SizeTia;
            IndexRepeat = DataSaveRam.SizeTia - ColorTia.SizePut0;
        }
        FIREWORK_GetRandomBangSound();
        break;
    case No1Diem:
        ColorTia.SizePut0 = 1;
        FIREWORK_RandomDataPut(ColorTia.DataPut, ColorTia.SizePut0, ColorFix, 1);
        if (ResetLanNo)
            SoLanNo = 6 - SolanNoTabe[FIREWORK_GetRandomMinMax(0, sizeof(SolanNoTabe))];
        IndexLedTia = -1;
        IndexRepeat = DataSaveRam.SizeTia - 1;
        FIREWORK_GetRandomBangSound();
        break;
    case No1DiemCoTam:
    case No1DiemCoTam2:
        ColorTia.SizePut0 = FIREWORK_GetRandomMinMax(1, 4);
        FIREWORK_RandomDataPut(ColorTia.DataPut, ColorTia.SizePut0, ColorFix, 2);
        ColorTia.SizePut1 = 1;
        FIREWORK_RandomDataPut(&ColorTia.DataPut[ColorTia.SizePut0], ColorTia.SizePut1, NULL, 1);
        if (ResetLanNo)
            SoLanNo = 6 - SolanNoTabe[FIREWORK_GetRandomMinMax(0, sizeof(SolanNoTabe))];
        if (TiaEffect == No1DiemCoTam2)
        {
            IndexLedTia = -ColorTia.SizePut0;
            IndexRepeat = DataSaveRam.SizeTia - 1;
        }
        else
        {
            IndexLedTia = -ColorTia.SizePut0;
            IndexRepeat = DataSaveRam.SizeTia - 1;
        }
        FIREWORK_GetRandomBangSound();
        break;
    case No1DiemDienDay:
        ColorTia.SizePut1 = ColorTia.SizePut0 = DataSaveRam.SizeTia;
        FIREWORK_RandomDataPut(ColorTia.DataPut, ColorTia.SizePut0, ColorFix, 1);
        FIREWORK_GetRandomBangSound();
        IndexLedTia = DataSaveRam.SizeTia;
        break;
    case No4Diem:
        ColorTia.SizePut0 = 4;
        FIREWORK_RandomDataPut(ColorTia.DataPut, ColorTia.SizePut0, ColorFix, 1);
        if (ResetLanNo)
            SoLanNo = FIREWORK_GetRandomMinMax(1, 4);
        FIREWORK_GetRandomBangSound();
        IndexLedTia = -ColorTia.SizePut0;
        IndexRepeat = DataSaveRam.SizeTia - 2;
        break;
    case No4DiemCoTam:
    case No4DiemCoTam2:
        ColorTia.SizePut0 = FIREWORK_GetRandomMinMax(1, 4);
        FIREWORK_RandomDataPut(ColorTia.DataPut, ColorTia.SizePut0, ColorFix, 2);
        ColorTia.SizePut1 = 4;
        FIREWORK_RandomDataPut(&ColorTia.DataPut[ColorTia.SizePut0], ColorTia.SizePut1, NULL, 1);
        if (ResetLanNo)
            SoLanNo = FIREWORK_GetRandomMinMax(1, 4);
        if (TiaEffect == No4DiemCoTam2)
        {
            IndexRepeat = DataSaveRam.SizeTia - 2;
            IndexLedTia = -ColorTia.SizePut0;
        }
        else
        {
            IndexLedTia = -ColorTia.SizePut0 - 2;
            IndexRepeat = DataSaveRam.SizeTia - 2;
        }
        FIREWORK_GetRandomBangSound();
        break;
    case No2PhanVung:
        ColorTia.SizePut0 = DataSaveRam.SizeTia / 2;
        FIREWORK_RandomDataPut(ColorTia.DataPut, ColorTia.SizePut0, ColorFix, 1);
        ColorTia.SizePut1 = DataSaveRam.SizeTia - ColorTia.SizePut0;
        FIREWORK_RandomDataPut(&ColorTia.DataPut[ColorTia.SizePut0], ColorTia.SizePut1, NULL, 1);
        if (ResetLanNo)
            SoLanNo = FIREWORK_GetRandomMinMax(1, 3);
        IndexLedTia = -ColorTia.SizePut0;
        IndexRepeat = ColorTia.SizePut0 - 1;
        FIREWORK_GetRandomBangSound();
        break;
    case No3PhanVung:
        ColorTia.SizePut0 = DataSaveRam.SizeTia / 3;
        FIREWORK_RandomDataPut(ColorTia.DataPut, ColorTia.SizePut0, ColorFix, 1);
        ColorTia.SizePut1 = DataSaveRam.SizeTia / 3;
        FIREWORK_RandomDataPut(&ColorTia.DataPut[ColorTia.SizePut0], ColorTia.SizePut1, NULL, 1);
        ColorTia.SizePut2 = DataSaveRam.SizeTia - ColorTia.SizePut0 - ColorTia.SizePut1;
        FIREWORK_RandomDataPut(&ColorTia.DataPut[2 * ColorTia.SizePut0], ColorTia.SizePut2, NULL, 1);
        if (ResetLanNo)
            SoLanNo = FIREWORK_GetRandomMinMax(1, 3);
        IndexLedTia = -ColorTia.SizePut0;
        IndexRepeat = ColorTia.SizePut0 - 1;
        FIREWORK_GetRandomBangSound();
        break;
    };
}
void FIREWORK_GiatTia(void)
{
    TiaState = STATE_TIA_GIAT;
    SoLanGiat = FIREWORK_GetRandomMinMax(24, 48);
    TypeGiat = (TYPE_GIAT)FIREWORK_GetRandomMinMax(0, 3);
    if (TypeGiat == GIAT_DON_DIEM)
    {
        ColorTia.SizePut0 = 1;
        FIREWORK_RandomDataPut(ColorTia.DataPut, ColorTia.SizePut0, NULL, 2);
    }
    else if (TypeGiat == GIAT_DA_DIEM)
    {
        ColorTia.SizePut0 = 4;
        for (uint8_t Index = 0; Index < ColorTia.SizePut0; Index++)
            FIREWORK_RandomDataPut(&ColorTia.DataPut[Index], 1, NULL, 2);
    }
    else if (TypeGiat == GIAT_CO_TAM)
    {
        ColorTia.SizePut0 = FIREWORK_GetRandomMinMax(1, 4);
        FIREWORK_RandomDataPut(ColorTia.DataPut, ColorTia.SizePut0, NULL, 2);
        FIREWORK_RandomDataPut(&ColorTia.DataPut[ColorTia.SizePut0], ColorTia.SizePut0, NULL, 2);
    }
    IndexGiat = 0;
    // FIREWORK_SetDMADACChannel((void *)TiengKetThuc, sizeof(TiengKetThuc), 8000);
    audio_gen_wav_file(&wav_file, "/crack.wav");
}
void FIREWORK_TatTia(void)
{
    TiaState = STATE_TIA_OFF;
    TatTia = (TYPE_TAT_TIA)FIREWORK_GetRandomMinMax(0, 4);
}
/*********************************** TEST FUNCTION *********************************************/
void FIREWORK_Test(void) {
    delay_ms(1000);
    audio_gen_wav_file(&wav_file, "/whistle.wav");
}
void FIREWORK_Loop() {
    // bool state_sound_p = *((bool *)arg);
    uint32_t milisecond = user_millis();
    /**
     * @brief ----------> Giải mã âm thanh 
     */
    if(Firework_State.Sound == true) {
        if(audio_gen_wav_is_running(&wav_file) == audio_gen_wav_running){
            audio_gen_wav_loop(&wav_file);
        }
    }
    /**
     * @brief ----------> Chạy hiệu ứng
     */
    if (ModeRun == RUN_EFFECT)
    {
        if (( milisecond - TimeCot) >= (FIREWORK_MAX_SPD_COT - DataSaveRam.SpeedCot))
        {
            TimeCot = milisecond;
            if (CotState == STATE_COT_OFF)
            {
                if (TiaState == STATE_TIA_OFF) // Chờ đến khi cho phép bắn tia mới
                {
                    CotState = (STATE_COT)StateCotTabe[FIREWORK_GetRandomMinMax(1, sizeof(StateCotTabe))];
                    ColorCot.SizePut0 = FIREWORK_GetRandomMinMax(6, 10);
                    FIREWORK_RandomDataPut(ColorCot.DataPut, ColorCot.SizePut0, NULL, 1);
                    IndexLedCot = -ColorCot.SizePut0 + 1;
                    IndexNo = 0;
                    audio_gen_wav_file(&wav_file, "/whistle.wav");
                }
            }
            else // Dịch cột
            {
                ++IndexLedCot;
                ++IndexNo;
                FIREWORK_ShiftLed(DataLeds, DataSaveRam.SizeCot, COLOR_BLACK, 1);
                if (IndexLedCot <= 0)
                    FIREWORK_PutLed(DataLeds, ColorCot.DataPut, ColorCot.SizePut0, DataSaveRam.SizeCot, IndexLedCot);
                if (CotState != STATE_COT_BAN1)
                {
                    if (IndexLedCot == (ColorCot.SizePut0 + 2 * DataSaveRam.SizeTia)) // Thêm 1 tia tiếp theo
                    {
                        CotState--;
                        IndexLedCot = -ColorCot.SizePut0 + 1;
                        FIREWORK_RandomDataPut(ColorCot.DataPut, ColorCot.SizePut0, NULL, 1);
                        audio_gen_wav_file(&wav_file, "/whistle.wav");
                    }
                }
                else
                {
                    if (IndexLedCot >= DataSaveRam.SizeCot + ColorCot.SizePut0)
                    {
                        CotState = STATE_COT_OFF;
                    }
                }
                if (IndexNo == DataSaveRam.SizeCot - 1)
                {
                    IndexNo -= 2 * ColorCot.SizePut0 + 2 * DataSaveRam.SizeTia - 1;
                    //            NoTo,
                    //            NoTo1DiemDau,
                    //            No3PhanVung1Diem,
                    //            NoToCoTam,
                    //            No1Diem,
                    //            No1DiemCoTam,
                    //            No1DiemDienDay,
                    //            No4Diem,
                    //            No4DiemCoTam,
                    //            No2PhanVung,
                    //            No3PhanVung,
                    //            NoToCoTam2,
                    //            No1DiemCoTam2,
                    //            No4DiemCoTam2
                    TypeTiaEffect = (TIA_TYPE_EFFECT)TypeTiaTable[FIREWORK_GetRandomMinMax(0, sizeof(TypeTiaTable))];
                    SoLanNo = (CotState != STATE_COT_BAN1) ? 1 : 0;
                    TimeTia = DataSaveRam.SpeedTia;
                    FIREWROK_NewTia(TypeTiaEffect, !SoLanNo, &DataLeds[DataSaveRam.SizeCot - 1]);
                    TiaState = STATE_TIA_NO;
                }
            }
        }
        if ((milisecond - TimeTia) >= (FIREWORK_MAX_SPD_TIA - DataSaveRam.SpeedTia))
        {
            TimeTia = milisecond;
            if (TiaState == STATE_TIA_NO)
            {
                switch (TypeTiaEffect)
                {
                case NoTo:
                case No1Diem:
                case No4Diem:
                case NoTo1DiemDau:
                case No3PhanVung1Diem:
                    FIREWORK_ShiftLed(&DataLeds[DataSaveRam.SizeCot], DataSaveRam.SizeTia, COLOR_BLACK, 1);
                    if (IndexLedTia <= 0)
                    {
                        FIREWORK_PutLed(&DataLeds[DataSaveRam.SizeCot], ColorTia.DataPut, ColorTia.SizePut0, DataSaveRam.SizeTia, IndexLedTia);
                    }
                    if (IndexLedTia == IndexRepeat)
                    {
                        if (SoLanNo-- != 0)
                        {
                            FIREWROK_NewTia(TypeTiaEffect, 0, NULL);
                        }
                            else // Dung No
                        {
                            if (FIREWORK_GetRandomMinMax(0, 2) == 0)
                            {
                                FIREWORK_TatTia();
                            }
                            else
                            {
                                FIREWORK_GiatTia();
                            }
                        }
                    }
                    break;
                case NoToCoTam:
                case No1DiemCoTam:
                case No4DiemCoTam:
                    FIREWORK_ShiftLed(&DataLeds[DataSaveRam.SizeCot + ColorTia.SizePut0], DataSaveRam.SizeTia - ColorTia.SizePut0, COLOR_BLACK, 1);
                    if (IndexLedTia <= 0)
                    {
                        FIREWORK_PutLed(&DataLeds[DataSaveRam.SizeCot], ColorTia.DataPut, ColorTia.SizePut0 + ColorTia.SizePut1, DataSaveRam.SizeTia, IndexLedTia);
                    }
                    if (IndexLedTia == IndexRepeat)
                    {
                        if (--SoLanNo != 0)
                        {
                            FIREWROK_NewTia(TypeTiaEffect, 0, NULL);
                        }
                        else // Dung No
                        {
                            if (FIREWORK_GetRandomMinMax(0, 2) == 0)
                            {
                                FIREWORK_TatTia();
                            }
                            else
                            {
                                FIREWORK_GiatTia();
                            }
                        }
                    }
                    break;
                case NoToCoTam2:
                case No1DiemCoTam2:
                case No4DiemCoTam2:
                    FIREWORK_ShiftLed(&DataLeds[DataSaveRam.SizeCot + ColorTia.SizePut0], DataSaveRam.SizeTia - ColorTia.SizePut0, COLOR_BLACK, 1);
                    if (IndexLedTia <= 0)
                    {
                        FIREWORK_PutLed(&DataLeds[DataSaveRam.SizeCot], ColorTia.DataPut, ColorTia.SizePut0, DataSaveRam.SizeTia, IndexLedTia);
                    }
                    else if (IndexLedTia <= ColorTia.SizePut0 + ColorTia.SizePut1)
                    {
                        FIREWORK_PutLed(&DataLeds[DataSaveRam.SizeCot + ColorTia.SizePut0], &ColorTia.DataPut[ColorTia.SizePut0], ColorTia.SizePut0 + ColorTia.SizePut1, DataSaveRam.SizeTia - ColorTia.SizePut0, IndexLedTia - ColorTia.SizePut0 - ColorTia.SizePut1);
                    }
                    if (IndexLedTia == IndexRepeat)
                    {
                        if (--SoLanNo != 0)
                        {
                            FIREWROK_NewTia(TypeTiaEffect, 0, NULL);
                        }
                        else // Dung No
                        {
                            if (FIREWORK_GetRandomMinMax(0, 2) == 0)
                            {
                                FIREWORK_TatTia();
                            }
                            else
                            {
                                FIREWORK_GiatTia();
                            }
                        }
                    }
                    break;
                case No2PhanVung:
                    if (IndexLedTia & 0x1) // Giảm 1/2 tốc độ
                    {
                        FIREWORK_ShiftLed(&DataLeds[DataSaveRam.SizeCot], ColorTia.SizePut0, COLOR_BLACK, 1);
                        FIREWORK_ShiftLed(&DataLeds[DataSaveRam.SizeCot + ColorTia.SizePut0], DataSaveRam.SizeTia - ColorTia.SizePut0, COLOR_BLACK, 1);
                    }
                    if (IndexLedTia <= 0)
                    {
                        FIREWORK_PutLed(&DataLeds[DataSaveRam.SizeCot], ColorTia.DataPut, ColorTia.SizePut0, ColorTia.SizePut0, IndexLedTia);
                        FIREWORK_PutLed(&DataLeds[DataSaveRam.SizeCot + ColorTia.SizePut0], &ColorTia.DataPut[ColorTia.SizePut0], ColorTia.SizePut0, DataSaveRam.SizeTia - ColorTia.SizePut0, IndexLedTia);
                    }
                    if (IndexLedTia == (2 * IndexRepeat))
                    {
                        if (--SoLanNo != 0)
                        {
                            FIREWROK_NewTia(TypeTiaEffect, 0, NULL);
                        }
                        else // Dung No
                        {
                            if (FIREWORK_GetRandomMinMax(0, 2) == 0)
                            {
                                FIREWORK_TatTia();
                            }
                            else
                            {
                                FIREWORK_GiatTia();
                            }
                        }
                    }
                    break;
                case No3PhanVung:
                    if ((IndexLedTia & 0x3) == 0) // Giảm 1/4 tốc độ
                    {
                        FIREWORK_ShiftLed(&DataLeds[DataSaveRam.SizeCot], ColorTia.SizePut0, COLOR_BLACK, 1);
                        FIREWORK_ShiftLed(&DataLeds[DataSaveRam.SizeCot + ColorTia.SizePut0], DataSaveRam.SizeTia - ColorTia.SizePut0, COLOR_BLACK, 1);
                        FIREWORK_ShiftLed(&DataLeds[DataSaveRam.SizeCot + 2 * ColorTia.SizePut0], DataSaveRam.SizeTia - 2 * ColorTia.SizePut0, COLOR_BLACK, 1);
                    }
                    if (IndexLedTia <= 0)
                    {
                        FIREWORK_PutLed(&DataLeds[DataSaveRam.SizeCot], ColorTia.DataPut, ColorTia.SizePut0, ColorTia.SizePut0, IndexLedTia);
                        FIREWORK_PutLed(&DataLeds[DataSaveRam.SizeCot + ColorTia.SizePut0], &ColorTia.DataPut[ColorTia.SizePut0], ColorTia.SizePut0, ColorTia.SizePut0, IndexLedTia);
                        FIREWORK_PutLed(&DataLeds[DataSaveRam.SizeCot + 2 * ColorTia.SizePut0], &ColorTia.DataPut[2 * ColorTia.SizePut0], DataSaveRam.SizeTia - 2 * ColorTia.SizePut0, DataSaveRam.SizeTia - 2 * ColorTia.SizePut0, IndexLedTia);
                    }
                    if (IndexLedTia == (4 * IndexRepeat))
                    {
                        if (--SoLanNo != 0)
                        {
                            FIREWROK_NewTia(TypeTiaEffect, 0, NULL);
                        }
                        else // Dung No
                        {
                            if (FIREWORK_GetRandomMinMax(0, 2) == 0)
                            {
                                FIREWORK_TatTia();
                            }
                            else
                            {
                                FIREWORK_GiatTia();
                            }
                        }
                    }
                    break;
                case No1DiemDienDay:
                    FIREWORK_ShiftLed(&DataLeds[DataSaveRam.SizeCot], ColorTia.SizePut1, COLOR_BLACK, 1);
                    if ((IndexLedTia + 1) >= ColorTia.SizePut1)
                    {
                        if (--ColorTia.SizePut1 >= DataSaveRam.SizeTia / 2)
                        {
                            IndexLedTia = 0;
                            FIREWORK_UpdatePixel(&DataLeds[DataSaveRam.SizeCot], ColorTia.DataPut[ColorTia.SizePut1], 0);
                            FIREWORK_GetRandomBangSound();
                        }
                        else // Dung No
                        {
                            FIREWORK_TatTia();
                        }
                    }
                    break;
                }
            }
            else if (TiaState == STATE_TIA_GIAT)
            {
                FIREWORK_FillBuffer(&DataLeds[DataSaveRam.SizeCot], COLOR_BLACK, DataSaveRam.SizeTia);
                switch (TypeGiat)
                {
                    case GIAT_DON_DIEM:
                        if ((IndexGiat & 0x7) >= 4)
                        {
                            if ((IndexGiat & 0x3) >= 2)
                                FIREWORK_PutLed(&DataLeds[DataSaveRam.SizeCot + DataSaveRam.SizeTia / 2 - 1], ColorTia.DataPut, 1, 1, 0);
                        }
                        else
                        {
                            if ((IndexGiat & 0x3) >= 2)
                                FIREWORK_PutLed(&DataLeds[DataSaveRam.SizeCot + DataSaveRam.SizeTia - 1], ColorTia.DataPut, 1, 1, 0);
                        }
                        break;
                    case GIAT_DA_DIEM:
                        if ((IndexGiat & 0x7) >= 4)
                        {
                            if ((IndexGiat & 0x3) >= 2)
                            {
                                FIREWORK_PutLed(&DataLeds[DataSaveRam.SizeCot + DataSaveRam.SizeTia / 4 - 1], &ColorTia.DataPut[1], 1, 1, 0);
                                FIREWORK_PutLed(&DataLeds[DataSaveRam.SizeCot + DataSaveRam.SizeTia * 3 / 4 - 1], &ColorTia.DataPut[3], 1, 1, 0);
                            }
                        }
                        else
                        {
                            if ((IndexGiat & 0x3) >= 2)
                            {
                                FIREWORK_PutLed(&DataLeds[DataSaveRam.SizeCot + DataSaveRam.SizeTia - 1], &ColorTia.DataPut[0], 1, 1, 0);
                                FIREWORK_PutLed(&DataLeds[DataSaveRam.SizeCot + DataSaveRam.SizeTia / 2 - 1], &ColorTia.DataPut[2], 1, 1, 0);
                            }
                        }
                        break;
                    case GIAT_CO_TAM:
                        if ((IndexGiat & 0x7) >= 4)
                        {
                            if ((IndexGiat & 0x3) >= 2)
                                FIREWORK_PutLed(&DataLeds[DataSaveRam.SizeCot + DataSaveRam.SizeTia / 2 - 1], &ColorTia.DataPut[ColorTia.SizePut0], 1, 1, 0);
                        }
                        else
                        {
                            if ((IndexGiat & 0x3) >= 2)
                                FIREWORK_PutLed(&DataLeds[DataSaveRam.SizeCot + DataSaveRam.SizeTia - 1], &ColorTia.DataPut[ColorTia.SizePut0], 1, 1, 0);
                        }
                        if ((IndexGiat & 0x3) >= 2)
                            FIREWORK_PutLed(&DataLeds[DataSaveRam.SizeCot], ColorTia.DataPut, ColorTia.SizePut0, DataSaveRam.SizeTia, 0);
                        break;
                }
                if (++IndexGiat >= SoLanGiat)
                {
                    FIREWORK_TatTia();
                }
            }
            else
            {
                if (TatTia == TAT_NGAY)
                {
                    FIREWORK_FillBuffer(&DataLeds[DataSaveRam.SizeCot], COLOR_BLACK, DataSaveRam.SizeTia);
                }
                else if (TatTia == TAT_DAN)
                {
                    if ((IndexLedTia & 0x3) == 0) // Giảm 1/4 tốc độ
                        FIREWORK_ReduceLight(&DataLeds[DataSaveRam.SizeCot], DataSaveRam.SizeTia, 0.75f);
                }
                else if (TatTia == DICH_TRAI_DAN)
                {
                    if ((IndexLedTia & 0x3) == 0) // Giảm 1/4 tốc độ
                        FIREWORK_ShiftLed(&DataLeds[DataSaveRam.SizeCot], DataSaveRam.SizeTia, COLOR_BLACK, 1);
                }
            }
            ++IndexLedTia;
        }
        if ((milisecond - CountToUpDate ) > TIME_RUN) {
            CountToUpDate = milisecond;
            if(Firework_State.Effect == true) {
                FIREWORK_DisplayData(DataLeds, DataSaveRam.SizeCot + DataSaveRam.SizeTia);
            }
            else {
                FIREWORK_FillColor(COLOR_BLACK, MAX_CHIP_SERI);
            }
        }
    }
}
void FIREWORK_Test_FullLED() {
    static uint32_t reload_led;
    static uint32_t changeColor;
    static uint8_t currentColorID = 0;
    Color_RGB_t currentColor = COLOR_RED;
    uint32_t currentTick = user_millis();
    /* Đổi màu */
    if((currentTick - changeColor) >= 1000) {
        changeColor = currentTick;
        currentColorID = (currentColorID + 1) % 4;
    }
    switch(currentColorID) {
        case 0 : {      // Red
            currentColor = COLOR_RED;
            break;
        }
        case 1 : {      // Green
            currentColor = COLOR_GREEN;
            break;
        }
        case 2 : {      // Blue
            currentColor = COLOR_BLUE;
            break;
        }
        case 3 : {      // White
            currentColor = COLOR_WHITE;
            break;
        }
    }
    /* Refresh LED */
    if((currentTick - reload_led) >= 50) {
        reload_led = currentTick;
        /* In ra số LED cột hiện tại bằng màu trắng */
        for(uint16_t idx_led = 0; idx_led < MAX_CHIP_SERI; idx_led++) {
            DataLeds[idx_led] = currentColor;
        }
        /* Đẩy ra LED */
        FIREWORK_DisplayData(DataLeds, MAX_CHIP_SERI);
    }
}
void FIREWORK_Test_nCot() {
    static uint32_t reload_led;

    if((user_millis() - reload_led) >= 100) {
        reload_led = user_millis();
        /* In ra số LED cột hiện tại bằng màu trắng */
        for(uint16_t idx_led = 0; idx_led < MAX_CHIP_SERI; idx_led++) {
            if(idx_led < DataSaveRam.SizeCot) {
                DataLeds[idx_led] = COLOR_GREEN;
            }
            else {
                DataLeds[idx_led] = (Color_RGB_t){0, 0, 0};            
            }
        }
        /* Đẩy ra LED */
        FIREWORK_DisplayData(DataLeds, MAX_CHIP_SERI);
    }
}
void FIREWORK_Test_nTia() {
    static uint32_t reload_led;

    if((user_millis() - reload_led) >= 100) {
        reload_led = user_millis();
        /* In ra số LED tia hiện tại bằng màu Đỏ */
        for(uint16_t idx_led = 0; (DataSaveRam.SizeCot + idx_led) < MAX_CHIP_SERI; idx_led++) {
            if(idx_led < DataSaveRam.SizeTia) {
                DataLeds[DataSaveRam.SizeCot + idx_led] = COLOR_RED;
            }
            else {
                DataLeds[DataSaveRam.SizeCot + idx_led] = (Color_RGB_t){0, 0, 0};            
            }
        }  
        /* Đẩy ra LED */
        FIREWORK_DisplayData(DataLeds, MAX_CHIP_SERI);
    }
}
void FIREWORK_Test_Speed_Cot() {
    static uint32_t reload_led;
    static int16_t pos_led = -15;
    uint8_t color_temp = 0;
    uint16_t Meteor_Length = (DataSaveRam.SizeCot / 2);
    if(Meteor_Length == 0) Meteor_Length = 1;

    if(DataSaveRam.SpeedCot < FIREWORK_MIN_SPD_COT) DataSaveRam.SpeedCot = FIREWORK_MIN_SPD_COT;
    if(DataSaveRam.SpeedCot > FIREWORK_MAX_SPD_COT) DataSaveRam.SpeedCot = FIREWORK_MAX_SPD_COT;

    if((user_millis() - reload_led) >= (FIREWORK_MAX_SPD_COT - DataSaveRam.SpeedCot)) {
        reload_led = user_millis();
        /* Tạo hiệu ứng sao băng */
        for(int16_t idx_led = 0; idx_led < Meteor_Length; idx_led++) {
            if(((pos_led + idx_led) >= 0) && ((pos_led + idx_led) < DataSaveRam.SizeCot)) {
                color_temp = ((uint16_t)idx_led * 186) / Meteor_Length;
                DataLeds[pos_led + idx_led] = (Color_RGB_t){0, color_temp, 0};
            }
        }
        /* Đẩy ra LED */
        FIREWORK_DisplayData(DataLeds, MAX_CHIP_SERI);
        if(pos_led > DataSaveRam.SizeCot) pos_led = -Meteor_Length;
        pos_led++;
    }
}
void FIREWORK_Test_Speed_Tia() {
    static uint32_t reload_led;
    static int16_t pos_led = 0;
    uint8_t color_temp = 0;
    uint16_t Meteor_Length = (DataSaveRam.SizeTia / 2);
    if(Meteor_Length == 0) Meteor_Length = 1;

    if(DataSaveRam.SpeedTia < FIREWORK_MIN_SPD_TIA) DataSaveRam.SpeedTia = FIREWORK_MIN_SPD_TIA;
    if(DataSaveRam.SpeedTia > FIREWORK_MAX_SPD_TIA) DataSaveRam.SpeedTia = FIREWORK_MAX_SPD_TIA;

    if((user_millis() - reload_led) >= (FIREWORK_MAX_SPD_TIA - DataSaveRam.SpeedTia)) {
        reload_led = user_millis();
        /* Tạo hiệu ứng sao băng */
        for(uint8_t idx_led = 0; idx_led < Meteor_Length; idx_led++) {
            if(((pos_led + idx_led) >= 0) && ((pos_led + idx_led) < DataSaveRam.SizeTia)) {
                color_temp = (idx_led * 186) / Meteor_Length;
                DataLeds[DataSaveRam.SizeCot + pos_led + idx_led] = (Color_RGB_t){color_temp, 0, 0};
            }
        }
        /* Đẩy ra LED */
        FIREWORK_DisplayData(DataLeds, MAX_CHIP_SERI);
        pos_led++;
        if(pos_led > DataSaveRam.SizeTia) pos_led = -Meteor_Length;
    }
}

/* ********************************** PUBLIC FUNCTION ******************************************** */
void FIREWORK_Init() {
    i2s_no_dac_init(&i2s_wav);
    audio_gen_wav_file(&wav_file, "/whistle.wav");
    audio_gen_wav_regist_drv_output(&wav_file, &i2s_wav);
    i2s_no_dac_setGain(&i2s_wav, 1);
}