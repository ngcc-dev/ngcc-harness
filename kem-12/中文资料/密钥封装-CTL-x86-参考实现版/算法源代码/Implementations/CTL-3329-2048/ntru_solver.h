#ifndef NTRU_SOLVER_H
#define NTRU_SOLVER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NTRU_Q       3329
#define NTRU_LOGN    11u
#define NTRU_N       ((size_t)1u << NTRU_LOGN)

typedef struct {
    unsigned babai_max_rounds_per_level;
    unsigned babai_stall_limit_per_level;
    unsigned babai_max_rounds_final;
    unsigned babai_stall_limit_final;
    unsigned target_margin_bits;
} SolveParams;

typedef struct {
    unsigned recursive_levels;
    unsigned babai_rounds_total;
    unsigned babai_stall_breaks;
    unsigned babai_zero_k_breaks;
    unsigned final_cleanup_rounds;
} SolveStats;

#define SOLVE_PARAMS_DEFAULT  {128u, 32u, 256u, 64u, 8u}
#define SOLVE_STATS_ZERO      {0u, 0u, 0u, 0u, 0u}

bool solve_ntru_3329_2048_clean(
    const int8_t *f,
    const int8_t *g,
    int32_t *F_out,
    int32_t *G_out,
    SolveStats *stats,
    const SolveParams *params);

bool compute_w_3329_2048_clean(
    const int8_t *f,
    const int8_t *g,
    const int32_t *F,
    const int32_t *G,
    int32_t *w_out);

bool verify_ntru_i32(
    const int8_t *f,
    const int8_t *g,
    const int32_t *F,
    const int32_t *G,
    unsigned logn,
    int32_t q);

#ifdef __cplusplus
}
#endif

#endif
