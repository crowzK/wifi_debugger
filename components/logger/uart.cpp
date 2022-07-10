/*
Copyright (C) Yudoc Kim <craven@crowz.kr>
 
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
 
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
 
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <thread>
#include <memory>
#include <string>
#include <algorithm>
#include <iostream>
#include <sstream>

#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include "uart.hpp"

#include "blocking_queue.hpp"

static const int RX_BUF_SIZE = 1024;

//-------------------------------------------------------------------
// UartTx
//-------------------------------------------------------------------
UartTx::UartTx(int uartPortNum):
    Client(DebugMsgTx::create(), uartPortNum),
    cUartNum(uartPortNum)
{
}

bool UartTx::writeStr(const MsgProxy::Msg& msg)
{
    uart_write_bytes(cUartNum, msg.str.c_str(), msg.str.length());
    return true;
}

//-------------------------------------------------------------------
// UartRx
//-------------------------------------------------------------------
UartRx::UartRx(int uartPortNum):
    Task(__func__),
    cUartNum(uartPortNum)
{
    
}

std::string string_to_hex(const std::string& input)
{
    static const char hex_digits[] = "0123456789ABCDEF";

    std::string output;
    output.reserve(input.length() * 2);
    for (unsigned char c : input)
    {
        output.push_back(hex_digits[c >> 4]);
        output.push_back(hex_digits[c & 15]);
    }
    return output;
}

void UartRx::task()
{
    char buffer[RX_BUF_SIZE + 1];
    while(mRun)
    {
        std::vector<uint8_t> rcvBuffer;
        rcvBuffer.resize(RX_BUF_SIZE + 1);
        const int rxBytes = uart_read_bytes(cUartNum, buffer, RX_BUF_SIZE, 1);
        if(rxBytes)
        {
            buffer[rxBytes] = 0;
            DebugMsgRx::create().write(buffer);
        }
    }
}

//-------------------------------------------------------------------
// UartService
//-------------------------------------------------------------------

UartService& UartService::create()
{
    static UartService service;
    return service;
}

UartService::UartService() :
    mConfig{.baudRate = 1500000, .uartNum = 1}
{
    init(mConfig);
}

void UartService::init(const Config& cfg)
{
    mConfig = cfg;
    const uart_config_t uart_config = 
    {
        .baud_rate = mConfig.baudRate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    uart_driver_delete(mConfig.uartNum);

    // We won't use a buffer for sending data.
    ESP_ERROR_CHECK(uart_driver_install(mConfig.uartNum, RX_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(mConfig.uartNum, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(mConfig.uartNum, cTxPin, cRxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    
    pUartRx.reset();
    pUartTx.reset();
    pUartRx = std::make_unique<UartRx>(mConfig.uartNum);
    pUartRx->start();
    pUartTx = std::make_unique<UartTx>(mConfig.uartNum);
}
