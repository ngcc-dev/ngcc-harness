#ifndef SECURE_BZERO_H
#define SECURE_BZERO_H

#include <stddef.h>

void secure_bzero(void *ptr, size_t len);

#endif