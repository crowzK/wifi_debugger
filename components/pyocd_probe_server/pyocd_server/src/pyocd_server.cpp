#include <sstream>
#include <algorithm>
#include "pyocd_server.hpp"
#include "gpio_swd.hpp"
#include <esp_log.h>
#include "lwip/sockets.h"

static const char * TAG = "PyOcdServer";


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

Request::ArgumentType Request::getArgType(Cmd cmd)
{
    switch(cmd)
    {
    default:
    case Request::readprop:  
    case Request::connect:
    case Request::swd_sequence:     // ????
    case Request::jtag_sequence:    //???
    case Request::assert_reset:
        return ArgumentType::eString;
    case Request::hello:
    case Request::set_clock:
    case Request::read_dp:
    case Request::read_ap:
    case Request::swo_start:
        return ArgumentType::eInt;
    case Request::lock:
    case Request::open:
    case Request::close:
    case Request::unlock:
    case Request::disconnect:
    case Request::reset:
    case Request::is_reset_asserted:
    case Request::flush:
    case Request::swo_stop:
    case Request::swo_read:
        return ArgumentType::eNone;
    case Request::swj_sequence:
    case Request::write_dp:
    case Request::write_ap:
    case Request::read_ap_multiple:
    case Request::write_ap_multiple:
    case Request::get_memory_interface_for_ap:
    case Request::read_mem:
    case Request::write_mem:
    case Request::read_block32:
    case Request::write_block32:
    case Request::write_block8:
    case Request::read_block8:
        return ArgumentType::eArray;
    }
}

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
// PyOcdParser
//-------------------------------------------------------------------
PyOcdParser::PyOcdParser(int socket) :
    mSocket(socket),
    mId(-1),
    mRequest(Request::cmdSize),
    mKey(Key::eInvalid)
{
    pSwd = std::make_unique<GpioSwd>();
}

void PyOcdParser::sendOkay()
{
    char buffer[128] = {};
    const int len = snprintf(buffer, 128, "{\"id\": %d, \"status\": 0}\n", mId);
    send(mSocket, buffer, len, 0);
}

void PyOcdParser::sendString(const char * str)
{
    char buffer[128] = {};
    const int len = snprintf(buffer, 128, "{\"id\": %d, \"status\": 0, \"result\": %s}\n", mId, str);
    send(mSocket, buffer, len, 0);
}

void PyOcdParser::sendInt(uint32_t val)
{
    char buffer[128] = {};
    const int len = snprintf(buffer, 128, "{\"id\": %d, \"status\": 0, \"result\": %u}\n", mId, val);
    send(mSocket, buffer, len, 0);
}
    
void PyOcdParser::sendArray(const std::vector<uint32_t>& vector)
{
    {
        char buffer[128] = {};
        const int len = snprintf(buffer, 128, "{\"id\": %d, \"status\": 0, \"result\": [", mId);
        send(mSocket, buffer, len, 0);
    }
    std::ostringstream arr;
    arr << vector[0];
    for(int i = 1; i < vector.size(); i++)
    {
        arr << ", " << vector[i];
    }
    arr << "]}\n";
    send(mSocket, arr.str().c_str(), arr.str().size(), 0);
}

void PyOcdParser::sendError(const char * str)
{
    char buffer[128] = {};
    const int len = snprintf(buffer, 128, "{\"id\": %d, \"status\": 1, \"error\": \"%s\"}\n", mId, str);
    send(mSocket, buffer, len, 0);
}

void PyOcdParser::startDocument()
{
}

