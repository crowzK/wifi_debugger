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

#pragma once

#include "swd.hpp"
#include "driver/gpio.h"

class GpioSwd : public Swd
{
public:
    GpioSwd();
    ~GpioSwd() = default;

    uint32_t sequence(uint64_t data, uint8_t bitLength) override;
    Response write(Cmd cmd, uint32_t data) override;
    Response read(Cmd cmd, uint32_t& data) override;

protected:
#if (CONFIG_M5STACK_CORE | CONFIG_TTGO_T1)
    static constexpr gpio_num_t cPinSwClk = (gpio_num_t)23;
    static constexpr gpio_num_t cPinSwDio = (gpio_num_t)19;
#elif CONFIG_WIFI_DEBUGGER_V_0_1
    static constexpr gpio_num_t cPinSwClk = (gpio_num_t)4;
    static constexpr gpio_num_t cPinSwDio = (gpio_num_t)2;
#elif CONFIG_WIFI_DEBUGGER_V_0_2
    static constexpr gpio_num_t cPinSwClk = (gpio_num_t)6;
    static constexpr gpio_num_t cPinSwDio = (gpio_num_t)4;
#elif CONFIG_WIFI_DEBUGGER_V_0_4
    static constexpr gpio_num_t cPinSwClk = (gpio_num_t)41;
    static constexpr gpio_num_t cPinSwDio = (gpio_num_t)42;
#elif CONFIG_WIFI_DEBUGGER_V_0_6
    static constexpr gpio_num_t cPinSwClk = (gpio_num_t)39;
    static constexpr gpio_num_t cPinSwDio = (gpio_num_t)42;
#endif

    inline void delay();
    inline void clkCycle();
    inline void writeBit(bool bit);
    inline bool readBit();
    inline void sendRequest(uint32_t request);
    inline uint32_t readAck();
};
