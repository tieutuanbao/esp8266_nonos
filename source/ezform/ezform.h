/**
 * @file ezform.h
 * @author Tieu Tuan Bao (tieutuanbao@gmail.com)
 * @brief
 * @version 0.1
 * @date 2022-08-04
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef __EZFORM_H__
#define __EZFORM_H__

#include <stdint.h>
#include "c_types.h"
#include "mem.h"
#include "port_macros.h"

#include "color.h"

#define STIMER_MAX          10
#define LED_DUTY_MAX        10
#define LED_MAX             3
#define KEY_MAX             3

/**
 * @brief --------------> key object <--------------
 */
typedef enum {
    KeyPressed      =   true,
    KeyReleased     =   false
} KeyEventState_t;

typedef struct {
    bool (*SignalInput_Get)(void *Arg);
    void (*PressCallback)(void *Arg);
    void (*ReleaseCallback)(void *Arg);
    KeyEventState_t Event;
    bool Enable;
    struct {
        bool Signal;
        uint8_t TimeMs;
        uint32_t Tick;
    } Debounce;
    void *Arg;
} IO_Key_t;

/**
 * @brief --------------> software timer object <--------------
 */
typedef struct {
    uint32_t Counter;
    uint32_t Compare;
    bool Enable;
    bool Repeat;
    void (*Callback)(void *Arg);
    void *Arg;
} STimer_t;

/**
 * @brief --------------> single led object <--------------
 */
typedef struct {
    struct {
        uint32_t On;
        uint32_t Off;
    } Time;
    struct {
        uint32_t On;
        uint32_t Off;
    } Interval;
    void (*Output_Driver)(bool);
    uint8_t Brightness;         /* Độ sáng */
    uint8_t DutyCounter;        /* counter độ sáng */
    int8_t Repeat;              /* Lặp lại [ -1: vô hạn, 0: không có tác động, 0 < repeat < 100: số lần lặp lại ] */
} Led_t;

typedef struct {
    void (*InitCallback)(void *Arg);
    void (*RunCallback)(void *Arg);
    void (*QuitCallback)(void *Arg);
    void *Arg;
} Form_t;

typedef struct {
    struct {
        Form_t *Running;
        Form_t *Next;
    } Form;
    struct {
        void *Index[STIMER_MAX];
        uint8_t Count;
    } STimer;
    struct {
        Led_t *Index[LED_MAX];
        uint8_t Count;
    } Led;
    struct {
        IO_Key_t *Index[KEY_MAX];
        uint8_t Count;
    } Key;
} Application_t;

extern volatile uint32_t __tick_ms_counter;        /* Đưa biến này tăng vô hạn trong timer 1ms */

uint32_t user_millis(void);
void delay_nop(uint32_t nnop);
void delay_ms(uint32_t ms);
void delay_us_nop(uint32_t us);

bool STimer_Start(STimer_t *Obj, uint32_t Compare, void (*Callback)(), void *Arg, bool Repeat);
void STimer_Stop(STimer_t *Obj);
__attribute__((section(".text"))) void Application_Led_Handler(Application_t *App_Obj);

void Application_Led_Config(Led_t *LedObj, uint8_t Brightness, uint32_t TimeOn_ms, uint32_t TimeOff_ms, int8_t Repeat);
void Application_Add_STimer(Application_t *Obj, STimer_t *STimer);
void Application_Add_Led(Application_t *Obj, Led_t *Led);
void Application_Add_Key(Application_t *Obj, IO_Key_t *Key);

void Application_Form_Switch(Application_t *app_obj, Form_t *form_obj);
void Application_Run(Application_t *app_obj);

#endif