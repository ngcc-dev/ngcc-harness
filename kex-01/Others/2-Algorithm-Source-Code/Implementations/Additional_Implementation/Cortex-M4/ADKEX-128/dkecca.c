/*
 * DKEM IND-CCA layer (Fujisaki-Okamoto transform), dkem-sjg1006 design
 * (updates v20260602: KDF moved before the one-time pad).
 *
 * KeyGen:  sk = (skCPA | pk | z),  z = rejection secret.
 * Encaps:  r   = XOF(coins || rho),  rho = trailing SEEDBYTES of pk (matrix seed);
 *          (ct, cpa_ss) = CPA.Enc(pk, r);
 *          K = KDF(cpa_ss)  [dkem-sjg1006: KDF of the RAW cpa_ss, before the OTP];
 *          cpa_ss ^= coins  (one-time pad);  tag = cpa_ss -> ct[CPA_CTBYTES:].
 * Decaps:  recover coins = tag ^ CPA.Dec(sk, ct);  re-encapsulate;
 *          fail = !ct-equal (full ciphertext, incl. tag);
 *          Kbar = KDF(z || tag);  implicit rejection cmov(ss, K, 1-fail).
 *
 * All hashing routes through the dke_hash abstraction so every DKE_HASH backend
 * works; DKE_HASH=0 (SM3) reproduces the official KATs:
 *   XOF        -> pseudoXOF
 *   KDF/K      -> sm3hash(256) for SSBYTES==32, pseudohash(512) for SSBYTES==64
 */
#include "parameters.h"
#include "dkecpa.h"
#include "dke_hash.h"
#include "dkecca.h"
#include "verify.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* KEM key-derivation function: output DKE_SSBYTES bytes.
 *   SSBYTES==32 -> sm3hash(256, ...)      via dke_hash256
 *   SSBYTES==64 -> pseudohash(512, ...)   via dke_hash_extended */
static void dke_kem_kdf(uint8_t out[DKE_SSBYTES],
                        const uint8_t *in, size_t in_bytes) {
#if DKE_SSBYTES == 32
    dke_hash256(in, (unsigned long long)in_bytes * 8, out);
#else
    dke_hash_extended(DKE_SSBYTES * 8, in, (unsigned long long)in_bytes * 8, out);
#endif
}

void DKE_CCA_keygen_derand(uint8_t pk[DKE_PKBYTES],
                           uint8_t sk[DKE_SKBYTES],
                           const uint8_t coins[DKE_SEEDBYTES + DKE_SSBYTES]) {
    DKE_CPA_keygen_derand(pk, sk, coins);
    /* sk = (skCPA | pk | z) */
    memcpy(sk + DKE_CPA_SKABYTES, pk, DKE_PKBYTES);
    memcpy(sk + DKE_CPA_SKABYTES + DKE_PKBYTES, coins + DKE_SEEDBYTES, DKE_SSBYTES);
}

void DKE_CCA_enc_derand(uint8_t ct[DKE_CTBYTES],
                        uint8_t ss[DKE_SSBYTES],
                        const uint8_t pk[DKE_PKBYTES],
                        const uint8_t coins[DKE_SEEDBYTES]) {
    uint8_t r[DKE_SEEDBYTES + DKE_N / 8];   /* CPA encryption randomness */
    uint8_t buf[2 * DKE_SEEDBYTES];         /* (coins | rho) */
    uint8_t cpa_ss[DKE_SSBYTES];            /* provisional CPA shared secret */
    unsigned int i;

    /* buf = coins || rho, where rho = trailing SEEDBYTES of pk (the matrix seed) */
    memcpy(buf, coins, DKE_SEEDBYTES);
    memcpy(buf + DKE_SEEDBYTES, pk + DKE_PKBYTES - DKE_SEEDBYTES, DKE_SEEDBYTES);

    /* r = XOF(buf), |r| = SEEDBYTES + N/8 (== 2*SSBYTES) */
    dke_xof((unsigned long long)(DKE_SEEDBYTES + DKE_N / 8) * 8,
            buf, (unsigned long long)(2 * DKE_SEEDBYTES) * 8, r);

    /* CPA encryption */
    DKE_CPA_enc_derand(ct, cpa_ss, pk, r);

    /* Final shared secret K = KDF(cpa_ss) — BEFORE the one-time pad
     * (dkem-sjg1006 update: KDF input is the raw CPA shared secret, not the tag) */
    dke_kem_kdf(ss, cpa_ss, DKE_SSBYTES);

    /* One-time pad: cpa_ss ^= coins (coins holds SSBYTES == SEEDBYTES bytes) */
    for (i = 0; i < DKE_SSBYTES; i++)
        cpa_ss[i] ^= coins[i];

    /* Emplace tag (last SSBYTES of ct) */
    memcpy(ct + DKE_CPA_CTBYTES, cpa_ss, DKE_SSBYTES);
}

void DKE_CCA_dec(uint8_t ss[DKE_SSBYTES],
                 const uint8_t sk[DKE_SKBYTES],
                 const uint8_t ct[DKE_CTBYTES]) {
    uint8_t coins[DKE_SSBYTES];     /* tag, then recovered encaps coins */
    uint8_t cpa_ssA[DKE_SSBYTES];
    uint8_t ctA[DKE_CTBYTES];
    uint8_t k[DKE_SSBYTES];
    uint8_t rej_buf[2 * DKE_SSBYTES];
    unsigned int i;
    int fail;

    /* coins currently holds the tag */
    memcpy(coins, ct + DKE_CPA_CTBYTES, DKE_SSBYTES);

    /* CPA decryption recovers the provisional CPA shared secret */
    DKE_CPA_dec(cpa_ssA, sk, ct);

    /* Undo OTP: coins = tag ^ cpa_ssA */
    for (i = 0; i < DKE_SSBYTES; i++)
        coins[i] ^= cpa_ssA[i];

    /* Re-encapsulate with recovered coins (pk = sk + CPA_SKABYTES) */
    DKE_CCA_enc_derand(ctA, k, sk + DKE_CPA_SKABYTES, coins);

    /* Full-ciphertext comparison (includes the tag) */
    fail = DKE_verify(ct, ctA, DKE_CTBYTES);

    /* Rejection key Kbar = KDF(z || tag); z = trailing SSBYTES of sk */
    memcpy(rej_buf, sk + DKE_SKBYTES - DKE_SSBYTES, DKE_SSBYTES);
    memcpy(rej_buf + DKE_SSBYTES, ct + DKE_CPA_CTBYTES, DKE_SSBYTES);
    dke_kem_kdf(ss, rej_buf, 2 * DKE_SSBYTES);

    /* Implicit rejection */
    DKE_cmov(ss, k, DKE_SSBYTES, (uint8_t)(1 - fail));
}
