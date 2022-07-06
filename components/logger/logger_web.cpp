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

#include <esp_log.h>
#include "cmd.hpp"
#include "logger_web.hpp"
#include "uart.hpp"

/* A simple example that demonstrates using websocket echo server
 */
static const char *TAG = "logger";

//-------------------------------------------------------------------
// IndexHandler
//-------------------------------------------------------------------
IndexHandler& IndexHandler::create()
{
    static IndexHandler ih;
    return ih;
}

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

//-------------------------------------------------------------------
// WebLogSender
//-------------------------------------------------------------------
WebLogSender::WebLogSender(httpd_handle_t hd, int fd) :
    Client(DebugMsgRx::create(),(int)fd),
    hd(hd),
    fd(fd)
{

}

WebLogSender::~WebLogSender()
{

}

bool WebLogSender::write(const MsgProxy::Msg& msg)
{
    httpd_ws_frame_t ws_pkt = {};
    std::string str;
    if(msg.strStart)
    {
        str = MsgProxy::getTime(msg.time);
    }
    
    str += msg.data;
    ws_pkt.payload = (uint8_t*)str.c_str();
    ws_pkt.len = str.length();
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    if(httpd_ws_send_frame_async(hd, fd, &ws_pkt) != ESP_OK)
    {
        ESP_LOGE(TAG, "Error fd %d", fd);
        return false;
    }
    return true;
}

//-------------------------------------------------------------------
// WsHandler
//-------------------------------------------------------------------
WsHandler& WsHandler::create()
{
    static WsHandler ws;
    return ws;
}

WsHandler::WsHandler() :
    UriHandler("/ws", HTTP_GET, true),
    mStrStart(true)
{
}

static std::string string_to_hex(const std::string& input)
{
    static const char hex_digits[] = "0123456789ABCDEF";

    std::string output;
    output.reserve(input.length() * 2);
    for (unsigned char c : input)
    {
        output.push_back(hex_digits[c >> 4]);
        output.push_back(hex_digits[c & 15]);
    }
    return output;
}
esp_err_t WsHandler::userHandler(httpd_req *req)
{
    std::unique_lock<std::mutex> lock(mMutex);

    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "Handshake done, the new connection was opened");
        return ESP_OK;
    }

    if(not DebugMsgRx::create().isAdded(httpd_req_to_sockfd(req)))
    {
        new WebLogSender(req->handle, httpd_req_to_sockfd(req));
    }

    static constexpr uint32_t cBufferLeng = 128;
    std::vector<uint8_t> tx;
    tx.resize(cBufferLeng + 1);
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
            const auto cfg = UartService::create().getCfg();
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
    tx[ws_pkt.len] = 0;
    auto msgs = MsgProxy::convToMsg((char*)tx.data());
    for(auto& msg: msgs)
    {
        if(msg.data.length() == 0)
        {
            continue;
        }
        if(mStrStart)
        {
            msg.strStart = true;
            mStrStart = false;
        }
        if(msg.data.back() == '\n')
        {
            mStrStart = true;
        }
        DebugMsgTx::create().write(msg);
    }

    return ret;
}
