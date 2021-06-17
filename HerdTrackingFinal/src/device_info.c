#include "device_info.h"
ts_device_info device_info;

u8 masterMac[6] = {0xfe, 0xf5, 0xc4, 0x96, 0x7c, 0x1f};
u8 slave1[6] = {0xfe, 0xf5, 0xc4, 0x96, 0x82, 0x7c};
u8 slave2[6] = {0x52, 0x02, 0x91, 0xec, 0xaa, 0xce};
u8 slave3[6] = {0xfe, 0xf5, 0xc4, 0x97, 0x83, 0x93};
u8 slave4[6] = {0xfe, 0xf5, 0xc4, 0x96, 0x81, 0x4b};
u8 slave5[6] = {0x86, 0xcc, 0xa8, 0xb0, 0xa2, 0xf4};

u8 masterKey[16] = {
    0x00, 0x01, 0x02, 0x03, 
    0x04, 0x05, 0x06, 0x07, 
    0x08, 0x09, 0x0a, 0x0b, 
    0x0c, 0x0d, 0x0e, 0x0f,
};

void ICACHE_FLASH_ATTR create_device_info(int device_id, int master_info)
{
    ts_device_info device_info;
    spi_flash_erase_sector(CONFIG_SECTOR);
    device_info.device_id = device_id;
    device_info.master_info = master_info;
    device_info.pair_num = 6;
    os_memcpy(device_info.peerMAC[0], masterMac, 6);
    os_memcpy(device_info.peerMAC[1], slave1, 6);
    os_memcpy(device_info.peerMAC[2], slave2, 6);
    os_memcpy(device_info.peerMAC[3], slave3, 6);
    os_memcpy(device_info.peerMAC[4], slave4, 6);
    os_memcpy(device_info.peerMAC[5], slave5, 6);
    spi_flash_write(CONFIG_ADDRESS ,(uint32 *)(&device_info),sizeof(ts_device_info));
}

void ICACHE_FLASH_ATTR read_device_info()
{
    spi_flash_read(CONFIG_ADDRESS ,(uint32 *)(&device_info),sizeof(ts_device_info));
}