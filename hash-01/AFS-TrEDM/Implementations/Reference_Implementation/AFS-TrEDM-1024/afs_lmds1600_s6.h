/*
 * AFS-LMDS-1600-S6 lane-friendly linear diffusion layer.
 *
 * This layer replaces the legacy row-wise mask-table diffusion.
 * It is specified as the six-direction rotating single-MDS layer:
 *
 *     L_r = tau_{d_r}^{-1} o L_P o tau_{d_r},
 *     d_r = [INF, 0, 1, 2, 3, 4][r mod 6],
 *     L_P = mu o pi_AFS o rho_AFS o M_col.
 *
 * State layout: A[x + 5*y] is the 64-bit lane at coordinate (x,y).
 */

#ifndef AFS_LMDS1600_S6_H
#define AFS_LMDS1600_S6_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void afs_lmds1600_s6(uint64_t A[25], unsigned round);

#ifdef __cplusplus
}
#endif

#endif /* AFS_LMDS1600_S6_H */
