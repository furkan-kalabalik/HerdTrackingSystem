#ifndef COMM_H
#define COMM_H

#include "espnow.h"
#include "osapi.h"
#include "user_interface.h"
#include "device_info.h"
#include "common_types.h"
#include "location_utils.h"

void send_master_exist();
extern os_timer_t master_exist_timer;
extern ts_message master_shutdown;
#endif