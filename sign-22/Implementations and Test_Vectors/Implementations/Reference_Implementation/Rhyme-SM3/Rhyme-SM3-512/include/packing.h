#ifndef RHYME_PACKING_H
#define RHYME_PACKING_H
#include <stdint.h>
#include <stddef.h>
#include "params.h"
#include "poly.h"

/* secret basis B  (Construction 4 "cut-F", ours = transpose of the
 * HAWK/MNTRU-generated full (d+1)x(d+1) unimodular matrix):
 *   rows 0..D_UNI-1 (= d rows) are SHORT (|coeff| <= ~ETA after CBD; int8),
 *   row D_UNI (the (d+1)-th, big solved NTRU row) holds large coeffs (int16).
 * Every row has D_SOLVE = d+1 column-polys.
 * Column 0 of B is the secret column; its first d entries are
 * s_tail = (sgen | egen).  The (d+1)-th entry of column 0 (row_big[0]) belongs
 * to the discarded extension dimension and never enters the signature. */
typedef struct {
    poly row_small[D_UNI][D_SOLVE];  /* d short rows, each d+1 polys */
    poly row_big[D_SOLVE];           /* the long (d+1)-th row, d+1 polys */
} secret_basis;

#define pack_pk RHYME_NAMESPACE(pack_pk)
#define unpack_pk RHYME_NAMESPACE(unpack_pk)
#define pack_sk RHYME_NAMESPACE(pack_sk)
#define unpack_sk RHYME_NAMESPACE(unpack_sk)
#define pack_w RHYME_NAMESPACE(pack_w)
#define pack_sig RHYME_NAMESPACE(pack_sig)
#define unpack_sig RHYME_NAMESPACE(unpack_sig)

void pack_pk(uint8_t pk[CRYPTO_PUBLICKEYBYTES], const uint8_t seedA[SEEDBYTES],
             const poly b[K]);
void unpack_pk(uint8_t seedA[SEEDBYTES], poly b[K],
               const uint8_t pk[CRYPTO_PUBLICKEYBYTES]);

void pack_sk(uint8_t sk[CRYPTO_SECRETKEYBYTES],
             const uint8_t pk[CRYPTO_PUBLICKEYBYTES],
             const secret_basis *B, const uint8_t key[SEEDBYTES]);
void unpack_sk(uint8_t pk[CRYPTO_PUBLICKEYBYTES], secret_basis *B,
               uint8_t key[SEEDBYTES], const uint8_t sk[CRYPTO_SECRETKEYBYTES]);

/* pack w in [0,2q)^N per poly, POLY2Q_BITS each, K polys -> for hashing */
#define W_PACKEDBYTES (K * POLY_PACKEDBYTES_2Q)
void pack_w(uint8_t buf[W_PACKEDBYTES], const poly w[K]);

/* signature = c-seed bytes? we transmit challenge as hash output c_tilde (CRHBYTES)
 * + encoded z (z1, z_rest[D_REST]).  returns length or -1 on overflow. */
int pack_sig(uint8_t sig[CRYPTO_BYTES], size_t *siglen,
             const uint8_t c_tilde[CTILDEBYTES], const poly *z1, const poly z_rest[D_REST]);
int unpack_sig(uint8_t c_tilde[CTILDEBYTES], poly *z1, poly z_rest[D_REST],
               const uint8_t *sig, size_t siglen);

#endif
