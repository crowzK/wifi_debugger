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

#include "swd.hpp"
#include <esp_log.h>
#include "debug_cm.h"

// Default NVIC and Core debug base addresses
// TODO: Read these addresses from ROM.
#define NVIC_Addr (0xe000e000)
#define DBG_Addr (0xe000edf0)

// AP CSW register, base value
#define CSW_VALUE (CSW_RESERVED | CSW_MSTRDBG | CSW_HPROT | CSW_DBGSTAT | CSW_SADDRINC)

#define DCRDR 0xE000EDF8
#define DCRSR 0xE000EDF4
#define DHCSR 0xE000EDF0
#define REGWnR (1 << 16)

#define MAX_SWD_RETRY 100   // 10
#define MAX_TIMEOUT 1000000 // Timeout for syscalls on target

static const char *TAG = "Swd";

Swd::Swd() : mCsw(UINT32_MAX)
{
}

bool Swd::errorCheck(const char *func, Response res)
{
    if (res != Response::Ok)
    {
        ESP_LOGE(TAG, "%s Res %d", func, (int)res);
        return false;
    }
    return true;
}

bool Swd::cleareErrors()
{
    return writeDp(DP_ABORT, STKCMPCLR | STKERRCLR | WDERRCLR | ORUNERRCLR);
}

bool Swd::readDp(uint8_t addr, uint32_t &dp)
{
    auto res = read(SWD_REG_DP | SWD_REG_R | SWD_REG_ADR(addr), dp);
    return errorCheck(__func__, res);
}

bool Swd::writeDp(uint8_t addr, uint32_t dp)
{
    auto res = write(SWD_REG_DP | SWD_REG_W | SWD_REG_ADR(addr), dp);
    return errorCheck(__func__, res);
}

bool Swd::readAp(uint8_t addr, uint32_t &ap, bool dpSelect)
{
    if (dpSelect)
    {
        constexpr uint32_t apsel = DP_SELECT & 0xff000000;
        constexpr uint32_t bank_sel = DP_SELECT & APBANKSEL;
        if (not writeDp(DP_SELECT, apsel | bank_sel))
        {
            return false;
        }
    }
    read(SWD_REG_AP | SWD_REG_R | SWD_REG_ADR(addr), ap);
    auto res = read(SWD_REG_AP | SWD_REG_R | SWD_REG_ADR(addr), ap);
    return errorCheck(__func__, res);
}

bool Swd::readApMultiple(uint32_t *pBuffer, uint32_t length)
{
    uint32_t ap;
    Response res;

    res = read(SWD_REG_AP | SWD_REG_R | AP_DRW, ap);
    if (not errorCheck(__func__, res))
    {
        return false;
    }
    for (int i = 0; i < length - 1; i++)
    {
        res = read(SWD_REG_AP | SWD_REG_R | AP_DRW, ap);
        if (not errorCheck(__func__, res))
        {
            return false;
        }
        pBuffer[i] = ap;
    }
    res = read(SWD_REG_DP | SWD_REG_R | SWD_REG_ADR(DP_RDBUFF), ap);
    pBuffer[length - 1] = ap;
    return errorCheck(__func__, res);
}

bool Swd::writeAp(uint8_t addr, uint32_t ap, bool dpSelect)
{
    if (dpSelect)
    {
        constexpr uint32_t apsel = DP_SELECT & 0xff000000;
        constexpr uint32_t bank_sel = DP_SELECT & APBANKSEL;
        if (not writeDp(DP_SELECT, apsel | bank_sel))
        {
            return false;
        }
        if ((addr == AP_CSW) and (ap == mCsw))
        {
            return true;
        }
    }

    auto res = write(SWD_REG_AP | SWD_REG_W | SWD_REG_ADR(addr), ap);
    if (not errorCheck(__func__, res))
    {
        return false;
    }

    res = read(SWD_REG_DP | SWD_REG_R | SWD_REG_ADR(DP_RDBUFF), ap);
    return errorCheck(__func__, res);
}

bool Swd::writeApMultiple(const uint32_t *pBuffer, uint32_t length)
{
    Response res;
    for (int i = 0; i < length; i++)
    {
        res = write(SWD_REG_AP | SWD_REG_W | AP_DRW, pBuffer[i]);
        if (not errorCheck(__func__, res))
        {
            return false;
        }
    }
    uint32_t dummy;
    res = read(SWD_REG_DP | SWD_REG_R | SWD_REG_ADR(DP_RDBUFF), dummy);
    return errorCheck(__func__, res);
}

