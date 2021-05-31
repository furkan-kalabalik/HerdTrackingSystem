#include "osapi.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "espnow.h"
#include "softuart.h"

#define TOTAL_SLAVE 4

os_timer_t gprsAttach;
os_timer_t sendingSession; 
os_timer_t sendProcess;
sheep_loc gps_info[TOTAL_SLAVE];
Softuart softuart;

int currentSheep = 0;

u8 peerList[10][6];
u8 totalPeer = 0;
char buffer[64] = {0};
char PATCH_REQ[1024] = {0};
bool connection = false;

static void ICACHE_FLASH_ATTR connectToDB();
static void ICACHE_FLASH_ATTR accessToDB();
static void ICACHE_FLASH_ATTR startSendingSession();
static void ICACHE_FLASH_ATTR sendToDB();

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

static void ICACHE_FLASH_ATTR startSendingSession()
{
    os_timer_disarm(&sendProcess);//Cancel timer timing
    os_timer_setfn(&sendProcess,(os_timer_func_t *)accessToDB,NULL);//Set the timer callback function
    os_timer_arm(&sendProcess,4000,0);//Turn on the software timer for 500ms
}

static void ICACHE_FLASH_ATTR sendToDB()
{
    os_printf("%s", PATCH_REQ);
    Softuart_Puts(&softuart, PATCH_REQ);
    os_printf("Sended to DB for sheep %d\n", currentSheep+1);
    currentSheep++;
    if(currentSheep != 4)
    {
        os_timer_disarm(&sendProcess);//Cancel timer timing
        os_timer_setfn(&sendProcess,(os_timer_func_t *)accessToDB,NULL);//Set the timer callback function
        os_timer_arm(&sendProcess,4000,0);//Turn on the software timer for 500ms
    }
    else
    {
        currentSheep = 0;
        os_timer_disarm(&sendingSession);//Cancel timer timing
        os_timer_setfn(&sendingSession,(os_timer_func_t *)startSendingSession,NULL);//Set the timer callback function
        os_timer_arm(&sendingSession,120000,0);//Turn on the software timer for 500ms
    }
}

static void ICACHE_FLASH_ATTR connectToDB()
{
    char patch_data[128] = {0};
    char post_sheep[128] = {0};
    char *latitude;
    char *longitude;
    Softuart_Puts(&softuart, "AT+CIPSEND\r\n");
    if(gps_info[currentSheep].latitude[0] == '0')
        latitude = gps_info[currentSheep].latitude+1;
    else
        latitude = gps_info[currentSheep].latitude;
    if(gps_info[currentSheep].longitude[0] == '0')
        longitude = gps_info[currentSheep].longitude+1;
    else
        longitude = gps_info[currentSheep].longitude;
    os_sprintf(patch_data, "{\"latitude\":%s, \"longitude\": %s}", latitude, longitude);
    os_printf("PATCH DATA:%s\n", patch_data);
    os_sprintf(PATCH_REQ, "PATCH /sheeps/sheep-%d/.json HTTP/1.1\r\nHost: herdtrackingsystem-default-rtdb.firebaseio.com\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n{\"latitude\": %s,\"longitude\": %s}\x1a", currentSheep+1,os_strlen(patch_data), latitude, longitude);
    os_timer_disarm(&sendProcess);//Cancel timer timing
    os_timer_setfn(&sendProcess,(os_timer_func_t *)sendToDB,NULL);//Set the timer callback function
    os_timer_arm(&sendProcess,4000,0);//Turn on the software timer for 500ms
}

static void ICACHE_FLASH_ATTR accessToDB()
{
    Softuart_Puts(&softuart, "AT+CIPSTART=\"TCP\",\"herdtrackingsystem-default-rtdb.firebaseio.com\",443\r\n");
    os_timer_disarm(&sendProcess);//Cancel timer timing
    os_timer_setfn(&sendProcess,(os_timer_func_t *)connectToDB,NULL);//Set the timer callback function
    os_timer_arm(&sendProcess,6000,0);//Turn on the software timer for 500ms
}

void onRecvData (u8 *mac_addr, u8 *data, u8 len)
{
    sheep_loc temp;

    os_memcpy(&temp, data, sizeof(sheep_loc));
    os_printf("DATA received from Sheep-%d --> Time: %d:%d:%d, Latitute. %s, Longtitute. %s\n", 
        temp.sheep_id, temp.hour+3, temp.min, temp.sec, temp.latitude, temp.longitude);
    os_memcpy(&gps_info[temp.sheep_id-1], &temp, sizeof(sheep_loc));
}

static void ICACHE_FLASH_ATTR enableSSL()
{
    Softuart_Puts(&softuart, "AT+CIPSSL=1\r\n");
    os_printf("AT+CIPSSL=1\n");
    connection = true;
}

