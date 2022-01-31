#include "gpio_swd.hpp"

GpioSwd::GpioSwd()
{
    
}

uint32_t GpioSwd::sequence(uint32_t data, uint8_t bitLength) const
{
    return 0;
}

GpioSwd::Response GpioSwd::write(Cmd cmd, uint32_t data) const
{
    return Response::Error;
}

GpioSwd::Response GpioSwd::read(Cmd cmd, uint32_t& data) const
{
    return Response::Error;
}

