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

#include "pyocd_io_uart.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include "tusb_console.h"

//-------------------------------------------------------------------
// PyOcdIoUart
//-------------------------------------------------------------------
static const char *TAG = "Console";
PyOcdIoUart* PyOcdIoUart::pPyOcdIoUart;
void PyOcdIoUart::cdcRxCallback(int itf, void *ev)
{
    if(not pPyOcdIoUart)
    {
        return;
    }
    /* initialization */
    size_t rx_size = 0;

    /* read */
    esp_err_t ret = tinyusb_cdcacm_read(TINYUSB_CDC_ACM_1, (uint8_t*)pPyOcdIoUart->mRxBuff, CONFIG_TINYUSB_CDC_RX_BUFSIZE, &rx_size);
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "Read error");
        return;
    }
    pPyOcdIoUart->mRxBuff[rx_size] = 0;
    pPyOcdIoUart->mPyOcdParser.parse(pPyOcdIoUart->mRxBuff, rx_size);
}

void PyOcdIoUart::cdcLineStateChangedCallback(int itf, void *ev)
{

}

void PyOcdIoUart::cdcLineCodingChangedCallback(int itf, void *ev)
{

}

PyOcdIoUart::PyOcdIoUart() :
    PyOcdIo(nullptr),
    mPyOcdParser(*this)
{
    pPyOcdIoUart = this;
    tinyusb_config_cdcacm_t acm_cfg = {
        .usb_dev = TINYUSB_USBDEV_0,
        .cdc_port = TINYUSB_CDC_ACM_1,
        .rx_unread_buf_sz = 64,
        .callback_rx = (tusb_cdcacm_callback_t)&cdcRxCallback, // the first way to register a callback
        .callback_rx_wanted_char = NULL,
        .callback_line_state_changed = NULL,
        .callback_line_coding_changed = NULL
    };
    ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));
    ESP_LOGI(TAG, "Py OCD IO UART start");
}

PyOcdIoUart::~PyOcdIoUart()
{
    pPyOcdIoUart = nullptr;
}

uint32_t PyOcdIoUart::send(const char* message, uint32_t len)
{
    uint32_t sent = tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_1, (uint8_t*)message, len);
    tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_1, 0);
    return sent;
}
