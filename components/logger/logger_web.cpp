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
        .user_ctx   = this}
{
    httpd_register_uri_handler(serverHandle, &this->uri);
}

UriHandler::~UriHandler()
{
    httpd_unregister_uri_handler(serverHandle, uri.uri, uri.method);
}

esp_err_t UriHandler::handler(httpd_req_t *req)
{
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
// WebLoggerRx
//*******************************************************************
WebLoggerRx::WebLoggerRx(httpd_handle_t _hd, BlockingQueue<std::vector<uint8_t>>& queue) :
    hd(_hd),
    run(false),
    queue(queue)
{
    
}

WebLoggerRx::~WebLoggerRx()
{
    stop();
}

void WebLoggerRx::start()
{
    if(not run)
    {
        run = true;
        threadHandle = std::thread([this]{thread();});
    }
}

void WebLoggerRx::stop()
{
    if(run)
    {
        run = false;
        threadHandle.join();
    }
}

void WebLoggerRx::enroll(int fd)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    auto it = std::find(fds.begin(), fds.end(), fd);
    if(it == fds.end())
    {
        ESP_LOGI(TAG, "Add fd %d", fd);
        fds.push_back(fd);
    }
}

bool WebLoggerRx::isRun() const
{
    return run;
}

void WebLoggerRx::getTime(char* buffer)
{
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);

    std::time_t t = tv_now.tv_sec;
    tm local = *localtime(&t);
    int ms = tv_now.tv_usec / 1000;

    sprintf(buffer, "[%02d:%02d:%02d:%03d]", local.tm_hour, local.tm_min, local.tm_sec, ms);
    buffer[14] = ' ';
}

void WebLoggerRx::sendMsg(const std::vector<uint8_t>&msg)
{  
    httpd_ws_frame_t ws_pkt = {};
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t*)msg.data();
    ws_pkt.len = msg.size();
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    auto it = fds.begin();
    for(; it != fds.end(); ++it)
    {
        if(httpd_ws_send_frame_async(hd, *it, &ws_pkt) != ESP_OK)
        {
            ESP_LOGI(TAG, "Remove fd %d", *it);
            fds.erase(it);
            break;
        }
    }
}

void WebLoggerRx::thread()
{
    esp_log_level_set(TAG, ESP_LOG_INFO);
    ESP_LOGI(TAG, "start");
    static constexpr uint32_t timeStrLen = 15;
	std::vector<uint8_t> strBuffer;
	strBuffer.reserve(512);
    strBuffer.resize(timeStrLen);
    while (run) 
    {
        std::vector<uint8_t> msg;
        if(queue.pop(msg, std::chrono::milliseconds(1000)) and msg.size())
        {
            std::lock_guard<std::recursive_mutex> lock(mutex);

            for(int i = 0; i < msg.size(); ++i)
            {
                uint8_t d = msg[i];
				if(d == '\r')
				{
					continue;
				}
				if(d == '\n' or strBuffer.size() >= 512)
				{
					strBuffer.push_back('\r');
					strBuffer.push_back('\n');
					getTime((char*)&strBuffer[0]);
                    sendMsg(strBuffer);
                    strBuffer.resize(timeStrLen);
					continue;
				}
				strBuffer.push_back(d);
            }
        }
    }
}

//*******************************************************************
// WsHandler
//*******************************************************************
WsHandler* WsHandler::pWsHandler;

WsHandler::WsHandler(httpd_handle_t server, BlockingQueue<std::vector<uint8_t>>& txQ, BlockingQueue<std::vector<uint8_t>>& rxQ, ConnectCb&& cb) :
    serverHandle(server),
    uri{.uri        = "/ws",
        .method     = HTTP_GET,
        .handler    = handler,
        .user_ctx   = this,
        .is_websocket = true},
    txQ(txQ),
    rxQ(rxQ),
    cb(cb)
{
    pWsHandler = this;
    httpd_register_uri_handler(serverHandle, &uri);
}

WsHandler::~WsHandler()
{
    httpd_unregister_uri_handler(serverHandle, uri.uri, uri.method);
}

esp_err_t WsHandler::handler(httpd_req *req)
{
    if(not pWsHandler->pWebLoggerRx)
    {
        pWsHandler->pWebLoggerRx.reset();
        pWsHandler->pWebLoggerRx = std::make_unique<WebLoggerRx>(req->handle, pWsHandler->rxQ);
        pWsHandler->pWebLoggerRx->start();
    }
    pWsHandler->pWebLoggerRx->enroll(httpd_req_to_sockfd(req));

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

    if(tx[0] == 17)
    {
        std::string delimiter = " ";
        std::string cmd((char*)(tx.data() + 1));
        size_t pos = cmd.find(delimiter);
        std::string baud = cmd.substr(0, pos);
        std::string port = cmd.substr(pos + 5, cmd.length());
        int nbaud = std::stoi(baud);
        int nport = std::stoi(port);
        ESP_LOGI(TAG, "connect[%s] baud:%d port:%d", tx.data() + 1, nbaud, nport);
        if(pWsHandler->cb)
        {
            pWsHandler->cb(nport, nbaud);
        }
        return ret;
    }

    tx.resize(ws_pkt.len);
    pWsHandler->txQ.push(tx, std::chrono::milliseconds(100));
    return ret;
}

//*******************************************************************
// WebLogger
//*******************************************************************
WebLogger::WebLogger() :
    serverHandle(nullptr),
    txQ(10),
    rxQ(10)
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
            pLogger->pLoggerHandler = std::make_unique<WsHandler>(pLogger->serverHandle, pLogger->txQ, pLogger->rxQ,
                [&pLogger](int port, int baudrate)
                {
                    if((pLogger->pUartService)
                        and (pLogger->pUartService->getPort() == port)
                        and (pLogger->pUartService->getBaudRate() == baudrate))
                    {
                        return;
                    }
                    pLogger->pUartService.reset();
                    pLogger->pUartService = std::make_unique<UartService>(port);
                    pLogger->pUartService->start(baudrate, pLogger->txQ, pLogger->rxQ);
                });
            pLogger->pIndexHandler.reset();
            pLogger->pIndexHandler = std::make_unique<IndexHandler>(pLogger->serverHandle);

        }
    }
    else if(event_base == WIFI_EVENT)
    {
        pLogger->pIndexHandler.reset();
        pLogger->pLoggerHandler.reset();
        httpd_stop(pLogger->serverHandle);
        pLogger->serverHandle = nullptr;
    }
}

//*******************************************************************

void start_logger_web()
{
    static WebLogger logger;
}
