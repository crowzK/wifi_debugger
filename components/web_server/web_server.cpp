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

#include "web_server.hpp"
#include <esp_log.h>
#include <algorithm>

static const char *TAG = "web_sever";

//*******************************************************************
// UriHandler
//*******************************************************************
UriHandler::UriHandler(const char* uri, httpd_method_t method, esp_err_t (*wsHandler)(httpd_req_t *r)) :
    cUri{.uri        = uri,
        .method     = method,
        .handler    = (wsHandler != nullptr) ? wsHandler : handler,
        .user_ctx   = this,
        .is_websocket = wsHandler != nullptr,
        .handle_ws_control_frames = false,
        .supported_subprotocol = nullptr
    }
{
    WebServer::get().add(*this);
}

UriHandler::~UriHandler()
{
    WebServer::get().remove(*this);
}

void UriHandler::start(httpd_handle_t serverHandle)
{
    if(esp_err_t err = httpd_register_uri_handler(serverHandle, &cUri))
    {
        ESP_LOGE(TAG, "UriHandler start err [%d]", err);
    }
}

void UriHandler::stop(httpd_handle_t serverHandle)
{
    if(esp_err_t err =  httpd_unregister_uri_handler(serverHandle, cUri.uri, cUri.method))
    {
        ESP_LOGE(TAG, "UriHandler stop err [%d]", err);
    }
}

esp_err_t UriHandler::handler(httpd_req_t *req)
{
    UriHandler* pHandle = reinterpret_cast<UriHandler*>(req->user_ctx);
    if(pHandle == nullptr)
    {
        ESP_LOGE(TAG, "req->user_ctx is null");
        return ESP_FAIL;
    }
    return pHandle->userHandler(req);
}


//*******************************************************************
// WebServer
//*******************************************************************
WebServer& WebServer::get()
{
    static WebServer server;
    return server;
}

void WebServer::add(UriHandler& handler)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    mUriHandlers.push_back(&handler);
    if(serverHandle)
    {
        handler.start(serverHandle);
    }
}

void WebServer::remove(UriHandler& handler)
{
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    mUriHandlers.remove_if([handler = &handler](UriHandler* pHandle){return (UriHandler*)&handler == pHandle;});
    if(serverHandle)
    {
        handler.stop(serverHandle);
    }
}

WebServer::WebServer() :
    serverHandle(nullptr)
{
    ESP_LOGI(TAG, "start");
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&serverHandle, &config) != ESP_OK)
    {
        ESP_LOGI(TAG, "Error starting server!");
        return;
    }
    
    // Registering the ws handler
    ESP_LOGI(TAG, "Registering URI handlers");
    //httpd_register_uri_handler(serverHandle, &ws);

    for(UriHandler* pHandler : mUriHandlers)
    {
        pHandler->start(serverHandle);
    }
}

WebServer::~WebServer()
{

}
