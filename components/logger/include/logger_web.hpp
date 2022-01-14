#ifndef LOGGER_WEB_H
#define LOGGER_WEB_H

#include <stdint.h>
#include <vector>
#include <atomic>
#include <thread>
#include <memory>
#include <mutex>
#include <list>
#include <functional>
#include "esp_http_server.h"
#include "uart.hpp"
#include "blocking_queue.hpp"
#include "esp_event.h"
#include "sdcard.hpp"
#include "task.hpp"

class FileServerHandler;

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

class WebLogSender : public Client
{
public:
    const httpd_handle_t hd;
    const int fd;

    WebLogSender(httpd_handle_t hd, int fd);
    ~WebLogSender();

protected:
    bool write(const std::vector<uint8_t>& msg);
};

class WsHandler
{
public:
    WsHandler(httpd_handle_t server);
    ~WsHandler();

protected:
    const httpd_handle_t serverHandle;
    const httpd_uri_t uri;
    static esp_err_t handler(httpd_req *req);
};


class WebLogger
{
public:
    WebLogger();
    ~WebLogger();

protected:
    httpd_handle_t serverHandle;
    std::unique_ptr<IndexHandler> pIndexHandler;
    std::unique_ptr<WsHandler> pLoggerHandler;
    std::unique_ptr<FileServerHandler> pFileServerHandler;

    static void handler(WebLogger* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
};

void start_logger_web();

#endif //LOGGER_WEB_H
