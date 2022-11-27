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

#ifndef UART_BYPASS_HPP
#define UART_BYPASS_HPP

#include <stdio.h>
#include <mutex>
#include <string>
#include "uart.hpp"
#include "console.hpp"
#include "task.hpp"

class LineEndMap : protected Cmd
{
public:
    enum Map
    {
        eCrLf,
        eCrCrLf,
        eLfCr,
        eLfCrLf,
    };

    LineEndMap();
    ~LineEndMap();
    Map getMap() { return mLineEndMap; }

protected:
    Map mLineEndMap;
    std::string mLineEndMapStr;
    std::string help() override;
    bool setLineEnd(const std::string& str);
    bool excute(const std::vector<std::string>& args) override;
};

//! To by pass uart debug message to the USB CDC
class UartByPass : public Client, protected Cmd, protected Task
{
public:
    UartByPass();
    ~UartByPass() = default;

protected:
    BlockingQueue<std::string> mQueue;
    LineEndMap mLineEndMode;

    bool writeStr(const MsgProxy::Msg& msg) override;
    std::string help();
    bool excute(const std::vector<std::string>& args);
    void task() override;
};

#endif
