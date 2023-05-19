#include "ets_sys.h"
#include "osapi.h"
#include "eagle_soc.h"
#include "mem.h"
#include "user_interface.h"
#include "main.h"

bool check_memleak_debug_enable(void)
{
    return MEMLEAK_DEBUG_ENABLE;
}

static const partition_item_t at_partition_table[] = {
    { SYSTEM_PARTITION_BOOTLOADER, 						0x0, 												0x1000},
    { SYSTEM_PARTITION_OTA_1,   						0x1000, 											SYSTEM_PARTITION_OTA_SIZE},
    { SYSTEM_PARTITION_OTA_2,   						SYSTEM_PARTITION_OTA_2_ADDR, 						SYSTEM_PARTITION_OTA_SIZE},
    { SYSTEM_PARTITION_RF_CAL,  						SYSTEM_PARTITION_RF_CAL_ADDR, 						0x1000},
    { SYSTEM_PARTITION_PHY_DATA, 						SYSTEM_PARTITION_PHY_DATA_ADDR, 					0x1000},
    { SYSTEM_PARTITION_SYSTEM_PARAMETER, 				SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR, 			0x3000},
};

void user_pre_init(void)
{
    if(!system_partition_table_regist(at_partition_table, sizeof(at_partition_table)/sizeof(at_partition_table[0]), SPI_FLASH_SIZE_MAP)) {
		os_printf("system_partition_table_regist fail\r\n");
		while(1);
	}
}

static os_event_t s_loop_queue[LOOP_QUEUE_SIZE];
void loop(os_event_t *events);
void setup();

void __disableWiFiAtBootTime (void) {
    wifi_set_opmode_current(0/*WIFI_OFF*/);
    wifi_fpm_set_sleep_type(MODEM_SLEEP_T);
    wifi_fpm_open();
    wifi_fpm_do_sleep(0xFFFFFFF);
}

void init_done() {
    setup();
    system_os_post(LOOP_TASK_PRIORITY, 0, 0);
}

void user_init(void)
{
    system_update_cpu_freq(SYS_CPU_160MHZ);
    system_set_os_print(0);

    // struct rst_info *rtc_info = system_get_rst_info();

	// os_printf("reset reason: %x\n", rtc_info->reason);

	// if (rtc_info->reason == REASON_WDT_RST ||
	// 	rtc_info->reason == REASON_EXCEPTION_RST ||
	// 	rtc_info->reason == REASON_SOFT_WDT_RST) {
	// 	if (rtc_info->reason == REASON_EXCEPTION_RST) {
	// 		os_printf("Fatal exception (%d):\n", rtc_info->exccause);
	// 	}
	// 	os_printf("epc1=0x%08x, epc2=0x%08x, epc3=0x%08x, excvaddr=0x%08x, depc=0x%08x\n",
	// 			rtc_info->epc1, rtc_info->epc2, rtc_info->epc3, rtc_info->excvaddr, rtc_info->depc);
	// }

    __disableWiFiAtBootTime();
    /* Khởi tạo task */
    system_os_task(loop,
        LOOP_TASK_PRIORITY, s_loop_queue,
        LOOP_QUEUE_SIZE);

    system_init_done_cb(&init_done);
}