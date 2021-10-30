#ifndef UASRT_HPP
#define UASRT_HPP

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void start_uart_service(QueueHandle_t txQ, QueueHandle_t rxQ);

#endif //UASRT_HPP