bool Swd::writeMemory(uint32_t addr, uint32_t transferSize, uint32_t data)
{
    uint32_t csw;
    uint32_t tmp;
    switch (transferSize)
    {
    case 8:
        csw = CSW_VALUE | CSW_SIZE8;
        tmp = data << ((addr & 0x03) << 3);
        break;
    case 16:
        csw = CSW_VALUE | CSW_SIZE16;
        tmp = data << ((addr & 0x02) << 3);
        break;
    case 32:
        csw = CSW_VALUE | CSW_SIZE32;
        tmp = data;
        break;
    default:
        return false;
    }
    if (not writeAp(AP_CSW, csw, true))
    {
        return false;
    }

    // TAR
    auto res = write(SWD_REG_AP | SWD_REG_W | AP_TAR, addr);
    if (not errorCheck(__func__, res))
    {
        return false;
    }

    // data
    res = write(SWD_REG_AP | SWD_REG_W | AP_DRW, tmp);
    if (not errorCheck(__func__, res))
    {
        return false;
    }

    // dummy read
    res = read(SWD_REG_DP | SWD_REG_R | SWD_REG_ADR(DP_RDBUFF), tmp);
    return errorCheck(__func__, res);
}

bool Swd::readMemory(uint32_t addr, uint32_t transferSize, uint32_t &data)
{
    uint32_t csw;
    switch (transferSize)
    {
    case 8:
        csw = CSW_VALUE | CSW_SIZE8;
        break;
    case 16:
        csw = CSW_VALUE | CSW_SIZE16;
        break;
    case 32:
        csw = CSW_VALUE | CSW_SIZE32;
        break;
    default:
        return false;
    }

    // CSW
    if (not writeAp(AP_CSW, csw, true))
    {
        return false;
    }

    // TAR
    Response res = write(SWD_REG_AP | SWD_REG_W | AP_TAR, addr);
    if (not errorCheck(__func__, res))
    {
        return false;
    }

    uint32_t tmp;
    // dummy read
    res = read(SWD_REG_AP | SWD_REG_R | AP_DRW, tmp);
    if (not errorCheck(__func__, res))
    {
        return false;
    }

    // read data
    res = read(SWD_REG_AP | SWD_REG_R | SWD_REG_ADR(DP_RDBUFF), tmp);
    if (not errorCheck(__func__, res))
    {
        return false;
    }

    switch (transferSize)
    {
    case 8:
        data = tmp >> ((addr & 0x03) << 3);
        break;
    case 16:
        data = tmp >> ((addr & 0x02) << 3);
        break;
    case 32:
        data = tmp;
        break;
    default:
        return false;
    }
    return true;
}

bool Swd::writeMemoryBlcok32(uint32_t addr, const uint32_t *pBuffer, uint32_t length)
{
    if (not writeAp(AP_CSW, (CSW_VALUE | CSW_SIZE32), true))
    {
        return false;
    }

    // TAR
    Response res = write(SWD_REG_AP | SWD_REG_W | AP_TAR, addr);
    if (not errorCheck(__func__, res))
    {
        return false;
    }

    return writeApMultiple(pBuffer, length);
}

bool Swd::readMemoryBlcok32(uint32_t addr, uint32_t *pBuffer, uint32_t length)
{
    if (not writeAp(AP_CSW, (CSW_VALUE | CSW_SIZE32), true))
    {
        return false;
    }

    // TAR
    Response res = write(SWD_REG_AP | SWD_REG_W | AP_TAR, addr);
    if (not errorCheck(__func__, res))
    {
        return false;
    }

    return readApMultiple(pBuffer, length);
}

bool Swd::readGPR(uint32_t n, uint32_t &regVal)
{
    int i = 0, timeout = 100;

    if (not writeMemory(DCRSR, 32, n))
    {
        return false;
    }

    // wait for S_REGRDY
    for (i = 0; i < timeout; i++)
    {
        if (!readMemory(DHCSR, 32, regVal))
        {
            return false;
        }

        if (regVal & S_REGRDY)
        {
            break;
        }
    }

    if (i == timeout)
    {
        return false;
    }

    if (not readMemory(DCRDR, 32, regVal))
    {
        return false;
    }

    return true;
}

bool Swd::writeGPR(uint32_t n, uint32_t regVal)
{
    int i = 0, timeout = 100;

    if (not writeMemory(DCRDR, 32, regVal))
    {
        return false;
    }

    if (not writeMemory(DCRSR, 32, n | REGWnR))
    {
        return false;
    }

    // wait for S_REGRDY
    for (i = 0; i < timeout; i++)
    {
        if (not readMemory(DHCSR, 32, regVal))
        {
            return false;
        }

        if (regVal & S_REGRDY)
        {
            return true;
        }
    }

    return false;
}

bool Swd::waitUntilHalted()
{
    // Wait for target to stop
    uint32_t val, i, timeout = MAX_TIMEOUT;

    for (i = 0; i < timeout; i++)
    {
        if (not readMemory(DBG_HCSR, 32, val))
        {
            return false;
        }

        if (val & S_HALT)
        {
            return true;
        }
    }

    return false;
}

bool Swd::jtagToSwd()
{
    reset();
    switchMode(0xe79e);
    reset();
    uint32_t id;
    readIdCode(id);
    return true;
}

bool Swd::reset()
{
    uint64_t tmp = 0xffffffffffffffff;
    sequence(tmp, 51);
    return true;
}

bool Swd::switchMode(uint16_t mode)
{
    sequence(mode, 16);
    return true;
}

bool Swd::readIdCode(uint32_t &id)
{
    sequence(0, 8);
    return readDp(0, id);
}
