#include <esp_log.h>
#include "cmd.hpp"
#include "logger_web.hpp"
#include "uart.hpp"

/* A simple example that demonstrates using websocket echo server
 */
static const char *TAG = "logger";

//*******************************************************************
// IndexHandler
//*******************************************************************
IndexHandler::IndexHandler() :
    UriHandler("/", HTTP_GET)
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
WsHandler::WsHandler() :
    UriHandler("/ws", HTTP_GET, wshandler)
{
}

esp_err_t WsHandler::wshandler(httpd_req *req)
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

void start_logger_web()
{
    static IndexHandler indexHandler;
    static WsHandler wsHandler;
    UartService::get();
}
