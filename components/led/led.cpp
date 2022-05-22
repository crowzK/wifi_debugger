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

#include "led.hpp"

Led::Led(gpio_num_t gpio) :
    cGpio(gpio),
    mEnable(false)
{
    if(cGpio < GPIO_NUM_MAX)
    {
        gpio_reset_pin(cGpio);
        /* Set the GPIO as a push/pull output */
        gpio_set_direction(cGpio, GPIO_MODE_OUTPUT);
        gpio_set_drive_capability(cGpio, GPIO_DRIVE_CAP_3);
    }
}

Led::~Led()
{

}

void Led::on(bool on)
{
    mEnable = on;
    if(cGpio < GPIO_NUM_MAX)
    {
        gpio_set_level(cGpio, mEnable);
    }
}

void Led::toggle(void)
{
    on(not mEnable);
}
