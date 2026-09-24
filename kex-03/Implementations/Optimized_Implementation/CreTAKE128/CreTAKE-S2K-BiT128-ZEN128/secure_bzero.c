#include "secure_bzero.h"

void secure_bzero(void *ptr, size_t len)
{
    volatile unsigned char *p = (volatile unsigned char *)ptr;

    while (len-- > 0) {
        *p++ = 0;
    }
}