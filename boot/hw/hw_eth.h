#ifndef HW_ETH_H
#define HW_ETH_H

#include <cfg/compiler.h>

#if 1
	#include <drv/dp83848i.h>

	#define PHY_HW_INIT()       \
		do                      \
		{                       \
			PIOB_OER = BV(26);  \
			PIOB_SODR = BV(26); \
			PIOB_PER = BV(26);  \
		} while (0)
#else
	#include <drv/dm9161a.h>

	#define PHY_HW_INIT()       \
		do                      \
		{                       \
			PIOB_OER = BV(18);  \
			PIOB_CODR = BV(18); \
			PIOB_PER = BV(18);  \
		} while (0)
#endif

#endif // HW_ETH_H
