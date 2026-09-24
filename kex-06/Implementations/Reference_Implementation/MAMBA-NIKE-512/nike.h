#ifndef NIKE_H
#define NIKE_H

#include "poly.h"
#include "randombytes.h"
#include "crypto_stream_chacha20.h"
#include "error_correction.h"
#include <math.h>
#include <stdio.h>

int nike_keygen(unsigned char *send, poly *sk);
int nike_sharedb(unsigned char *sharedkey, unsigned char *send, const unsigned char *received);
int nike_shareda(unsigned char *sharedkey, const poly *ska, const unsigned char *pk, const unsigned char *received);

#endif
