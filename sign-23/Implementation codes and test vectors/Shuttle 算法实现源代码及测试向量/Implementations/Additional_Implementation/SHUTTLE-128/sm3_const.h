/*
The software is provided by the Institute of Commercial Cryptography
Standards (ICCS), and is used for algorithm submissions in the
Next-generation Commercial Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will
be uninterrupted or error-free in all cases. ICCS will take no
responsibility for the use of the software or the results thereof, if the
software is used for any other purposes.
*/

/*
 * SM3 constants shared by the AVX2 (8-way) and AVX512 (16-way)
 * implementations.
 *
 * These are *not* magic numbers: every value here is reproducible and
 * audited by the generator script gen_sm3_const.py (run `python3
 * gen_sm3_const.py --check` to confirm this header matches the
 * spec-derived values).
 *
 *   - SM3_IV  : the 8 initial digest words (GB/T 32905-2016).
 *   - SM3_T[] : the 64 per-round constants Tj[i] = ROTL32(Tbase, i & 31),
 * where Tbase = 0x79CC4519 for rounds 0..15 and 0x7A879D8A for 16..63. The
 * reference (SHUTTLE/ref/auxfunc.c) recomputes these on the fly via
 * L_SHIFT(Tbase, i & 0x1F); we precompute them so the vector round can
 * simply broadcast SM3_T[i].
 */

#ifndef SM3_CONST_H
#define SM3_CONST_H

#include <stdint.h>

/* GB/T 32905-2016 SM3 initial value. */
static const uint32_t SM3_IV[8] = {0x7380166F, 0x4914B2B9, 0x172442D7,
                                   0xDA8A0600, 0xA96F30BC, 0x163138AA,
                                   0xE38DEE4D, 0xB0FB0E4E};

/* Per-round constants Tj = ROTL32(Tbase, i & 31).  See gen_sm3_const.py.
 */
static const uint32_t SM3_T[64] = {
    0x79CC4519, 0xF3988A32, 0xE7311465, 0xCE6228CB, 0x9CC45197, 0x3988A32F,
    0x7311465E, 0xE6228CBC, 0xCC451979, 0x988A32F3, 0x311465E7, 0x6228CBCE,
    0xC451979C, 0x88A32F39, 0x11465E73, 0x228CBCE6, 0x9D8A7A87, 0x3B14F50F,
    0x7629EA1E, 0xEC53D43C, 0xD8A7A879, 0xB14F50F3, 0x629EA1E7, 0xC53D43CE,
    0x8A7A879D, 0x14F50F3B, 0x29EA1E76, 0x53D43CEC, 0xA7A879D8, 0x4F50F3B1,
    0x9EA1E762, 0x3D43CEC5, 0x7A879D8A, 0xF50F3B14, 0xEA1E7629, 0xD43CEC53,
    0xA879D8A7, 0x50F3B14F, 0xA1E7629E, 0x43CEC53D, 0x879D8A7A, 0x0F3B14F5,
    0x1E7629EA, 0x3CEC53D4, 0x79D8A7A8, 0xF3B14F50, 0xE7629EA1, 0xCEC53D43,
    0x9D8A7A87, 0x3B14F50F, 0x7629EA1E, 0xEC53D43C, 0xD8A7A879, 0xB14F50F3,
    0x629EA1E7, 0xC53D43CE, 0x8A7A879D, 0x14F50F3B, 0x29EA1E76, 0x53D43CEC,
    0xA7A879D8, 0x4F50F3B1, 0x9EA1E762, 0x3D43CEC5};

#endif /* SM3_CONST_H */
