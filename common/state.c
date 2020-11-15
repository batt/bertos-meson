#include "state.h"

#define LOG_LEVEL LOG_LVL_INFO
#include <cfg/log.h>

BoardState _st_state;
static const char *state_names[] =
    {
        "BOOT",
        "STARTUP",
        "IDLE",
        "ACQUISITION",
        "SERVICE",
};

STATIC_ASSERT(countof(state_names) == ST_CNT);

void st_setState(BoardState new_state)
{
	if (new_state != _st_state)
		LOG_INFO("Changing state to %s\n", state_names[new_state]);
	_st_state = new_state;
}

const char *st_name(void)
{
	return state_names[_st_state];
}
