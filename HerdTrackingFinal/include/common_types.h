#ifndef COMMON_H
#define COMMON_H

typedef enum
{
    MASTER_EXIST = 10,
    SLAVE_LOCATION = 20,
    MASTER_SHUTDOWN = 30,
}  __attribute__((aligned(4))) te_message_types;

typedef struct 
{
    te_message_types message_type;
    char data[128];
}  __attribute__((aligned(4))) ts_message;


#endif