/**
 * @file ezform.c
 * @author Tieu Tuan Bao (tieutuanbao@gmail.com)
 * @brief
 * @version 0.1
 * @date 2022-08-04
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "ezform.h"

volatile uint32_t __tick_ms_counter = 0;       /* Đưa biến này tăng vô hạn trong timer 1ms */

/* begin only esp8266 */
// static uint32_t micros_at_last_overflow_tick = 0;
// static uint32_t micros_overflow_count = 0;

// #define  MAGIC_1E3_wLO  0x4bc6a7f0    // LS part
// #define  MAGIC_1E3_wHI  0x00418937    // MS part, magic multiplier

// uint32_t user_millis(void) {
//     union {
//         uint64_t  q;     // Accumulator, 64-bit, little endian
//         uint32_t  a[2];  // ..........., 32-bit  segments
//     } acc;
//     acc.a[1] = 0;       // Zero high-acc
    
//     // Get usec system time, usec overflow counter
//     uint32_t  m = system_get_time();
//     uint32_t  c = micros_overflow_count +
//                     ((m < micros_at_last_overflow_tick) ? 1 : 0);

//     // (a) Init. low-acc with high-word of 1st product. The right-shift
//     //     falls on a byte boundary, hence is relatively quick.
    
//     acc.q  = ( (uint64_t)( m * (uint64_t)MAGIC_1E3_wLO ) >> 32 );

//     // (b) Offset sum, low-acc
//     acc.q += ( m * (uint64_t)MAGIC_1E3_wHI );

//     // (c) Offset sum, low-acc
//     acc.q += ( c * (uint64_t)MAGIC_1E3_wLO );

//     // (d) Truncated sum, high-acc
//     acc.a[1] += (uint32_t)( c * (uint64_t)MAGIC_1E3_wHI );
//     return ( __tick_ms_counter = acc.a[1] );  // Extract result, high-acc
// }
/* end only esp8266 */

uint32_t user_millis(void) {
    return __tick_ms_counter;
}

void delay_nop(uint32_t nnop) {
	while(nnop--){
		asm("nop");
	}
}
void delay_ms(uint32_t ms) {
    uint32_t delay_interval = user_millis();
    while((user_millis() - delay_interval) < ms);
}
void delay_us_nop(uint32_t us){    
    for(;us;us--){
        delay_nop(10);
    }
}


/**
 * @brief Khởi tạo software timer
 * 
 * @param Obj Con trỏ đến software timer
 * @param Compare Giá trị định thời
 * @param Callback Hàm callback khi có sự kiện tràn timer
 * @param Arg tham số bổ xung
 * @param Repeat lặp lại
 * @return true 
 * @return false 
 */
bool STimer_Start(STimer_t *Obj, uint32_t Compare, void (*Callback)(), void *Arg, bool Repeat) {
    if(Callback == 0) return false;
    Obj->Callback = Callback;
    Obj->Arg = Arg;
    Obj->Compare = Compare;
    Obj->Counter = user_millis();
    Obj->Repeat = Repeat;
    Obj->Enable = true;
    return true;
}
/**
 * @brief Dừng timer
 * 
 * @param STimerObj Con trỏ đến timer
 */
void STimer_Stop(STimer_t *Obj) {
    Obj->Callback = 0;
    Obj->Enable = false;
}
/**
 * @brief Hàm xử lý LED đơn
 * Đưa hàm này vào ngắt timer, tối thiểu 1ms 
 * @param sled_list danh sách led đơn
 */
Led_t *LedObj;
bool StateLed = false;

__attribute__((section(".text"))) void Application_Led_Handler(Application_t *App_Obj) {
    static uint32_t __CurrentTick = 0;
    for(uint8_t idx_led = 0; idx_led < App_Obj->Led.Count; idx_led++) {
        LedObj = (App_Obj->Led.Index[idx_led]);
        if(LedObj->Repeat != 0) {
            __CurrentTick = user_millis();
            /* Thời gian bật tắt */
            if((__CurrentTick - LedObj->Interval.On) < LedObj->Time.On) {
                StateLed = true;
            }
            else {
                if((__CurrentTick - LedObj->Interval.Off) < LedObj->Time.Off) {
                    StateLed = false;
                }
                else {
                    LedObj->Interval.On = __CurrentTick;
                    LedObj->Interval.Off = LedObj->Interval.On + LedObj->Time.On;
                    if(LedObj->Time.On > 0) StateLed = true;
                    else StateLed = false;
                    if(LedObj->Repeat > 0) 
                    {
                        LedObj->Repeat--;
                    }
                }
            }
            
            /* Điều khiển PWM */
            if((LedObj->DutyCounter++ < LedObj->Brightness) && (StateLed == true)) {
                LedObj->Output_Driver(true);
            }
            else {
                LedObj->Output_Driver(false);
            }
            /* Reset duty counter */
            if(LedObj->DutyCounter >= LED_DUTY_MAX) {
                LedObj->DutyCounter = 0;
            }
        }
    }
}

