#ifndef ADKEX_PARAMETERS_H
#define ADKEX_PARAMETERS_H

/*
ADKEX (KEX + SIG, 3-pass mutual auth) — unified parameters.
One ADKEX_MODE-switched header for all three security levels (sizes in BITS).

  ADKEX_MODE = 128 -> DKEM-128 ephemeral KEM + ML-DSA-44 (level 2)
             = 256 -> DKEM-256 ephemeral KEM + ML-DSA-87 (level 5)
             = 512 -> DKEM-512 ephemeral KEM + ML-DSA-87 (level 5)

The lattice/poly core lives in core/ (the optimized DKEM tree); the signature
backend lives in dilithium/. DKE_MODE / DILITHIUM_MODE / ADKEX_SIG_MLDSA_LEVEL
are derived from ADKEX_MODE so a single -DADKEX_MODE=<n> drives the whole build.
*/

#ifndef ADKEX_MODE
#define ADKEX_MODE 128
#endif

/* ADKEX_MODE -> DKEM core mode + ML-DSA level + domain-separation labels. */
#if   ADKEX_MODE == 128
#  ifndef DKE_MODE
#    define DKE_MODE 128
#  endif
#  define ADKEX_SIG_MLDSA_LEVEL 2
#  define ADKEX_LABEL_A         "DKEX-KSIG-128-A"
#  define ADKEX_LABEL_B         "DKEX-KSIG-128-B"
#elif ADKEX_MODE == 256
#  ifndef DKE_MODE
#    define DKE_MODE 256
#  endif
#  define ADKEX_SIG_MLDSA_LEVEL 5
#  define ADKEX_LABEL_A         "DKEX-KSIG-256-A"
#  define ADKEX_LABEL_B         "DKEX-KSIG-256-B"
#elif ADKEX_MODE == 512
#  ifndef DKE_MODE
#    define DKE_MODE 512
#  endif
#  define ADKEX_SIG_MLDSA_LEVEL 5
#  define ADKEX_LABEL_A         "DKEX-KSIG-512-A"
#  define ADKEX_LABEL_B         "DKEX-KSIG-512-B"
#else
#  error "ADKEX_MODE must be 128, 256, or 512."
#endif

/* ML-DSA reference (dilithium) selects its own ring via DILITHIUM_MODE; keep it
   in lock-step with the chosen ML-DSA level (2/3/5). The build also passes this. */
#ifndef DILITHIUM_MODE
#  define DILITHIUM_MODE ADKEX_SIG_MLDSA_LEVEL
#endif
#ifndef ADKEX_SIG_BACKEND_MLDSA
#  define ADKEX_SIG_BACKEND_MLDSA
#endif

#include "parameters.h"   /* DKE_* sizes (DKE_MODE-switched) */
#include "adkex_sig.h"          /* ADKEX_SIG_{PK,SK,SN,COIN}BITS for the chosen level */

/* ---- mode-independent ADKEX sizes (all derived from DKE_* and ADKEX_SIG_*) ---- */

#define ADKEX_PASSES_NUM       3
#define ADKEX_N                DKE_N

/* DKEX (CPA) primitive sizes for the ephemeral KEM (BITS). */
#define ADKEX_SEEDBITS         (8 * (DKE_SEEDBYTES))
#define ADKEX_SSBITS           (8 * (DKE_SSBYTES))
#define ADKEX_DKEXPKBITS       (8 * (DKE_PKBYTES))         /* M_1 size */
#define ADKEX_DKEXSKBITS       (8 * (DKE_CPA_SKABYTES))    /* sk_e size */
#define ADKEX_DKEXCTBITS       (8 * (DKE_CPA_CTBYTES))     /* M_2 size */

/* SIG primitive sizes (from adkex_sig.h, in BITS). */
#define ADKEX_SIGPKBITS        ADKEX_SIG_PKBITS
#define ADKEX_SIGSKBITS        ADKEX_SIG_SKBITS
#define ADKEX_SIGSNBITS        ADKEX_SIG_SNBITS

/* Long-term-key sizes exposed to ICCS (BITS). Both sides carry a SIG key pair. */
#define ADKEX_PKBITS           ADKEX_SIGPKBITS
#define ADKEX_SKBITS           ADKEX_SIGSKBITS