static void ICACHE_FLASH_ATTR getIP()
{
    Softuart_Puts(&softuart, "AT+CIFSR\r\n");
    os_printf("AT+CIFSR\n");
    os_timer_disarm(&gprsAttach);//Cancel timer timing
	os_timer_setfn(&gprsAttach,(os_timer_func_t *)enableSSL,NULL);//Set the timer callback function
	os_timer_arm(&gprsAttach,6000,0);//Turn on the software timer for 500ms
}
static void ICACHE_FLASH_ATTR upCellular()
{
    Softuart_Puts(&softuart, "AT+CIICR\r\n");
    os_printf("AT+CIICR\n");
    os_timer_disarm(&gprsAttach);//Cancel timer timing
	os_timer_setfn(&gprsAttach,(os_timer_func_t *)getIP,NULL);//Set the timer callback function
	os_timer_arm(&gprsAttach,6000,0);//Turn on the software timer for 500ms
}

static void ICACHE_FLASH_ATTR apnSet()
{
    Softuart_Puts(&softuart, "AT+CSTT=\"internet\"\r\n");
    os_printf("AT+CSTT=\"internet\"\n");
    os_timer_disarm(&gprsAttach);//Cancel timer timing
	os_timer_setfn(&gprsAttach,(os_timer_func_t *)upCellular,NULL);//Set the timer callback function
	os_timer_arm(&gprsAttach,6000,0);//Turn on the software timer for 500ms
}

static void ICACHE_FLASH_ATTR chipMux()
{
    Softuart_Puts(&softuart, "AT+CIPMUX=0\r\n");
    os_printf("AT+CIPMUX=0\n");
    os_timer_disarm(&gprsAttach);//Cancel timer timing
	os_timer_setfn(&gprsAttach,(os_timer_func_t *)apnSet,NULL);//Set the timer callback function
	os_timer_arm(&gprsAttach,3000,0);//Turn on the software timer for 500ms
}

static void ICACHE_FLASH_ATTR chipShut()
{
    Softuart_Puts(&softuart, "AT+CIPSHUT\r\n");
    os_printf("AT+CIPSHUT\n");
    os_timer_disarm(&gprsAttach);//Cancel timer timing
	os_timer_setfn(&gprsAttach,(os_timer_func_t *)chipMux,NULL);//Set the timer callback function
	os_timer_arm(&gprsAttach,3000,0);//Turn on the software timer for 500ms
}

void initMasterEspNow()
{
    u8 masterKey[16] = {
        0x00, 0x01, 0x02, 0x03, 
		0x04, 0x05, 0x06, 0x07, 
		0x08, 0x09, 0x0a, 0x0b, 
		0x0c, 0x0d, 0x0e, 0x0f,
    };
    u8 macAddrMaster[6];
    wifi_get_macaddr(0x00, macAddrMaster);
    os_printf("MAC ADDRESS of MASTER: %02x:%02x:%02x:%02x:%02x:%02x\n", MAC2STR(macAddrMaster));

    if(esp_now_init() == 0)
        os_printf("ESPNOW init done!\n");
    else
    {
        os_printf("ESPNOW init fail!\n");
        return;
    }

    if(esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER) == 0)
        os_printf("ESPNOW mode setted to MASTER!\n");
    else
    {
        os_printf("ESPNOW mode set fail!\n");
        return;
    }

    if(esp_now_register_recv_cb(onRecvData) == 0)
        os_printf("ESPNOW register cb set done!\n");
    else
    {
        os_printf("ESPNOW register cb set fail!\n");
        return;
    }

    u8 slave1[6] = {0xfe, 0xf5, 0xc4, 0x96, 0x82, 0x7c};
    u8 slave2[6] = {0x52, 0x02, 0x91, 0xec, 0xaa, 0xce};
    u8 slave3[6] = {0xfe, 0xf5, 0xc4, 0x97, 0x83, 0x93};
    u8 slave4[6] = {0xfe, 0xf5, 0xc4, 0x96, 0x81, 0x4b};
    os_memcpy(peerList[totalPeer], slave1, 6);
    totalPeer++;
    os_memcpy(peerList[totalPeer], slave2, 6);
    totalPeer++;
    os_memcpy(peerList[totalPeer], slave3, 6);
    totalPeer++;
    os_memcpy(peerList[totalPeer], slave4, 6);
    totalPeer++;
    for(int i = 0; i < totalPeer; i++)
        if(esp_now_add_peer(peerList[i], ESP_NOW_ROLE_SLAVE, 1, masterKey, 16) == 0)
            os_printf("Slave: %02x:%02x:%02x:%02x:%02x:%02x added as pair!\n", MAC2STR(peerList[i]));
    
    for(int i = 0; i < TOTAL_SLAVE; i++)
        gps_info[i].sheep_id = i+1;

    os_timer_disarm(&gprsAttach);//Cancel timer timing
	os_timer_setfn(&gprsAttach,(os_timer_func_t *)chipShut,NULL);//Set the timer callback function
	os_timer_arm(&gprsAttach,3000,0);//Turn on the software timer for 500ms

    os_timer_disarm(&sendingSession);//Cancel timer timing
	os_timer_setfn(&sendingSession,(os_timer_func_t *)startSendingSession,NULL);//Set the timer callback function
	os_timer_arm(&sendingSession,120000,0);//Turn on the software timer for 500ms
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
    wifi_set_opmode(STATION_MODE);
    system_init_done_cb(initMasterEspNow);

    Softuart_SetPinRx(&softuart,14);	
    Softuart_SetPinTx(&softuart,12);
    Softuart_Init(&softuart,9600);
}