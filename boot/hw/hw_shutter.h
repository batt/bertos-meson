#ifndef HW_SHUTTER_H
#define HW_SHUTTER_H

#include <io/cm3.h>
#include <drv/gpio_stm32.h>

#define SHUTTER_FAULT_BIT 2 //PD2

#define SHUTTER_SLEEP_PIN    BV(5) //PD5
#define SHUTTER_FAULT_PIN    BV(SHUTTER_FAULT_BIT)
#define SHUTTER_DIR_PIN      BV(6) //PD6
#define SHUTTER_OPEN_SW_PIN  BV(7) //PD7
#define SHUTTER_CLOSE_SW_PIN BV(8) //PD8

#define SHUTTER_DIR_CLOSE 0
#define SHUTTER_DIR_OPEN  1

#define SHUTTER_PIN_INIT()                                                                                                           \
	do                                                                                                                               \
	{                                                                                                                                \
		RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;                                                                                         \
		stm32_gpioPinConfig(GPIOD, SHUTTER_SLEEP_PIN, GPIO_MODE_OUT_PP, GPIO_SPEED_50MHZ);                                           \
		stm32_gpioPinConfig(GPIOD, SHUTTER_FAULT_PIN | SHUTTER_OPEN_SW_PIN | SHUTTER_CLOSE_SW_PIN, GPIO_MODE_IPU, GPIO_SPEED_50MHZ); \
		SHUTTER_SLEEP(true);                                                                                                         \
		stm32_gpioPinConfig(GPIOD, SHUTTER_DIR_PIN, GPIO_MODE_OUT_PP, GPIO_SPEED_50MHZ);                                             \
		SHUTTER_DIR(SHUTTER_DIR_CLOSE);                                                                                              \
	} while (0)

/** True if motor driver is in fault. */
#define SHUTTER_FAULT() (!stm32_gpioPinRead(GPIOD, SHUTTER_FAULT_PIN))

/** True if the OPEN shutter switch is active. */
#define SHUTTER_OPENED() (!stm32_gpioPinRead(GPIOD, SHUTTER_OPEN_SW_PIN))

/** True if the CLOSE shutter switch is active. */
#define SHUTTER_CLOSED() (!stm32_gpioPinRead(GPIOD, SHUTTER_CLOSE_SW_PIN))

/** Set to true to disable driver */
#define SHUTTER_SLEEP(sleep) \
	stm32_gpioPinWrite(GPIOD, SHUTTER_SLEEP_PIN, !(sleep))

/** Set to SHUTTER_DIR_OPEN in order to open the shutter */
#define SHUTTER_DIR(open) \
	stm32_gpioPinWrite(GPIOD, SHUTTER_DIR_PIN, (open) != SHUTTER_DIR_OPEN)

#define SHUTTER_PWM_CH   1
#define SHUTTER_POLARITY PWM_POL_HIGH_PULSE

#endif //HW_SHUTTER_H
