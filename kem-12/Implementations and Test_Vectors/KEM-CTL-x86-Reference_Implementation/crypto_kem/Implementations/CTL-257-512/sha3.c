/*
 * SHA3 and SHAKE implementation using SM3 and pseudoXOF from auxfunc.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "ng_inner.h"
#include "auxfunc.h"

#if NTRUGEN_ASM_CORTEXM4
#error "Assembly implementation not supported with SM3"
#endif

void
shake_x4_flip(shake_x4_context *scx4, const shake_context *sc)
{
	for (int i = 0; i < 4; i ++) {
		shake_context sct = sc[i];
		sct.dptr = sct.rate - 1;
		sct.A[sct.rate >> 3] ^= (uint64_t)0x1F << ((sct.rate & 7) << 3);
		sct.A[(sct.rate - 1) >> 3] ^= (uint64_t)0x80 << (((sct.rate - 1) & 7) << 3);
		memcpy(scx4->A + (i * 25), sct.A, 25 * sizeof(uint64_t));
	}
	scx4->dptr = scx4->rate = sc[0].rate;
}

void
shake_x4_extract_words(shake_x4_context *scx4, uint64_t *dst, size_t num_x4)
{
	(void)scx4;
	(void)dst;
	(void)num_x4;
}

void
shake_init(shake_context *sc, unsigned size)
{
	sc->rate = 200 - (size_t)(size >> 2);
	sc->dptr = 0;
	memset(sc->A, 0, sizeof sc->A);
}

void
shake_inject(shake_context *sc, const void *in, size_t len)
{
	size_t dptr, rate;
	const uint8_t *buf;

	dptr = sc->dptr;
	rate = sc->rate;
	buf = in;
	while (len > 0) {
		size_t clen, u;

		clen = rate - dptr;
		if (clen > len) {
			clen = len;
		}
		for (u = 0; u < clen; u ++) {
			size_t v;

			v = u + dptr;
			sc->A[v >> 3] ^= (uint64_t)buf[u] << ((v & 7) << 3);
		}
		dptr += clen;
		buf += clen;
		len -= clen;
		if (dptr == rate) {
			dptr = 0;
		}
	}
	sc->dptr = (unsigned)dptr;
}

void
shake_flip(shake_context *sc)
{
	unsigned v;

	v = (unsigned)sc->dptr;
	sc->A[v >> 3] ^= (uint64_t)0x1F << ((v & 7) << 3);
	v = (unsigned)sc->rate - 1;
	sc->A[v >> 3] ^= (uint64_t)0x80 << ((v & 7) << 3);
	sc->dptr = sc->rate;
}

void
shake_extract(shake_context *sc, void *out, size_t len)
{
	size_t dptr, rate;
	uint8_t *buf;

	dptr = sc->dptr;
	rate = sc->rate;
	buf = out;
	while (len > 0) {
		size_t clen;

		if (dptr == rate) {
			dptr = 0;
		}
		clen = rate - dptr;
		if (clen > len) {
			clen = len;
		}
		len -= clen;
		while (clen -- > 0) {
			*buf ++ = (uint8_t)(sc->A[dptr >> 3]
				>> ((dptr & 7) << 3));
			dptr ++;
		}
	}
	sc->dptr = (unsigned)dptr;
}

void
sha3_init(sha3_context *sc, unsigned size)
{
	shake_init(sc, size);
}

void
sha3_update(sha3_context *sc, const void *in, size_t len)
{
	shake_inject(sc, in, len);
}

void
sha3_close(sha3_context *sc, void *out)
{
	unsigned v;
	uint8_t *buf;
	size_t u, len_bytes;

	v = (unsigned)sc->dptr;
	sc->A[v >> 3] ^= (uint64_t)0x06 << ((v & 7) << 3);
	v = (unsigned)sc->rate - 1;
	sc->A[v >> 3] ^= (uint64_t)0x80 << ((v & 7) << 3);

	buf = out;
	len_bytes = (200 - sc->rate) >> 1;
	for (u = 0; u < len_bytes; u ++) {
		buf[u] = (uint8_t)(sc->A[u >> 3] >> ((u & 7) << 3));
	}
}