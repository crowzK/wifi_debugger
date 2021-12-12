#include "logger_web.hpp"
#include "uart.hpp"
#include "blocking_queue.hpp"
#include "smart_config.hpp"
#include "sntp.h"

extern "C" void app_main(void)
{
    startSmartConfig();
    start_logger_web();

    // time zone setting
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
    setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0", 1);
    tzset();
}