/* Final shared-key bit-length. */
#define ADKEX_KDF_OUTBITS      ADKEX_SSBITS
/* derive_ss KDF input = st = (ss || T) (single binder T, v20260618). */
#define ADKEX_KDF_INBITS       (2 * ADKEX_SSBITS)

/* Wire-message sizes (BITS). */
#define ADKEX_M1_BITS          (ADKEX_DKEXPKBITS)
#define ADKEX_M2_BITS          (ADKEX_DKEXCTBITS + ADKEX_SIGSNBITS)
#define ADKEX_M3_BITS          (ADKEX_SIGSNBITS)
#define ADKEX_TOTAL_MSG_BITS   (ADKEX_M1_BITS + ADKEX_M2_BITS + ADKEX_M3_BITS)

/* State buffer sizes (BITS).
       init_a: st_A = pk_A;  pass1: st_A = sk_e || M_1 || pk_A;  pass3: st_A = ss_raw
       init_b: st_B = pk_B;  pass2: st_B = ss_raw || T_A */
#define ADKEX_STA_PASS1_BITS   (ADKEX_DKEXSKBITS + ADKEX_DKEXPKBITS + ADKEX_SIGPKBITS)
#define ADKEX_STA_MAX_BITS     ADKEX_STA_PASS1_BITS
#define ADKEX_STB_PASS2_BITS   (2 * ADKEX_SSBITS)
#define ADKEX_STB_MAX_BITS     (ADKEX_SIGPKBITS > ADKEX_STB_PASS2_BITS \
                                ? ADKEX_SIGPKBITS : ADKEX_STB_PASS2_BITS)

/* Bit-offsets into st_A after pass1 */
#define ADKEX_STA_OFF_SK_E_BITS  0
#define ADKEX_STA_OFF_M1_BITS    (ADKEX_DKEXSKBITS)
#define ADKEX_STA_OFF_PK_A_BITS  (ADKEX_DKEXSKBITS + ADKEX_DKEXPKBITS)

/* Bit-offsets into st_A after pass3 = (ss || T) */
#define ADKEX_STA_OFF_SS_BITS    0
#define ADKEX_STA_OFF_T_BITS     (ADKEX_SSBITS)

/* Bit-offsets into st_B after pass2 = (ss || T) */
#define ADKEX_STB_OFF_SS_BITS    0
#define ADKEX_STB_OFF_T_BITS     (ADKEX_SSBITS)
#define ADKEX_STB_OFF_PK_B_BITS  0

/* Bit-offsets into m_2 = M_2 || sigma_B. */
#define ADKEX_M2_OFF_M2_BITS     0
#define ADKEX_M2_OFF_SIGMA_BITS  (ADKEX_DKEXCTBITS)

/* Randomness consumed by each derandomized call (BITS). */
#define ADKEX_INIT_A_COINBITS         ADKEX_SIG_COINBITS
#define ADKEX_INIT_B_COINBITS         ADKEX_SIG_COINBITS
#define ADKEX_PASS1_COINBITS          (ADKEX_SEEDBITS)
#define ADKEX_PASS2_DKEX_COINBITS     (ADKEX_SEEDBITS + ADKEX_N)

#define ADKEX_LABEL_BITS       128
/* Single binder T = H(LABEL_A || LABEL_B || pk_A || pk_B || M_1 || M_2; N) (v20260618). */
#define ADKEX_TRANSCRIPT_BITS  (2 * ADKEX_LABEL_BITS + 2 * ADKEX_SIGPKBITS + ADKEX_DKEXPKBITS + ADKEX_DKEXCTBITS)

/* Transcript / KDF hash. SM3 emits 256 bits, so for the 512-bit shared key
   (DKE-512, SSBYTES=64) the reference switches to the pseudo-XOF; <=256-bit
   modes use sm3hash directly. Same (outbits,msg,msgbits,out) call shape. */
#include "auxfunc.h"
#if ADKEX_SSBITS <= 256
#  define ADKEX_KDF(ob, in, inb, out)  sm3hash((int)(ob), (in), (inb), (out))
#else
#  define ADKEX_KDF(ob, in, inb, out)  pseudoXOF((unsigned long long)(ob), (in), (inb), (out))
#endif

#endif /* ADKEX_PARAMETERS_H */
