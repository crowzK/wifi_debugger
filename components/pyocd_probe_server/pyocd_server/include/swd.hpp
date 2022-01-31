#include <stdint.h>

//! SWD interface
class Swd
{
public:
    enum Response
    {
        Ok          = 0,
        Wait        = 1,
        Fault       = 0x2,
        Error       = 0x4,
        Mismatch    = 0x8,
        ParityError = 0x10,
    };
    using Cmd = uint8_t;

    virtual ~Swd() = default;

    virtual uint32_t sequence(uint32_t data, uint8_t bitLength) const = 0;
    virtual Response write(Cmd cmd, uint32_t data) const = 0;
    virtual Response read(Cmd cmd, uint32_t& data) const = 0;
};
