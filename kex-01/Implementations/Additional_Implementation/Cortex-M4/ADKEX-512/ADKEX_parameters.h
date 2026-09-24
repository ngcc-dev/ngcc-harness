#ifndef ADKEX_PARAMETERS_H
#define ADKEX_PARAMETERS_H

/*
ADKEX (KEMTLS-PDK) — unified parameters (sizes in BITS).
One ADKEX_MODE-switched header for all three security levels. KEMTLS is
KEM-authenticated: the long-term credential is the DKEM *CCA* KEM (no signature),
and the 2-pass handshake combines an ephemeral + a static + a server KEM secret.

  ADKEX_MODE = 128 -> DKEM-128 (n=256,q=3329)
             = 256 -> DKEM-256 (n=256,q=3329)
             = 512 -> DKEM-512 (n=512,q=7681)

A single -DADKEX_MODE=<n> drives DKE_MODE, the domain-separation label, and the
256/512-bit KDF choice.
*/

#ifndef ADKEX_MODE
#define ADKEX_MODE 128
#endif

#if   ADKEX_MODE == 128
#  ifndef DKE_MODE
#    define DKE_MODE 128
#  endif
#  define ADKEX_LABEL  "ADKEX-KEMTLS-128"
#elif ADKEX_MODE == 256
#  ifndef DKE_MODE
#    define DKE_MODE 256
#  endif
#  define ADKEX_LABEL  "ADKEX-KEMTLS-256"
#elif ADKEX_MODE == 512
#  ifndef DKE_MODE
#    define DKE_MODE 512
#  endif
#  define ADKEX_LABEL  "ADKEX-KEMTLS-512"
#else
#  error "ADKEX_MODE must be 128, 256, or 512."
#endif

#include "parameters.h"   /* DKE_* CCA-KEM sizes (DKE_MODE-switched) */

/* ---- mode-independent ADKEX (KEMTLS) sizes ---- */
#define ADKEX_PASSES_NUM       2
#define ADKEX_N                DKE_N

#define ADKEX_SEEDBITS         (8 * (DKE_SEEDBYTES))
#define ADKEX_SSBITS           (8 * (DKE_SSBYTES))
#define ADKEX_PKBITS           (8 * (DKE_PKBYTES))
#define ADKEX_SKBITS           (8 * (DKE_SKBYTES))
#define ADKEX_CTBITS           (8 * (DKE_CTBYTES))

/* Final shared-key bit-length (= n in the spec). */
#define ADKEX_KDF_OUTBITS      ADKEX_SSBITS

/* Wire-message sizes (BITS). */
#define ADKEX_M1_BITS          (ADKEX_PKBITS + ADKEX_CTBITS)
#define ADKEX_M2_BITS          (ADKEX_CTBITS)
#define ADKEX_TOTAL_MSG_BITS   (ADKEX_M1_BITS + ADKEX_M2_BITS)

/* State buffers.  st_A = sk_e || ss_s || m_1 ;  st_B = ss_e || ss_s || T */
#define ADKEX_STA_MAX_BITS     (ADKEX_SKBITS + ADKEX_SSBITS + ADKEX_M1_BITS)
#define ADKEX_STB_MAX_BITS     (3 * ADKEX_SSBITS)

#define ADKEX_STA_OFF_SK_E_BITS  0
#define ADKEX_STA_OFF_SS_S_BITS  (ADKEX_SKBITS)
#define ADKEX_STA_OFF_M1_BITS    (ADKEX_SKBITS + ADKEX_SSBITS)

#define ADKEX_STB_OFF_SS_E_BITS  0
#define ADKEX_STB_OFF_SS_S_BITS  (ADKEX_SSBITS)
#define ADKEX_STB_OFF_T_BITS     (2 * ADKEX_SSBITS)

/* Randomness consumed by each derandomized call. */
#define ADKEX_INIT_B_COINBITS          (ADKEX_SEEDBITS + ADKEX_SSBITS)
#define ADKEX_PASS1_KEYGEN_COINBITS    (ADKEX_SEEDBITS + ADKEX_SSBITS)
#define ADKEX_PASS1_ENCAPS_COINBITS    (ADKEX_SEEDBITS)
#define ADKEX_PASS1_COINBITS           (ADKEX_PASS1_KEYGEN_COINBITS + ADKEX_PASS1_ENCAPS_COINBITS)
#define ADKEX_PASS2_COINBITS           (ADKEX_SEEDBITS)

#define ADKEX_LABEL_BITS       128
#define ADKEX_TRANSCRIPT_BITS  (ADKEX_LABEL_BITS + ADKEX_PKBITS + ADKEX_M1_BITS + ADKEX_M2_BITS)

/* Transcript / KDF hash: SM3 (256-bit) for <=256, pseudo-XOF for the 512-bit key. */
#include "auxfunc.h"
#if ADKEX_SSBITS <= 256
#  define ADKEX_KDF(ob, in, inb, out)  sm3hash((int)(ob), (in), (inb), (out))
#else
#  define ADKEX_KDF(ob, in, inb, out)  pseudoXOF((unsigned long long)(ob), (in), (inb), (out))
#endif

#endif /* ADKEX_PARAMETERS_H */