/**
 * @brief Hàm cấu hình led đơn
 * 
 * @param LedObj Con trỏ đến led đơn
 * @param Brightness Độ sáng
 * @param TimeOn_ms Thời gian sáng
 * @param time_off_ms Thời gian tắt
 * @param Repeat Lặp lại
 */
void Application_Led_Config(Led_t *LedObj, uint8_t Brightness, uint32_t TimeOn_ms, uint32_t TimeOff_ms, int8_t Repeat) {
    if(LedObj->Output_Driver == 0) return;
    LedObj->Brightness = Brightness;
    LedObj->Time.On = TimeOn_ms;
    LedObj->Time.Off = TimeOff_ms;
    LedObj->Repeat = Repeat;
}
/**
 * @brief Chuyển form
 * 
 * @param Obj Application đang chạy
 * @param Form Form tiếp theo
 */
void Application_Form_Switch(Application_t *Obj, Form_t *Form) {
    Obj->Form.Next = Form;
}
/**
 * @brief Thêm Timer vào chương trình
 * 
 * @param Obj 
 * @param STimer 
 */
void Application_Add_STimer(Application_t *Obj, STimer_t *STimer) {
    if(Obj->STimer.Count < STIMER_MAX) {
        Obj->STimer.Index[Obj->STimer.Count] = STimer;
        Obj->STimer.Count++;
    }
}
/**
 * @brief Thêm Led vào chương trình
 * 
 * @param Obj 
 * @param Led 
 */
void Application_Add_Led(Application_t *Obj, Led_t *Led) {
    if(Obj->Led.Count < LED_MAX) {
        Obj->Led.Index[Obj->Led.Count] = Led;
        Obj->Led.Count++;
    }
}
/**
 * @brief Thêm nút nhấn vào chương trình
 * 
 * @param Obj 
 * @param Key 
 */
void Application_Add_Key(Application_t *Obj, IO_Key_t *Key) {
    if(Obj->Key.Count < KEY_MAX) {
        Obj->Key.Index[Obj->Key.Count] = Key;
        Obj->Key.Count++;
    }
}
/**
 * @brief Chạy chương trình trong Loop
 * 
 * @param Obj 
 */
void Application_Run(Application_t *Obj) {
    static STimer_t *STimerObj;
    IO_Key_t *KeyObj = 0;
    /* Software timer handler */
    for(uint8_t idx_stimer = 0; idx_stimer < Obj->STimer.Count; idx_stimer++) {
        STimerObj = (STimer_t *)(Obj->STimer.Index[idx_stimer]);
        if(STimerObj->Enable == true){
            if((user_millis() - STimerObj->Counter) >= STimerObj->Compare) {
                if(STimerObj->Callback) {
                    STimerObj->Callback(STimerObj->Arg);
                    STimerObj->Enable = STimerObj->Repeat;
                    STimerObj->Counter = user_millis();
                }
            }
        }
    }
    /* Key Handler */
    for(uint8_t idx_key = 0; idx_key < Obj->Key.Count; idx_key++) {
        KeyObj = Obj->Key.Index[idx_key];
        if(KeyObj->Enable == true) {
            if(KeyObj->Debounce.Signal != KeyObj->SignalInput_Get(KeyObj)) {
                KeyObj->Debounce.Signal = KeyObj->SignalInput_Get(KeyObj);
                KeyObj->Debounce.Tick = user_millis();
            }
            if(KeyObj->Debounce.Signal == false) {
                if(KeyObj->Event == KeyPressed) {
                    KeyObj->Event = KeyReleased;
                    if(KeyObj->ReleaseCallback) {
                        KeyObj->ReleaseCallback(KeyObj->Arg);
                    }
                }
            }
            else {
                if((user_millis() - KeyObj->Debounce.Tick) >= KeyObj->Debounce.TimeMs) {
                    if(KeyObj->Event == KeyReleased) {
                        KeyObj->Event = KeyPressed;
                        if(KeyObj->PressCallback) {
                            KeyObj->PressCallback(KeyObj->Arg);
                        }
                    }
                }
            }
        }
    }
    /* Form Handler */
    if((Obj->Form.Next != 0)) { /* Cho phép chuyển form */
        if(Obj->Form.Running) {
            if(Obj->Form.Running->QuitCallback) {
                Obj->Form.Running->QuitCallback(Obj->Form.Running->Arg);      /* Chạy callback thoát form nếu có */
            }
        }
        if(Obj->Form.Next->InitCallback) {
            Obj->Form.Next->InitCallback(Obj->Form.Next->Arg);          /* Chạy callback khởi tạo form nếu có */
        }
        Obj->Form.Running = Obj->Form.Next;           /* Đổi form (mang hình thức để return chứ chưa thực sự đổi form) */
        Obj->Form.Next = 0;
    }
    else {      /* Chạy tiếp form đang chạy */
        if(Obj->Form.Running->RunCallback) {
            Obj->Form.Running->RunCallback(Obj->Form.Running->Arg);
        }
    }
}
