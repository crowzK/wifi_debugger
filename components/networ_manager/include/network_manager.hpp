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

#ifndef NETWORK_MAANGER_HPP
#define NETWORK_MAANGER_HPP

#include <functional>
#include <mutex>
#include "driver/gpio.h"
#include "button.hpp"

class NetworkManager
{
public:
    static NetworkManager& create();

    bool init();
    bool provision();
    bool disconnect();
    bool removeProvision();

protected:
#if CONFIG_M5STACK_CORE
    static constexpr gpio_num_t cParingPin = gpio_num_t::GPIO_NUM_MAX;
#elif CONFIG_TTGO_T1
    static constexpr gpio_num_t cParingPin = gpio_num_t::GPIO_NUM_MAX;
#elif CONFIG_WIFI_DEBUGGER_V_0_1
    static constexpr gpio_num_t cParingPin = gpio_num_t::GPIO_NUM_21;
#elif CONFIG_WIFI_DEBUGGER_V_0_2
    static constexpr gpio_num_t cParingPin = gpio_num_t::GPIO_NUM_9;
#endif

    Button mPairBut;
    NetworkManager();
    ~NetworkManager() = default;
};

#endif // NETWORK_MAANGER_HPP
