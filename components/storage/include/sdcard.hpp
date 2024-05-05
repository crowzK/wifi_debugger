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
#include "sdkconfig.h"
#include <mutex>

//! It is SD card class inherit logger client
class SdCard
{
public:
    SdCard(const char* mountPoint);
    ~SdCard();
    bool isInit() const;
protected:
    const char* cMountPoint;
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
#elif CONFIG_WIFI_DEBUGGER_V_0_1
    static constexpr int cPinMiso = 6;
    static constexpr int cPinMosi = 8;
    static constexpr int cPinClk = 7;
    static constexpr int cPinCs = 9;
    static constexpr int cSpiPort = 1;
#elif CONFIG_WIFI_DEBUGGER_V_0_2
    static constexpr int cPinMiso = 3;
    static constexpr int cPinMosi = 1;
    static constexpr int cPinClk = 2;
    static constexpr int cPinCs = 10;
    static constexpr int cSpiPort = 1;
#elif CONFIG_WIFI_DEBUGGER_V_0_4
    static constexpr int cClk = 12;
    static constexpr int cCmd = 13;
    static constexpr int cD0 = 11;
    static constexpr int cD1 = 10;
    static constexpr int cD2 = 21;
    static constexpr int cD3 = 14;
    static constexpr int cCd = 9;
#elif CONFIG_WIFI_DEBUGGER_V_0_6
    static constexpr int cClk = 21;
    static constexpr int cCmd = 14;
    static constexpr int cD0 = 47;
    static constexpr int cD1 = 48;
    static constexpr int cD2 = 12;
    static constexpr int cD3 = 13;
    static constexpr int cCd = 45;
#endif

#if CONFIG_SD_SDIO

#endif
    void* pSdcard;
    bool mInit;
};

#endif
