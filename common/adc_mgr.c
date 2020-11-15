#include "adc_mgr.h"
#include "common/protocol.h"

#include <cpu/irq.h>
#include <cpu/power.h>

#include <io/cm3.h>
#include <drv/adc.h>
#include <drv/timer.h>
#include <drv/irq_cm3.h>
#include <drv/gpio_stm32.h>
#include <cfg/compiler.h>
#include <cfg/macros.h>
#include <mware/parser.h>

#define LOG_LEVEL LOG_LVL_INFO
#include <cfg/log.h>

#if (ARCH & ARCH_KK348) || (USE_KK348 == 1)

	#define ADC_PIN_INIT()                                                               \
		do                                                                               \
		{ /* PF5, PF10*/                                                                 \
			RCC->AHB1ENR |= RCC_AHB1ENR_GPIOFEN;                                         \
			stm32_gpioPinConfig(GPIOF, BV(5) | BV(10), GPIO_MODE_AIN, GPIO_SPEED_50MHZ); \
		} while (0)

	#define ADC_DEV        ADC3
	#define DMA_CHANNEL    2 // ADC3
	#define ADC_CLOCK_MASK RCC_APB2ENR_ADC3EN
	#define ADC_3V3_CH     8  // PF10
	#define ADC_POWER_CH   15 // PF5

#elif (ARCH & ARCH_KK353) || (ARCH & ARCH_KK354) || (ARCH & ARCH_DSPLOG)

	#define ADC_PIN_INIT()                                                      \
		do                                                                      \
		{ /* PA6, PC0 */                                                        \
			RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOCEN);        \
			stm32_gpioPinConfig(GPIOA, BV(6), GPIO_MODE_AIN, GPIO_SPEED_50MHZ); \
			stm32_gpioPinConfig(GPIOC, BV(0), GPIO_MODE_AIN, GPIO_SPEED_50MHZ); \
		} while (0)

	#define ADC_DEV        ADC1
	#define DMA_CHANNEL    0 // ADC1
	#define ADC_CLOCK_MASK RCC_APB2ENR_ADC1EN
	#define ADC_3V3_CH     10 // PC0
	#define ADC_POWER_CH   6  // PA6
#else
	#error "ARCH missing!"
#endif

#define DMA_STREAM 0

#define DMA_IRQ     DMA2STREAM0_IRQHANDLER
#define SAMPLE_RATE 10000

// Set ADC_MGR_STROBE to 1 in order to enable debugging of adc isr duration.
#define ADC_MGR_STROBE 0
#if ADC_MGR_STROBE
	#define STROBE_IO             GPIOE
	#define STROBE_PIN            BV(15)
	#define ADC_MGR_STROBE_LOW()  (stm32_gpioPinWrite(STROBE_IO, STROBE_PIN, false))
	#define ADC_MGR_STROBE_HIGH() (stm32_gpioPinWrite(STROBE_IO, STROBE_PIN, true))
	#define ADC_MGR_STROBE_INIT()                                                           \
		do                                                                                  \
		{                                                                                   \
			RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN;                                            \
			stm32_gpioPinConfig(STROBE_IO, STROBE_PIN, GPIO_MODE_OUT_PP, GPIO_SPEED_50MHZ); \
			ADC_MGR_STROBE_HIGH();                                                          \
		} while (0)
#else
	#define ADC_MGR_STROBE_LOW()
	#define ADC_MGR_STROBE_HIGH()
	#define ADC_MGR_STROBE_INIT()
#endif

/*
 * Adc channel table.
 */
static const uint8_t adc_ch[] =
    {
        ADC_3V3_CH,
        ADC_POWER_CH,
};

STATIC_ASSERT(ADC_CHANNELS == countof(adc_ch));

typedef struct Iir
{
	int32_t x[2];
	int32_t y[2];
} Iir;

static uint16_t dma_buffer[ADC_CHANNELS * 2];
static Iir filters[ADC_CHANNELS];

#define FILTER_ENABLE 1
#define IIR_FREQ      1
#if (IIR_FREQ == 75)
	#define IIR_GAIN  126
	#define IIR_CONST 0.5095254495
	#define IIR_SHIFT 9
#elif (IIR_FREQ == 66)
	#define IIR_GAIN  113
	#define IIR_CONST 0.5579652750
	#define IIR_SHIFT 9
#elif (IIR_FREQ == 11)
	#define IIR_GAIN  23
	#define IIR_CONST 0.9118460631
	#define IIR_SHIFT 9
#elif (IIR_FREQ == 8)
	#define IIR_GAIN  1
	#define IIR_CONST 0.9390625058
	#define IIR_SHIFT 5
#elif (IIR_FREQ == 1)
	#define IIR_GAIN  2
	#define IIR_CONST 0.9916015598
	#define IIR_SHIFT 9
#else
	#error "Filter constants for this frequency not defined"
#endif

uint16_t adc_mgr_read(AdcChannels ch)
{
	ASSERT(ch < ADC_CHANNELS);

#if FILTER_ENABLE
	/*
		 * The shift (>> IIR_SHIFT) is needed in order to get the integer part
		 * (filter state uses IIR_SHIFT bits fixed point arithmetic).
		 */
	return filters[ch].y[1] >> IIR_SHIFT;
#else
	#warning "ADC filter disabled"
	return filters[ch].y[1];
#endif
}

