#ifndef USER_CONFIG_H
#define USER_CONFIG_H

typedef struct
{  
    uint8_t sheep_id;
    char latitude[16];
    char longitude[16];
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
} sheep_loc;

#endif