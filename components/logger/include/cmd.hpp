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

#ifndef _CMD_HPP_
#define _CMD_HPP_

#include <stdint.h>
#include <string>
#include <vector>

class Cmd
{
public:
    enum class Type : uint8_t
    {
        eInvalid,
        eClientToSever = 17,
        eServerToClient = 18
    };
    enum class SubCmd : uint8_t
    {
        eUartSetting = 1,
        eInvalid,
    };
    Cmd(Type, SubCmd);
    Cmd(uint8_t* buffer, uint32_t size);
    ~Cmd() = default;

    Type getCmdType() const { return cType; };
    SubCmd getSubCmd() const { return cSubCmd; }
    std::vector<uint8_t> getCmd();

protected:
    Type cType;
    SubCmd cSubCmd;
    std::string mCmd;
};

class UartSetting : public Cmd
{
public:
    UartSetting(int baudrate, int port);
    ~UartSetting() = default;
    uint8_t getBaudrate() const;
    uint8_t getUartPort() const;
};

#endif 
