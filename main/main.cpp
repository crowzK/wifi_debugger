#include "logger_web.hpp"
#include "uart.hpp"
#include "blocking_queue.hpp"
#include "provisioning_manager.hpp"
#include "sntp.h"
#include "sdkconfig.h"
#include "sdcard.hpp"
#include "file_server.hpp"
#include "pyocd_server.hpp"
#include <esp_log.h>
#include <esp_wifi.h>

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
    start_logger_web();
    static PyOcdServer pyocd;
    
    static SdCard sdcard;
    start_file_server();
    while(sntp_getreachability(0) == 0)
    {
        vTaskDelay(100);
    }
    sdcard.init();
}
