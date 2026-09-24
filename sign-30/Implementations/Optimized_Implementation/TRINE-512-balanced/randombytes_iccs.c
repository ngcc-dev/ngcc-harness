#include "randombytes.h"

#include <limits.h>
#include <stddef.h>

#include "drng.h"

extern DRNG_ctx drng_algorithm;

int randombytes(
    unsigned char *out,
    unsigned long long outlen)
{
  if (out == NULL && outlen != 0u)
    return RANDOMBYTES_BAD_OUTPUT;

  if (outlen > ULLONG_MAX / 8u)
    return RANDOMBYTES_BAD_LENGTH;

  return get_random_number(&drng_algorithm, out, outlen * 8u) == 0
             ? RANDOMBYTES_SUCCESS
             : RANDOMBYTES_PROVIDER_FAILURE;
}
