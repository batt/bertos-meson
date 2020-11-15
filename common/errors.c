#include "common/system.h"

#include "common/measures.h"
#include <cfg/compiler.h>
#include <cfg/macros.h>

int system_errors(void)
{
#if (ARCH & ARCH_ETH_TEST)
	return 0;
#else
	return meas_error();
#endif
}
