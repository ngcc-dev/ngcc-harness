/*
 * rcdt_tables.h -- 96-bit reverse-CDT (RCDT) base-sampler threshold
 * tables.
 *
 * GENERATED CONSTANTS: the table bytes inside the @@AUTOGEN:rcdt@@ region
 * are emitted by tools/gen_rcdt.py (a thin production emitter wrapping the
 * audited math reference tools/BaseSampler.py).  Do NOT hand-edit the
 * region; run `python3 gen_rcdt.py` to regenerate and `python3 gen_rcdt.py
 * --check` for the reproducibility/drift gate (these are wired into `make
 * tables` / `make check-consts`).  Audit log: tools/log/rcdt_tables.txt.
 *
 * Each threshold Z[i] is a 96-bit unsigned integer stored as 3x32-bit
 * little-endian limbs: value = Z[i][0] + Z[i][1]*2^32 + Z[i][2]*2^64,
 * encoding round(Pr[X >= i+1] * 2^96) for a half-discrete-Gaussian X
 * centred at 0.  A base sample is z = #{ i : v <_u Z[i] } over a 96-bit
 * uniform draw v (see ref/sampler.c::cdt_scan96).
 *
 *   SHUTTLE_RCDT_Z        sigma_s = 825/256 = 3.22265625, 36 rows.
 *                         The wide masking / BLISS base sampler,
 * MODE-INDEPENDENT (identical across SHUTTLE-128/256/512).
 *   SHUTTLE_RCDT_NOISE_0_85   sigma = 0.85,  9 rows  (keygen secret
 * noise). SHUTTLE_RCDT_NOISE_0_90   sigma = 0.90, 10 rows  (keygen secret
 * noise). SHUTTLE_RCDT_NOISE_1_00   sigma = 1.00, 11 rows  (keygen secret
 * noise).
 *
 * The per-set choice of which noise table feeds s (sigma_1) and e
 * (sigma_2) is made in ref/sampler.h (RCDT_NOISE_S / RCDT_NOISE_E
 * aliases).
 *
 * INV-NOMAX (table invariant, asserted at generation time -- see
 * gen_rcdt.py and ref/sampler.h): for every row, the MID and HIGH limbs
 * satisfy Z[i][1] != 0xFFFFFFFF and Z[i][2] != 0xFFFFFFFF (the LOW limb is
 * unconstrained -- it never receives an incoming borrow).  This lets the
 * constant-time borrow-FOLD compare add the incoming borrow to the
 * threshold limb without 32-bit wrap; it constrains only the PUBLIC table,
 * so it does not affect constant-time.  See ref/sampler.c for the full
 * argument (K11).
 *
 * The tables are `static const` so this header may be included directly by
 * any translation unit (ref/sampler.c, the test driver) without a separate
 * .c file or a namespace rename; each TU that does not reference a given
 * table simply drops it as unused static data.  The arrays are referenced
 * through the sampler API, so in practice only the including TU's used
 * entries remain.
 */
#ifndef SHUTTLE_RCDT_TABLES_H
#define SHUTTLE_RCDT_TABLES_H

#include <stdint.h>

/* @@AUTOGEN:rcdt@@ BEGIN */
/* 96-bit reverse-CDT thresholds (3x32-bit little-endian limbs); see
 * gen_rcdt.py. INV-NOMAX (mid+high limb != 0xFFFFFFFF) asserted on every
 * row at generation. */
/* 96-bit RCDT -> 3x32-bit little-endian limbs; 36 rows; INV-NOMAX
 * verified. */
