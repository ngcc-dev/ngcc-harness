#ifndef LOOM_PARAMS_H
#define LOOM_PARAMS_H

// #define LOOM_USE_SHAKE

#ifdef LOOM_USE_SHAKE
#define WEAVER_USE_SHAKE
#define SHA3_MODE
#endif

#define LOOM_INITIATOR 1
#define LOOM_RESPONDER 0

#if LOOM_MODE == 1
  #define WEAVER_MODE 1
  #define SHUTTLE_MODE 128
#  if defined(LOOM_AVX2)
  #define LOOM_NAMESPACE(s) loom1_avx2_##s
#  else
  #define LOOM_NAMESPACE(s) loom1_ref_##s
#  endif
  #define LOOM_INSTANCE_NAME "LoomKEX-128"
  #define LOOM_MACBYTES     16
  #define LOOM_NONCEBYTES   32
  
#elif LOOM_MODE == 3
  #define WEAVER_MODE 3
  #define SHUTTLE_MODE 256
#  if defined(LOOM_AVX2)
  #define LOOM_NAMESPACE(s) loom3_avx2_##s
#  else
  #define LOOM_NAMESPACE(s) loom3_ref_##s
#  endif
  #define LOOM_INSTANCE_NAME "LoomKEX-256"
  #define LOOM_MACBYTES     32
  #define LOOM_NONCEBYTES   32
  
#elif LOOM_MODE == 5
  #define WEAVER_MODE 5
  #define SHUTTLE_MODE 512
#  if defined(LOOM_AVX2)
  #define LOOM_NAMESPACE(s) loom5_avx2_##s
#  else
  #define LOOM_NAMESPACE(s) loom5_ref_##s
#  endif
  #define LOOM_INSTANCE_NAME "LoomKEX-512"
  #define LOOM_MACBYTES     64
  #define LOOM_NONCEBYTES   64
  
#else
  #error "LOOM_MODE must be 1, 3, or 5"
#endif

#include "kemparams.h"
#include "sigparams.h"

/* Max identity length (bytes); same bound as gmsm ZA() uidLen < 8192. */
// #define LOOM_MAX_IDBYTES  8191
#define LOOM_MAX_IDBYTES  32
#define LOOM_IDLENBYTES   2

#if WEAVER_MODE == 1
#define LOOM_KEM_PKBYTES   752
#define LOOM_KEM_SKBYTES   960
#define LOOM_KEM_CTBYTES   736
#define LOOM_KEM_SSBYTES   16
#elif WEAVER_MODE == 3
#define LOOM_KEM_PKBYTES   1312
#define LOOM_KEM_SKBYTES   1664
#define LOOM_KEM_CTBYTES   1344
#define LOOM_KEM_SSBYTES   32
#elif WEAVER_MODE == 5
#define LOOM_KEM_PKBYTES   2880
#define LOOM_KEM_SKBYTES   3328
#define LOOM_KEM_CTBYTES   2944
#define LOOM_KEM_SSBYTES   64
#endif

#if SHUTTLE_MODE == 128
  #define LOOM_SIG_PKBYTES  1264
  #define LOOM_SIG_SKBYTES  2288
  #define LOOM_SIG_MAXBYTES 1183
#elif SHUTTLE_MODE == 256
  #define LOOM_SIG_PKBYTES  1952
  #define LOOM_SIG_SKBYTES  3680
  #define LOOM_SIG_MAXBYTES 2417
#elif SHUTTLE_MODE == 512
  #define LOOM_SIG_PKBYTES  3648
  #define LOOM_SIG_SKBYTES  7104
  #define LOOM_SIG_MAXBYTES 5001
#endif

#define LOOM_PKBYTES       LOOM_SIG_PKBYTES
#define LOOM_SKBYTES       LOOM_SIG_SKBYTES

#define LOOM_SPIHALFBYTES  4
#define LOOM_SPIBYTES      8
#define LOOM_MSG1BYTES     (LOOM_SPIBYTES + LOOM_NONCEBYTES + LOOM_KEM_PKBYTES)
#define LOOM_MSG2BYTES     (LOOM_SPIBYTES + LOOM_NONCEBYTES + LOOM_KEM_CTBYTES)
/* LOOM_DISPUTE: 2-byte sig length prefix; not in AKE PDF wire format. */
#define LOOM_SIGLENBYTES   2
#define LOOM_MSG3BODY_MAX  (LOOM_IDLENBYTES + LOOM_MAX_IDBYTES \
                            + LOOM_SIGLENBYTES + LOOM_SIG_MAXBYTES)
#define LOOM_MSG3BYTES     (LOOM_MSG3BODY_MAX + LOOM_MACBYTES)
#define LOOM_MSG4BYTES     LOOM_MSG3BYTES

#define MAX(a,b) ((a)>(b)?(a):(b))
#define LOOM_EXCH_LENMAX   MAX(LOOM_KEM_CTBYTES, LOOM_KEM_PKBYTES)
#define LOOM_DTBS_BUFLEN   (1 + LOOM_SPIBYTES + LOOM_KEM_CTBYTES + LOOM_KEM_PKBYTES)

#define LOOM_MACINPUT_BUFLEN (1 + LOOM_SPIBYTES + LOOM_MAX_IDBYTES)

#endif
