#ifndef HW_MIRROR_H
#define HW_MIRROR_H

#include <io/cm3.h>
#include <drv/gpio_stm32.h>

#define MIRROR_FAULT_BIT 1 //PD1

#define MIRROR_SLEEP_PIN BV(4) //PD4
#define MIRROR_FAULT_PIN BV(MIRROR_FAULT_BIT)

#define MIRROR_PIN_INIT()                                                                 \
	do                                                                                    \
	{                                                                                     \
		RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;                                              \
		stm32_gpioPinConfig(GPIOD, MIRROR_FAULT_PIN, GPIO_MODE_IPU, GPIO_SPEED_50MHZ);    \
		stm32_gpioPinConfig(GPIOD, MIRROR_SLEEP_PIN, GPIO_MODE_OUT_PP, GPIO_SPEED_50MHZ); \
		MIRROR_SLEEP(true);                                                               \
	} while (0)

/** True if motor driver is in fault. */
#define MIRROR_FAULT() (!stm32_gpioPinRead(GPIOD, MIRROR_FAULT_PIN))

/** Set to true to disable driver */
#define MIRROR_SLEEP(sleep) \
	stm32_gpioPinWrite(GPIOD, MIRROR_SLEEP_PIN, !(sleep))

#define MIRROR_PWM_CH   2
#define MIRROR_POLARITY PWM_POL_HIGH_PULSE

#endif //HW_MIRROR_H
