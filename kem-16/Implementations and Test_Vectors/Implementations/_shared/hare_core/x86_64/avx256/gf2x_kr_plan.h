#ifndef HARE_GF2X_KR_PLAN_H
#define HARE_GF2X_KR_PLAN_H

/*
 * Compile-time GF2X plan for HARE v1.5 KR instances.
 *
 * The plan is generated from public parameters only.  It makes the selected GF2X
 * schedule explicit and auditable, instead of relying on implicit arithmetic
 * in gf2x.c.  All values are word counts unless otherwise stated.
 *
 * The selected kernel uses a top-level word-aligned Toom-3 split and the
 * existing dense Karatsuba/PCLMUL sub-multiplier.  This header is deliberately
 * conservative: it does not introduce secret sparse multiplication or
 * secret-dependent control flow.
 */

#if defined(HARE_BACKEND_KR)

#if (PARAM_N == 20899)
#define HARE_GF2X_PLAN_NAME "HARE-128-kr-toom3-v1"
#define HARE_GF2X_PLAN_NWORDS 327U
#define HARE_GF2X_PLAN_TOOM_T 109U
#define HARE_GF2X_PLAN_TOOM_E 111U
#define HARE_GF2X_PLAN_LEAF_THRESHOLD 16U
#define HARE_GF2X_PLAN_USE_TOOM3_TOP 1
#elif (PARAM_N == 52379)
#define HARE_GF2X_PLAN_NAME "HARE-256-kr-toom3-v1"
#define HARE_GF2X_PLAN_NWORDS 819U
#define HARE_GF2X_PLAN_TOOM_T 273U
#define HARE_GF2X_PLAN_TOOM_E 275U
#define HARE_GF2X_PLAN_LEAF_THRESHOLD 16U
#define HARE_GF2X_PLAN_USE_TOOM3_TOP 1
#elif (PARAM_N == 104869)
#define HARE_GF2X_PLAN_NAME "HARE-384-kr-toom3-v1"
#define HARE_GF2X_PLAN_NWORDS 1639U
#define HARE_GF2X_PLAN_TOOM_T 547U
#define HARE_GF2X_PLAN_TOOM_E 549U
#define HARE_GF2X_PLAN_LEAF_THRESHOLD 16U
#define HARE_GF2X_PLAN_USE_TOOM3_TOP 1
#elif (PARAM_N == 173981)
#define HARE_GF2X_PLAN_NAME "HARE-512-kr-toom3-v1"
#define HARE_GF2X_PLAN_NWORDS 2719U
#define HARE_GF2X_PLAN_TOOM_T 907U
#define HARE_GF2X_PLAN_TOOM_E 909U
#define HARE_GF2X_PLAN_LEAF_THRESHOLD 16U
#define HARE_GF2X_PLAN_USE_TOOM3_TOP 1
#endif

#endif /* HARE_BACKEND_KR */

#endif /* HARE_GF2X_KR_PLAN_H */
