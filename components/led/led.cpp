#include "led.hpp"

Led::Led(gpio_num_t gpio) :
    cGpio(gpio),
    mEnable(false)
{
    gpio_reset_pin(cGpio);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(cGpio, GPIO_MODE_OUTPUT);
}

Led::~Led()
{

}

void Led::on(bool on)
{
    mEnable = on;
    gpio_set_level(cGpio, mEnable);
}

void Led::toggle(void)
{
    on(not mEnable);
}