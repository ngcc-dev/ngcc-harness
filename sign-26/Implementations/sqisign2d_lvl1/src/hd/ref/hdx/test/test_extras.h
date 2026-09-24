
#ifndef TEST_EXTRAS_H
#define TEST_EXTRAS_H

#include <time.h>
#include <stdlib.h>
#include <fp.h>
#include <fp2.h>
#include <curve_extras.h>

#define PASSED 0
#define FAILED 1

// Comparing "nword" elements, a=b? : (1) a!=b, (0) a=b
int compare_words(digit_t *a, digit_t *b, unsigned int nwords);

// Multiprecision subtraction for testing, assumes a > b
void sub_test(digit_t *out, digit_t *a, digit_t *b, unsigned int nwords);

#endif
