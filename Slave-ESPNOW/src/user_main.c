#include "osapi.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "espnow.h"
#include "softuart.h"
#include "stdlib.h"

#define FIND_AND_NUL(s, p, c) ( \
   (p) = strchr(s, c), \
   *(p) = '\0', \
   ++(p), \
   (p))

u8 masterMac[6] = {0xfc, 0xf5, 0xc4, 0x96, 0x7c, 0x1f};
os_timer_t read_uart; 
Softuart softuart;
char buffer[1024] = {0};
char str[144] = {0};
int read_head = 0;
sheep_loc gps_info;

static void ICACHE_FLASH_ATTR readGpsData()
{
    uint8_t readed = 0;
    char dollar = '$';
    char *first;
    char *second;	
    char *rmc, *time, *validation, *lat, *lat_dir, *lon, *lon_dir, *empty;
    char temp[8] = {0};
    int tempInt;
    
    os_printf(".");

	if(Softuart_Available(&softuart)) {
        readed = Softuart_Readline(&softuart, &buffer[read_head], 144);
        read_head += readed;
	}
    else
    {
        if(buffer[0] != 0)
        {
            os_memset(str, 0, 144);
            first = os_strstr(buffer,"$GPRMC");
            second = os_strchr(first+1, dollar);
            os_memcpy(str, first, second-first);

            os_printf("%s\n", str);

            rmc = str;
            time = FIND_AND_NUL(rmc, time, ',');
            validation = FIND_AND_NUL(time, validation, ',');
            lat = FIND_AND_NUL(validation, lat, ',');
            lat_dir = FIND_AND_NUL(lat, lat_dir, ',');
            lon = FIND_AND_NUL(lat_dir, lon, ',');
            lon_dir = FIND_AND_NUL(lon, lon_dir, ',');
            empty = FIND_AND_NUL(lon_dir, empty, ',');
            
            os_strncpy(temp, &time[0], 2);
            gps_info.hour = atoi(temp);
            os_strncpy(temp, &time[2], 2);
            gps_info.min = atoi(temp);
            os_strncpy(temp, &time[4], 2);
            gps_info.sec = atoi(temp);
            os_strncpy(&gps_info.latitute[0], lat, os_strlen(lat)); 
            os_strncpy(&gps_info.longtitute[0], lon, os_strlen(lon));

            esp_now_send(masterMac, (u8*)&gps_info, sizeof(sheep_loc));
            
            os_memset(str, 0, 144);
            os_memset(buffer, 0, 1024);

            system_deep_sleep_set_option(1);	//Option 1 = Reboot with radio recalibration and wifi on.
			system_deep_sleep(30000000);
        }
    }
}

uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map)
    {
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

void onSendData(u8 *mac_addr, u8 status)
{
	if(status == 0)
		os_printf("Data sended to master--> %02x:%02x:%02x:%02x:%02x:%02x\n", MAC2STR(mac_addr));
	else
		os_printf("Data couldn't sended to master--> %02x:%02x:%02x:%02x:%02x:%02x\n", MAC2STR(mac_addr));
}

void check_pair_cb()
{
	char data[] = "Hello from Slave-1";
	if(esp_now_is_peer_exist(masterMac) > 0)
	{
		os_printf("Pair %02x:%02x:%02x:%02x:%02x:%02x exist!\n", MAC2STR(masterMac));
		esp_now_send(masterMac, (u8*)data, os_strlen(data));
	}
	else
		os_printf("Pair %02x:%02x:%02x:%02x:%02x:%02x doesn't exist!\n", MAC2STR(masterMac));
}

void initSlaveEspNow()
{
    u8 masterKey[16] = {
        0x00, 0x01, 0x02, 0x03, 
		0x04, 0x05, 0x06, 0x07, 
		0x08, 0x09, 0x0a, 0x0b, 
		0x0c, 0x0d, 0x0e, 0x0f,
    };
    u8 macAddrMaster[6];
    wifi_get_macaddr(0x01, macAddrMaster);
    os_printf("MAC ADDRESS of slave: %02x:%02x:%02x:%02x:%02x:%02x\n", MAC2STR(macAddrMaster));

    if(esp_now_init() == 0)
        os_printf("ESPNOW init done!\n");
    else
    {
        os_printf("ESPNOW init fail!\n");
        return;
    }

    if(esp_now_set_self_role(ESP_NOW_ROLE_SLAVE) == 0)
        os_printf("ESPNOW mode setted to SLAVE!\n");
    else
    {
        os_printf("ESPNOW mode set fail!\n");
        return;
    }

    if(esp_now_register_send_cb(onSendData) == 0)
        os_printf("ESPNOW register cb set done!\n");
    else
    {
        os_printf("ESPNOW register cb set fail!\n");
        return;
    }

	if(esp_now_add_peer(masterMac, ESP_NOW_ROLE_CONTROLLER, 1, masterKey, 16) == 0)
		os_printf("Master: %02x:%02x:%02x:%02x:%02x:%02x added as pair!\n", MAC2STR(masterMac));
	else
		os_printf("Problem! Pair not added!\n");

    gps_info.sheep_id = 4;

    os_timer_disarm(&read_uart);//Cancel timer timing
	os_timer_setfn(&read_uart,(os_timer_func_t *)readGpsData,NULL);//Set the timer callback function
	os_timer_arm(&read_uart,100,1);//Turn on the software timer for 500ms
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_init(void)
{
    wifi_set_opmode(SOFTAP_MODE);
    system_init_done_cb(initSlaveEspNow);
    //init software uart
    Softuart_SetPinRx(&softuart,4);	
    Softuart_SetPinTx(&softuart,5);
    Softuart_Init(&softuart,9600);

}