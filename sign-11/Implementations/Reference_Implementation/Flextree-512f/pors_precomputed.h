#ifndef PORS_PRECOMPUTED_H
#define PORS_PRECOMPUTED_H

#define str(s) #s
#define xstr(s) str(s)

#include "params.h"
#include "params/pors_precomputed-flextree-512f.h"

static inline int pors_precomp_binom_table_get(pors_precomp_binom_table *table,
                                               uint32_t r)
{
    if (r < 2u || r > SPX_PORS_FP_K) {
        table->offsets = 0;
        table->data = 0;
        return -1;
    }

    *table = pors_binom_tables[r];
    return table->offsets == 0 ? -1 : 0;
}

static inline uint32_t pors_u256_significant_limbs(const pors_u256 *x)
{
    uint32_t i = PORS_U256_LIMBS;

    while (i > 0u && x->limb[i - 1u] == 0u) {
        i--;
    }

    return i;
}

static inline int pors_precomp_u256_cmp_limbs_sig(const uint64_t *data,
                                                  uint32_t len,
                                                  const pors_u256 *target,
                                                  uint32_t target_len)
{
    if (len < target_len) {
        return -1;
    }
    if (len > target_len) {
        return 1;
    }

    while (len > 0u) {
        const uint64_t ai = data[len - 1u];
        const uint64_t bi = target->limb[len - 1u];

        if (ai < bi) {
            return -1;
        }
        if (ai > bi) {
            return 1;
        }
        len--;
    }

    return 0;
}

static inline int pors_precomp_binom_table_cmp_sig(
    const pors_precomp_binom_table *table, uint32_t row,
    const pors_u256 *target, uint32_t target_len)
{
    const uint32_t off = table->offsets[row];
    const uint32_t len = table->offsets[row + 1u] - off;

    return pors_precomp_u256_cmp_limbs_sig(table->data + off, len, target,
                                           target_len);
}

#endif
