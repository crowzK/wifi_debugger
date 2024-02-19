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

#pragma once

#include <stdint.h>
#include <functional>

class PyOcdIo
{
public:
    using RcvCallback = std::function<void(char* rcvMsg, int len)>;
    PyOcdIo(RcvCallback&& callback) : 
        mRcvCallback(callback)
    {
        
    }
    virtual ~PyOcdIo() = default;
    virtual uint32_t send(const char* message, uint32_t len) = 0;

protected:
    RcvCallback mRcvCallback;
};
