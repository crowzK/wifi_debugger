/* WebSocket Echo Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

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

/* A simple example that demonstrates using websocket echo server
 */
static const char *TAG = "ws_echo_server";

static std::unique_ptr<WebLoggerRx> pWebLoggerRx;
static BlockingQueue<std::vector<uint8_t>>* pRxQ;
static BlockingQueue<std::vector<uint8_t>>* pTxQ;

/*
 * This handler echos back the received ws data
 * and triggers an async send if certain message received
 */
static esp_err_t echo_handler(httpd_req_t *req)
{
    if((not pWebLoggerRx) or (pWebLoggerRx->fd != httpd_req_to_sockfd(req)))
    {
        pWebLoggerRx.reset();
        pWebLoggerRx = std::make_unique<WebLoggerRx>(httpd_req_to_sockfd(req), req->handle);
        pWebLoggerRx->start();
    }

    uint8_t buf[128] = { 0 };
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = buf;
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 128);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
        return ret;
    }
    ESP_LOGI(TAG, "Got packet with message: %s", ws_pkt.payload);
    ESP_LOGI(TAG, "Packet type: %d", ws_pkt.type);

    std::vector<uint8_t> tx;
    for(int i = 0; i < ws_pkt.len; i ++)
    {
        tx.push_back(ws_pkt.payload[i]);
    }
    pTxQ->push(tx, std::chrono::milliseconds(1000));
    return ret;
}

static const httpd_uri_t ws = {
        .uri        = "/ws",
        .method     = HTTP_GET,
        .handler    = echo_handler,
        .user_ctx   = NULL,
        .is_websocket = true
};

static esp_err_t root_handler(httpd_req_t *req)
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

/* URI handler for / */
static const httpd_uri_t root = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = root_handler,
        .user_ctx  = NULL
};


static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Registering the ws handler
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &ws);
        httpd_register_uri_handler(server, &root);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
}

static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver");
        stop_webserver(*server);
        *server = NULL;
    }
}

static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}

////////////////////////////////////////////////////////////////////
// WebLoggerRx
////////////////////////////////////////////////////////////////////
WebLoggerRx::WebLoggerRx(int _fd, httpd_handle_t _hd) :
    fd(_fd),
    hd(_hd),
    run(false)
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
        threadHandle = std::thread([this]{thread(*pRxQ);});
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

bool WebLoggerRx::isRun() const
{
    return run;
}

void WebLoggerRx::thread(BlockingQueue<std::vector<uint8_t>>& queue)
{
    esp_log_level_set("WebLoggerRx", ESP_LOG_INFO);
    ESP_LOGI("WebLoggerRx", "start");

    while (run) 
    {
        std::vector<uint8_t> msg;
        if(queue.pop(msg, std::chrono::milliseconds(1000)) and msg.size())
        {
            httpd_ws_frame_t ws_pkt;
            memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
            ws_pkt.payload = msg.data();
            ws_pkt.len = msg.size();
            ws_pkt.type = HTTPD_WS_TYPE_TEXT;

            httpd_ws_send_frame_async(hd, fd, &ws_pkt);
        }
    }
}

////////////////////////////////////////////////////////////////////
// start_logger_web
////////////////////////////////////////////////////////////////////
void start_logger_web(BlockingQueue<std::vector<uint8_t>>& _txQ, BlockingQueue<std::vector<uint8_t>>& _rxQ)
{
    pRxQ = &_rxQ;
    pTxQ = &_txQ;

    static httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));

    /* Start the server for the first time */
    server = start_webserver();
}
