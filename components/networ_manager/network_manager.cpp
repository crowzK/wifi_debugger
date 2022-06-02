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

#include <stdio.h>
#include <stdio_ext.h>
#include <string.h>
#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_system.h"
#include "network_manager.hpp"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "NetworkManager";

//-------------------------------------------------------------------
// NetworkManager
//-------------------------------------------------------------------
void NetworkManager::wifiEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    NetworkManager* pNm = reinterpret_cast<NetworkManager*>(arg);
    pNm->eventHandler(event_base, event_id, event_data);
}

void NetworkManager::eventHandler(esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    auto now=std::chrono::steady_clock::now();
    if(not mMutex.try_lock_until(now + std::chrono::seconds(2)))
    {
        ESP_LOGE(TAG, "mMutex try fail");
        return;
    }
    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "Esp wifi start");
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_STOP:
            ESP_LOGI(TAG, "Esp wifi stop");
            esp_wifi_disconnect();
            break;
        default:
            break;
        }
    } 
    else if (event_base == IP_EVENT)
    {
        switch (event_id)
        {
        case IP_EVENT_STA_GOT_IP:
        {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
            xEventGroupSetBits(mWifiEventGroup, WIFI_CONNECTED_EVENT);
            break;
        }
        default:
            break;
        }
    }
    else if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "Disconnected. Connecting to the AP again...");
            esp_wifi_connect();
            break;
        default:
            break;
        }
    }
    mMutex.unlock();
}

bool NetworkManager::removeProvision()
{
    wifi_config_t wifi_config{};
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    return true;
}

bool NetworkManager::init()
{
    std::lock_guard<std::recursive_timed_mutex> lock(mMutex);

    /* Initialize NVS partition */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        /* NVS partition was truncated
         * and needs to be erased */
        ESP_ERROR_CHECK(nvs_flash_erase());

        /* Retry nvs_flash_init */
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    /* Initialize TCP/IP */
    ESP_ERROR_CHECK(esp_netif_init());

    /* Initialize the event loop */
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    mWifiEventGroup = xEventGroupCreate();

    /* Register our event handler for Wi-Fi, IP and Provisioning related events */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifiEventHandler, this));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifiEventHandler, this));

    /* Initialize Wi-Fi including netif with default config */
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config;
    esp_wifi_set_storage(WIFI_STORAGE_FLASH);
    ESP_ERROR_CHECK( esp_wifi_get_config(WIFI_IF_STA, &wifi_config) );
    if(strlen((char*)wifi_config.sta.ssid) != 0)
    {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_start());
    
        ESP_LOGI(TAG, "SSID:%s", wifi_config.sta.ssid);
        ESP_LOGI(TAG, "PASSWORD:%s", wifi_config.sta.password);
    }
    else
    {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
        ESP_ERROR_CHECK(esp_wifi_start());
    }

    esp_wifi_set_ps(WIFI_PS_NONE);
    mPairBut.enable();
    return true;
}

bool NetworkManager::join(const char *ssid, const char *pass, int timeout_ms)
{
    {
        std::lock_guard<std::recursive_timed_mutex> lock(mMutex);

        wifi_config_t wifi_config{};
        strlcpy((char *) wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
        if (pass) {
            strlcpy((char *) wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));
        }
        ESP_ERROR_CHECK( esp_wifi_stop() );
        ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_FLASH) );
        ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
        ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
        ESP_ERROR_CHECK( esp_wifi_start() );
    }

    int bits = xEventGroupWaitBits(mWifiEventGroup, WIFI_CONNECTED_EVENT,
                                   pdFALSE, pdTRUE, timeout_ms / portTICK_PERIOD_MS);
    return (bits & WIFI_CONNECTED_EVENT) != 0;
}

NetworkManager &NetworkManager::create()
{
    static NetworkManager manager;
    return manager;
}

NetworkManager::NetworkManager() :
    mPairBut(cParingPin, [this](Button::Event evt)
    {
        if(evt == Button::Event::eLongPress)
        {
            ESP_LOGI(TAG, "Remove provision info");
            removeProvision();
            esp_restart();
        }
    })
{
#if CONFIG_WIFI_DEBUGGER_V_0_1
    gpio_reset_pin(gpio_num_t::GPIO_NUM_20);
    gpio_set_direction(gpio_num_t::GPIO_NUM_20, GPIO_MODE_OUTPUT);
    gpio_set_level(gpio_num_t::GPIO_NUM_20, 0);
#endif
}


//-------------------------------------------------------------------
// WifiCmd
//-------------------------------------------------------------------
WifiCmd::JoinArgs WifiCmd::mJoinArgs;

WifiCmd::WifiCmd() :
    cCmd{.command = "join", .help = "Join WiFi AP as a station", .hint = nullptr, .func = connect, .argtable = &mJoinArgs}
{
    mJoinArgs.timeout = arg_int0(NULL, "timeout", "<t>", "Connection timeout, ms");
    mJoinArgs.ssid = arg_str1(NULL, NULL, "<ssid>", "SSID of AP");
    mJoinArgs.password = arg_str0(NULL, NULL, "<pass>", "PSK of AP");
    mJoinArgs.end = arg_end(2);

    esp_console_cmd_register(&cCmd);
}

WifiCmd::~WifiCmd()
{
    //esp_console_cmd_register(&cCmd);
}

int WifiCmd::connect(int argc, char** argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &mJoinArgs);
    if (nerrors != 0) {
        arg_print_errors(stderr, mJoinArgs.end, argv[0]);
        return 1;
    }
    ESP_LOGI(__func__, "Connecting to '%s'",
             mJoinArgs.ssid->sval[0]);

    /* set default value*/
    if (mJoinArgs.timeout->count == 0) {
        mJoinArgs.timeout->ival[0] = 10000;
    }

    bool connected = NetworkManager::create().join(mJoinArgs.ssid->sval[0],
                               mJoinArgs.password->sval[0],
                               mJoinArgs.timeout->ival[0]);
    if (!connected) {
        ESP_LOGW(__func__, "Connection timed out");
        return 1;
    }
    ESP_LOGI(__func__, "Connected");
    return 0;
}

//-------------------------------------------------------------------
// WifiInfoCmd
//-------------------------------------------------------------------
WifiInfoCmd::WifiInfoCmd() :
    cCmd{.command = "info", .help = "print system information", .hint = nullptr, .func = info, .argtable = nullptr}

{
    esp_console_cmd_register(&cCmd);
}

int WifiInfoCmd::info(int argc, char** argv)
{
    wifi_config_t wifi_config;
    esp_wifi_set_storage(WIFI_STORAGE_FLASH);
    esp_wifi_get_config(WIFI_IF_STA, &wifi_config);

    printf("\r\nSSID:%s\r\n", wifi_config.sta.ssid);
    printf("PASSWORD:%s\r\n", wifi_config.sta.password);
    return 0;
}