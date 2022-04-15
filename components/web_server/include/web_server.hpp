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

//! URI(Uniform Resource Identifier) handler
class UriHandler
{
public:
    const httpd_uri_t cUri;
    UriHandler(const char* uri, httpd_method_t method, bool wsSocket = false);
    ~UriHandler();

protected:
    friend class WebServer;
    static esp_err_t handler(httpd_req_t *req);

    //! \brief child class must implemantation this handler
    virtual esp_err_t userHandler(httpd_req *req) = 0;

    //! \brief Start URI handler
    void start(httpd_handle_t serverHandle);

    //! \brief Stop URI handler
    void stop(httpd_handle_t serverHandle);
};

//! Web server
class WebServer
{
public:
    static WebServer& create();

protected:
    friend class UriHandler;

    std::recursive_mutex mMutex;
    std::list<UriHandler*> mUriHandlers;

    httpd_handle_t serverHandle;

    WebServer();
    ~WebServer();
    
    //! \brief Add URI handler to the server
    void add(UriHandler& handler);

    //! \brief Remove URI handler from the server
    void remove(UriHandler& handler);
};


#endif // WEB_SERVER_HPP
