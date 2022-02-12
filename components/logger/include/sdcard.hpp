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

#ifndef SDCARD_HPP
#define SDCARD_HPP

#include <stdio.h>
#include "debug_msg_handler.hpp"
#include "sdkconfig.h"
#include <mutex>

class SdCard : public Client
{
public:
    SdCard();
    ~SdCard();
    void init();
    bool write(const char* msg, uint32_t length);
    bool write(const std::vector<uint8_t>& msg) override;

protected:
    static const char* cMountPoint;
#if CONFIG_M5STACK_CORE
    static constexpr int cPinMiso = 19;
    static constexpr int cPinMosi = 23;
    static constexpr int cPinClk = 18;
    static constexpr int cPinCs = 4;
    static constexpr int cSpiPort = 2;
#elif CONFIG_TTGO_T1
    static constexpr int cPinMiso = 2;
    static constexpr int cPinMosi = 15;
    static constexpr int cPinClk = 14;
    static constexpr int cPinCs = 13;
    static constexpr int cSpiPort = 1;
#endif
    std::recursive_mutex mMutex;

    void* pSdcard;
    FILE* pFile;
    bool mInited;

    FILE* createFile();
};

#endif
