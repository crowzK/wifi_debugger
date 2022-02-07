#pragma once

#include "swd.hpp"
#include "driver/gpio.h"
#include "driver/spi_master.h"

class SpiSwd : public Swd
{
public:
    SpiSwd();
    ~SpiSwd() = default;

    uint32_t sequence(uint64_t data, uint8_t bitLength) override;
    Response write(Cmd cmd, uint32_t data) override;
    Response read(Cmd cmd, uint32_t& data) override;

protected:
    static constexpr gpio_num_t cPinSwClk = (gpio_num_t)19;
    static constexpr gpio_num_t cPinSwDio = (gpio_num_t)23;
};
