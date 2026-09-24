#ifndef SYMMETRIC_H
#define SYMMETRIC_H

#include "auxfunc.h"
#include "params.h"
#include "string.h"
#include <stddef.h>
#include <stdint.h>

#define ASCON_XOF_BLOCKBYTES 8

#define hash_h(OUT, IN, INBYTES)                                              \
	pseudoXOF (KEM_SSBYTES * 8, IN, INBYTES * 8, OUT)
#define hash_g(OUT, IN, INBYTES)                                              \
	pseudoXOF (2 * KEM_SSBYTES * 8, IN, INBYTES * 8, OUT)
#define kdf(OUT, IN, INBYTES) pseudoXOF (KEM_SSBYTES * 8, OUT, INBYTES * 8, IN)

#endif /* SYMMETRIC_H */
