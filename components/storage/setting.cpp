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

#include "setting.hpp"
#include <esp_log.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "nvs_handle.hpp"

static const char* TAG = "Setting";

Setting& Setting::create()
{
    static Setting setting;
    return setting;
}

uint32_t Setting::getDebugUartBaud() const
{
    esp_err_t err;
    uint32_t baud;
    err = mHandle->get_item("uart_baud", baud);
    if((baud == 0) or (err != ESP_OK))
    {
        baud = cDefaultBaud;
        setDebugUartBaud(baud);
    }
    return baud;
}

void Setting::setDebugUartBaud(uint32_t baudrate) const
{
    esp_err_t err;
    err = mHandle->set_item("uart_baud", baudrate);
    ESP_LOGI(TAG, "%s", (err != ESP_OK) ? "Failed!\n" : "Done\n");
}

uint32_t Setting::getLienEnd() const
{
    esp_err_t err;
    uint32_t lineEnd;
    err = mHandle->get_item("lien_end", lineEnd);
    if((lineEnd == 0) or (err != ESP_OK))
    {
        lineEnd = cDefaultLienEnd;
        setLienEnd(lineEnd);
    }
    return lineEnd;
}

void Setting::setLienEnd(uint32_t lineEnd) const
{
    esp_err_t err;
    err = mHandle->set_item("lien_end", lineEnd);
    ESP_LOGI(TAG, "%s", (err != ESP_OK) ? "Failed!\n" : "Done\n");
}

Setting::Setting()
{
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

    esp_err_t result;
    mHandle = nvs::open_nvs_handle("storage", NVS_READWRITE, &result);
}

Setting::~Setting()
{

}

