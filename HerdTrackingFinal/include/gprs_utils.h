#ifndef GPRS_UTILS_H
#define GPRS_UTILS_H

#include "softuart.h"
#include "osapi.h"
#include "user_interface.h"
#include "espconn.h"

extern Softuart gprs_uart;
extern os_timer_t gprsAttach;
extern os_timer_t sendingSession; 
extern os_timer_t sendProcess;

#define CIPSHUT     "AT+CIPSHUT\r\n"
#define CIPMUX      "AT+CIPMUX=0\r\n"
#define CSTT        "AT+CSTT=\"internet\"\r\n"
#define CIICR       "AT+CIICR\r\n"
#define CIFSR       "AT+CIFSR\r\n"
#define CIPSSL      "AT+CIPSSL=1\r\n"
#define CIPSTART    "AT+CIPSTART=\"TCP\",\"herdtrackingsystem-default-rtdb.firebaseio.com\",443\r\n"
#define CIPSEND     "AT+CIPSEND\r\n"


void ICACHE_FLASH_ATTR initializeGPRS();
void ICACHE_FLASH_ATTR sendingSessionFunc();
#endif