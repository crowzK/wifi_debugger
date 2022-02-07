#pragma once

#include "swd.hpp"
#include "driver/gpio.h"

class GpioSwd : public Swd
{
public:
    GpioSwd();
    ~GpioSwd() = default;

    uint32_t sequence(uint64_t data, uint8_t bitLength) override;
    Response write(Cmd cmd, uint32_t data) override;
    Response read(Cmd cmd, uint32_t& data) override;

protected:
    static constexpr gpio_num_t cPinSwClk = (gpio_num_t)23;
    static constexpr gpio_num_t cPinSwDio = (gpio_num_t)19;

    inline void delay();
    inline void clkCycle();
    inline void writeBit(bool bit);
    inline bool readBit();
    inline void sendRequest(uint32_t request);
    inline uint32_t readAck();
};
