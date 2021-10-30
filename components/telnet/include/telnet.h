#ifndef TELNET_HPP
#define TELNET_HPP

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void start_telnet(QueueHandle_t txQ, QueueHandle_t rxQ);

#endif