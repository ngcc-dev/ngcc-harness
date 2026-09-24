/*
 * Canonical single-message AVX2 AFS-p-S6[1600,20] path for AFS-TrEDM-S6.
 *
 * This translation unit deliberately reuses the audited S6 opt64 round core
 * while forcing the AVX2 AFS-64 nonlinear layer enabled in afs_p1600.c. The
 * linear layer remains afs_lmds1600_s6_opt64(A, round), so no legacy linear
 * code or x2 hooks are part of this canonical AVX2 target.
 */
#ifndef AFS_TREDM_USE_AVX2
#define AFS_TREDM_USE_AVX2 1
#endif

#include "afs_p1600.c"
