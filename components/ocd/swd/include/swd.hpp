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

#pragma once

#include <stdint.h>
#include <vector>
#include "dap.hpp"

// Debug Port Register Addresses
#define DP_IDCODE                       0x00U   // IDCODE Register (SW Read only)
#define DP_ABORT                        0x00U   // Abort Register (SW Write only)
#define DP_CTRL_STAT                    0x04U   // Control & Status
#define DP_WCR                          0x04U   // Wire Control Register (SW Only)
#define DP_SELECT                       0x08U   // Select Register (JTAG R/W & SW W)
#define DP_RESEND                       0x08U   // Resend (SW Read Only)
#define DP_RDBUFF                       0x0CU   // Read Buffer (Read Only)

//! SWD interface
class Swd : public Dap
{
public:
    enum class TargetState
    {
        eResetHold,
        eResetRun,
        eResetProgram,
        eNoDebug,
        eDebug,
        eHalt,
        eRun,
    };

    enum Response
    {
        Ok          = 1,
        Wait        = 2,
        Fault       = 0x4,
        Error       = 0x8,
        Mismatch    = 0x10,
        ParityError = 0x12,
    };
    
    //! ARM General Purpose Registors
    struct GPRs
    {
        uint32_t r[16];
        uint32_t xpsr;
    };

	struct ProgramSysCall
	{
		uint32_t breakPoint;		// RAM start + 1 (LR)
		uint32_t staticBase;		// data section (SB)
		uint32_t stackPointer;		// initiali stack pointer (SP)
	};

    enum class FlashAlgoRetType
    {
		cBool,      // Ctype return success: 0, fails != 0
		cPointer
	};

    using Cmd = uint8_t;
    Swd();
    virtual ~Swd() = default;

    virtual uint32_t sequence(uint64_t data, uint8_t bitLength) = 0;
    virtual Response write(Cmd cmd, uint32_t data) = 0;
    virtual Response read(Cmd cmd, uint32_t& data) = 0;

    inline uint8_t getCmd(Cmd cmd)
    {
        uint32_t parity = __builtin_popcount(cmd);
        return 0x81 | (cmd << 1) | ((parity & 1) << 5);
    }

    bool cleareErrors();
    bool readDp(uint8_t addr, uint32_t& dp);
    bool writeDp(uint8_t addr, uint32_t dp);
    bool readAp(uint8_t addr, uint32_t& ap, bool dpSelect = false);
    bool readApMultiple(uint32_t* pBuffer, uint32_t length);
    bool writeAp(uint8_t addr, uint32_t ap, bool dpSelect = false);
    bool writeApMultiple(const uint32_t* pBuffer, uint32_t length);

    bool writeMemory(uint32_t addr, uint32_t transferSize, uint32_t data);
    bool readMemory(uint32_t addr, uint32_t transferSize, uint32_t& data);
    bool writeMemoryBlcok32(uint32_t addr, const uint32_t* pBuffer, uint32_t length);
    bool readMemoryBlcok32(uint32_t addr, uint32_t* pBuffer, uint32_t length);

    bool errorCheck(const char* func, Response res);

    bool readGPR(uint32_t n, uint32_t& regVal);
    bool writeGPR(uint32_t n, uint32_t regVal);
    bool writeDebugState(GPRs& gprs);

    bool waitUntilHalted();
    bool jtagToSwd();

    bool sysCallExec(const ProgramSysCall& sysCallParam, uint32_t entry, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, FlashAlgoRetType return_type);
    bool initDebug();
    bool setStateByHw(TargetState state);
    bool setStateBySw(TargetState state);
    void printPC();
private:
    uint32_t mCsw;
    bool reset();
    bool switchMode(uint16_t mode);
    bool readIdCode(uint32_t& id);
};
