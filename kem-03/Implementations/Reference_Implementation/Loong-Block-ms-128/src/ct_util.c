#include "ct_util.h"

int loong_ct_equal(const unsigned char *a, const unsigned char *b,
                   unsigned long long len)
{
	unsigned char diff = 0;
	unsigned long long i;

	if (len != 0 && (a == 0 || b == 0)) {
		return 0;
	}

	for (i = 0; i < len; i++) {
		diff = (unsigned char)(diff | (unsigned char)(a[i] ^ b[i]));
	}

	return diff == 0;
}

void loong_ct_select(unsigned char *out, const unsigned char *if_one,
                     const unsigned char *if_zero, unsigned long long len,
                     unsigned char mask)
{
	unsigned long long i;
	unsigned char m = (unsigned char)(0U - (unsigned int)(mask != 0));

	if (len != 0 && (out == 0 || if_one == 0 || if_zero == 0)) {
		return;
	}

	for (i = 0; i < len; i++) {
		out[i] = (unsigned char)((if_one[i] & m) | (if_zero[i] & (unsigned char)~m));
	}
}

void loong_secure_bzero(void *ptr, unsigned long long len)
{
	volatile unsigned char *p = (volatile unsigned char *)ptr;

	if (ptr == 0) {
		return;
	}

	while (len != 0) {
		*p = 0;
		p++;
		len--;
	}
}
