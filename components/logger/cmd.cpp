#include "cmd.hpp"
#include <esp_log.h>

Cmd::Cmd(uint8_t* buffer, uint32_t size) :
    cType(buffer[0] == (uint8_t)Type::eClientToSever ? Type::eClientToSever :
            buffer[0] == (uint8_t)Type::eServerToClient ? Type::eServerToClient :
            Type::eInvalid),
    cSubCmd(cType == Type::eInvalid ? SubCmd::eInvalid : SubCmd::eUartSetting)
{
    if(cType != Type::eInvalid)
    {
        mCmd = std::string((char*)(buffer + 2));
    }
}

uint8_t UartSetting::getBaudrate() const
{
    std::string delimiter = " ";
    std::string cmd = mCmd;
    size_t pos = cmd.find(delimiter);
    std::string baud = cmd.substr(0, pos);
    int nbaud = std::stoi(baud);
    ESP_LOGI("UartSetting", "baud:%d", nbaud);
    return nbaud;
}

uint8_t UartSetting::getUartPort() const
{
    std::string delimiter = " ";
    std::string cmd = mCmd;
    size_t pos = cmd.find(delimiter);
    std::string port = cmd.substr(pos + 5, cmd.length());
    int nport = std::stoi(port);
    ESP_LOGI("UartSetting", "port:%d", nport);
    return nport;
}
