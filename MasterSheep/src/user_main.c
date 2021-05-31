#include "osapi.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"

struct softap_config ap_config;
struct espconn *pconn = NULL;

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

void onRecv (void *arg, char *pdata, unsigned short len)
{
	os_printf("DATA received\n");
	char buffer[32] = {0};
	os_memcpy(buffer, pdata, len);
	os_printf("DATA: %s\n", buffer);
}

void startServer()
{
	esp_tcp *ptcp = NULL;

    pconn = (struct espconn *)os_zalloc(sizeof(struct espconn));
    ptcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));

    ptcp->local_port = 8000;

    pconn->type = ESPCONN_TCP;
    pconn->state = ESPCONN_NONE;
    pconn->proto.tcp = ptcp;
    pconn->recv_callback = onRecv;

    espconn_accept(pconn);


    os_printf("start server :8000 ... OK!\n");
}

void tcp_server_init()//tcp cilent initialization
{
	wifi_set_opmode(SOFTAP_MODE);//Set to station mode to save flash
	wifi_softap_get_config_default(&ap_config);
	os_memset(ap_config.ssid, 0, 32);
	os_memset(ap_config.password, 0, 32);
	os_strcpy(ap_config.ssid, "MasterSheep");
	os_strcpy(ap_config.password, "masterpass");
	ap_config.ssid_len = os_strlen("MasterSheep");

	startServer();
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
	tcp_server_init();
}