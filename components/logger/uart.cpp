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
// UartThread
////////////////////////////////////////////////////////////////////
UartThread::UartThread(const char* taskName) :
    cName(taskName),
    run(false)
{

}

UartThread::~UartThread()
{
    ESP_LOGI(cName, "terminate");
    stop();
}

void UartThread::stop()
{
    if(run)
    {
        run = false;
        threadHandle.join();
    }
}

bool UartThread::isRun() const
{
    return run;
}

void UartThread::start(BlockingQueue<std::vector<uint8_t>>& queue)
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
UartTx::UartTx(int uartPortNum):
    UartThread(__func__),
    cUartNum(uartPortNum)
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
            uart_write_bytes(cUartNum, msg.data(), msg.size());
        }
    }
}

////////////////////////////////////////////////////////////////////
// UartRx
////////////////////////////////////////////////////////////////////
UartRx::UartRx(int uartPortNum):
    UartThread(__func__),
    cUartNum(uartPortNum)
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
        const int rxBytes = uart_read_bytes(cUartNum, rcvBuffer.data(), RX_BUF_SIZE, 10);
        if(rxBytes)
        {
            rcvBuffer.resize(rxBytes);
            queue.push(rcvBuffer, std::chrono::milliseconds(100));
        }
    }
}

////////////////////////////////////////////////////////////////////
// UartService
////////////////////////////////////////////////////////////////////
UartService::UartService(int uartPortNum) :
    cUartNum(uartPortNum),
    txTask(uartPortNum),
    rxTask(uartPortNum)
{

}

UartService::~UartService()
{
    stop();
}

bool UartService::isRun() const
{
    return txTask.isRun() or rxTask.isRun();
}

void UartService::start(int baudRate, BlockingQueue<std::vector<uint8_t>>& _txQ, BlockingQueue<std::vector<uint8_t>>& _rxQ)
{
    const uart_config_t uart_config = 
    {
        .baud_rate = baudRate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    // We won't use a buffer for sending data.
    ESP_ERROR_CHECK(uart_driver_install(cUartNum, RX_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(cUartNum, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(cUartNum, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    
    if(not txTask.isRun())
    {
        txTask.start(_txQ);
    }
    if(not rxTask.isRun())
    {
        rxTask.start(_rxQ);
    }
}

void UartService::stop()
{
    uart_driver_delete(cUartNum);
    if(txTask.isRun())
    {
        txTask.stop();
    }
    if(rxTask.isRun())
    {
        rxTask.stop();
    }
}

////////////////////////////////////////////////////////////////////
// start_uart_service
////////////////////////////////////////////////////////////////////
void start_uart_service(BlockingQueue<std::vector<uint8_t>>& _txQ, BlockingQueue<std::vector<uint8_t>>& _rxQ)
{
    static UartService uartService(UART_NUM_2);

    if(not uartService.isRun())
    {
        uartService.start(230400, _txQ, _rxQ);
    }
}
