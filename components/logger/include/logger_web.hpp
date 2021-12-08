#ifndef LOGGER_WEB_H
#define LOGGER_WEB_H

#include <stdint.h>
#include <vector>
#include <atomic>
#include <thread>
#include "esp_http_server.h"
#include "../../blocking_queue/include/blocking_queue.hpp"


class WebLoggerRx
{
public:
    const int fd;
    const httpd_handle_t hd;

    WebLoggerRx(int fd, httpd_handle_t hd);
    ~WebLoggerRx();
    void start();
    void stop();
    bool isRun() const;

private:
    std::atomic<bool> run;
    std::thread threadHandle;

    void thread(BlockingQueue<std::vector<uint8_t>>& queue);
};


void start_logger_web(BlockingQueue<std::vector<uint8_t>>& _txQ, BlockingQueue<std::vector<uint8_t>>& _rxQ);

#endif //LOGGER_WEB_H