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
    virtual ~Swd() = default;

    virtual uint32_t sequence(uint64_t data, uint8_t bitLength) = 0;
    virtual Response write(Cmd cmd, uint32_t data) = 0;
    virtual Response read(Cmd cmd, uint32_t& data) = 0;

    bool cleareErrors();
    bool readDp(uint8_t addr, uint32_t& dp);
    bool writeDp(uint8_t addr, uint32_t dp);
    bool readAp(uint8_t addr, uint32_t& ap);
    bool readApMultiple(uint8_t addr, std::vector<uint32_t>&out);
    bool writeAp(uint8_t addr, uint32_t ap);
    bool writeApMultiple(uint8_t addr, std::vector<uint32_t>&in);
};