static DECLARE_ISR(adc_mgr_isr)
{
	ADC_MGR_STROBE_LOW();

	int base;
	if (DMA2->STR[DMA_STREAM].CR & DMA_CR_TARGET_1)
		base = 0;
	else
		base = ADC_CHANNELS;

	uint16_t *buf = &dma_buffer[base];

	for (unsigned i = 0; i < ADC_CHANNELS; i++)
	{
#if FILTER_ENABLE
		/*
		 * This filter is designed to work at 4140Hz,
		 * update the coefficients if the number of channels
		 * is modified.
		 */
		STATIC_ASSERT(SAMPLE_RATE / ADC_CHANNELS == 5000);

		/*
		 * Read channel and apply a low pass filter, butterworth approximation,
		 * at about IIR_FREQ Hz.
		 * Filter state uses IIR_SHIFT bits fixed point arithmetic in order to
		 * be fast but retain sufficient precision and stability.
		 */
		filters[i].x[0] = filters[i].x[1];
		filters[i].x[1] = buf[i] * IIR_GAIN;
		filters[i].y[0] = filters[i].y[1];
		filters[i].y[1] = filters[i].x[0] + filters[i].x[1] +
		                  INT_MULT(filters[i].y[0], IIR_CONST, 8);
#else
		filters[i].y[1] = buf[i];
#endif
	}
	DMA_CLEAR_TCI(DMA2, DMA_STREAM);
	ADC_MGR_STROBE_HIGH();
}

MAKE_CMD(int_adc_read, "d", "d",
         ({
	         ResultCode r = RC_OK;
	         if (args[1].l < 0 || args[1].l >= ADC_CHANNELS)
	         {
		         protocol_reply(RC_ERROR, "Invalid channel.");
		         r = RC_ERROR;
	         }
	         else
	         {
		         protocol_reply(RC_OK, NULL);
		         protocol_printf("%04d\n", adc_mgr_read(args[1].l));
	         }

	         r;
         }),
         0);

void adc_mgr_init(void)
{
	ADC_MGR_STROBE_INIT();

	RCC->APB2ENR |= ADC_CLOCK_MASK;
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
	ADC_PIN_INIT();

	DMA2->STR[DMA_STREAM].NDTR = ADC_CHANNELS;
	DMA2->STR[DMA_STREAM].M0AR = (uint32_t)dma_buffer;
	DMA2->STR[DMA_STREAM].M1AR = (uint32_t)(dma_buffer + ADC_CHANNELS);

	/* setup DMA */
	DMA2->STR[DMA_STREAM].CR |= DMA_CR_CHAN(DMA_CHANNEL) | DMA_CR_DBUF | DMA_CR_PRI_MED |
	                            DMA_CR_MSIZE_16 | DMA_CR_PSIZE_16 | DMA_CR_MINC |
	                            DMA_CR_DIR_P2M | DMA_CR_TCIE;
	DMA2->STR[DMA_STREAM].PAR = (uint32_t)&ADC_DEV->DR;
	/* clear interrupt enabled flag (just in case) */
	DMA_CLEAR_TCI(DMA2, DMA_STREAM);
	sysirq_setHandler(DMA_IRQ, adc_mgr_isr);

	/* Set ADC conversion clock to 26.5MHz */
	ADC_COMM->CCR &= ~ADC_CCR_PRESCALER_MASK;
	ADC_COMM->CCR |= ADC_CCR_PRESCALER_2;

	ADC_DEV->CR1 = ADC_CR1_RES_12B | ADC_CR1_SCAN;
	ADC_DEV->CR2 = ADC_CR2_EOCS | ADC_CR2_ADON | ADC_CR2_EXTEN_0 | ADC_CR2_EXTSEL_3 | ADC_CR2_DDS;

	/* this bit must be set separately for DMA to work */
	ADC_DEV->CR2 |= ADC_CR2_DMA;

	/* enable DMA */
	DMA2->STR[DMA_STREAM].CR |= DMA_CR_EN;

	/* set channel sequence and sampling time */
	for (size_t i = 0; i < ADC_CHANNELS; ++i)
	{
		stm32_adcSetChannelSequence(ADC_DEV, adc_ch[i], i);
		stm32_adcSetChannelSampleTime(ADC_DEV, adc_ch[i], ADC_SAMPLETIME_28CYCLES);
	}
	stm32_adcSetChannelSequenceLength(ADC_DEV, ADC_CHANNELS);

	/* Program Timer3 CH1 to generate trigger events */
	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
	/* APB1 freq directly used */
	TIM3->PSC = 0;
	/* Count in order to have a frequency of SAMPLE_RATE */
	TIM3->ARR = ((CPU_FREQ / 2) / SAMPLE_RATE);

	/* Select update event as TRGO */
	TIM3->CR2 = TIM_CR2_MMS_1;

	/* Enable timer */
	TIM3->CR1 = TIM_CR1_CEN;

	REGISTER_CMD(int_adc_read);
}
