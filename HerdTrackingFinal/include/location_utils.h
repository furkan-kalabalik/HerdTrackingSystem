#ifndef LOCATION_H
#define LOCATION_H


#include "osapi.h"
#include "user_interface.h"
#include "device_info.h"
#include "softuart.h"
#include "espnow.h"
#include "common_types.h"

extern Softuart gps_uart;
extern os_timer_t read_gps_uart_timer;
extern os_timer_t stream_received_location_timer;

typedef struct
{  
    uint8_t sheep_id;
    char latitute[16];
    char longitute[16];
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
}  __attribute__((aligned(4))) ts_sheep_location;


extern ts_sheep_location locations[MAX_PAIR_NUM];
extern uint8_t valid_location[MAX_PAIR_NUM];

void ICACHE_FLASH_ATTR readGpsData();
void ICACHE_FLASH_ATTR sendReceivedLocations();
void ICACHE_FLASH_ATTR start_gps_read();

#endif