#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	int16_t now, prev;
	now = -32700;
	prev = 32760;

	printf("diff %d\n"(now - prev));
	return 0;
}
