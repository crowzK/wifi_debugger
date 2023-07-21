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

#ifndef _STATUS_HPP_
#define _STATUS_HPP_

#include <stdint.h>
#include <mutex>
#include "led.hpp"

//! To indicate Debugger status
class Status
{
public:
    //! \brief Create Status single tone instance.
    static Status& create();

    void on(bool on = true);
    void blink();

protected:
#if (CONFIG_M5STACK_CORE | CONFIG_TTGO_T1)
    static constexpr int cLedGpio = 22;
#elif CONFIG_WIFI_DEBUGGER_V_0_1
    static constexpr int cLedGpio = GPIO_NUM_MAX;
#elif CONFIG_WIFI_DEBUGGER_V_0_2
    static constexpr int cLedGpio = 0;
#elif CONFIG_WIFI_DEBUGGER_V_0_4
    static constexpr int cLedGpio = 4;
#endif
    std::recursive_mutex mMutex;
    Led mLed;

    Status();
    ~Status() = default;
};

#endif //_STATUS_HPP_
