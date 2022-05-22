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

#include <esp_log.h>
#include <esp_wifi.h>
#include "logger_web.hpp"
#include "uart.hpp"
#include "blocking_queue.hpp"
#include "provisioning_manager.hpp"
#include "sntp.h"
#include "sdkconfig.h"
#include "file_server.hpp"
#include "pyocd_server.hpp"
#include "log_file.hpp"
#include "ota.hpp"

extern "C" void app_main(void)
{
    startProvisioning();
    esp_wifi_set_ps(WIFI_PS_NONE);
    // time zone setting
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
    setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0", 1);
    tzset();

    Ota::create();
    IndexHandler::create();
    WsHandler::create();
    UartService::create();
    PyOcdServer::create();
    FileServerHandler::create();
    LogFile::create().init();
}
