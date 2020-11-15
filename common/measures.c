#include "common/adc_mgr.h"
#include "measures.h"
#include "common/config.h"
#include "common/dli.h" /* dli_get, dli_set */
#include "common/protocol.h"
#include "errors.h"
#include "common/check.h"

#include <cfg/macros.h>

#include <kern/sem.h>
#include <kern/rtask.h>
#include <algo/table.h>
#include <mware/parser.h>
#include <algo/table.h>
#include <drv/lm75.h>
#include <drv/timer.h>

#define LOG_LEVEL LOG_LVL_WARN
#include <cfg/log.h>

#include <stdio.h>  /* snprintf */
#include <string.h> /* strlen */
#include <math.h>

typedef int (*read_func_t)(int ch);

typedef struct ConversionEntry
{
	const char desc[16];
	const char unit[8];
	unsigned print_precision;
	read_func_t read;
	const Table *table;
	size_t table_size;
} ConversionEntry;

static I2c *i2c;
static Semaphore *i2c_sem;
static int errors;
static int current_errors;

/* ADC reference voltage */
#define VREF_ADC 2.500

/*
 * Since this module uses only integer arithmetics, all measured values
 * are multiplied by this constant in order to retain sufficient precision.
 */
#define PRECISION 1000

/* Ratio of a voltage divider.*/
#define DIV_RATIO(r1, r2) ((float)(r1) / ((r1) + (r2)))

/* Convert raw adc values to voltage values. */
#define VOLT(adc_val) ((float)(adc_val)*VREF_ADC / 4095)

/*
 * Returns the voltage applied in input to a voltage divider given an ADC value,
 * taking into account the required precision.
 * The divider is connected with R1 in series with the signal
 * and R2 to GND.
 */
#define DIV_GND(adc_val, r1, r2) \
	((int)(PRECISION * VOLT(adc_val) / DIV_RATIO(r2, r1) + 0.5))

/*
 * Same as above but R2 is connected to VREF.
 * Used in order to measure negative voltages.
 */
#define DIV_VREF(adc_val, r1, r2) \
	((int)(PRECISION * (VOLT(adc_val) - (1 - DIV_RATIO(r2, r1)) * VREF_ADC) / DIV_RATIO(r2, r1) + 0.5))

#define ADC_READ (read_func_t) adc_mgr_read

static int board_tempRead(int ch)
{
	ch -= ADC_CHANNELS;
	ASSERT(ch >= 0);
	ASSERT(ch < 2);
	sem_obtain(i2c_sem);
	int ret = lm75_read(i2c, ch) * PRECISION / 10;
	sem_release(i2c_sem);

	return ret;
}

static const Table table_3v3[] =
    {
        {0, 0},
        {4095, DIV_GND(4095, 22000, 33000)},
};

#if ((ARCH & ARCH_KK354) && !defined(USE_KK353))
static const Table table_48v[] =
    {
        {0, 0},
        {4095, DIV_GND(4095, 56000, 2200)},
};
#else
static const Table table_24v[] =
    {
        {0, 0},
        {4095, DIV_GND(4095, 56000, 4700)},
};
#endif

static const ConversionEntry conv_table[] =
    {
        {"+3.3V supply", "V", 2, ADC_READ, table_3v3, countof(table_3v3)},
#if ((ARCH & ARCH_KK354) && !defined(USE_KK353))
        {"+48V supply", "V", 1, ADC_READ, table_48v, countof(table_48v)},
#else
        {"+24V supply", "V", 1, ADC_READ, table_24v, countof(table_24v)},
#endif
        {"Board temp", "°C", 1, board_tempRead, NULL, 0},
};

