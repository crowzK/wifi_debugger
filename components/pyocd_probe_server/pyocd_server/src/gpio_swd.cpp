#include "gpio_swd.hpp"
#include <esp_log.h>

inline void GpioSwd::delay()
{
    for(int i = 0; i < 0; i ++)
    {
        ;
    }
}

inline void GpioSwd::clkCycle()
{
    gpio_set_level(cPinSwClk, false);
    delay();
    gpio_set_level(cPinSwClk, true);
    delay();
}

inline void GpioSwd::writeBit(bool bit)
{
    gpio_set_level(cPinSwDio, bit);
    gpio_set_level(cPinSwClk, false);
    delay();
    gpio_set_level(cPinSwClk, true);
    delay();
}

inline bool GpioSwd::readBit()
{
    int in;
    gpio_set_level(cPinSwClk, false);
    delay();
    in = gpio_get_level(cPinSwDio);
    gpio_set_level(cPinSwClk, true);
    delay();

    return in;
}

inline void GpioSwd::sendRequest(uint32_t request)
{
    uint32_t parity = 0U;
    writeBit(true);         /* Start Bit */
    uint32_t bit = request >> 0;
    writeBit(bit & 1);          /* APnDP Bit */
    parity += bit;
    bit = request >> 1;
    writeBit(bit & 1);          /* RnW Bit */
    parity += bit;
    bit = request >> 2;
    writeBit(bit & 1);          /* A2 Bit */
    parity += bit;
    bit = request >> 3;
    writeBit(bit & 1);          /* A3 Bit */
    parity += bit;
    writeBit(parity & 1);       /* Parity Bit */
    writeBit(false);        /* Stop Bit */
    writeBit(true);         /* Park Bit */
}

inline uint32_t GpioSwd::readAck()
{
    uint32_t bit = readBit();
    uint32_t ack = (bit << 0);
    bit = readBit();
    ack |= (bit << 1);
    bit = readBit();
    ack |= (bit << 2);
    return ack;
}

GpioSwd::GpioSwd()
{
    gpio_reset_pin(cPinSwDio);
    gpio_reset_pin(cPinSwClk);
    gpio_set_direction(cPinSwDio, GPIO_MODE_OUTPUT);
    gpio_set_direction(cPinSwClk, GPIO_MODE_OUTPUT);
    gpio_set_level(cPinSwDio, false);
    gpio_set_level(cPinSwClk, false);
}

uint32_t GpioSwd::sequence(uint64_t data, uint8_t bitLength)
{
    for (int i = 0; i < bitLength; ++i)
    {
        writeBit((data & 1) > 0);
        data >>= 1;
    }
    return 0;
}

GpioSwd::Response GpioSwd::write(Cmd cmd, uint32_t data)
{
    sendRequest(cmd);
    gpio_set_direction(cPinSwDio, GPIO_MODE_INPUT);
    clkCycle();

    Response ack = (Response)readAck();
    clkCycle();
    gpio_set_direction(cPinSwDio, GPIO_MODE_OUTPUT);

    uint32_t parity = 0;
    uint32_t val = data;
    for(int i = 32; i; i--)
    {
        writeBit((val & 1) > 0);
        parity += val;
        val >>= 1;
    }
    writeBit((parity & 1) > 0);
    gpio_set_level(cPinSwDio, false);
    clkCycle();
    clkCycle();
    gpio_set_level(cPinSwDio, true);
    return ack;
}

GpioSwd::Response GpioSwd::read(Cmd cmd, uint32_t& data)
{
    sendRequest(cmd);
    gpio_set_direction(cPinSwDio, GPIO_MODE_INPUT);
    clkCycle();

    Response ack = (Response)readAck();
    uint32_t parity = 0;
    uint32_t val = 0;
    for(int i = 32; i; i--)
    {
        bool bit = readBit();
        parity += bit;
        val >>= 1;
        val |= bit << 31;
    }
    bool bit = readBit();
    if((parity ^ bit) & 1)
    {
        ack = Response::ParityError;
    }
    data = val;
    clkCycle();
    gpio_set_direction(cPinSwDio, GPIO_MODE_OUTPUT);
    gpio_set_level(cPinSwDio, false);
    clkCycle();
    clkCycle();
    gpio_set_level(cPinSwDio, true);
    return ack;
}

