#include "random.h"
#include "drng.h"
#include <stdint.h>

extern DRNG_ctx drng_algorithm;

int randombytes(unsigned char *buffer, unsigned int size)
{
	if (size == 0U)
	{
		return 0;
	}
	if (buffer == 0 && size != 0U)
	{
		return -1;
	}
	return get_random_number(&drng_algorithm, buffer,
						 (unsigned long long)size * 8ULL);
}
