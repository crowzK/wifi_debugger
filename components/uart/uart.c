/* UART asynchronous example, that uses separate RX and TX tasks

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include "uart.h"
#include "../../components/telnet/include/msg_buffer.h"

static const int RX_BUF_SIZE = 1024;

static QueueHandle_t txQ;
static QueueHandle_t rxQ;

#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)

void init(void) {
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    // We won't use a buffer for sending data.
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_2, RX_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_2, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

static void tx_task(void *arg)
{
    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    while (1)
    {
        MsgBuffer msg;
        if(xQueueReceive(txQ, &msg, 1000 / portTICK_RATE_MS))
        {
            if(msg.len > 0 && msg.pMessge)
            {
                uart_write_bytes(UART_NUM_2, msg.pMessge, msg.len);
            }
            if(msg.pMessge)
            {
                free(msg.pMessge);
            }
        }
    }
}

static void rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    MsgBuffer msg = {};
    while (1) 
    {
        if(msg.pMessge == NULL)
        {
            msg.pMessge = (uint8_t*) malloc(RX_BUF_SIZE+1);
        }
        const int rxBytes = uart_read_bytes(UART_NUM_2, msg.pMessge, RX_BUF_SIZE, 1000 / portTICK_RATE_MS);
        if(rxBytes)
        {
            msg.len = rxBytes;
            BaseType_t result = xQueueSendToBack(rxQ, &msg, 10);
            if(errQUEUE_FULL == result)
            {
                free(msg.pMessge);
            }
            msg.pMessge = NULL;
        }
    }

    if(msg.pMessge)
    {
        free(msg.pMessge);
    }
}

void start_uart_service(QueueHandle_t _txQ, QueueHandle_t _rxQ)
{
    txQ = _txQ;
    rxQ = _rxQ;

    init();

    if(rxQ)
    {
        xTaskCreate(rx_task, "uart_rx_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
    }
    if(txQ)
    {
        xTaskCreate(tx_task, "uart_tx_task", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);
    }
}
