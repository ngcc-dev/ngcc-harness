// SPDX-License-Identifier: MIT
// Origami public API and parameter-derived constants.

#ifndef ORIGAMI_H
#define ORIGAMI_H

#include <stddef.h>
#include <stdint.h>

#include "origami_params.h"

#ifndef ORIGAMI_OPT
#define ORIGAMI_OPT REF
#endif

#define ORIGAMI_JOIN_(prefix, q, d, f) _origami_##prefix##_##q##_d##d##_##f
#define ORIGAMI_JOIN(prefix, q, d, f) ORIGAMI_JOIN_(prefix, q, d, f)
#define ORIGAMI_NAMESPACE(f) ORIGAMI_JOIN(ORIGAMI_OPT, ORIGAMI_q, ORIGAMI_MAIN_ZONES, f)

#define ORIGAMI_TOTAL_ZONES (ORIGAMI_MAIN_ZONES + 1)

#define SEED_LENGTH_PRIVATE ORIGAMI_SEED_SK_BYTES
#define SEED_LENGTH_PUBLIC  ORIGAMI_SEED_PK_BYTES
#define SEED_LENGTH         SEED_LENGTH_PRIVATE
#define BYTES_SK            SEED_LENGTH_PRIVATE
#define BYTES_SALT          ORIGAMI_SALT_BYTES
#define BYTES_DIGEST        64

#if ORIGAMI_q == 16
#define PACK_GF 2
#define PACK_BYTES 1
#else
#error "Origami engineering profile currently implements q = 16"
#endif

#define BYTES_GF(x) ((PACK_BYTES * (x) + PACK_GF - 1) / PACK_GF)

#define ORIGAMI_MAIN_L2 (ORIGAMI_MAIN_L * ORIGAMI_MAIN_L)
#define ORIGAMI_TAIL_L2 (ORIGAMI_TAIL_L * ORIGAMI_TAIL_L)

#define ORIGAMI_MAIN_FLAT_V (ORIGAMI_MAIN_K * ORIGAMI_MAIN_V * ORIGAMI_MAIN_L2)
#define ORIGAMI_MAIN_FLAT_O (ORIGAMI_MAIN_K * ORIGAMI_MAIN_O * ORIGAMI_MAIN_L2)
#define ORIGAMI_MAIN_FLAT_N (ORIGAMI_MAIN_FLAT_V + ORIGAMI_MAIN_FLAT_O)
#define ORIGAMI_MAIN_FLAT_M (ORIGAMI_MAIN_M * ORIGAMI_MAIN_L2)

#define ORIGAMI_TAIL_FLAT_V (ORIGAMI_TAIL_K * ORIGAMI_TAIL_V * ORIGAMI_TAIL_L2)
#define ORIGAMI_TAIL_FLAT_O (ORIGAMI_TAIL_K * ORIGAMI_TAIL_O * ORIGAMI_TAIL_L2)
#define ORIGAMI_TAIL_FLAT_N (ORIGAMI_TAIL_FLAT_V + ORIGAMI_TAIL_FLAT_O)
#define ORIGAMI_TAIL_FLAT_M (ORIGAMI_TAIL_M * ORIGAMI_TAIL_L2)

#define ORIGAMI_N (ORIGAMI_MAIN_ZONES * ORIGAMI_MAIN_FLAT_N + ORIGAMI_TAIL_FLAT_N)
#define ORIGAMI_M (ORIGAMI_MAIN_ZONES * ORIGAMI_MAIN_FLAT_M + ORIGAMI_TAIL_FLAT_M)

#define ORIGAMI_MAX_L 2
#define ORIGAMI_MAX_L2 (ORIGAMI_MAX_L * ORIGAMI_MAX_L)
#define ORIGAMI_MAX_K ((ORIGAMI_MAIN_K > ORIGAMI_TAIL_K) ? ORIGAMI_MAIN_K : ORIGAMI_TAIL_K)
#define ORIGAMI_MAX_FLAT_O ORIGAMI_MAIN_FLAT_O
#define ORIGAMI_MAX_FLAT_M ORIGAMI_MAIN_FLAT_M
#define ORIGAMI_MAX_KNOWN ORIGAMI_N

