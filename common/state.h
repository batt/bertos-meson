#ifndef STATE_H
#define STATE_H

#include <cfg/macros.h>
#include <cfg/debug.h>

typedef enum BoardState
{
	ST_BOOT,
	ST_STARTUP,
	ST_IDLE,
	ST_ACQUISITION,
	ST_SERVICE,

	ST_CNT
} BoardState;

INLINE BoardState st_state(void)
{
	extern BoardState _st_state;
	return _st_state;
}

const char *st_name(void);

void st_setState(BoardState new_state);

#endif //STATE_H
