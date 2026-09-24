#include "KeccakSpongetimes4.h"

#ifdef XKCP_has_KeccakP1600
#if !defined(SUPERCOP)
#include "KeccakP-1600-times4-SnP.h"
#else
#include <libkeccak.a.headers/KeccakP-1600-times4-SnP.h>
#endif

#define prefix KeccakWidth1600times4
#define PlSnP KeccakP1600times4
#define PlSnP_width 1600
#define PlSnP_Permute KeccakP1600times4_PermuteAll_24rounds
#if defined(KeccakF1600times4_FastLoop_supported)
#endif
#include "KeccakSpongetimes4.inc"
#undef prefix
#undef PlSnP
#undef PlSnP_width
#undef PlSnP_Permute
#undef PlSnP_FastLoop_Absorb
#endif
