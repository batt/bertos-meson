#include "status_led.h"
#include "measures.h"
#include "errors.h"
#include "common/state.h"
#include "hw/hw_led.h"
#include <kern/rtask.h>

static bool status_led_process(UNUSED_ARG(void *, userdata))
{
	static bool on = false;
	static int cnt = 1;

	if (!--cnt)
	{
		if (meas_currentErrors() != 0)
			cnt = 1;
		else
			cnt = 3;
		on = !on;
		LED_ON(on);
	}
	return true;
}

void status_led_init(void)
{
	LED_INIT();

	rtask_add(status_led_process, 167, NULL);
}
