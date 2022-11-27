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

#ifndef OPTION_HPP
#define OPTION_HPP

#include <memory>

namespace nvs
{
    class NVSHandle;
}

class Setting
{
public:
    static Setting& create();

    uint32_t getDebugUartBaud() const;
    void setDebugUartBaud(uint32_t baudrate) const;
    uint32_t getLienEnd() const;
    void setLienEnd(uint32_t lineEnd) const;

protected:
    static constexpr uint32_t cDefaultBaud = 230400;
    static constexpr uint32_t cDefaultLienEnd = 3; // default lfcr
    std::shared_ptr<nvs::NVSHandle> mHandle;

    Setting();
    ~Setting();
};



#endif // OPTION_HPP
