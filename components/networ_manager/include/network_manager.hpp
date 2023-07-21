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
#include "console.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

class WifiCmd : protected Cmd
{
public:
    WifiCmd();
    ~WifiCmd() = default;

private:
    bool excute(const std::vector<std::string>& args) override;
    std::string help() override;
};

class WifiInfoCmd : protected Cmd
{
public:
    WifiInfoCmd();
    ~WifiInfoCmd() = default;

protected:
   // const esp_console_cmd_t cCmd;
    bool excute(const std::vector<std::string>& args) override;
    std::string help() override;
};

class NetworkManager
{
public:
    static NetworkManager& create();

    bool init();
    bool removeProvision();
    bool join(const char *ssid, const char *pass, int timeout_ms);

protected:
    std::recursive_timed_mutex mMutex;

#if CONFIG_M5STACK_CORE
    static constexpr gpio_num_t cParingPin = gpio_num_t::GPIO_NUM_MAX;
#elif CONFIG_TTGO_T1
    static constexpr gpio_num_t cParingPin = gpio_num_t::GPIO_NUM_MAX;
#elif CONFIG_WIFI_DEBUGGER_V_0_1
    static constexpr gpio_num_t cParingPin = gpio_num_t::GPIO_NUM_21;
#elif CONFIG_WIFI_DEBUGGER_V_0_2
    static constexpr gpio_num_t cParingPin = gpio_num_t::GPIO_NUM_9;
#elif CONFIG_WIFI_DEBUGGER_V_0_4
    static constexpr gpio_num_t cParingPin = gpio_num_t::GPIO_NUM_0;
#endif

    Button mPairBut;
    WifiCmd mWifiCmd;
    WifiInfoCmd mWifiInfo;
    static constexpr int WIFI_CONNECTED_EVENT = BIT0;
    EventGroupHandle_t mWifiEventGroup;

    NetworkManager();
    ~NetworkManager() = default;
    static void wifiEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    void eventHandler(esp_event_base_t event_base, int32_t event_id, void *event_data);
};

#endif // NETWORK_MAANGER_HPP
