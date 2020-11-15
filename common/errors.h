#ifndef COMMON_ERRORS
#define COMMON_ERRORS

#include <cfg/macros.h>

typedef enum CommonErrors
{
	ERR_P3V3_LOW = BV(0),
	ERR_P3V3_HIGH = BV(1),
	ERR_POWER_LOW = BV(8),
	ERR_POWER_HIGH = BV(9),

	ERR_BOARD0_HIGH = BV(29),
} CommonErrors;

#define ERR_VOLTAGE_ERR_MASK \
	(ERR_P3V3_LOW |          \
	 ERR_P3V3_HIGH |         \
	 ERR_POWER_LOW |         \
	 ERR_POWER_HIGH)

int system_errors(void);

#endif /* COMMON_ERRORS */
