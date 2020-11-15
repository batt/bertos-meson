/**
 * \defgroup measures Measures conversion & checking
 * \{
 *
 * \file
 * \author Francesco Sacchi <batt@develer.com>
 * \brief Measures conversion & checking
 */
#ifndef MEASURES_H
#define MEASURES_H

#include "common/adc_mgr.h"
#include <drv/i2c.h>
#include <kern/sem.h>

typedef enum MeasureFormat
{
	MEAS_VERBOSE,
	MEAS_TERSE,
} MeasureFormat;

int meas_read(AdcChannels ch);
void meas_format(AdcChannels ch, MeasureFormat format);
void meas_init(I2c *i2c, Semaphore *i2c_sem);
int meas_error(void);
bool meas_tempProtectionOn(void);
int meas_currentErrors(void);

/** \} */
#endif
