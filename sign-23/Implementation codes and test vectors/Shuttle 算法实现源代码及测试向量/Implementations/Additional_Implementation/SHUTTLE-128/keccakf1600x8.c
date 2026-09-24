/*
 * keccakf1600x8.c -- 8-way (AVX-512) Keccak-f[1600] permutation, 24
 * rounds.
 *
 * Derived from the eXtended Keccak Code Package (XKCP),
 *   https://github.com/XKCP/XKCP
 * file Modes/.../KeccakP-1600-times8-SIMD512.c (the AVX-512,
 * fully-unrolled 8-way permutation, originally by Gilles Van Assche et
 * al., CC0/public domain).
 *
 * MODIFICATIONS for this AVX-512 backend:
 *   - Stripped to ONLY the 24-round PermuteAll (the IRS fips202x8 wrappers
 * do their own absorb via _mm512_i64gather_epi64 XOR and squeeze via lane
 *     extract, so none of the XKCP SnP byte-accessors, the 12/6/4-round
 *     variants, the FastLoop-absorb, K12 or Kravatte helpers, nor the
 *     KeccakP1600times4 code are needed and were removed).
 *   - The local round-constant table was removed; the permutation now
 * reads the SINGLE shared symbol KeccakF_RoundConstants from ref/fips202.c
 *     (namespaced lithium_fips202_ref_KeccakF_RoundConstants), so the
 * whole link has exactly one Keccak round-constant table (check-consts
 * gate).
 *   - Inlined the two config defines (fullUnrolling + useAVX512) and
 * dropped the align.h / SnP.h / brg_endian.h / SIMD512-config.h includes
 * (the kept code does not reference them).
 *   - Re-namespaced the exported permutation to
 * lithium_fips202x8_avx512_*. Only AVX-512 macros (vpternarylogic /
 * vprolq) are kept; the SIMULATE_AVX512 scalar fallback was removed.  The
 * round arithmetic is byte-for-byte XKCP.
 */
#include <immintrin.h>
#include <stdint.h>

#include "fips202.h" /* KeccakF_RoundConstants (single shared RC table) */
#include "fips202x8.h" /* FIPS202X8_NAMESPACE */

typedef __m512i V512;

#define XOR(a, b) _mm512_xor_si512(a, b)
#define XOR3(a, b, c) _mm512_ternarylogic_epi64(a, b, c, 0x96)
#define XOR5(a, b, c, d, e) XOR3(XOR3(a, b, c), d, e)
#define ROL(a, offset) _mm512_rol_epi64(a, offset)
#define Chi(a, b, c) _mm512_ternarylogic_epi64(a, b, c, 0xD2)
#define CONST8_64(a) _mm512_set1_epi64(a)

/* Reuse ref's single round-constant table (no second copy). */
#define KeccakP1600RoundConstants KeccakF_RoundConstants

/* Export under the fips202x8 namespace (matches the extern in
 * fips202x8.c). */
#define KeccakP1600times8_PermuteAll_24rounds \
    FIPS202X8_NAMESPACE(KeccakP1600times8_PermuteAll_24rounds)

#define KeccakP_DeclareVars       \
    V512 _Ba, _Be, _Bi, _Bo, _Bu; \
    V512 _Da, _De, _Di, _Do, _Du; \
    V512 _ba, _be, _bi, _bo, _bu; \
    V512 _ga, _ge, _gi, _go, _gu; \
    V512 _ka, _ke, _ki, _ko, _ku; \
    V512 _ma, _me, _mi, _mo, _mu; \
    V512 _sa, _se, _si, _so, _su

