#include "afs_p1600.h"
#include "afs_sbox64.h"
#include "afs_lmds1600_s6.h"

/* Function splitmix64_value: computes the public SplitMix64 value used for round/lane constant derivation. */
static uint64_t splitmix64_value(uint64_t x)
{
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}

/* Function afs_round_lane_constant: derives RC32(round,lane) from the documented public SplitMix64 schedule. */
static uint32_t afs_round_lane_constant(unsigned round, unsigned lane)
{
    const uint64_t seed = 0xB4C5F1D62E1738A9ULL; /* public SplitMix64 domain seed. */
    uint64_t idx = (uint64_t)round * 25ULL + (uint64_t)lane;
    uint64_t z = splitmix64_value(seed + idx * 0x9E3779B97F4A7C15ULL);
    return (uint32_t)(z ^ (z >> 32));
}

/* Function afs_p1600_round: applies one AFS-p-S6 permutation round to the 1600-bit state. */
static void afs_p1600_round(uint64_t A[AFS_P1600_LANES], unsigned round)
{
    unsigned i;

    /*
     * Nonlinear layer: one selected AFS-64 S-box per 64-bit lane.
     * The public RC32(round,lane) constants remain round/lane dependent and
     * continue to break rotational/translation regularities after replacing
     * the previous linear layer by AFS-LMDS-1600-S6.
     */
    for (i = 0U; i < AFS_P1600_LANES; i++) {
        A[i] = afs64_t5_k2(A[i], afs_round_lane_constant(round, i));
    }

    /*
     * Lane-friendly AFS-LMDS-1600-S6 linear layer:
     *     L_r = tau_{d_r}^{-1} o L_P o tau_{d_r},
     *     d_r = [INF,0,1,2,3,4][r mod 6].
     */
    afs_lmds1600_s6(A, round);
}

/* Function afs_p1600_permute: applies a consecutive range of AFS-p-S6 rounds. */
void afs_p1600_permute(uint64_t A[AFS_P1600_LANES], unsigned first_round, unsigned nr)
{
    unsigned i;
    for (i = 0U; i < nr; i++) {
        afs_p1600_round(A, first_round + i);
    }
}

/* Function afs_p1600_g: applies the front half g of the split permutation. */
void afs_p1600_g(uint64_t A[AFS_P1600_LANES])
{
    afs_p1600_permute(A, 0U, AFS_P1600_SPLIT_ROUNDS);
}

/* Function afs_p1600_h: applies the back half h of the split permutation. */
void afs_p1600_h(uint64_t A[AFS_P1600_LANES])
{
    afs_p1600_permute(A, AFS_P1600_SPLIT_ROUNDS, AFS_P1600_SPLIT_ROUNDS);
}
