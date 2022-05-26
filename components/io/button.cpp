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

#include "button.hpp"
#include <esp_log.h>

Button::Button(gpio_num_t gpio, std::function<void(Event evt)>&& callback) :
    cGpio(gpio),
    mEnable(false),
    mCb(callback)
{
    if(cGpio < GPIO_NUM_MAX)
    {
        gpio_reset_pin(cGpio);
        /* Set the GPIO as a input */
        gpio_set_direction(cGpio, GPIO_MODE_INPUT);
        gpio_set_pull_mode(cGpio, gpio_pull_mode_t::GPIO_PULLUP_ONLY);
    }
}

Button::~Button()
{
    enable(false);
}

void Button::enable(bool en)
{
    if(mEnable == en)
    {
        return;
    }

    mEnable = en;
    if(cGpio >= GPIO_NUM_MAX)
    {
        return;
    }
    if(mEnable)
    {
        mTimer.start(SWTimer::Mode::ePeriodic, 100, [this]
        {
            bool press = not gpio_get_level(cGpio);
            if(not mCb)
            {
                mLastStatus = press;
                return;
            }
            if(press)
            {
                if(mPressedTimeMs == 0)
                {
                    mCb(Event::ePress);
                }
                mPressedTimeMs += 100;
                if(mPressedTimeMs > 3000)
                {
                    mCb(Event::eLongPress);
                }
            }
            else if(mLastStatus)
            {
                mPressedTimeMs = 0;
                mCb(Event::eRelease);
            }
            mLastStatus = press;
        });
    }
    else
    {
        mTimer.stop();
    }
}

