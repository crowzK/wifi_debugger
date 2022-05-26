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

#ifndef SW_TIMER_HPP
#define SW_TIMER_HPP

#include "esp_timer.h"
#include <functional>

class SWTimer
{
public:
    enum class Mode
    {
        ePeriodic,
        eOneshot
    };
    SWTimer();
    ~SWTimer();
    void start(Mode mode, uint32_t periodMs, std::function<void()>&& cb);
    void stop();
protected:
    const esp_timer_create_args_t cTimerArg;
    esp_timer_handle_t mTimerHandle;
    std::function<void()> mCallback;
    
    static void cb(void* arg);
};


#endif //SW_TIMER_HPP
