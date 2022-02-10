#pragma once

#include <stdint.h>
#include <vector>

//! SWD interface
class Swd
{
public:
    enum Response
    {
        Ok          = 1,
        Wait        = 2,
        Fault       = 0x4,
        Error       = 0x8,
        Mismatch    = 0x10,
        ParityError = 0x12,
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

private:
    uint32_t mCsw;
};
