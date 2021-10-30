#ifndef MSG_BUFFER_H
#define MSG_BUFFER_H

#include <stdint.h>


typedef struct
{
    uint32_t len;
    uint8_t* pMessage;
} MsgBuffer;


#endif