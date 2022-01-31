#include "swd.hpp"

class GpioSwd : public Swd
{
public:
    GpioSwd();
    ~GpioSwd() = default;

    uint32_t sequence(uint32_t data, uint8_t bitLength) const override;
    Response write(Cmd cmd, uint32_t data) const override;
    Response read(Cmd cmd, uint32_t& data) const override;

protected:
    static constexpr int cPinSwClk = 19;
    static constexpr int cPinSwDio = 23;
};