#define NUMGF_PK ((long)ORIGAMI_M * (long)ORIGAMI_N * ((long)ORIGAMI_N + 1) / 2)

/*
 * The submitted compact public key is pk = (param_id, seed_pk, Rpk).
 * Rpk stores explicit GF(16) residual coefficients, so its field-element
 * count is exactly twice its byte count.
 */
#define NUMGF_RPK ((long)ORIGAMI_RPK_BYTES * 2L)
#define NUMGF_PSEED (NUMGF_PK - NUMGF_RPK)
#define BYTES_PK (4 + SEED_LENGTH_PUBLIC + ORIGAMI_RPK_BYTES)

#define NUMGF_SIG ORIGAMI_N
#define BYTES_SIGNATURE (BYTES_GF(NUMGF_SIG) + BYTES_SALT)

#define ORIGAMI_GF_HASH ORIGAMI_M
#define BYTES_HASH BYTES_GF(ORIGAMI_GF_HASH)

typedef struct {
    int is_tail;
    int k;
    int v;
    int o;
    int m;
    int l;
    int l2;
    int a;
    int b;
    int delta;
    int flat_v;
    int flat_o;
    int flat_n;
    int flat_m;
    int n_offset;
    int m_offset;
} zone_info_t;

typedef struct {
    uint8_t sk_seed[SEED_LENGTH_PRIVATE];
    uint8_t coeff_seed[SEED_LENGTH_PRIVATE];
    uint8_t pk_seed[SEED_LENGTH_PUBLIC];
    uint16_t secret_to_public[ORIGAMI_N];
    uint16_t public_to_secret[ORIGAMI_N];
    uint16_t rho[ORIGAMI_TOTAL_ZONES][ORIGAMI_MAX_FLAT_O];
    uint16_t rho_inv[ORIGAMI_TOTAL_ZONES][ORIGAMI_MAX_FLAT_O];
    uint8_t *R_coeffs;
    uint8_t T_powers[ORIGAMI_TOTAL_ZONES][ORIGAMI_MAX_K][ORIGAMI_MAX_L][ORIGAMI_MAX_L2];
    uint8_t C[ORIGAMI_TOTAL_ZONES][ORIGAMI_MAX_K][ORIGAMI_MAX_L2];
    uint8_t C_inv[ORIGAMI_TOTAL_ZONES][ORIGAMI_MAX_K][ORIGAMI_MAX_L2];
} ph_expanded_SK;

typedef struct {
    uint32_t param_id;
    uint8_t pk_seed[SEED_LENGTH_PUBLIC];
    uint16_t secret_to_public[ORIGAMI_N];
    uint16_t public_to_secret[ORIGAMI_N];
    uint8_t *R_coeffs;
} ph_expanded_PK;

#ifdef ORIGAMI_ENABLE_STATS
typedef struct {
    uint64_t signatures;
    uint64_t zone_attempts[ORIGAMI_TOTAL_ZONES];
    uint64_t zone_retries[ORIGAMI_TOTAL_ZONES];
    uint64_t zone_rank_failures[ORIGAMI_TOTAL_ZONES];
    uint64_t zone_failures[ORIGAMI_TOTAL_ZONES];
    uint32_t zone_max_attempts[ORIGAMI_TOTAL_ZONES];
} origami_stats_t;
#endif

int ORIGAMI_NAMESPACE(genkeys)(uint8_t *pk, uint8_t *sk, const uint8_t *seed);
int ORIGAMI_NAMESPACE(sk_expand)(ph_expanded_SK *skx, const uint8_t *sk);
int ORIGAMI_NAMESPACE(sign)(const ph_expanded_SK *skx, uint8_t *sig,
                            const uint8_t *digest, size_t len_digest,
                            const uint8_t *salt);
