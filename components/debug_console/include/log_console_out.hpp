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

#ifndef LOG_CONSOLE_OUT_HPP
#define LOG_CONSOLE_OUT_HPP

#include <stdio.h>
#include <mutex>
#include <string>
#include "uart.hpp"
#include "console.hpp"


//! It is SD card class inherit logger client
class LogConsoleOut : public Client, protected Cmd
{
public:
    LogConsoleOut();
    ~LogConsoleOut() = default;

protected:
    bool write(const std::vector<uint8_t>& msg) override;
    std::string help();
    bool excute(const std::vector<std::string>& args);
};

#endif
