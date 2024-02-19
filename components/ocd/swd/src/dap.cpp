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

#include "dap.hpp"
#include <thread>

static const char *TAG = "Dap";
using namespace std::chrono_literals;
Dap::Dap()
{
    gpio_reset_pin(cPinReset);
    gpio_set_direction(cPinReset, GPIO_MODE_OUTPUT);
    gpio_set_level(cPinReset, true);
}

Dap::~Dap()
{

}

bool Dap::setReset(bool assert)
{
    gpio_set_level(cPinReset, not assert);
    std::this_thread::sleep_for(100ms);
    return true;
}