#define KeccakP_ThetaRhoPiChi(_L1, _L2, _L3, _L4, _L5, _Bb1, _Bb2, _Bb3, \
                              _Bb4, _Bb5, _Rr1, _Rr2, _Rr3, _Rr4, _Rr5)  \
    _Bb1 = XOR(_L1, _Da);                                                \
    _Bb2 = XOR(_L2, _De);                                                \
    _Bb3 = XOR(_L3, _Di);                                                \
    _Bb4 = XOR(_L4, _Do);                                                \
    _Bb5 = XOR(_L5, _Du);                                                \
    if (_Rr1 != 0)                                                       \
        _Bb1 = ROL(_Bb1, _Rr1);                                          \
    _Bb2 = ROL(_Bb2, _Rr2);                                              \
    _Bb3 = ROL(_Bb3, _Rr3);                                              \
    _Bb4 = ROL(_Bb4, _Rr4);                                              \
    _Bb5 = ROL(_Bb5, _Rr5);                                              \
    _L1 = Chi(_Ba, _Be, _Bi);                                            \
    _L2 = Chi(_Be, _Bi, _Bo);                                            \
    _L3 = Chi(_Bi, _Bo, _Bu);                                            \
    _L4 = Chi(_Bo, _Bu, _Ba);                                            \
    _L5 = Chi(_Bu, _Ba, _Be);

#define KeccakP_ThetaRhoPiChiIota0(_L1, _L2, _L3, _L4, _L5, _rc)       \
    _Ba = XOR5(_ba, _ga, _ka, _ma, _sa); /* Theta effect */            \
    _Be = XOR5(_be, _ge, _ke, _me, _se);                               \
    _Bi = XOR5(_bi, _gi, _ki, _mi, _si);                               \
    _Bo = XOR5(_bo, _go, _ko, _mo, _so);                               \
    _Bu = XOR5(_bu, _gu, _ku, _mu, _su);                               \
    _Da = ROL(_Be, 1);                                                 \
    _De = ROL(_Bi, 1);                                                 \
    _Di = ROL(_Bo, 1);                                                 \
    _Do = ROL(_Bu, 1);                                                 \
    _Du = ROL(_Ba, 1);                                                 \
    _Da = XOR(_Da, _Bu);                                               \
    _De = XOR(_De, _Ba);                                               \
    _Di = XOR(_Di, _Be);                                               \
    _Do = XOR(_Do, _Bi);                                               \
    _Du = XOR(_Du, _Bo);                                               \
    KeccakP_ThetaRhoPiChi(_L1, _L2, _L3, _L4, _L5, _Ba, _Be, _Bi, _Bo, \
                          _Bu, 0, 44, 43, 21, 14);                     \
    _L1 = XOR(_L1, _rc) /* Iota */

#define KeccakP_ThetaRhoPiChi1(_L1, _L2, _L3, _L4, _L5)                \
    KeccakP_ThetaRhoPiChi(_L1, _L2, _L3, _L4, _L5, _Bi, _Bo, _Bu, _Ba, \
                          _Be, 3, 45, 61, 28, 20)

#define KeccakP_ThetaRhoPiChi2(_L1, _L2, _L3, _L4, _L5)                \
    KeccakP_ThetaRhoPiChi(_L1, _L2, _L3, _L4, _L5, _Bu, _Ba, _Be, _Bi, \
                          _Bo, 18, 1, 6, 25, 8)

#define KeccakP_ThetaRhoPiChi3(_L1, _L2, _L3, _L4, _L5)                \
    KeccakP_ThetaRhoPiChi(_L1, _L2, _L3, _L4, _L5, _Be, _Bi, _Bo, _Bu, \
                          _Ba, 36, 10, 15, 56, 27)

#define KeccakP_ThetaRhoPiChi4(_L1, _L2, _L3, _L4, _L5)                \
    KeccakP_ThetaRhoPiChi(_L1, _L2, _L3, _L4, _L5, _Bo, _Bu, _Ba, _Be, \
                          _Bi, 41, 2, 62, 55, 39)

