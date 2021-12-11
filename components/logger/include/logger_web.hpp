#ifndef LOGGER_WEB_H
#define LOGGER_WEB_H

#include <stdint.h>
#include <vector>
#include <atomic>
#include <thread>
#include <memory>
#include <mutex>
#include <list>
#include "esp_http_server.h"
#include "uart.hpp"
#include "../../blocking_queue/include/blocking_queue.hpp"


class UriHandler
{
public:
    const httpd_handle_t serverHandle;
    const httpd_uri_t uri;
    UriHandler(httpd_handle_t server, const char* uri, httpd_method_t method);
    ~UriHandler();

protected:
    static esp_err_t handler(httpd_req_t *req);
    virtual esp_err_t userHandler(httpd_req *req) = 0;
};

class IndexHandler : public UriHandler
{
public:
    IndexHandler(httpd_handle_t server);
    ~IndexHandler() = default;

protected:
    virtual esp_err_t userHandler(httpd_req *req) override;
};

class WebLoggerRx
{
public:
    const httpd_handle_t hd;

    WebLoggerRx(httpd_handle_t hd, BlockingQueue<std::vector<uint8_t>>& queue);
    ~WebLoggerRx();
    void start();
    void stop();
    bool isRun() const;
    void enroll(int fd);

private:
    std::atomic<bool> run;
    std::thread threadHandle;
    std::list<int> fds;
    std::recursive_mutex mutex;
    BlockingQueue<std::vector<uint8_t>>& queue;

    void thread();
};

class WsHandler
{
public:
    WsHandler(httpd_handle_t server, BlockingQueue<std::vector<uint8_t>>& txQ, BlockingQueue<std::vector<uint8_t>>& rxQ);
    ~WsHandler();

protected:
    static WsHandler* pWsHandler;
    const httpd_handle_t serverHandle;
    const httpd_uri_t uri;
    std::unique_ptr<WebLoggerRx> pWebLoggerRx;
    BlockingQueue<std::vector<uint8_t>>& txQ;
    BlockingQueue<std::vector<uint8_t>>& rxQ;
    static esp_err_t handler(httpd_req *req);
};

class WebLogger
{
public:
    WebLogger();
    ~WebLogger();

protected:
    httpd_handle_t serverHandle;
    BlockingQueue<std::vector<uint8_t>> txQ;
    BlockingQueue<std::vector<uint8_t>> rxQ;
    std::unique_ptr<UartService> pUartService;
    std::unique_ptr<IndexHandler> pIndexHandler;
    std::unique_ptr<WsHandler> pLoggerHandler;

    static void handler(WebLogger* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
};

void start_logger_web();

#endif //LOGGER_WEB_H