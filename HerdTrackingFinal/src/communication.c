#include "communication.h"
#include "gprs_utils.h"

os_timer_t master_exist_timer;
os_timer_t slave_shutdown_timer;
bool master_exist_in_slave = false;
bool shutdown_signal = false;


ts_message master_ready = {
    .message_type = MASTER_EXIST
};

ts_message master_shutdown = {
    .message_type = MASTER_SHUTDOWN
};

void ICACHE_FLASH_ATTR slave_shutdown()
{
    system_deep_sleep_set_option(1);	//Option 1 = Reboot with radio recalibration and wifi on.
    system_deep_sleep(60000000);
}


void ICACHE_FLASH_ATTR send_master_exist()
{
    esp_now_send(NULL, (u8*)&master_ready, sizeof(ts_message));
}

void ICACHE_FLASH_ATTR master_receive_cb(u8 *mac_addr, u8 *data, u8 len)
{
    os_printf("Master receive!\n");
    ts_message message;
    ts_sheep_location location;
    os_memcpy(&message, data, len);
    switch (message.message_type)
    {
    case MASTER_EXIST:
        os_printf("Already master!\n");
        break;
    case SLAVE_LOCATION:
        os_memcpy(&location, message.data, sizeof(ts_sheep_location));
        os_memcpy(&locations[location.sheep_id], &location, sizeof(ts_sheep_location));
        valid_location[location.sheep_id] = 1;
        os_printf("Location info received from sheep %d!\n", location.sheep_id);
        break;
    default:
        os_printf("Unknown message type!\n");
        break;
    }
}

void ICACHE_FLASH_ATTR master_send_cb(u8 *mac_addr, u8 status)
{
    os_printf("Master send status to mac: %02x:%02x:%02x:%02x:%02x:%02x --> %d\n", MAC2STR(mac_addr), status);
}

void ICACHE_FLASH_ATTR slave_receive_cb(u8 *mac_addr, u8 *data, u8 len)
{
    os_printf("Slave receive!\n");
    ts_message message;
    ts_sheep_location location;
    os_memcpy(&message, data, len);
    switch (message.message_type)
    {
    case MASTER_EXIST:
        os_printf("Master exist!\n");
        if(!master_exist_in_slave)
        {
            os_timer_disarm(&master_exist_timer);
            os_timer_setfn(&master_exist_timer, send_master_exist, NULL);
            os_timer_arm(&master_exist_timer, 10000, 1);

            os_timer_disarm(&stream_received_location_timer);//Cancel timer timing
            os_timer_setfn(&stream_received_location_timer,(os_timer_func_t *)sendReceivedLocations,NULL);//Set the timer callback function
            os_timer_arm(&stream_received_location_timer,5000,1);//Turn on the software timer for 500ms

            os_timer_disarm(&slave_shutdown_timer);
            os_timer_setfn(&slave_shutdown_timer, slave_shutdown, NULL);
            os_timer_arm(&slave_shutdown_timer, 300000, 0);
        }
        break;
    case SLAVE_LOCATION:
        os_memcpy(&location, message.data, sizeof(ts_sheep_location));
        os_memcpy(&locations[location.sheep_id], &location, sizeof(ts_sheep_location));
        valid_location[location.sheep_id] = 1;
        os_printf("Location info received from sheep %d!\n", location.sheep_id);
        break;
    case MASTER_SHUTDOWN:
        if(!shutdown_signal)
        {
            shutdown_signal = true;
            os_timer_disarm(&read_gps_uart_timer);
            os_timer_disarm(&stream_received_location_timer);
            os_timer_disarm(&master_exist_timer);
            os_printf("Slave shutting down in 10 second\n");
            esp_now_send(NULL, (u8*)&master_shutdown, sizeof(ts_message));
            os_timer_disarm(&slave_shutdown_timer);
            os_timer_setfn(&slave_shutdown_timer, slave_shutdown, NULL);
            os_timer_arm(&slave_shutdown_timer, 10000, 0);
        }
        break;
    default:
        os_printf("Unknown message type!\n");
        break;
    }
}

void ICACHE_FLASH_ATTR slave_send_cb(u8 *mac_addr, u8 status)
{
    os_printf("Slave send status: %d\n", status);
}

void ICACHE_FLASH_ATTR init_esp_now()
{
    if(esp_now_init() == 0)
        os_printf("ESP-Now initialized!\n");
    else
        os_printf("ESP-Now init failed!\n");

    if(esp_now_set_self_role(ESP_NOW_ROLE_COMBO) == 0)
        os_printf("ESP-Now role selected!\n");
    else
        os_printf("ESP-Now role select failed!\n");
    
    if(device_info.device_id == 0)
    {
        if(esp_now_register_recv_cb(master_receive_cb) == 0)
            os_printf("ESP-Now recv cb setted!\n");
        else
            os_printf("ESP-Now recv cb failed!\n");

        if(esp_now_register_send_cb(master_send_cb) == 0)
            os_printf("ESP-Now send cb setted!\n");
        else
            os_printf("ESP-Now send cb failed!\n");
    }
    else
    {
        if(esp_now_register_recv_cb(slave_receive_cb) == 0)
            os_printf("ESP-Now recv cb setted!\n");
        else
            os_printf("ESP-Now recv cb failed!\n");

        if(esp_now_register_send_cb(slave_send_cb) == 0)
            os_printf("ESP-Now send cb setted!\n");
        else
            os_printf("ESP-Now send cb failed!\n");
    }

    for(int i = 0; i < device_info.pair_num; i++)
        if(i != device_info.device_id && esp_now_add_peer(device_info.peerMAC[i],ESP_NOW_ROLE_COMBO, 1, masterKey, 16) == 0)
            os_printf("Device %d added as pair\n", i);
}