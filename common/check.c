#include "check.h"
#include "measures.h"

int check_min(const CheckEntry *e)
{
	int val = meas_read((int)e->arg);
	if (val < *e->limit)
		return e->error;
	else
		return 0;
}

int check_max(const CheckEntry *e)
{
	int val = meas_read((int)e->arg);
	if (val > *e->limit)
		return e->error;
	else
		return 0;
}
