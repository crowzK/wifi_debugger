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

#ifndef WEB_SERVER_HPP
#define WEB_SERVER_HPP

#include <list>
#include <mutex>
#include "esp_http_server.h"
#include <esp_event.h>

class UriHandler
{
public:
    const httpd_uri_t cUri;
    UriHandler(const char* uri, httpd_method_t method, bool wsSocket = false);
    ~UriHandler();

    void start(httpd_handle_t serverHandle);
    void stop(httpd_handle_t serverHandle);

protected:
    static esp_err_t handler(httpd_req_t *req);
    virtual esp_err_t userHandler(httpd_req *req) = 0;
};

class WebServer
{
public:
    static WebServer& get();
    void add(UriHandler& handler);
    void remove(UriHandler& handler);

protected:
    std::recursive_mutex mMutex;
    std::list<UriHandler*> mUriHandlers;

    httpd_handle_t serverHandle;

    WebServer();
    ~WebServer();
};


#endif // WEB_SERVER_HPP
