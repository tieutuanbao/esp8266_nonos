#ifndef __MAIN_H
#define __MAIN_H

#define FW_VERSION  "171222.00"
#define HW_VERSION  "171222"

#if ((SPI_FLASH_SIZE_MAP == 0) || (SPI_FLASH_SIZE_MAP == 1))
    #error "The flash map is not supported"
#elif (SPI_FLASH_SIZE_MAP == 2)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0xfb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0xfc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0xfd000
#elif (SPI_FLASH_SIZE_MAP == 3)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0x1fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0x1fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0x1fd000
#elif (SPI_FLASH_SIZE_MAP == 4)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0x3fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0x3fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0x3fd000
#elif (SPI_FLASH_SIZE_MAP == 5)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x101000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0x1fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0x1fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0x1fd000
#elif (SPI_FLASH_SIZE_MAP == 6)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x101000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0x3fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0x3fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0x3fd000
#else
#error "The flash map is not supported"
#endif


#include "common_macros.h"
#include "color.h"

#define LOOP_TASK_PRIORITY          1
#define LOOP_QUEUE_SIZE             1

/* ------------ Define 1903 ------------ */
#define GPIO_1903_PORT_SET          (GPIO->out_set)
#define GPIO_1903_PORT_CLR          (GPIO->out_clear)

#define GPIO_1903_DI_CH0            4
#define GPIO_1903_DI_CH1            5

#define GPIO1903_DI_CH0_BIT_MASK    (1ULL << GPIO_1903_DI_CH0)
#define GPIO1903_DI_CH1_BIT_MASK    (1ULL << GPIO_1903_DI_CH1)

#define USC_ALL_MASK    (GPIO1903_DI_CH0_BIT_MASK|GPIO1903_DI_CH1_BIT_MASK)

/* ------------ Define 6803 ------------ */
#define GPIO_6803_DAT_PORT_SET      (GPIO->out_set)
#define GPIO_6803_DAT_PORT_CLR      (GPIO->out_clear)
#define GPIO_6803_CLK_PORT_SET      (GPIO->out_set)
#define GPIO_6803_CLK_PORT_CLR      (GPIO->out_clear)

#define GPIO_6803_DAT_PIN           4
#define GPIO_6803_CLK_PIN           5

#define GPIO_6803_DAT_PIN_BIT_MASK  (1ULL << GPIO_6803_DAT_PIN)
#define GPIO_6803_CLK_PIN_BIT_MASK  (1ULL << GPIO_6803_CLK_PIN)

/* ------------ Define Firework ------------ */
#define GPIO_BUTTON_SET_BIT_MASK     (1ULL << 15)
#define GPIO_BUTTON_UP_BIT_MASK      (1ULL << 0)
#define GPIO_BUTTON_DOWN_BIT_MASK    (1ULL << 2)

#define GPIO_LED_SET_BIT_MASK        (1ULL << 13)
#define GPIO_LED_COT_BIT_MASK        (1ULL << 12)
#define GPIO_LED_TIA_BIT_MASK        (1ULL << 14)

extern uint8_t *color_buf_p;

#endif