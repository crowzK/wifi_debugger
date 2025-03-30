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
#include "argtable3/argtable3.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_system.h"
#include "network_manager.hpp"
#include "status.hpp"
#include "setting.hpp"

static const char* TAG = "NetworkManager";

//-------------------------------------------------------------------
// NetworkManager
//-------------------------------------------------------------------
void NetworkManager::wifiEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	NetworkManager* pNm = reinterpret_cast<NetworkManager*>(arg);
	pNm->eventHandler(event_base, event_id, event_data);
}

void NetworkManager::eventHandler(esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	auto now = std::chrono::steady_clock::now();
	if(not mMutex.try_lock_until(now + std::chrono::seconds(2)))
	{
		ESP_LOGE(TAG, "mMutex try fail");
		return;
	}
	if(event_base == WIFI_EVENT)
	{
		switch(event_id)
		{
		case WIFI_EVENT_STA_START:
			ESP_LOGI(TAG, "Esp wifi start");
			esp_wifi_connect();
			Status::create().blink();
			break;
		case WIFI_EVENT_STA_STOP:
			ESP_LOGI(TAG, "Esp wifi stop");
			esp_wifi_disconnect();
			Status::create().blink();
			break;
		default:
			break;
		}
	}
	else if(event_base == IP_EVENT)
	{
		switch(event_id)
		{
		case IP_EVENT_STA_GOT_IP:
		{
			ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
			ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
			xEventGroupSetBits(mWifiEventGroup, WIFI_CONNECTED_EVENT);
			Status::create().on(true);
			break;
		}
		default:
			break;
		}
	}
	else if(event_base == WIFI_EVENT)
	{
		switch(event_id)
		{
		case WIFI_EVENT_STA_DISCONNECTED:
			ESP_LOGI(TAG, "Disconnected. Connecting to the AP again...");
			esp_wifi_connect();
			Status::create().blink();
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
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
	return true;
}

bool NetworkManager::init()
{
	std::lock_guard<std::recursive_timed_mutex> lock(mMutex);

	// to init nvs
	Setting::create();

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
	ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_config));
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

bool NetworkManager::join(const char* ssid, const char* pass, int timeout_ms)
{
	{
		std::lock_guard<std::recursive_timed_mutex> lock(mMutex);

		wifi_config_t wifi_config{};
		strlcpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
		if(pass)
		{
			strlcpy((char*)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));
		}
		ESP_ERROR_CHECK(esp_wifi_stop());
		ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
		ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
		ESP_ERROR_CHECK(esp_wifi_start());
	}

	int bits = xEventGroupWaitBits(mWifiEventGroup, WIFI_CONNECTED_EVENT,
		pdFALSE, pdTRUE, timeout_ms / portTICK_PERIOD_MS);
	return (bits & WIFI_CONNECTED_EVENT) != 0;
}

NetworkManager& NetworkManager::create()
{
	static NetworkManager manager;
	return manager;
}

NetworkManager::NetworkManager() : mPairBut(cParingPin, [this](Button::Event evt)
	{
		if(evt == Button::Event::eLongPress)
		{
			ESP_LOGI(TAG, "Remove provision info");
			removeProvision();
			esp_restart();
		} })
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

WifiCmd::WifiCmd() : Cmd("join")
{
}

bool WifiCmd::excute(const std::vector<std::string>& args)
{
	if(args.size() < 3)
	{
		printf("%s\n", help().c_str());
		return false;
	}

	ESP_LOGI(__func__, "Connecting to '%s'", args.at(1).c_str());
	constexpr int timeOut = 10000;

	bool connected = NetworkManager::create().join(args.at(1).c_str(),
		args.at(2).c_str(),
		timeOut);
	if(!connected)
	{
		ESP_LOGW(__func__, "Connection timed out");
		return false;
	}
	ESP_LOGI(__func__, "Connected");
	return true;
}

std::string WifiCmd::help()
{
	return std::string("join#<ssid>#<password>");
}

//-------------------------------------------------------------------
// WifiInfoCmd
//-------------------------------------------------------------------
WifiInfoCmd::WifiInfoCmd() : 
	Cmd("info")
{
}

std::string WifiInfoCmd::help()
{
	return std::string("it will print wifi ap info");
}

bool WifiInfoCmd::excute(const std::vector<std::string>& args)
{
	wifi_config_t wifi_config;
	esp_wifi_set_storage(WIFI_STORAGE_FLASH);
	esp_wifi_get_config(WIFI_IF_STA, &wifi_config);

	printf("\r\nSSID:%s\r\n", wifi_config.sta.ssid);
	printf("PASSWORD:%s\r\n", wifi_config.sta.password);

	// iterate over active interfaces, and print out IPs of "our" netifs
	esp_netif_t* netif = NULL;
	for(int i = 0; i < esp_netif_get_nr_of_ifs(); ++i)
	{
		netif = esp_netif_next_unsafe(netif);
		esp_netif_ip_info_t ip;
		ESP_ERROR_CHECK(esp_netif_get_ip_info(netif, &ip));
		printf("IP: " IPSTR "\n", IP2STR(&ip.ip));
		printf("GW: " IPSTR "\n", IP2STR(&ip.gw));
	}

	return true;
}
