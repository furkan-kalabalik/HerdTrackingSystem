#include "osapi.h"
#include "user_interface.h"
#include "device_info.h"
#include "gprs_utils.h"
#include "location_utils.h"
#include "communication.h"
#include "common_types.h"
#include "driver/uart.h"


static os_timer_t ptimer;
os_timer_t generalTimer;
/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABBBCDDD
 *                A : rf cal
 *                B : at parameters
 *                C : rf init data
 *                D : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }
    return rf_cal_sec;
}

void ICACHE_FLASH_ATTR slave_init()
{
    init_esp_now();

    os_timer_disarm(&read_gps_uart_timer);//Cancel timer timing
	os_timer_setfn(&read_gps_uart_timer,(os_timer_func_t *)start_gps_read,NULL);//Set the timer callback function
	os_timer_arm(&read_gps_uart_timer,20000,0);//Turn on the software timer for 500ms

}

void ICACHE_FLASH_ATTR master_init()
{
    init_esp_now();

    os_timer_disarm(&gprsAttach);//Cancel timer timing
    os_timer_setfn(&gprsAttach,(os_timer_func_t *)initializeGPRS,NULL);//Set the timer callback function
    os_timer_arm(&gprsAttach,10000,0);//Turn on the software timer for 500ms

    //Send periodic master exist info to all peers
    os_timer_disarm(&master_exist_timer);
    os_timer_setfn(&master_exist_timer, send_master_exist, NULL);
    os_timer_arm(&master_exist_timer, 10000, 1);
}

void ICACHE_FLASH_ATTR user_init(void)
{
    read_device_info(&device_info);
    os_printf("Device ID: %d\nMaster? %s\n", device_info.device_id, device_info.master_info ? "Yes":"No");
    wifi_set_opmode(SOFTAP_MODE);
    /*MASTER INIT*/
    if(device_info.master_info)
    {
        Softuart_SetPinRx(&gprs_uart,14);	
        Softuart_SetPinTx(&gprs_uart,12);
        Softuart_Init(&gprs_uart,9600);
        system_init_done_cb(master_init);
    }
    /*SLAVE INIT*/
    else
    {
        Softuart_SetPinRx(&gps_uart,4);	
        Softuart_SetPinTx(&gps_uart,5);
        Softuart_Init(&gps_uart,9600);
        system_init_done_cb(slave_init);
    }
}
