/*
 * DKE Cortex-M4 backend selector.
 *
 * Select exactly one backend at compile time (mutually exclusive):
 *
 *   -DDKE_USE_CORTEX_M4             Montgomery (3-insn SMULBB+SMLABB+ASR)
 *   -DDKE_USE_CORTEX_M4_BARRETT     Barrett-SMMULR (3 insn/reduce, pqm4 parity)
 *   -DDKE_USE_CORTEX_M4_PLANTARD    Plantard arithmetic (Huang, TCHES 2022)
 *
 * Each backend is a separate translation unit — compile only the matching .c:
 *   dke_m4_montgomery.c  — DKE_USE_CORTEX_M4
 *   dke_m4_barrett.c     — DKE_USE_CORTEX_M4_BARRETT
 *   dke_m4_plantard.c    — DKE_USE_CORTEX_M4_PLANTARD
 *
 * Supported configurations:
 *   DKE-128/256 (N=256, Q=3329): all three backends
 *   DKE-512     (N=512, Q=7681): all three backends
 */
#ifndef DKE_CORTEX_M4_H
#define DKE_CORTEX_M4_H

/* ── Mutual exclusion checks ──────────────────────────────────────── */
#if defined(DKE_USE_CORTEX_M4) && defined(DKE_USE_CORTEX_M4_BARRETT)
#  error "DKE_USE_CORTEX_M4 and DKE_USE_CORTEX_M4_BARRETT are mutually exclusive"
#endif
#if defined(DKE_USE_CORTEX_M4) && defined(DKE_USE_CORTEX_M4_PLANTARD)
#  error "DKE_USE_CORTEX_M4 and DKE_USE_CORTEX_M4_PLANTARD are mutually exclusive"
#endif
#if defined(DKE_USE_CORTEX_M4_BARRETT) && defined(DKE_USE_CORTEX_M4_PLANTARD)
#  error "DKE_USE_CORTEX_M4_BARRETT and DKE_USE_CORTEX_M4_PLANTARD are mutually exclusive"
#endif

/* ── Backend-specific ASM declarations ───────────────────────────── */
#if defined(DKE_USE_CORTEX_M4_BARRETT)
#  include "dke_m4_barrett.h"
#elif defined(DKE_USE_CORTEX_M4_PLANTARD)
   /* Plantard ASM extern declarations are internal to dke_m4_plantard.c */
#elif defined(DKE_USE_CORTEX_M4)
#  include "dke_m4_montgomery.h"
#endif

#endif /* DKE_CORTEX_M4_H */
