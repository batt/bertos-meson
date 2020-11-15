#include "board_id.h"

#include <drv/adc.h>
#include <drv/gpio_stm32.h>

#include <string.h>

#define LOG_LEVEL LOG_LVL_INFO
#include <cfg/log.h>

static int _hw_version;
int hw_version(void)
{
	return _hw_version;
}

typedef struct AdcPinConf
{
	struct stm32_adc *adc;
	int adc_ch;
	stm32_gpio *gpio;
	int gpio_pin;
	uint32_t adc_clock_mask;
	uint32_t gpio_clock_mask;
} AdcPinConf;

#define BOARD_PACKAGE_CLK  RCC_AHB1ENR_GPIODEN
#define BOARD_PACKAGE_GPIO GPIOD
#define BOARD_PACKAGE_PIN  3

static const AdcPinConf board_id_pin_conf[] = {
    {
        // STM32F207Vxxx
        // PD3 == 0
        // board_id PA4 ADCs IN4 (100pin LQFP)
        .adc = ADC2,
        .adc_ch = 4,
        .gpio = GPIOA,
        .gpio_pin = 4,
        .adc_clock_mask = RCC_APB2ENR_ADC2EN,
        .gpio_clock_mask = RCC_AHB1ENR_GPIOAEN,
    },

    {
        // STM32F207Zxxx, STM32F207Ixxx (144pin LQFP or BGA)
        // PD3 == 1
        // board_id PF4 ADC3 IN14
        .adc = ADC3,
        .adc_ch = 14,
        .gpio = GPIOF,
        .gpio_pin = 4,
        .adc_clock_mask = RCC_APB2ENR_ADC3EN,
        .gpio_clock_mask = RCC_AHB1ENR_GPIOFEN,
    },
};

static const AdcPinConf ad3_in9_hw_vers_conf[] = {
    {
        // hw_ver PF3  ADC3 IN9
        .adc = ADC3,
        .adc_ch = 9,
        .gpio = GPIOF,
        .gpio_pin = 3,
        .adc_clock_mask = RCC_APB2ENR_ADC3EN,
        .gpio_clock_mask = RCC_AHB1ENR_GPIOFEN,
    }};

static const AdcPinConf adc2_in5_hw_vers_conf[] = {
    {
        // hw_ver PA5 ADC2_IN5
        .adc = ADC2,
        .adc_ch = 5,
        .gpio = GPIOA,
        .gpio_pin = 5,
        .adc_clock_mask = RCC_APB2ENR_ADC2EN,
        .gpio_clock_mask = RCC_AHB1ENR_GPIOAEN,
    }};

static const char cpu_names[][24] = {
    "Unknown",
    "STM32F207ZG",
    "STM32F207VG",
    "STM32F207IG",
};
STATIC_ASSERT(countof(cpu_names) == CI_CNT);

static const BoardInfo board_info[] = {
    {
        .id = BOARD_ID_KK348,
        .name = "KK348",
        .cpu_id = CI_STM32F207ZG,
        .hw_pin = ad3_in9_hw_vers_conf,
    },
    {
        .id = BOARD_ID_KK353,
        .name = "KK353",
        .cpu_id = CI_STM32F207VG,
        .hw_pin = adc2_in5_hw_vers_conf,
    },
    {
        .id = BOARD_ID_KK354,
        .name = "KK354",
        .cpu_id = CI_STM32F207VG,
        .hw_pin = adc2_in5_hw_vers_conf,
    },
    {
        .id = BOARD_ID_STM3220G,
        .name = "STM3220G-EVAL",
        .cpu_id = CI_STM32F207IG,
        .hw_pin = adc2_in5_hw_vers_conf,
    },
};
STATIC_ASSERT(BOARD_CNT == countof(board_info));

static int _board_id;
int board_id(void)
{
	return _board_id;
}

static const BoardInfo *find_board(int id)
{
	for (unsigned i = 0; i < BOARD_CNT; i++)
	{
		if (board_info[i].id == id)
			return &board_info[i];
	}

	return NULL;
}

const BoardInfo *board_data(int id)
{
	return find_board(id);
}

const char *board_name(void)
{
	const BoardInfo *info = find_board(_board_id);
	if (info)
		return info->name;
	else
		return "Unknown";
}

CpuId board_cpu_id(void)
{
	const BoardInfo *info = find_board(_board_id);
	if (info)
		return info->cpu_id;
	else
		return CI_UNKNOWN;
}

const char *board_cpu(void)
{
	const BoardInfo *info = find_board(_board_id);
	if (info)
		return cpu_names[info->cpu_id];
	else
		return "Unknown";
}

static int read_adc_val(const AdcPinConf *cfg)
{
	RCC->APB2ENR |= cfg->adc_clock_mask;
	RCC->AHB1ENR |= cfg->gpio_clock_mask;

	stm32_gpioPinConfig(cfg->gpio, BV(cfg->gpio_pin), GPIO_MODE_AIN, GPIO_SPEED_50MHZ);

	cfg->adc->CR2 = 0;
	/* Read adc value */
	cfg->adc->CR2 = ADC_CR2_ADON;
	stm32_adcSetChannelSequence(cfg->adc, cfg->adc_ch, 0);
	stm32_adcSetChannelSampleTime(cfg->adc, cfg->adc_ch, ADC_SAMPLETIME_28CYCLES);
	stm32_adcSetChannelSequenceLength(cfg->adc, 1);

	cfg->adc->CR2 |= CR2_EXTTRIG_SWSTRT_SET;

	while (!(cfg->adc->SR & BV(SR_EOC)))
		;

	int val = cfg->adc->DR >> 7;
	cfg->adc->CR2 = 0;
	return val;
}

int board_id_init(void)
{
	RCC->AHB1ENR |= BOARD_PACKAGE_CLK;
	// Read package pin
	stm32_gpioPinConfig(BOARD_PACKAGE_GPIO, BV(BOARD_PACKAGE_PIN), GPIO_MODE_IPU, GPIO_SPEED_50MHZ);
	int cpu_id = stm32_gpioPinRead(BOARD_PACKAGE_GPIO, BV(BOARD_PACKAGE_PIN)) ? 1 : 0;
	const AdcPinConf *board_id_pin = &board_id_pin_conf[cpu_id];
	_board_id = read_adc_val(board_id_pin) | (cpu_id ? 0 : BOARD_ID_100PIN_CPUMASK);

	LOG_INFO("Board ID: %04X - %s\n", _board_id, board_name());
	LOG_INFO("CPU: %s\n", board_cpu());

	const BoardInfo *info;
	if ((info = find_board(_board_id)))
	{
		_hw_version = read_adc_val(info->hw_pin);
		LOG_INFO("HW version: %d\n", _hw_version);
	}
	else
	{
		_hw_version = -1;
		LOG_WARN("Unknown HW version for this board!\n");
	}

	return _board_id;
}
