#include <algorithm>
#include "pyocd_server.hpp"
#include "gpio_swd.hpp"
#include <esp_log.h>
#include "lwip/sockets.h"


//-------------------------------------------------------------------
// Request
//-------------------------------------------------------------------
const std::string Request::cCmdStr[32] = 
{
    "hello",
    "readprop",
    "open",
    "close",
    "lock",
    "unlock",
    "connect",
    "disconnect",
    "swj_sequence",
    "swd_sequence",
    "jtag_sequence",
    "set_clock",
    "reset",
    "assert_reset",
    "is_reset_asserted",
    "flush",
    "read_dp",
    "write_dp",
    "read_ap",
    "write_ap",
    "read_ap_multiple",
    "write_ap_multiple",
    "get_memory_interface_for_ap",
    "swo_start",
    "swo_stop",
    "swo_read",
    "read_mem",
    "write_mem",
    "read_block32",
    "write_block32",
    "read_block8",
    "write_block8"
};

std::string Request::toString(Cmd cmd)
{
    return std::string(cCmdStr[cmd]);
}

Request::Cmd Request::fromString(const std::string& str)
{
    for(int i = 0; i < Cmd::cmdSize; ++i)
    {
        if(str == cCmdStr[i])
        {
            return static_cast<Cmd>(i);
        }
    }
    return Cmd::cmdSize;
}

//-------------------------------------------------------------------
// PyOcdServer
//-------------------------------------------------------------------
static const char * TAG = "PyOcdServer";

PyOcdServer::PyOcdServer() : 
    ServerSocket(5555)
{

}

void PyOcdServer::sendResult(int socket, int id)
{
    mDoc.clear();
    mDoc["id"] = id;
    mDoc["status"] = 0;
    std::string ret;
    serializeJson(mDoc,ret);
    ret += '\n';
    send(socket, ret.c_str(), ret.size(), 0); 
}

bool PyOcdServer::serverMain(int acceptSocekt)
{
    char rx_buffer[512];
    while(true)
    {
        int len = recv(acceptSocekt, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0) {
            ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
            return false;
        }
        else if (len == 0) 
        {
            ESP_LOGW(TAG, "Connection closed");
            break;
        }
        else 
        {
            rx_buffer[len] = 0; // Null-terminate whatever is received and treat it like a string
            ESP_LOGI(TAG, "MSG %s", rx_buffer);
            mDoc.clear();
            deserializeJson(mDoc, rx_buffer);
            const int id = (int)mDoc["id"];
            if(mDoc["request"])
            {
                Request::Cmd cmd = Request::fromString(std::string(static_cast<const char*>(mDoc["request"])));
                switch(cmd)
                {
                case Request::hello:
                    ESP_LOGI(TAG, "version %d", (int)mDoc["arguments"][0]);
                    sendResult(acceptSocekt, id);                   
                    break;
                case Request::set_clock:
                    ESP_LOGI(TAG, "clock %d", (int)mDoc["arguments"][0]);
                    sendResult(acceptSocekt, id);                                      
                    break;
                case Request::lock:
                case Request::open:
                case Request::close:
                    sendResult(acceptSocekt, id);                                      
                    break;
                case Request::readprop:
                case Request::unlock:
                case Request::connect:
                case Request::disconnect:
                case Request::swj_sequence:
                case Request::swd_sequence:
                case Request::jtag_sequence:
                case Request::reset:
                case Request::assert_reset:
                case Request::is_reset_asserted:
                case Request::flush:
                case Request::read_dp:
                case Request::write_dp:
                case Request::read_ap:
                case Request::write_ap:
                case Request::read_ap_multiple:
                case Request::write_ap_multiple:
                case Request::get_memory_interface_for_ap:
                case Request::swo_start:
                case Request::swo_stop:
                case Request::swo_read:
                case Request::read_mem:
                case Request::write_mem:
                case Request::read_block32:
                case Request::write_block32:
                case Request::read_block8:
                case Request::write_block8:
                default:
                    ESP_LOGW(TAG, "Cmd %s Id %d", Request::toString(cmd).c_str(), id);
                    break;
                }
            }
        }
    }
    return true;
}
