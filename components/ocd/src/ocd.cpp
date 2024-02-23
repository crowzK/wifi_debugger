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

#include "fs_manager.hpp"
#include "ocd.hpp"
#include "pyocd_server.hpp"
#include "flash_algo.hpp"

Ocd& Ocd::create()
{
    static Ocd ocd;
    return ocd;
}

Ocd::Ocd() :
    mpPyOcdServer(std::make_unique<PyOcdServer>())
{
    auto& fsm = FsManager::create();

    if(fsm.mount())
    {
        std::string path = std::string(fsm.getMountPoint()) + std::string("STM32F4xx_512.FLM");
        mpFlashAlgo = std::make_unique<FlashAlgo>(path, FlashAlgo::RamInfo{.ramStartAddr = 0x20000000, .ramSize = 0x4000});
    }
}

Ocd::~Ocd()
{

}