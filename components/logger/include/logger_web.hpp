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

//! For handling index page
class IndexHandler : public UriHandler
{
public:
    //! \brief Create index page handler
    static IndexHandler& create();

protected:
    IndexHandler();
    ~IndexHandler() = default;
    virtual esp_err_t userHandler(httpd_req *req) override;
};

//! To send log messages(UART) to the user web browser.
class WebLogSender : public Client
{
public:
    const httpd_handle_t hd;
    const int fd;

    WebLogSender(httpd_handle_t hd, int fd);
    ~WebLogSender();

protected:
    bool write(const std::vector<uint8_t>& msg) override;
};

//! Web sockek handler
//! This will handle the message from the client
class WsHandler : public UriHandler
{
public:
    //! \brief Create web socket handler
    static WsHandler& create();

protected:
    WsHandler();
    ~WsHandler() = default;
    esp_err_t userHandler(httpd_req *req) override;
};

#endif //LOGGER_WEB_H
