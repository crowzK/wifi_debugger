/* UART asynchronous example, that uses separate RX and TX tasks

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <thread>
#include <memory>

#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include "uart.hpp"

#include "../blocking_queue/include/blocking_queue.hpp"

static const int RX_BUF_SIZE = 1024;

#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)

////////////////////////////////////////////////////////////////////
// UartService
////////////////////////////////////////////////////////////////////
UartService::UartService(const char* taskName) :
    cName(taskName),
    run(false)
{

}

UartService::~UartService()
{
    ESP_LOGI(cName, "terminate");
    stop();
}

void UartService::stop()
{
    if(run)
    {
        run = false;
        threadHandle.join();
    }
}

bool UartService::isRun() const
{
    return run;
}

void UartService::start(BlockingQueue<std::vector<uint8_t>>& queue)
{
    if(not run)
    {
        run = true;
        threadHandle = std::thread([this, &queue]{thread(queue);});
    }
}

////////////////////////////////////////////////////////////////////
// UartTx
////////////////////////////////////////////////////////////////////
UartTx::UartTx():
    UartService(__func__)
{

}

void UartTx::thread(BlockingQueue<std::vector<uint8_t>>& queue)
{
    esp_log_level_set(cName, ESP_LOG_INFO);
    ESP_LOGI(cName, "start");

    while(run)
    {
        std::vector<uint8_t> msg;
        if(queue.pop(msg, std::chrono::milliseconds(1000)) and msg.size())
        {
            uart_write_bytes(UART_NUM_2, msg.data(), msg.size());
        }
    }
}

////////////////////////////////////////////////////////////////////
// UartRx
////////////////////////////////////////////////////////////////////
UartRx::UartRx():
    UartService(__func__)
{

}

void UartRx::thread(BlockingQueue<std::vector<uint8_t>>& queue)
{
    esp_log_level_set(cName, ESP_LOG_INFO);
    ESP_LOGI(cName, "start");

    while (run) 
    {
        std::vector<uint8_t> rcvBuffer;
        rcvBuffer.resize(RX_BUF_SIZE + 1);
        const int rxBytes = uart_read_bytes(UART_NUM_2, rcvBuffer.data(), RX_BUF_SIZE, 10);
        if(rxBytes)
        {
            queue.push(rcvBuffer, std::chrono::milliseconds(100));
        }
    }
}

////////////////////////////////////////////////////////////////////
// start_uart_service
////////////////////////////////////////////////////////////////////
void start_uart_service(BlockingQueue<std::vector<uint8_t>>& _txQ, BlockingQueue<std::vector<uint8_t>>& _rxQ)
{
    static UartRx uartRx;
    static UartTx uartTx;

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

    if(not uartRx.isRun())
    {
        uartRx.start(_rxQ);
    }
    if(not uartTx.isRun())
    {
        uartTx.start(_txQ);
    }
}