#define KeccakP_4rounds(i)                                               \
    KeccakP_ThetaRhoPiChiIota0(_ba, _ge, _ki, _mo, _su,                  \
                               CONST8_64(KeccakP1600RoundConstants[i])); \
    KeccakP_ThetaRhoPiChi1(_ka, _me, _si, _bo, _gu);                     \
    KeccakP_ThetaRhoPiChi2(_sa, _be, _gi, _ko, _mu);                     \
    KeccakP_ThetaRhoPiChi3(_ga, _ke, _mi, _so, _bu);                     \
    KeccakP_ThetaRhoPiChi4(_ma, _se, _bi, _go, _ku);                     \
                                                                         \
    KeccakP_ThetaRhoPiChiIota0(                                          \
        _ba, _me, _gi, _so, _ku,                                         \
        CONST8_64(KeccakP1600RoundConstants[i + 1]));                    \
    KeccakP_ThetaRhoPiChi1(_sa, _ke, _bi, _mo, _gu);                     \
    KeccakP_ThetaRhoPiChi2(_ma, _ge, _si, _ko, _bu);                     \
    KeccakP_ThetaRhoPiChi3(_ka, _be, _mi, _go, _su);                     \
    KeccakP_ThetaRhoPiChi4(_ga, _se, _ki, _bo, _mu);                     \
                                                                         \
    KeccakP_ThetaRhoPiChiIota0(                                          \
        _ba, _ke, _si, _go, _mu,                                         \
        CONST8_64(KeccakP1600RoundConstants[i + 2]));                    \
    KeccakP_ThetaRhoPiChi1(_ma, _be, _ki, _so, _gu);                     \
    KeccakP_ThetaRhoPiChi2(_ga, _me, _bi, _ko, _su);                     \
    KeccakP_ThetaRhoPiChi3(_sa, _ge, _mi, _bo, _ku);                     \
    KeccakP_ThetaRhoPiChi4(_ka, _se, _gi, _mo, _bu);                     \
                                                                         \
    KeccakP_ThetaRhoPiChiIota0(                                          \
        _ba, _be, _bi, _bo, _bu,                                         \
        CONST8_64(KeccakP1600RoundConstants[i + 3]));                    \
    KeccakP_ThetaRhoPiChi1(_ga, _ge, _gi, _go, _gu);                     \
    KeccakP_ThetaRhoPiChi2(_ka, _ke, _ki, _ko, _ku);                     \
    KeccakP_ThetaRhoPiChi3(_ma, _me, _mi, _mo, _mu);                     \
    KeccakP_ThetaRhoPiChi4(_sa, _se, _si, _so, _su)

#define rounds24         \
    KeccakP_4rounds(0);  \
    KeccakP_4rounds(4);  \
    KeccakP_4rounds(8);  \
    KeccakP_4rounds(12); \
    KeccakP_4rounds(16); \
    KeccakP_4rounds(20)

#define copyFromState(pState) \
    _ba = pState[0];          \
    _be = pState[1];          \
    _bi = pState[2];          \
    _bo = pState[3];          \
    _bu = pState[4];          \
    _ga = pState[5];          \
    _ge = pState[6];          \
    _gi = pState[7];          \
    _go = pState[8];          \
    _gu = pState[9];          \
    _ka = pState[10];         \
    _ke = pState[11];         \
    _ki = pState[12];         \
    _ko = pState[13];         \
    _ku = pState[14];         \
    _ma = pState[15];         \
    _me = pState[16];         \
    _mi = pState[17];         \
    _mo = pState[18];         \
    _mu = pState[19];         \
    _sa = pState[20];         \
    _se = pState[21];         \
    _si = pState[22];         \
    _so = pState[23];         \
    _su = pState[24]

#define copyToState(pState) \
    pState[0] = _ba;        \
    pState[1] = _be;        \
    pState[2] = _bi;        \
    pState[3] = _bo;        \
    pState[4] = _bu;        \
    pState[5] = _ga;        \
    pState[6] = _ge;        \
    pState[7] = _gi;        \
    pState[8] = _go;        \
    pState[9] = _gu;        \
    pState[10] = _ka;       \
    pState[11] = _ke;       \
    pState[12] = _ki;       \
    pState[13] = _ko;       \
    pState[14] = _ku;       \
    pState[15] = _ma;       \
    pState[16] = _me;       \
    pState[17] = _mi;       \
    pState[18] = _mo;       \
    pState[19] = _mu;       \
    pState[20] = _sa;       \
    pState[21] = _se;       \
    pState[22] = _si;       \
    pState[23] = _so;       \
    pState[24] = _su

void KeccakP1600times8_PermuteAll_24rounds(void *states)
{
    V512 *statesAsLanes = (V512 *)states;
    KeccakP_DeclareVars;

    copyFromState(statesAsLanes);
    rounds24;
    copyToState(statesAsLanes);
}
