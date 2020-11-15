#include "hw/hw_i2c_bitbang.h"

const I2cPins i2c_pins[] = {
    {
        //KK348, scheda luci
        .port = GPIOF,
        .clock_en = RCC_AHB1ENR_GPIOFEN,
        .sda = 0,
        .scl = 1,
    },

    {
        //KK353, scheda ribaltina
        .port = GPIOB,
        .clock_en = RCC_AHB1ENR_GPIOBEN,
        .sda = 7,
        .scl = 6,
    },
};

STATIC_ASSERT(I2C_CONF_LEN == countof(i2c_pins));
