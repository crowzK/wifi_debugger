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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <iostream>
#include <sstream>
#include "esp_vfs_dev.h"
#include "esp_vfs_usb_serial_jtag.h"
#include "driver/usb_serial_jtag.h"
#include "hal/usb_serial_jtag_ll.h"
#include "uart_bypass.hpp"
#include "pyocd_io_console.hpp"
#include "pyocd_io_uart.hpp"
#include "esp_log.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include "tusb_console.h"

#include "console_s3.hpp"
//-------------------------------------------------------------------
// ConsoleS3
//-------------------------------------------------------------------
static const char *TAG = "ConsoleS3";

void ConsoleS3::cdcRxCallback(int itf, void *ev)
{
    mConsoleRx.notify_all();
}

void ConsoleS3::cdcLineStateChangedCallback(int itf, void *ev)
{
    cdcacm_event_t *event = (cdcacm_event_t*)ev;
    int dtr = event->line_state_changed_data.dtr;
    int rts = event->line_state_changed_data.rts;
    ESP_LOGI(TAG, "Line state changed on channel %d: DTR:%d, RTS:%d", itf, dtr, rts);
}

void ConsoleS3::cdcLineCodingChangedCallback(int itf, void *ev)
{
    cdcacm_event_t *event = (cdcacm_event_t*)ev;
    int baud = event->line_coding_changed_data.p_line_coding->bit_rate;
    ESP_LOGI(TAG, "Line coding changed on channel %d: baudrate:%d", itf, baud);
    if(baud == 230400)
    {
        esp_tusb_init_console(TINYUSB_CDC_ACM_0); // log to usb
        mUsbConnected = true;
    }
    else if(mUsbConnected)
    {
        esp_tusb_deinit_console(TINYUSB_CDC_ACM_0); // log to uart
        mUsbConnected = false;
    }
}

ConsoleS3::ConsoleS3() :
    Task(__func__)
{
}

bool ConsoleS3::init()
{
    /* Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);

    /* Setting TinyUSB up */
    ESP_LOGI(TAG, "USB initialization");

    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = NULL,
        .external_phy = false, // In the most cases you need to use a `false` value
        .configuration_descriptor = NULL,
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    tinyusb_config_cdcacm_t acm_cfg = {
        .usb_dev = TINYUSB_USBDEV_0,
        .cdc_port = TINYUSB_CDC_ACM_0,
        .rx_unread_buf_sz = 64,
        .callback_rx = (tusb_cdcacm_callback_t)&cdcRxCallback, // the first way to register a callback
        .callback_rx_wanted_char = NULL,
        .callback_line_state_changed = (tusb_cdcacm_callback_t)&cdcLineStateChangedCallback,
        .callback_line_coding_changed = (tusb_cdcacm_callback_t)&cdcLineCodingChangedCallback
    };
    ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));
    ESP_LOGI(TAG, "USB CDC console DONE");
    return true;
}

void ConsoleS3::task()
{
    int stdin_fileno = fileno(stdin);
    mpHelp = std::make_unique<Help>();
    mpUartConsole = std::make_unique<UartByPass>();
    mpPyOcdConsole = std::make_unique<PyOcdIoConsole>();
    mpPyOcdIoUart = std::make_unique<PyOcdIoUart>();

    help();
    std::string cmd;
    while(mRun)
    {
        std::unique_lock<std::mutex> lock(mMutexRxSync);
        mConsoleRx.wait_for(lock, std::chrono::milliseconds(1000));

        char c;
        while(read(stdin_fileno, &c, 1) > 0)
        {
            if(c == '\r')
            {
                ; // ignore
            }
            else if(c == '\n')
            {
                bool execute = false;
                if(cmd.size() > 0)
                {
                    putchar('\n');
                    putchar('\n');
                    auto args = split(cmd);
                    if(args.size() > 0)
                    {
                        std::lock_guard<std::recursive_mutex> lock(mMutex);
                        for(auto& pCmd : mCmdList)
                        {
                            if(pCmd->cCmd == args.at(0))
                            {
                                execute = true;
                                pCmd->excute(args);
                            }
                        }
                    }
                    cmd.clear();
                }
                if(not execute)
                {
                    help();
                }
            }
            else
            {
                cmd += c;
                putchar(c);
                fflush(stdout);
            }
        }
    }
};
