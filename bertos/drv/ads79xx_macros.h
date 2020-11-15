#ifndef ADS79XX_MACROS_H
#define ADS79XX_MACROS_H

#define ADS79xx_DATA_MASK           (0x0FFF)
#define ADS79xx_CH_MASK             (0xF000)
#define ADS79xx_DATA_BITS           (12)
#define ADS79xx_GET_DATA(val, bits) ((val & 0x0FFF) >> (12 - (bits)))
#define ADS79xx_GET_CHAN(val)       ((val & 0xF000) >> 12)

#endif /* ADS79XX_MACROS_H */
