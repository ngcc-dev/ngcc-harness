#ifndef UTILS_H
#define UTILS_H

#include "params.h"
#include "hashkdf.h"
#include "matrixmod.h"

int XOF(uint8_t **buf, size_t *length, const uint8_t *seed, size_t seed_len, int num);

int rnd_GF(Fq *out, trine_xof_state *xof);

int rnd_sys_mat(Fq *M, int M_r, int M_c, const uint8_t *seed, size_t seed_len);

int rnd_inv_matrix(Fq *M, int M_r, int M_c, uint8_t *seed, size_t seed_len);

int trine_parse_hash(
    const uint8_t *digest,
    size_t digest_len,
    trine_challenge_t *out,
    size_t out_len);

int solve(Fq *A, Fq *B_inv, Fq *G0prime, Fq Amm);

void pi(Fq *Gout, Fq *A, Fq *B, Fq *G);
void phi(Fq *Gout, Fq *A, Fq *B, Fq *C, Fq *G);

#endif
