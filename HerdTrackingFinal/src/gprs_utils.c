#include "gprs_utils.h"
#include "location_utils.h"
#include "communication.h"


Softuart gprs_uart;
os_timer_t gprsAttach;
os_timer_t sendingSession; 
os_timer_t sendProcess;
os_timer_t shutdown_timer;

int current_send_sheep = 1;

char PATCH_REQ[1024] = {0};

ts_message master_shutdown_master = {
    .message_type = MASTER_SHUTDOWN
};

void ICACHE_FLASH_ATTR shutdown()
{
    system_deep_sleep_set_option(1);	//Option 1 = Reboot with radio recalibration and wifi on.
    system_deep_sleep(60000000);
}

static void ICACHE_FLASH_ATTR chipShut();

static void ICACHE_FLASH_ATTR sendPatch()
{
    os_printf("send patch!\n");
    os_printf("%s", PATCH_REQ);
    Softuart_Puts(&gprs_uart, PATCH_REQ);
    os_printf("Sended to DB for sheep %d\n", current_send_sheep);

    os_timer_disarm(&sendProcess);//Cancel timer timing
    os_timer_setfn(&sendProcess,(os_timer_func_t *)sendingSessionFunc,NULL);//Set the timer callback function
    os_timer_arm(&sendProcess,5000,0);//Turn on the software timer for 500ms
    current_send_sheep++;
}

static void ICACHE_FLASH_ATTR connectToDB()
{
    char patch_data[128] = {0};
    char post_sheep[128] = {0};
    char *latitute;
    char *longitute;
    Softuart_Puts(&gprs_uart, CIPSEND);
    if(locations[current_send_sheep].latitute[0] == '0')
        latitute = locations[current_send_sheep].latitute+1;
    else
        latitute = locations[current_send_sheep].latitute;
    if(locations[current_send_sheep].longitute[0] == '0')
        longitute = locations[current_send_sheep].longitute+1;
    else
        longitute = locations[current_send_sheep].longitute;
    os_sprintf(patch_data, "{\"latitude\":%s, \"longitude\": %s}", latitute, longitute);
    os_printf("PATCH DATA:%s\n", patch_data);
    os_sprintf(PATCH_REQ, "PATCH /sheeps/sheep-%d/.json HTTP/1.1\r\nHost: herdtrackingsystem-default-rtdb.firebaseio.com\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n{\"latitude\": %s,\"longitude\": %s}\x1a", current_send_sheep,os_strlen(patch_data), latitute, longitute);

    
    os_timer_disarm(&sendProcess);//Cancel timer timing
    os_timer_setfn(&sendProcess,(os_timer_func_t *)sendPatch,NULL);//Set the timer callback function
    os_timer_arm(&sendProcess,5000,0);//Turn on the software timer for 500ms
}

static void ICACHE_FLASH_ATTR accessToDB()
{
    Softuart_Puts(&gprs_uart, CIPSTART);
    os_timer_disarm(&sendProcess);//Cancel timer timing
    os_timer_setfn(&sendProcess,(os_timer_func_t *)connectToDB,NULL);//Set the timer callback function
    os_timer_arm(&sendProcess,20000,0);//Turn on the software timer for 500ms
}

void ICACHE_FLASH_ATTR sendingSessionFunc()
{
    while(1)
    {
        if(current_send_sheep != MAX_PAIR_NUM)
        {
            if(valid_location[current_send_sheep])
            {
                os_timer_disarm(&sendProcess);//Cancel timer timing
                os_timer_setfn(&sendProcess,(os_timer_func_t *)accessToDB,NULL);//Set the timer callback function
                os_timer_arm(&sendProcess,1000,0);//Turn on the software timer for 500ms
                break;
            }
            else
                current_send_sheep++;
        }
        else
        {
            os_timer_disarm(&sendingSession);
            os_timer_disarm(&sendProcess);//Cancel timer timing
            os_timer_disarm(&master_exist_timer);
            esp_now_send(NULL, (u8*)&master_shutdown_master, sizeof(ts_message));
            Softuart_Puts(&gprs_uart, CIPSHUT);
            os_printf("AT+CIPSHUT\n");
            os_printf("Master shutting down in 10 second\n");
            os_timer_disarm(&shutdown_timer);//Cancel timer timing
            os_timer_setfn(&shutdown_timer,(os_timer_func_t *)shutdown,NULL);//Set the timer callback function
            os_timer_arm(&shutdown_timer,10000,0);//Turn on the software timer for 500ms
            break;
        }
    }
}

static void ICACHE_FLASH_ATTR enableSSL()
{
    Softuart_Puts(&gprs_uart, CIPSSL);
    os_printf("AT+CIPSSL=1\n");
    os_timer_disarm(&sendingSession);//Cancel timer timing
    os_timer_setfn(&sendingSession,(os_timer_func_t *)sendingSessionFunc,NULL);//Set the timer callback function
	os_timer_arm(&sendingSession,60000,0);//Turn on the software timer for 500ms
}

static void ICACHE_FLASH_ATTR getIP()
{
    Softuart_Puts(&gprs_uart, CIFSR);
    os_printf("AT+CIFSR\n");
    os_timer_disarm(&gprsAttach);//Cancel timer timing
	os_timer_setfn(&gprsAttach,(os_timer_func_t *)enableSSL,NULL);//Set the timer callback function
	os_timer_arm(&gprsAttach,3000,0);//Turn on the software timer for 500ms
}
static void ICACHE_FLASH_ATTR upCellular()
{
    Softuart_Puts(&gprs_uart, CIICR);
    os_printf("AT+CIICR\n");
    os_timer_disarm(&gprsAttach);//Cancel timer timing
	os_timer_setfn(&gprsAttach,(os_timer_func_t *)getIP,NULL);//Set the timer callback function
	os_timer_arm(&gprsAttach,60000,0);//Turn on the software timer for 500ms
}

static void ICACHE_FLASH_ATTR apnSet()
{
    Softuart_Puts(&gprs_uart, CSTT);
    os_printf("AT+CSTT=\"internet\"\n");
    os_timer_disarm(&gprsAttach);//Cancel timer timing
	os_timer_setfn(&gprsAttach,(os_timer_func_t *)upCellular,NULL);//Set the timer callback function
	os_timer_arm(&gprsAttach,3000,0);//Turn on the software timer for 500ms
}

static void ICACHE_FLASH_ATTR chipMux()
{
    Softuart_Puts(&gprs_uart, CIPMUX);
    os_printf("AT+CIPMUX=0\n");
    os_timer_disarm(&gprsAttach);//Cancel timer timing
	os_timer_setfn(&gprsAttach,(os_timer_func_t *)apnSet,NULL);//Set the timer callback function
	os_timer_arm(&gprsAttach,3000,0);//Turn on the software timer for 500ms
}

static void ICACHE_FLASH_ATTR chipShut()
{
    Softuart_Puts(&gprs_uart, CIPSHUT);
    os_printf("AT+CIPSHUT\n");
    os_timer_disarm(&gprsAttach);//Cancel timer timing
	os_timer_setfn(&gprsAttach,(os_timer_func_t *)chipMux,NULL);//Set the timer callback function
	os_timer_arm(&gprsAttach,3000,0);//Turn on the software timer for 500ms
}

void ICACHE_FLASH_ATTR initializeGPRS()
{
    chipShut();
}