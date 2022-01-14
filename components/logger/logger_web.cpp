#include "esp_http_server.h"
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_netif.h"
#include "logger_web.hpp"
#include <memory>
#include <algorithm>
#include <string>
#include <ctime>
#include "cmd.hpp"
#include "file_server.hpp"

/* A simple example that demonstrates using websocket echo server
 */
static const char *TAG = "logger";

//*******************************************************************
// UriHandler
//*******************************************************************
UriHandler::UriHandler(httpd_handle_t server, const char* uri, httpd_method_t method) :
    serverHandle(server),
    uri{.uri        = uri,
        .method     = method,
        .handler    = handler,
        .user_ctx   = this
    }
{
    if(esp_err_t err = httpd_register_uri_handler(serverHandle, &this->uri))
    {
        ESP_LOGE(TAG, "UriHandler start err [%d]", err);
    }
}

UriHandler::~UriHandler()
{
    httpd_unregister_uri_handler(serverHandle, uri.uri, uri.method);
}

esp_err_t UriHandler::handler(httpd_req_t *req)
{
    ESP_LOGE(TAG, "handler");
    UriHandler* pHandle = reinterpret_cast<UriHandler*>(req->user_ctx);
    if(pHandle == nullptr)
    {
        return ESP_FAIL;
    }
    return pHandle->userHandler(req);
}

//*******************************************************************
// IndexHandler
//*******************************************************************
IndexHandler::IndexHandler(httpd_handle_t server) :
    UriHandler(server, "/", HTTP_GET)
{
}

esp_err_t IndexHandler::userHandler(httpd_req *req)
{
    /* Get handle to embedded file upload script */
    extern const unsigned char upload_script_start[] asm("_binary_root_html_start");
    extern const unsigned char upload_script_end[]   asm("_binary_root_html_end");
    const size_t upload_script_size = (upload_script_end - upload_script_start);

    /* Add file upload form and script which on execution sends a POST request to /upload */
    httpd_resp_send_chunk(req, (const char *)upload_script_start, upload_script_size);

    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

//*******************************************************************
// WebLogSender
//*******************************************************************
WebLogSender::WebLogSender(httpd_handle_t hd, int fd) :
    Client(DebugMsgRx::get(),(int)fd),
    hd(hd),
    fd(fd)
{

}

WebLogSender::~WebLogSender()
{

}

bool WebLogSender::write(const std::vector<uint8_t>& msg)
{
    httpd_ws_frame_t ws_pkt = {};
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t*)msg.data();
    ws_pkt.len = msg.size();
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    if(httpd_ws_send_frame_async(hd, fd, &ws_pkt) != ESP_OK)
    {
        ESP_LOGE(TAG, "Error fd %d", fd);
        return false;
    }
    return true;
}

//*******************************************************************
// WsHandler
//*******************************************************************
WsHandler::WsHandler(httpd_handle_t server) :
    serverHandle(server),
    uri{.uri        = "/ws",
        .method     = HTTP_GET,
        .handler    = handler,
        .user_ctx   = this,
        .is_websocket = true
    }
{
    httpd_register_uri_handler(serverHandle, &uri);
}

WsHandler::~WsHandler()
{
    httpd_unregister_uri_handler(serverHandle, uri.uri, uri.method);
}

esp_err_t WsHandler::handler(httpd_req *req)
{
    if(not DebugMsgRx::get().isAdded(httpd_req_to_sockfd(req)))
    {
        new WebLogSender(req->handle, httpd_req_to_sockfd(req));
    }

    static constexpr uint32_t cBufferLeng = 128;
    std::vector<uint8_t> tx;
    tx.resize(cBufferLeng);
    httpd_ws_frame_t ws_pkt = {};
    ws_pkt.payload = tx.data();
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, cBufferLeng);
    
    if(ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
        return ret;
    }

    Cmd cmd(tx.data(), ws_pkt.len);
    if(cmd.getCmdType() == Cmd::Type::eClientToSever)
    {
        switch(cmd.getSubCmd())
        {
        case Cmd::SubCmd::eUartSetting:
        {
            const auto cfg = UartService::get().getCfg();
            UartSetting setting(cfg.baudRate, cfg.uartNum);
            auto cmd = setting.getCmd();
            httpd_ws_frame_t ws_pkt = {};
            memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
            ws_pkt.payload = cmd.data();
            ws_pkt.len = cmd.size();
            ws_pkt.type = HTTPD_WS_TYPE_TEXT;

            if(httpd_ws_send_frame_async(req->handle, httpd_req_to_sockfd(req), &ws_pkt) != ESP_OK)
            {
                ESP_LOGE(TAG, "Error fd %d", (int)req->handle);
            }
            break;
        }
        default:
            break;
        }
        return ret;
    }

    tx.resize(ws_pkt.len);
    DebugMsgTx::get().write(tx);
    return ret;
}

//*******************************************************************
// WebLogger
//*******************************************************************
WebLogger::WebLogger() :
    serverHandle(nullptr)
{
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, (esp_event_handler_t)&handler, this));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, (esp_event_handler_t)&handler, this));
}

WebLogger::~WebLogger()
{

}

void WebLogger::handler(WebLogger* pLogger, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if(event_base == IP_EVENT)
    {
        if(pLogger->serverHandle == nullptr)
        {
            httpd_config_t config = HTTPD_DEFAULT_CONFIG();

            // Start the httpd server
            ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
            if (httpd_start(&pLogger->serverHandle, &config) != ESP_OK)
            {
                ESP_LOGI(TAG, "Error starting server!");
                return;
            }
            
            // Registering the ws handler
            ESP_LOGI(TAG, "Registering URI handlers");
            //httpd_register_uri_handler(pLogger->serverHandle, &ws);
            pLogger->pLoggerHandler.reset();
            pLogger->pLoggerHandler = std::make_unique<WsHandler>(pLogger->serverHandle);
            pLogger->pIndexHandler.reset();
            pLogger->pIndexHandler = std::make_unique<IndexHandler>(pLogger->serverHandle);
            pLogger->pFileServerHandler.reset();
            pLogger->pFileServerHandler = std::make_unique<FileServerHandler>(pLogger->serverHandle);
        }
    }
    else if(event_base == WIFI_EVENT)
    {
        pLogger->pIndexHandler.reset();
        pLogger->pLoggerHandler.reset();
        pLogger->pFileServerHandler.reset();
        httpd_stop(pLogger->serverHandle);
        pLogger->serverHandle = nullptr;
    }
}

//*******************************************************************

void start_logger_web()
{
    static WebLogger logger;
    UartService::get();
}
