#ifndef LOGGER_WEB_H
#define LOGGER_WEB_H

#include <stdint.h>
#include <vector>
#include "web_server.hpp"
#include "debug_msg_handler.hpp"

class IndexHandler : public UriHandler
{
public:
    IndexHandler();
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

class WsHandler : public UriHandler
{
public:
    WsHandler();
    ~WsHandler() = default;

protected:
    // when web socket handler is called, the httpd_req->user_ctx is null, so it cannot use the userHandler
    static esp_err_t wshandler(httpd_req *req);
    esp_err_t userHandler(httpd_req *req) override { return ESP_OK; };
};

void start_logger_web();

#endif //LOGGER_WEB_H
