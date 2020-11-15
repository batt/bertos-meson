#include <cfg/macros.h>
#include <cfg/debug.h>

#include <drv/eth.h>

#include <io/cm3.h>

#include "drv/eth_stm32.h"
#include "eth_cfg.h"

#define KK348_USE_RMII 0

const Stm32EthCfg eth_cfg_kk348 = {
#if _DEBUG
    .e = {
        ._type = ETH_STM32,
    },
#endif
#if KK348_USE_RMII
    .mac_mode = STM32_MAC_RMII,
    .pins = {
        {
            .gpio = GPIOA,
            .pin_mask = BV(1) | BV(2) | BV(7),
            .clk_mask = RCC_AHB1ENR_GPIOAEN,
        },
        {
            .gpio = GPIOC,
            .pin_mask = BV(1) | BV(4) | BV(5),
            .clk_mask = RCC_AHB1ENR_GPIOCEN,
        },
        {
            .gpio = GPIOG,
            .pin_mask = BV(11) | BV(13) | BV(14),
            .clk_mask = RCC_AHB1ENR_GPIOGEN,
        },
        {
            /* Terminator */
            .gpio = NULL,
        },
    },
#else
    .mac_mode = STM32_MAC_MII,
    .pins = {
        {
            .gpio = GPIOA,
            .pin_mask = BV(0) | BV(1) | BV(2) | BV(3) | BV(7),
            .clk_mask = RCC_AHB1ENR_GPIOAEN,
        },
        {
            .gpio = GPIOB,
            .pin_mask = BV(0) | BV(1) | BV(8) | BV(10),
            .clk_mask = RCC_AHB1ENR_GPIOBEN,
        },
        {
            .gpio = GPIOC,
            .pin_mask = BV(1) | BV(2) | BV(3) | BV(4) | BV(5),
            .clk_mask = RCC_AHB1ENR_GPIOCEN,
        },
        {
            .gpio = GPIOG,
            .pin_mask = BV(11) | BV(13) | BV(14),
            .clk_mask = RCC_AHB1ENR_GPIOGEN,
        },
        {
            /* Terminator */
            .gpio = NULL,
        },
    },
#endif
};

const Stm32EthCfg eth_cfg_kk353_kk354 = {
#if _DEBUG
    .e = {
        ._type = ETH_STM32,
    },
#endif

    .mac_mode = STM32_MAC_MII,
    .pins = {
        {
            .gpio = GPIOA,
            .pin_mask = BV(0) | BV(1) | BV(2) | BV(3) | BV(7),
            .clk_mask = RCC_AHB1ENR_GPIOAEN,
        },
        {
            .gpio = GPIOB,
            .pin_mask = BV(0) | BV(1) | BV(8) | BV(10) | BV(11) | BV(12) | BV(13),
            .clk_mask = RCC_AHB1ENR_GPIOBEN,
        },
        {
            .gpio = GPIOC,
            .pin_mask = BV(1) | BV(2) | BV(3) | BV(4) | BV(5),
            .clk_mask = RCC_AHB1ENR_GPIOCEN,
        },
        {
            /* Terminator */
            .gpio = NULL,
        },
    },
};

const Stm32EthCfg eth_cfg_stm3220g = {
#if _DEBUG
    .e = {
        ._type = ETH_STM32,
    },
#endif

    .mac_mode = STM32_MAC_RMII,
    .pins = {
        {
            .gpio = GPIOA,
            .pin_mask = BV(1) | BV(2) | BV(7),
            .clk_mask = RCC_AHB1ENR_GPIOAEN,
        },
        {
            .gpio = GPIOC,
            .pin_mask = BV(1) | BV(4) | BV(5),
            .clk_mask = RCC_AHB1ENR_GPIOCEN,
        },
        {
            .gpio = GPIOG,
            .pin_mask = BV(11) | BV(13) | BV(14),
            .clk_mask = RCC_AHB1ENR_GPIOGEN,
        },
        {
            /* Terminator */
            .gpio = NULL,
        },
    },

};