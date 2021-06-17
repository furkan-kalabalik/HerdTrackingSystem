#ifndef DEVICE_INFO_H
#define DEVICE_INFO_H

#include "osapi.h"
#include "user_interface.h"


#define CONFIG_SECTOR               0x7b
#define CONFIG_ADDRESS              CONFIG_SECTOR * 4096
#define MAX_PAIR_NUM                10
#define MAC_LEN                     6

typedef struct
{
    uint8_t device_id;
    uint8_t master_info;
    uint8_t pair_num;
    uint8_t peerMAC[MAX_PAIR_NUM][MAC_LEN];
    uint8_t reserved;
}ts_device_info;

extern ts_device_info device_info;
extern u8 masterKey[16];

void create_device_info(int device_id, int master_info);
void read_device_info();

#endif