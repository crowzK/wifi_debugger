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
#include <chrono>

using namespace std::chrono_literals;

// Default NVIC and Core debug base addresses
// TODO: Read these addresses from ROM.
#define NVIC_Addr (0xe000e000)
#define DBG_Addr (0xe000edf0)

#include "debug_cm.h"

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

bool Swd::writeGPRs(GPRs &gprs)
{
    uint32_t i, status;

    if (not writeDp(DP_SELECT, 0))
    {
        return false;
    }

    // R0, R1, R2, R3
    for (i = 0; i < 4; i++)
    {
        if (not writeGPR(i, gprs.r[i]))
        {
            return false;
        }
    }

    // R9
    if (not writeGPR(9, gprs.r[9]))
    {
        return false;
    }

    // R13, R14, R15
    for (i = 13; i < 16; i++)
    {
        if (not writeGPR(i, gprs.r[i]))
        {
            return false;
        }
    }

    // xPSR
    if (not writeGPR(16, gprs.xpsr))
    {
        return false;
    }

    if (not writeMemory(DBG_HCSR, 32, DBGKEY | C_DEBUGEN | C_MASKINTS | C_HALT))
    {
        return false;
    }

    if (not writeMemory(DBG_HCSR, 32, DBGKEY | C_DEBUGEN | C_MASKINTS))
    {
        return false;
    }

    // check status
    if (not readDp(DP_CTRL_STAT, status))
    {
        return false;
    }

    if (status & (STICKYERR | WDATAERR))
    {
        return false;
    }

    return true;
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

bool Swd::readCoreRegister(uint32_t n, uint32_t &val)
{
    int i = 0, timeout = 100;

    if (!writeMemory(DCRSR, 32, n))
    {
        return false;
    }

    // wait for S_REGRDY
    for (i = 0; i < timeout; i++)
    {
        if (!readMemory(DHCSR, 32, val))
        {
            return false;
        }

        if (val & S_REGRDY)
        {
            break;
        }
    }

    if (i == timeout)
    {
        return false;
    }

    if (!readMemory(DCRDR, 32, val))
    {
        return false;
    }

    return true;
}

bool Swd::writeCoreRegister(uint32_t n, uint32_t val)
{
    int i = 0, timeout = 100;

    if (!writeMemory(DCRDR, 32, val))
    {
        return false;
    }

    if (!writeMemory(DCRSR, 32, n | REGWnR))
    {
        return false;
    }

    // wait for S_REGRDY
    for (i = 0; i < timeout; i++)
    {
        if (!readMemory(DHCSR, 32, val))
        {
            return false;
        }

        if (val & S_REGRDY)
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

bool Swd::sysCallExec(const program_syscall_t *sysCallParam, uint32_t entry, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, flash_algo_return_t return_type)
{
    GPRs gprs = {{0}, 0};
    // Call flash algorithm function on target and wait for result.
    gprs.r[0] = arg1;                         // R0: Argument 1
    gprs.r[1] = arg2;                         // R1: Argument 2
    gprs.r[2] = arg3;                         // R2: Argument 3
    gprs.r[3] = arg4;                         // R3: Argument 4
    gprs.r[9] = sysCallParam->static_base;    // SB: Static Base
    gprs.r[13] = sysCallParam->stack_pointer; // SP: Stack Pointer
    gprs.r[14] = sysCallParam->breakpoint;    // LR: Exit Point
    gprs.r[15] = entry;                       // PC: Entry Point
    gprs.xpsr = 0x01000000;                   // xPSR: T = 1, ISR = 0

    if (not writeGPRs(gprs))
    {
        return false;
    }

    if (not waitUntilHalted())
    {
        return false;
    }

    if (not readGPR(0, gprs.r[0]))
    {
        return false;
    }

    // remove the C_MASKINTS
    if (not writeMemory(DBG_HCSR, 32, DBGKEY | C_DEBUGEN | C_HALT))
    {
        return false;
    }

    if (return_type == FLASHALGO_RETURN_POINTER)
    {
        // Flash verify functions return pointer to byte following the buffer if successful.
        if (gprs.r[0] != (arg1 + arg2))
        {
            return false;
        }
    }
    else
    {
        // Flash functions return 0 if successful.
        if (gprs.r[0] != 0)
        {
            return false;
        }
    }

    return true;
}

bool Swd::initDebug()
{
    uint32_t tmp = 0;
    int i = 0;
    int timeout = 100;

    int8_t retries = 4;
    int8_t do_abort = 0;
    do
    {
        if (do_abort)
        {
            // do an abort on stale target, then reset the device
            writeDp(DP_ABORT, DAPABORT);
            setReset(true);
            setReset(false);
            do_abort = 0;
        }
        // swd_init();

        if (not jtagToSwd())
        {
            do_abort = 1;
            continue;
        }

        if (not cleareErrors())
        {
            do_abort = 1;
            continue;
        }

        if (not writeDp(DP_SELECT, 0))
        {
            do_abort = 1;
            continue;
        }

        // Power up
        if (not writeDp(DP_CTRL_STAT, CSYSPWRUPREQ | CDBGPWRUPREQ))
        {
            do_abort = 1;
            continue;
        }

        for (i = 0; i < timeout; i++)
        {
            if (not readDp(DP_CTRL_STAT, tmp))
            {
                do_abort = 1;
                break;
            }
            if ((tmp & (CDBGPWRUPACK | CSYSPWRUPACK)) == (CDBGPWRUPACK | CSYSPWRUPACK))
            {
                // Break from loop if powerup is complete
                break;
            }
        }
        if ((i == timeout) || (do_abort == 1))
        {
            // Unable to powerup DP
            do_abort = 1;
            continue;
        }

        if (not writeDp(DP_CTRL_STAT, CSYSPWRUPREQ | CDBGPWRUPREQ | TRNNORMAL | MASKLANE))
        {
            do_abort = 1;
            continue;
        }

        if (not writeDp(DP_SELECT, 0))
        {
            do_abort = 1;
            continue;
        }

        return 1;

    } while (--retries > 0);

    return 0;
}

bool Swd::setStateByHw(TargetState state)
{
    uint32_t val;
    int8_t ap_retries = 2;
    if (state != TargetState::eRun)
    {
        // init
    }
    switch (state)
    {
    default:
    case TargetState::eResetHold:
        setReset(true);
        break;
    case TargetState::eResetRun:
        setReset(true);
        setReset(false);
        break;
    case TargetState::eResetProgram:
        if (not initDebug())
        {
            return false;
        }
        while (writeMemory(DBG_HCSR, 32, DBGKEY | C_DEBUGEN) == 0)
        {
            if (--ap_retries <= 0)
            {
                return false;
            }
            // Target is in invalid state?
            setReset(true);
            setReset(false);
        }
        // Enable halt on reset
        if (not writeMemory(DBG_EMCR, 32, VC_CORERESET))
        {
            return false;
        }
        setReset(true);
        setReset(false);
        do
        {
            if (not readMemory(DBG_HCSR, 32, val))
            {
                return false;
            }
        } while ((val & S_HALT) == 0);

        // Disable halt on reset
        if (not writeMemory(DBG_EMCR, 32, 0))
        {
            return false;
        }
        break;
    case TargetState::eNoDebug:
        if (not writeMemory(DBG_HCSR, 32, DBGKEY))
        {
            return false;
        }
        break;
    case TargetState::eDebug:
        if (not jtagToSwd())
        {
            return false;
        }

        if (not cleareErrors())
        {
            return false;
        }

        // Ensure CTRL/STAT register selected in DPBANKSEL
        if (not writeDp(DP_SELECT, 0))
        {
            return false;
        }

        // Power up
        if (not writeDp(DP_CTRL_STAT, CSYSPWRUPREQ | CDBGPWRUPREQ))
        {
            return false;
        }

        // Enable debug
        if (not writeMemory(DBG_HCSR, 32, DBGKEY | C_DEBUGEN))
        {
            return false;
        }

        break;
    case TargetState::eHalt:
        if (not initDebug())
        {
            return false;
        }

        // Enable debug and halt the core (DHCSR <- 0xA05F0003)
        if (not writeMemory(DBG_HCSR, 32, DBGKEY | C_DEBUGEN | C_HALT))
        {
            return false;
        }

        // Wait until core is halted
        do
        {
            if (not readMemory(DBG_HCSR, 32, val))
            {
                return false;
            }
        } while ((val & S_HALT) == 0);
        break;
    case TargetState::eRun:
        if (not writeMemory(DBG_HCSR, 32, DBGKEY))
        {
            return false;
        }
        break;
    }
    return true;
}

bool Swd::setStateBySw(TargetState state)
{
    uint32_t val;
    if (state != TargetState::eRun)
    {
        // init
    }
    switch (state)
    {
    default:
    case TargetState::eResetHold:
        setReset(true);
        break;
    case TargetState::eResetRun:
        setReset(true);
        setReset(false);
        break;
    case TargetState::eResetProgram:
        setReset(true);
        setReset(false);

        if (not initDebug())
        {
            return 0;
        }

        // Power down
        // Per ADIv6 spec. Clear first CSYSPWRUPREQ followed by CDBGPWRUPREQ
        if (not readDp(DP_CTRL_STAT, val))
        {
            return 0;
        }

        if (not writeDp(DP_CTRL_STAT, val & ~CSYSPWRUPREQ))
        {
            return 0;
        }

        // Wait until ACK is deasserted
        do
        {
            if (not readDp(DP_CTRL_STAT, val))
            {
                return 0;
            }
        } while ((val & (CSYSPWRUPACK)) != 0);

        if (not writeDp(DP_CTRL_STAT, val & ~CDBGPWRUPREQ))
        {
            return 0;
        }

        // Wait until ACK is deasserted
        do
        {
            if (not readDp(DP_CTRL_STAT, val))
            {
                return 0;
            }
        } while ((val & (CDBGPWRUPACK)) != 0);
        break;
    case TargetState::eNoDebug:
        if (not writeMemory(DBG_HCSR, 32, DBGKEY))
        {
            return false;
        }
        break;
    case TargetState::eDebug:
        if (not jtagToSwd())
        {
            return false;
        }

        if (not cleareErrors())
        {
            return false;
        }

        // Ensure CTRL/STAT register selected in DPBANKSEL
        if (not writeDp(DP_SELECT, 0))
        {
            return false;
        }

        // Power up
        if (not writeDp(DP_CTRL_STAT, CSYSPWRUPREQ | CDBGPWRUPREQ))
        {
            return false;
        }

        // Enable debug
        if (not writeMemory(DBG_HCSR, 32, DBGKEY | C_DEBUGEN))
        {
            return false;
        }

        break;
    case TargetState::eHalt:
        if (not initDebug())
        {
            return false;
        }

        // Enable debug and halt the core (DHCSR <- 0xA05F0003)
        if (not writeMemory(DBG_HCSR, 32, DBGKEY | C_DEBUGEN | C_HALT))
        {
            return false;
        }

        // Wait until core is halted
        do
        {
            if (not readMemory(DBG_HCSR, 32, val))
            {
                return false;
            }
        } while ((val & S_HALT) == 0);
        break;
    case TargetState::eRun:
        if (not writeMemory(DBG_HCSR, 32, DBGKEY))
        {
            return false;
        }
        break;
    }
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
