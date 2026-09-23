#include "loong_api_random.h"
#include "loong_status.h"
#include "drng.h"

#include <limits.h>

extern DRNG_ctx drng_algorithm;

int loong_api_random_bits(unsigned char *out, unsigned long long out_len_bits)
{
	int rc;

	if (out_len_bits == 0) {
		return LOONG_SUCCESS;
	}
	if (out == 0) {
		return LOONG_ERR_NULL;
	}

	rc = get_random_number(&drng_algorithm, out, out_len_bits);
	return rc == 0 ? LOONG_SUCCESS : LOONG_ERR_API_PKC;
}

int loong_api_random_bytes(unsigned char *out, unsigned long long out_len_bytes)
{
	if (out_len_bytes > ULLONG_MAX / 8ULL) {
		return LOONG_ERR_OVERFLOW;
	}

	return loong_api_random_bits(out, out_len_bytes * 8ULL);
}
