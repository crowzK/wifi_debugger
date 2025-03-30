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

#include "sw_timer.hpp"

SWTimer::SWTimer() :
    cTimerArg{
            .callback = &cb,
            .arg = (void*) this,
            .dispatch_method = esp_timer_dispatch_t::ESP_TIMER_TASK,
            .name = __func__,
            .skip_unhandled_events = false
        },
    mTimerHandle(nullptr)
{
    esp_timer_create(&cTimerArg, &mTimerHandle);
}

SWTimer::~SWTimer()
{
    esp_timer_stop(mTimerHandle);
    esp_timer_delete(mTimerHandle);
}

void SWTimer::start(Mode mode, uint32_t periodMs, std::function<void()>&& cb)
{
    switch (mode)
    {
    default:
    case Mode::eOneshot:
        esp_timer_start_once(mTimerHandle, periodMs * 1000);
        break;
    case Mode::ePeriodic:
        esp_timer_start_periodic(mTimerHandle, periodMs * 1000);
        break;
    }
    mCallback = cb;
}

void SWTimer::stop()
{
    esp_timer_stop(mTimerHandle);
    mCallback = nullptr;
}


void SWTimer::cb(void *arg)
{
    SWTimer* pTimer = (SWTimer*)arg;
    if(pTimer->mCallback)
    {
        pTimer->mCallback();
    }
}