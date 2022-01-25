#include "cmd.hpp"
#include <esp_log.h>
#include <sstream>

Cmd::Cmd(Type type, SubCmd subCmd) :
    cType(type),
    cSubCmd(subCmd)
{

}

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
std::vector<uint8_t> Cmd::getCmd()
{
    std::vector<uint8_t> cmd;
    cmd.push_back((uint8_t)cType);
    cmd.push_back((uint8_t)cSubCmd);

    for(auto& c : mCmd)
    {
        cmd.push_back(c);
    }
    return cmd;
}

UartSetting::UartSetting(int baudrate, int port) :
    Cmd(Type::eServerToClient, SubCmd::eUartSetting)
{
    std::ostringstream cmd;
    cmd << baudrate << " " << port;
    mCmd = cmd.str();
}

uint8_t UartSetting::getBaudrate() const
{
    std::string delimiter = " ";
    std::string cmd = mCmd;
    size_t pos = cmd.find(delimiter);
    std::string baud = cmd.substr(0, pos);
    int nbaud = std::stoi(baud);
    return nbaud;
}

uint8_t UartSetting::getUartPort() const
{
    std::string delimiter = " ";
    std::string cmd = mCmd;
    size_t pos = cmd.find(delimiter);
    std::string port = cmd.substr(pos + 5, cmd.length());
    int nport = std::stoi(port);
    return nport;
}