void PyOcdParser::endDocument()
{
    switch(mRequest)
    {
    case Request::hello:
        ESP_LOGI(TAG, "version %d", (int)mIntArgument);
        sendOkay();                   
        break;
    case Request::set_clock:
        ESP_LOGI(TAG, "clock %d", (int)mIntArgument);
        sendOkay();                                         
        break;
    case Request::lock:
    case Request::close:
    case Request::unlock:
    case Request::disconnect:
        sendOkay();
        break;
    case Request::connect:
        pSwd->cleareErrors();
        sendOkay();
        break;
    case Request::open:
    {
        uint64_t swj = 72057594037927935;
        pSwd->sequence(swj, 51);
        swj = 59294;
        pSwd->sequence(swj, 16);
        swj = 72057594037927935;
        pSwd->sequence(swj, 51);
        swj = 0;
        pSwd->sequence(swj, 8);
        sendOkay();
        break;
    }
    case Request::readprop:
    {
        if(mStrArgument == std::string("capabilities"))
        {
            const char* str = "[\"BANKED_DP_REGISTERS\", \"APv2_ADDRESSES\"]";
            sendString(str);
        } 
        else if(mStrArgument == std::string("supported_wire_protocols"))
        {
            const char* str = "[\"DEFAULT\", \"SWD\"]";
            sendString(str);
        } 
        else if(mStrArgument == std::string("wire_protocol"))
        {
            const char* str = "\"SWD\"";
            sendString(str);
        }            
        break;
    }
    case Request::swj_sequence:
    {
        uint64_t swj = mArrayArgument[1];
        if(mArrayArgument.size() > 2)
        {
            swj = (swj << 32) | mArrayArgument[2];
        }
        pSwd->sequence(swj, mArrayArgument[0]);
        sendOkay();
        break;
    }
    case Request::read_dp:
    {
        uint32_t val;
        if(pSwd->readDp(mIntArgument, val))
        {
            sendInt(val);
        }
        else
        {
            sendError("WifiDebugger: ACK FAULT received");
        }
        break;
    }
    case Request::write_dp:
    {
        if(pSwd->writeDp(mArrayArgument[0], mArrayArgument[1]))
        {
            sendOkay();
        }
        else
        {
            sendError("WifiDebugger: ACK FAULT received");
        }
        break;
    }
    case Request::read_ap:
    {
        uint32_t val;
        if(pSwd->readAp(mIntArgument, val))
        {
            sendInt(val);
        }
        else
        {
            sendError("WifiDebugger: ACK FAULT received");
        }
        break;
    }
    case Request::write_ap:
    {
        if(pSwd->writeAp(mArrayArgument[0], mArrayArgument[1]))
        {
            sendOkay();
        }
        else
        {
            sendError("WifiDebugger: ACK FAULT received");
        }
        break;
    }
    case Request::flush:
    case Request::get_memory_interface_for_ap:
    {
        sendOkay();
        //sendInt(0);
        break;
    }
    case Request::read_ap_multiple:
    {
        std::vector<uint32_t> array;
        array.resize(mArrayArgument[1]);
        if(pSwd->readApMultiple(&array[0], array.size()))
        {
            sendArray(array);
        }
        else
        {
            sendError("WifiDebugger: ACK FAULT received");
        }
        break;
    }
    case Request::write_ap_multiple:
    {
        if(pSwd->writeApMultiple(&mArrayArgument[1], mArrayArgument.size() - 1))
        {
            sendOkay();
        }
        else
        {
            sendError("WifiDebugger: ACK FAULT received");
        }
        break;
    }
    case Request::read_mem:
    {
        uint32_t data;
        if(pSwd->readMemory(mArrayArgument[1], mArrayArgument[2], data))
        {
            sendInt(data);
        }
        else
        {
            sendError("WifiDebugger: ACK FAULT received");
        }
        break;
    }
    case Request::write_mem:
    {
        if(pSwd->writeMemory(mArrayArgument[1], mArrayArgument[3], mArrayArgument[2]))
        {
            sendOkay();
        }
        else
        {
            sendError("WifiDebugger: ACK FAULT received");
        }
        break;
    }
    case Request::read_block32:
    {
        std::vector<uint32_t> array;
        array.resize(mArrayArgument[2]);
        if(pSwd->readMemoryBlcok32(mArrayArgument[1], &array[0], array.size()))
        {
            sendArray(array);
        }
        else
        {
            sendError("WifiDebugger: ACK FAULT received");
        }
        break;
    }
    case Request::write_block32:
    {
        if(pSwd->writeMemoryBlcok32(mArrayArgument[1], &mArrayArgument[2], mArrayArgument.size() - 3))
        {
            sendOkay();
        }
        else
        {
            sendError("WifiDebugger: ACK FAULT received");
        }
        break;
    }

    case Request::swd_sequence:
    case Request::jtag_sequence:
    case Request::reset:
    case Request::assert_reset:
    case Request::is_reset_asserted:
    case Request::swo_start:
    case Request::swo_stop:
    case Request::swo_read:
    case Request::read_block8:
    case Request::write_block8:
    default:
        ESP_LOGW(TAG, "Cmd %s Id %d", Request::toString(mRequest).c_str(), mId);
        break;
    }
    mStrArgument.clear();
    mArrayArgument.clear();
}

void PyOcdParser::startObject()
{
}

void PyOcdParser::endObject()
{
}

void PyOcdParser::startArray()
{
}

void PyOcdParser::endArray()
{
}

void PyOcdParser::key(const char *key)
{
    const std::string _key(key);
    if(_key == std::string("id"))
    {
        mKey = Key::eId;
    }
    else if(_key == std::string("request"))
    {
        mKey = Key::eRequest;
    }
    else if(_key == std::string("arguments"))
    {
        mKey = Key::eArguments;
    }
    else
    {
        mKey = Key::eInvalid;
        ESP_LOGE(TAG, "%s: %s", __func__, key);
    }
}

void PyOcdParser::value(const char *value)
{
    switch(mKey)
    {
    case Key::eId:
        mId = std::stoi(value);
        break;
    case Key::eRequest:
        mRequest = Request::fromString(std::string(value));
        break;
    case Key::eArguments:
        switch(Request::getArgType(mRequest))
        {
        case Request::ArgumentType::eInt:
            mIntArgument = std::stoi(value);
            break;
        case Request::ArgumentType::eString:
            mStrArgument = std::string(value);
            break;
        case Request::ArgumentType::eArray:
        {
            char* pEnd;
            uint64_t data = std::strtoull(value, &pEnd, 10);
            if(data > UINT32_MAX)
            {
                mArrayArgument.push_back(data >> 32);
            }
            mArrayArgument.push_back(data & UINT32_MAX);
            break;
        }
        default:
            break;
        }
        break;
    default:
        ESP_LOGE(TAG, "%s: %s", __func__, value);
        break;
    }
}

void PyOcdParser::whitespace(char c)
{
}

void PyOcdParser::error( const char *message )
{
    ESP_LOGE(TAG, "%s", message);
}

//-------------------------------------------------------------------
// PyOcdServer
//-------------------------------------------------------------------
PyOcdServer::PyOcdServer() : 
    ServerSocket(5555)
{

}

bool PyOcdServer::serverMain(int acceptSocekt)
{
    PyOcdParser listener(acceptSocekt);
    mPaser.setListener(&listener);
    char rx_buffer[512];
    while(true)
    {
        const int len = recv(acceptSocekt, rx_buffer, sizeof(rx_buffer) - 1, 0);
        rx_buffer[len] = 0;
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
            //ESP_LOGW(TAG, "%s", rx_buffer);
            for(int i = 0; i < len; i++)
            {
                mPaser.parse(rx_buffer[i]);
            }
        }
    }
    return true;
}