int ORIGAMI_NAMESPACE(pk_expand)(ph_expanded_PK *pkx, const uint8_t *pk);
int ORIGAMI_NAMESPACE(verify)(const ph_expanded_PK *pkx, const uint8_t *sig,
                              const uint8_t *digest, size_t len_digest);

void ORIGAMI_NAMESPACE(sk_free)(ph_expanded_SK *skx);
void ORIGAMI_NAMESPACE(pk_free)(ph_expanded_PK *pkx);

#ifdef ORIGAMI_ENABLE_STATS
void ORIGAMI_NAMESPACE(stats_reset)(void);
void ORIGAMI_NAMESPACE(stats_snapshot)(origami_stats_t *out);
#endif

static inline void origami_init_zones(zone_info_t zones[ORIGAMI_TOTAL_ZONES]) {
    int n_offset = 0;
    int m_offset = 0;

    for (int j = 0; j < ORIGAMI_MAIN_ZONES; j++) {
        zones[j].is_tail = 0;
        zones[j].k = ORIGAMI_MAIN_K;
        zones[j].v = ORIGAMI_MAIN_V;
        zones[j].o = ORIGAMI_MAIN_O;
        zones[j].m = ORIGAMI_MAIN_M;
        zones[j].l = ORIGAMI_MAIN_L;
        zones[j].l2 = ORIGAMI_MAIN_L2;
        zones[j].a = ORIGAMI_MAIN_A;
        zones[j].b = ORIGAMI_MAIN_B;
        zones[j].delta = ORIGAMI_MAIN_DELTA;
        zones[j].flat_v = ORIGAMI_MAIN_FLAT_V;
        zones[j].flat_o = ORIGAMI_MAIN_FLAT_O;
        zones[j].flat_n = ORIGAMI_MAIN_FLAT_N;
        zones[j].flat_m = ORIGAMI_MAIN_FLAT_M;
        zones[j].n_offset = n_offset;
        zones[j].m_offset = m_offset;
        n_offset += zones[j].flat_n;
        m_offset += zones[j].flat_m;
    }

    zones[ORIGAMI_MAIN_ZONES].is_tail = 1;
    zones[ORIGAMI_MAIN_ZONES].k = ORIGAMI_TAIL_K;
    zones[ORIGAMI_MAIN_ZONES].v = ORIGAMI_TAIL_V;
    zones[ORIGAMI_MAIN_ZONES].o = ORIGAMI_TAIL_O;
    zones[ORIGAMI_MAIN_ZONES].m = ORIGAMI_TAIL_M;
    zones[ORIGAMI_MAIN_ZONES].l = ORIGAMI_TAIL_L;
    zones[ORIGAMI_MAIN_ZONES].l2 = ORIGAMI_TAIL_L2;
    zones[ORIGAMI_MAIN_ZONES].a = ORIGAMI_TAIL_A;
    zones[ORIGAMI_MAIN_ZONES].b = ORIGAMI_TAIL_B;
    zones[ORIGAMI_MAIN_ZONES].delta = ORIGAMI_TAIL_DELTA;
    zones[ORIGAMI_MAIN_ZONES].flat_v = ORIGAMI_TAIL_FLAT_V;
    zones[ORIGAMI_MAIN_ZONES].flat_o = ORIGAMI_TAIL_FLAT_O;
    zones[ORIGAMI_MAIN_ZONES].flat_n = ORIGAMI_TAIL_FLAT_N;
    zones[ORIGAMI_MAIN_ZONES].flat_m = ORIGAMI_TAIL_FLAT_M;
    zones[ORIGAMI_MAIN_ZONES].n_offset = n_offset;
    zones[ORIGAMI_MAIN_ZONES].m_offset = m_offset;
}

#endif