static const uint32_t SHUTTLE_RCDT_Z[36][3] = {
    {0x402F7FA2U, 0x12C69654U, 0xC7999458U},
    {0x8E7FB711U, 0xE6782481U, 0x91D9D07AU},
    {0x08E4AE6CU, 0x9C4E92B4U, 0x63548C39U},
    {0xF198871FU, 0x153DAC5BU, 0x3EC320F8U},
    {0x5B379FE3U, 0xC0D5F1C9U, 0x24A7FB57U},
    {0xA2890D59U, 0x0A373A8CU, 0x13BAD6C5U},
    {0x85ECF03BU, 0x77F6EF00U, 0x09C34493U},
    {0x06BD7625U, 0x657531BDU, 0x046EB351U},
    {0x0AE5CC87U, 0xF6A4CE61U, 0x01D7ED5BU},
    {0x8E8C4290U, 0x80C6FB16U, 0x00B39140U},
    {0x945A8C25U, 0xFC91CEB4U, 0x003E7104U},
    {0xFFE7F159U, 0xE3BC50BAU, 0x0013D35DU},
    {0xDBA3E1F9U, 0x799C55C0U, 0x0005BE50U},
    {0x16FF3692U, 0x1CB2C576U, 0x00018465U},
    {0x654B748FU, 0x2018307DU, 0x00005D81U},
    {0xCAAE107AU, 0xB2CD4BA0U, 0x00001481U},
    {0x5842B889U, 0x6D1B1765U, 0x00000418U},
    {0x030C33B1U, 0x9BDF6112U, 0x000000BEU},
    {0x13DE9A9DU, 0x89A793B6U, 0x0000001FU},
    {0xBA01FE36U, 0xBF71FA38U, 0x00000004U},
    {0x7C388C2BU, 0xA673B059U, 0x00000000U},
    {0xA0C47B37U, 0x14BB85E9U, 0x00000000U},
    {0x82A7DEC0U, 0x02592B23U, 0x00000000U},
    {0xDC786C9AU, 0x003DE9B7U, 0x00000000U},
    {0xF203DEDBU, 0x0005CC00U, 0x00000000U},
    {0xC474C30EU, 0x00007E4EU, 0x00000000U},
    {0x85F8ADDFU, 0x000009C5U, 0x00000000U},
    {0xE4013000U, 0x000000AFU, 0x00000000U},
    {0x3D57DDC7U, 0x0000000BU, 0x00000000U},
    {0xA7163F8FU, 0x00000000U, 0x00000000U},
    {0x08D11741U, 0x00000000U, 0x00000000U},
    {0x006C385FU, 0x00000000U, 0x00000000U},
    {0x0004B6EAU, 0x00000000U, 0x00000000U},
    {0x00002FC3U, 0x00000000U, 0x00000000U},
    {0x000001B6U, 0x00000000U, 0x00000000U},
    {0x0000000DU, 0x00000000U, 0x00000000U}};
/* 96-bit RCDT -> 3x32-bit little-endian limbs; 9 rows; INV-NOMAX verified.
 */
static const uint32_t SHUTTLE_RCDT_NOISE_0_85[9][3] = {
    {0xC3CB73B8U, 0xF67BCC8DU, 0x5C747AA6U},
    {0x110C8F76U, 0xC2A05C28U, 0x0A978F6EU},
    {0x9AB7E17DU, 0x2DDCC90AU, 0x00533DE7U},
    {0xBC9B329AU, 0xE0213A50U, 0x0000A6CAU},
    {0xB024ADB1U, 0x1B1BFD11U, 0x00000054U},
    {0xC041BCDFU, 0x0AA35023U, 0x00000000U},
    {0xDD3862DDU, 0x00005653U, 0x00000000U},
    {0xAF89D7B3U, 0x00000000U, 0x00000000U},
    {0x0000596FU, 0x00000000U, 0x00000000U}};
/* 96-bit RCDT -> 3x32-bit little-endian limbs; 10 rows; INV-NOMAX
 * verified. */
static const uint32_t SHUTTLE_RCDT_NOISE_0_90[10][3] = {
    {0x63C3A91BU, 0x73D08C99U, 0x62C00D07U},
    {0x3E38EB38U, 0x9625D715U, 0x0DEDB092U},
    {0x53FCB3E2U, 0xE73FB18AU, 0x009DB3B9U},
    {0x1CCE8231U, 0xBAF2F8FAU, 0x00021365U},
    {0x81920B96U, 0x79103336U, 0x0000020CU},
    {0x73DC07FBU, 0x96E7AD8BU, 0x00000000U},
    {0x9BDF1816U, 0x000CA3F6U, 0x00000000U},
    {0xE1610238U, 0x0000004EU, 0x00000000U},
    {0x008F3A79U, 0x00000000U, 0x00000000U},
    {0x0000004BU, 0x00000000U, 0x00000000U}};
/* 96-bit RCDT -> 3x32-bit little-endian limbs; 11 rows; INV-NOMAX
 * verified. */
static const uint32_t SHUTTLE_RCDT_NOISE_1_00[11][3] = {
    {0x2BFDBDFCU, 0xB7D318D4U, 0x6DFDA4E6U},
    {0x2AA2B001U, 0xB85F106CU, 0x156E867AU},
    {0x15454C5FU, 0x1625B451U, 0x01ABEA39U},
    {0x26C842F8U, 0xE66F73EEU, 0x000CADCCU},
    {0xB773BC08U, 0x4710A6BDU, 0x000023CEU},
    {0x0F92A8ADU, 0x5D28DCBBU, 0x00000025U},
    {0xD8D1C6D8U, 0x0E5DF25BU, 0x00000000U},
    {0xB536290EU, 0x00020893U, 0x00000000U},
    {0x1CBE1E4DU, 0x0000001BU, 0x00000000U},
    {0x0084FE20U, 0x00000000U, 0x00000000U},
    {0x000000EFU, 0x00000000U, 0x00000000U}};
/* @@AUTOGEN:rcdt@@ END */

#endif /* SHUTTLE_RCDT_TABLES_H */
