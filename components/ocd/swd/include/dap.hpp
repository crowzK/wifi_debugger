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

#include <stdint.h>
#include "driver/gpio.h"

//! Debug access port
class Dap
{
public:
    Dap();
    ~Dap();

    //! hardware reset
    bool setReset(bool assert);

private:
#if (CONFIG_M5STACK_CORE | CONFIG_TTGO_T1 | CONFIG_WIFI_DEBUGGER_V_0_1)
    static constexpr gpio_num_t cPinReset = (gpio_num_t)12;
#elif CONFIG_WIFI_DEBUGGER_V_0_2
    static constexpr gpio_num_t cPinReset = (gpio_num_t)8;
#elif CONFIG_WIFI_DEBUGGER_V_0_4
    static constexpr gpio_num_t cPinReset = (gpio_num_t)2;
#elif CONFIG_WIFI_DEBUGGER_V_0_6
    static constexpr gpio_num_t cPinReset = (gpio_num_t)38;
#endif
};