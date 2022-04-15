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

#include "cmd.hpp"
#include <esp_log.h>
#include <sstream>
#include "status.hpp"

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
    std::string uart = Status::create().getError(Status::Error::eSdcard) ? "SD Error" : "SD Okay";
    std::ostringstream cmd;
    cmd << baudrate << " " << port << " " << uart;
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
