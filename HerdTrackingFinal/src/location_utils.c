#include "location_utils.h"
#include "communication.h"
#include "gprs_utils.h"
#define FIND_AND_NUL(s, p, c) ( \
   (p) = strchr(s, c), \
   *(p) = '\0', \
   ++(p), \
   (p))

ts_sheep_location locations[MAX_PAIR_NUM];
uint8_t valid_location[MAX_PAIR_NUM] = {0};

Softuart gps_uart;
os_timer_t read_gps_uart_timer;
os_timer_t stream_received_location_timer;

char __attribute__((aligned(4))) buffer[1024] = {0};
char __attribute__((aligned(4))) str[128] = {0};
int read_head = 0;

int current_sheep = 1;


void ICACHE_FLASH_ATTR start_gps_read()
{
     system_deep_sleep_set_option(1);	//Option 1 = Reboot with radio recalibration and wifi on.
    system_deep_sleep(10000000);
    // os_timer_disarm(&read_gps_uart_timer);//Cancel timer timing
	// os_timer_setfn(&read_gps_uart_timer,(os_timer_func_t *)readGpsData,NULL);//Set the timer callback function
	// os_timer_arm(&read_gps_uart_timer,100,1);//Turn on the software timer for 500ms
}

void ICACHE_FLASH_ATTR readGpsData()
{
    uint8_t readed = 0;
    char dollar = '$';
    char *first;
    char *second;	
    char *rmc, *time, *validation, *lat, *lat_dir, *lon, *lon_dir, *empty;
    char  temp[32] = {0};
    int tempInt;
    char read_allow = 0;
    
    os_printf(".");

	if(Softuart_Available(&gps_uart)) {
        readed = Softuart_Readline(&gps_uart, &buffer[read_head], 64);
        read_head += readed;
        os_printf("Readed %d\n", readed);
        if(read_head > 1024 - 63)
            read_allow = 1;
	}
    else
        read_allow = 1;

    if(read_allow)
    {
        if(os_strlen(buffer) > 512)
        {
            os_memset(str, 0, 128);
            first = os_strstr(buffer,"$GPRMC");
            second = os_strchr(first+1, dollar);

            if(!first || !second)
            {
                read_head = 0;
                return;
            }

            os_memcpy(str, first, second-first);
            if(os_strlen(str) < 64)
            {
                read_head = 0;
                return;
            }
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
            locations[device_info.device_id].hour = atoi(temp);
            os_strncpy(temp, &time[2], 2);
            locations[device_info.device_id].min = atoi(temp);
            os_strncpy(temp, &time[4], 2);
            locations[device_info.device_id].sec = atoi(temp);
            os_strncpy(&locations[device_info.device_id].latitute[0], lat, os_strlen(lat)); 
            os_strncpy(&locations[device_info.device_id].longitute[0], lon, os_strlen(lon));
            locations[device_info.device_id].sheep_id = device_info.device_id;

            valid_location[device_info.device_id] = 1;
            os_memset(str, 0, 144);
            os_memset(buffer, 0, 1024);

            os_timer_disarm(&read_gps_uart_timer);//Cancel timer timing
            os_timer_setfn(&read_gps_uart_timer,(os_timer_func_t *)start_gps_read,NULL);//Set the timer callback function
            os_timer_arm(&read_gps_uart_timer,20000,0);//Turn on the software timer for 500ms
            read_head = 0;
        }
    }
}

void ICACHE_FLASH_ATTR sendReceivedLocations()
{
    ts_message message;
    message.message_type = SLAVE_LOCATION;
    if(valid_location[current_sheep] == 1)
    {
        os_memcpy(message.data, &locations[current_sheep], sizeof(ts_sheep_location));
        esp_now_send(NULL,(u8*)&message, sizeof(ts_message));
    }
    current_sheep +=1;
    if(current_sheep == device_info.pair_num)
        current_sheep = 1;
}

