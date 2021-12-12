#ifndef LED_HPP
#define LED_HPP

#include "driver/gpio.h"

class Led
{
public:
    Led(gpio_num_t gpio);
    ~Led();
    void on(bool on = true);
    void toggle(void);

protected:
    const gpio_num_t cGpio;
    bool mEnable;
};

#endif // LED_HPP
