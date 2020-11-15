/**
 * \defgroup adc_mgr ADC manager
 * \{
 *
 * This module manage the 10-bit system ADC converter.
 * This is *not* the high speed ADC connected to the IR sensors.
 * To this ADC are connected diagnostic and low priority signals like power
 * supply voltage or board temperature.
 *
 * Conversions are performed continuosly at a constant sample rate (8250 Hz).
 * There are eleven channels, all of them converted in a round robin fashion,
 * resulting in a 750Hz sample rate for each channel.
 *
 * ADC samples are low-pass filtered with a digital IIR filter in order
 * to smooth the signal. The filter cutoff frequency is very low: ~10Hz.
 *
 * \file
 * \author Francesco Sacchi <batt@develer.com>
 *
 */
#ifndef ADC_MGR_H
#define ADC_MGR_H

#include <cfg/compiler.h>

/**
 * List of all available ADC channels.
 */
typedef enum AdcChannels
{
	ADC_CH_P3V3 = 0, ///< +3.3V power supply
	ADC_CH_POWER,    ///< Main power supply (24/48V)

	ADC_CHANNELS ///< Maximim number of ADC channels
} AdcChannels;

/**
 * Read ADC channel \a ch.
 * \param ch the channel you want to read.
 * \return 10-bit sample value.
 */
uint16_t adc_mgr_read(AdcChannels ch);

/** Initialize the system ADC manager. */
void adc_mgr_init(void);

/** \} */

#endif
