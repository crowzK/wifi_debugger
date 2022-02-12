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
