#include "osapi.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"

os_timer_t checktimer_wifestate;//Timer structure
os_timer_t send_timer;
struct espconn user_tcp_conn;//tcp structure

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

void ICACHE_FLASH_ATTR user_tcp_sent_cb(void *arg)//Send data successfully callback function
{
	os_printf("Send data successfully!\r\n");
}
void ICACHE_FLASH_ATTR user_tcp_discon_cb(void *arg)//The connection is normally disconnected callback function
{
	os_printf("Disconnected successfully!\r\n");
}
void ICACHE_FLASH_ATTR user_tcp_recv_cb(void *arg,char *pdata,unsigned short len)//Receive data successfully callback function
{
	os_printf("Data received: %s\r\n",pdata);
	espconn_sent((struct espconn *)arg,"Received successfully!",strlen("Data received!"));
}
void ICACHE_FLASH_ATTR user_tcp_recon_cb(void *arg,sint8 err)//tcp abnormal disconnection callback function
{
	os_printf("Connection error, error code: %d",err);
	espconn_connect((struct espconn *)arg);//reconnect
}

void ICACHE_FLASH_ATTR send_data(void)
{
	espconn_send(&user_tcp_conn, "Hello from slave sheep", os_strlen("Hello from slave sheep"));
}

void ICACHE_FLASH_ATTR user_tcp_connent_cb(void *arg)//tcp connection establishment success callback function
{
	struct espconn *pespconn = arg;
	os_printf("tcp connection successful");
	espconn_regist_recvcb(pespconn,user_tcp_recv_cb);//Register and receive a successful callback function
	espconn_regist_sentcb(pespconn,user_tcp_sent_cb);//Register and send successfully callback function
	espconn_regist_disconcb(pespconn,user_tcp_discon_cb);//Register the normal disconnection callback function
    os_timer_disarm(&send_timer);//Cancel timer timing
	os_timer_setfn(&send_timer,(os_timer_func_t *)send_data,NULL);//Set the timer callback function
	os_timer_arm(&send_timer,5000,1);//Turn on the software timer for 500ms
}

void ICACHE_FLASH_ATTR my_station_init(struct ip_addr *remote_ip,
		struct ip_addr *local_ip, int remote_port) {
	user_tcp_conn.proto.tcp = (esp_tcp *) os_zalloc(sizeof(esp_tcp));  //Allocate space
	user_tcp_conn.type = ESPCONN_TCP;  //Set the type to TCP protocol
	os_memcpy(user_tcp_conn.proto.tcp->local_ip, local_ip, 4);
	os_memcpy(user_tcp_conn.proto.tcp->remote_ip, remote_ip, 4);
	user_tcp_conn.proto.tcp->local_port = espconn_port();  //Local port
	user_tcp_conn.proto.tcp->remote_port = remote_port;  //Destination port
	//Register the successful connection callback function and reconnect callback function
	espconn_regist_connectcb(&user_tcp_conn, user_tcp_connent_cb);//Register the callback function after the TCP connection is successfully established
	espconn_regist_reconcb(&user_tcp_conn, user_tcp_recon_cb);//Register the callback function when the TCP connection is abnormally disconnected, you can reconnect in the callback function
	//Enable connection
	espconn_connect(&user_tcp_conn);
}
void ICACHE_FLASH_ATTR check_WIFIstate(void)
{
	uint8 getstate;//Define the connection state variable
	getstate=wifi_station_get_connect_status();//Get sta connection status
	if(getstate == STATION_GOT_IP)//Check if sta is connected to ap
	{
		os_printf("WIFI connection is successful!");
		os_timer_disarm(&checktimer_wifestate);//Turn off the software timer
		struct ip_info info;
		const char remote_ip[4]={192,168,4,1};//tcp server ip address, the server must first use this ip to open
		wifi_get_ip_info(STATION_IF,&info);//Query the ip of the module station
		my_station_init((struct ip_addr *)remote_ip,&info.ip,8000);//Connect to port 8000 of the target server
	}
}

void ICACHE_FLASH_ATTR tcp_client_init()//tcp cilent initialization
{
	wifi_set_opmode(0x01);//Set to station mode to save flash
	struct station_config stationconf;//stationa structure
	os_strcpy(stationconf.ssid,"MasterSheep");//Set the username of the incoming route
	os_strcpy(stationconf.password,"masterpass");//Set the password to connect to the router
	wifi_station_set_config(&stationconf);//Set the wifi interface configuration and save it to flash
	wifi_station_connect();//Connect to the router
	os_timer_disarm(&checktimer_wifestate);//Cancel timer timing
	os_timer_setfn(&checktimer_wifestate,(os_timer_func_t *)check_WIFIstate,NULL);//Set the timer callback function
	os_timer_arm(&checktimer_wifestate,200,1);//Turn on the software timer for 500ms
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
	tcp_client_init();
}