DECLARE_CHECK(measures_cfg, check_table,
              CHECK(33_min, ERR_P3V3_LOW, 1650, 3300, 3135, check_min, 0),
              CHECK(33_max, ERR_P3V3_HIGH, 3300, 4950, 3465, check_max, 0),
#if ((ARCH & ARCH_KK354) && !defined(USE_KK353))
              CHECK(480_min, ERR_POWER_LOW, 24000, 48000, 45600, check_min, 1),
              CHECK(480_max, ERR_POWER_HIGH, 48000, 72000, 50400, check_max, 1),
#else
              CHECK(240_min, ERR_POWER_LOW, 12000, 24000, 22800, check_min, 1),
              CHECK(240_max, ERR_POWER_HIGH, 24000, 36000, 25200, check_max, 1),
#endif
              CHECK_END(board0_temp, ERR_BOARD0_HIGH, 0, 120000, 90000, check_max, 2));

int meas_read(AdcChannels ch)
{
	ASSERT(ch < countof(conv_table));
	read_func_t read = conv_table[ch].read;
	ASSERT(read);
	int adc_val = read(ch);
	const Table *t = conv_table[ch].table;

	if (t)
	{
		int ret = table_linearInterpolation(t, conv_table[ch].table_size, adc_val);
		LOG_INFO("Read ch%d, adc_val %d, interpolation %d\n", ch, adc_val, ret);
		return ret;
	}
	else
		return adc_val;
}

void meas_format(AdcChannels ch, MeasureFormat format)
{
	int val = meas_read(ch);

	if (format == MEAS_VERBOSE)
		protocol_printf("%s: %.*f%s\n",
		                conv_table[ch].desc, conv_table[ch].print_precision,
		                (float)val / PRECISION, conv_table[ch].unit);
	else if (format == MEAS_TERSE)
		protocol_printf("%.*f\n",
		                conv_table[ch].print_precision, (float)val / PRECISION);
	else
		ASSERT(0);
}

MAKE_CMD(meas_read, "d", "d",
         ({
	         ResultCode r = RC_OK;
	         if (args[1].l < 0 || args[1].l >= (long)countof(conv_table))
	         {
		         protocol_reply(RC_ERROR, "Index out of range.");
		         r = RC_ERROR;
	         }
	         else
	         {
		         protocol_printf("%d ", RC_OK);
		         meas_format(args[1].l, MEAS_VERBOSE);
		         protocol_printf("%d\n", meas_read(args[1].l));
	         }
	         r;
         }),
         0);

MAKE_CMD(meas_read_all, "", "s",
         ({
	         (void)args;
	         protocol_reply(RC_OK, NULL);
	         for (unsigned i = 0; i < countof(conv_table); i++)
		         meas_format(i, MEAS_VERBOSE);
	         RC_OK;
         }),
         0);

typedef uint32_t (*stat_get_t)(void);
struct StatsSave
{
	const char *name;
	stat_get_t get_delta;
};

/*
 * Measurement time interval [ms]
 * NOTE: do not reduce this inteval below 200ms, otherwise
 * temperatures will be locked. This is because the temperature sensor
 * cannot handle fast conversion times.
 */
#define MEASURE_INTERVAL 200

static bool check_range(void *arg)
{
	(void)arg;

	current_errors = 0;

	for (unsigned i = 0; i < countof(check_table); ++i)
		current_errors |= CHECK_DO(check_table[i]);

	errors |= current_errors;

	return true;
}

MAKE_CMD(get_frequencies, "", "d",
         ({
	         (void)args;
	         protocol_reply(RC_OK, NULL);
	         protocol_printf("%d\n", CPU_FREQ);
	         RC_OK;
         }),
         0);

void meas_init(I2c *_i2c, Semaphore *_i2c_sem)
{
	ASSERT(_i2c);
	ASSERT(_i2c_sem);
	i2c = _i2c;
	i2c_sem = _i2c_sem;
	REGISTER_CMD(meas_read);
	REGISTER_CMD(meas_read_all);
	REGISTER_CMD(get_frequencies);

	config_register(&measures_cfg);
	config_load(&measures_cfg);

	rtask_add(check_range, MEASURE_INTERVAL, 0);
}

/**
 * Return and clear the current error mask
 */
int meas_error(void)
{
	int e = errors;
	errors = 0;
	return e;
}

int meas_currentErrors(void)
{
	return current_errors;
}
