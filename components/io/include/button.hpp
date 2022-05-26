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

#ifndef BUTTON_HPP
#define BUTTON_HPP

#include <functional>
#include <mutex>
#include "driver/gpio.h"
#include "sw_timer.hpp"

class Button
{
public:
    enum class Event
    {
        ePress,
        eLongPress,
        eRelease,
    };
    Button(gpio_num_t gpio, std::function<void(Event evt)>&& callback);
    ~Button();
    void enable(bool en = true);

protected:
    std::recursive_mutex mMutex;
    const gpio_num_t cGpio;
    bool mEnable;
    bool mLastStatus;
    SWTimer mTimer;
    uint32_t mPressedTimeMs;
    std::function<void(Event evt)> mCb;
};

#endif // BUTTON_HPP
