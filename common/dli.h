/**
 * \defgroup dli Device Life Information
 * \{
 *
 * \file
 * \author Francesco Sacchi <batt@develer.com>
 * \brief Device Life Information.
 *
 */

#ifndef DLI_H
#define DLI_H

#include <io/kblock.h>
#include <io/kfile.h>
#include <kern/sem.h>

/**
 * MAC address of the device.
 *
 * Use this key in dli_get() to retrieve the MAC address.
 */
#define SERIALNO_KEY "serial_no"

/**
 * Maximum lenght for DLI values.
 */
#define DLI_VAL_MAX_LEN 128

/**
 * Initialize the DLI module.
 * This function requires a copy of the same KBlock device in order to work properly.
 * \param blk1 First copy of the same kblock device.
 * \param blk2 Second copy of the same kblock device.
 * \param blk3 Third copy of the same kblock device.
 * \param blk4 Fourth copy of the same kblock device.
 * \param sem Semaphore used to lock access to the i2c bus
 */
void dli_init(KBlock *blk1, KBlock *blk2, KBlock *blk3, KBlock *blk4, Semaphore *sem);

/**
 * Get a key from the DLI.
 *
 * \param key the key you want to read.
 * \param default_val the default value used in case the key is not found or on errors.
 * \param value the buffer which will be filled with the value found in key.
 * \param len lenght of the value buffer.
 * \return true if the key is found, false if the key is not found or on errors.
 **/
bool dli_get(const char *key, const char *default_val, char *value, size_t len);

/**
 * Set the DLI key to value.
 * \param key the key you want to set. If it does not exist, it will be created.
 * \param value the value of the key you want to set.
 * \return true if all is ok, false on errors.
 */
bool dli_set(const char *key, const char *value);

/**
 * Update statistical information for the given key.
 *
 * Helper function to update the operational log for the given key. Read
 * the current value from DLI, add \a delta and store the updated information.
 * If the key is not found, the read value defaults to 0.
 *
 * \param key Value to be read
 * \param delta Amount to be added to the current value.
 * \return true if conversion of the stored value and storing of the updated
 *      value was ok, false otherwise.
 */
bool dli_updateStat(const char *key, int delta);

/**
 * Reset all DLI information.
 * \warning This function will wipe out all DLI configurations!
 */
void dli_reset(void);

/** \} */
#endif
