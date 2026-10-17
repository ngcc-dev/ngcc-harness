/* kex-03-1 (K2K extension): the generic K2K double-key encapsulation absorbs
 * only 64 of its 512 random bits (twokem.c, pseudoXOF third argument in bits).
 * Linked against unmodified objects built from the submitted CreTAKE-K2K-ZEN*
 * sources in a scratch copy. get_random_number is wrapped only to (a) record
 * and (b) perturb bytes 8..63 of 64-byte requests, showing that they do not
 * influence the transcript or the key. The weak-forward-secrecy attacker uses
 * the transcript plus both long-term secret keys, corrupted after the session;
 * a reduced 2^HIDE_BITS search with the other 64-HIDE_BITS bits given as a
 * hint stands in for the full 2^64 search. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "KEX_AlgorithmInstance.h"
#include "primitive_interfaces.h"
#include "cretake_params.h"
#include "auxfunc.h"
#include "drng.h"
#ifndef HIDE_BITS
#define HIDE_BITS 16
#endif
DRNG_ctx drng_algorithm;
int __real_get_random_number(DRNG_ctx *ctx, unsigned char *out, unsigned long long bits);
static int mode = 0; static int target_idx = -1; static unsigned char rec[16][64]; static int nrec = 0; static unsigned rs = 12345;
int __wrap_get_random_number(DRNG_ctx *ctx, unsigned char *out, unsigned long long bits) {
    int r = __real_get_random_number(ctx, out, bits);
    if (bits == 512 && mode) {
        if (mode == 2 && nrec == target_idx) for (int i = 8; i < 64; i++) { rs = rs * 1103515245u + 12345u; out[i] = (unsigned char)(rs >> 16); }
        if (nrec < 16) memcpy(rec[nrec++], out, 64);
    }
    return r;
}
typedef unsigned long long ull;
int main(void) {
    unsigned char seed[48]; for (int i = 0; i < 48; i++) seed[i] = (unsigned char)(3 * i + 1);
    init_random_number(&drng_algorithm, seed, 48);
    static unsigned char pkA[8192], skA[16384], pkB[8192], skB[16384], st[65536], sta[65536], m1[65536];
    static unsigned char m2[65536], m2b[65536], stb[4096], stb2[4096], ssA[256], ssB[256];
    ull l1, l2, l3, sl, m1l, m2l, m2bl, stbl, stb2l, ssal, ssbl;
    kex_init_a(pkA, &l1, skA, &l2, st, &l3); kex_init_b(pkB, &l1, skB, &l2, st, &l3);
    if (kex_generate_pass1_msg_a(skA, SKI_LEN, pkB, PKR_LEN, sta, &sl, m1, &m1l)) return 2;
    DRNG_ctx saved; memcpy(&saved, &drng_algorithm, sizeof saved);
    mode = 1; nrec = 0;
    if (kex_generate_pass2_msg_b(skB, SKR_LEN, pkA, PKI_LEN, m1, m1l, stb, &stbl, m2, &m2l) != 1) return 2;
    int nhonest = nrec; unsigned char x_true[8]; memcpy(x_true, rec[nrec - 1], 8);
    memcpy(&drng_algorithm, &saved, sizeof saved); mode = 2; nrec = 0; target_idx = nhonest - 1; /* perturb only the double-key KEM's own 64-byte request */
    kex_generate_pass2_msg_b(skB, SKR_LEN, pkA, PKI_LEN, m1, m1l, stb2, &stb2l, m2b, &m2bl);
    mode = 0;
    printf("64-byte DRBG requests in pass2: %d; perturbing bytes 8..63 of the twokem_enc request: m2 identical=%s, responder key identical=%s\n",
           nhonest, (m2l == m2bl && !memcmp(m2, m2b, m2l)) ? "yes" : "no", (stbl == stb2l && !memcmp(stb, stb2, stbl)) ? "yes" : "no");
    if (kex_derive_ss_b(skB, SKR_LEN, pkA, PKI_LEN, m1, m1l, stb, stbl, ssB, &ssbl)) return 2;
    if (kex_derive_ss_a(skA, SKI_LEN, pkB, PKR_LEN, m2, m2l, sta, sl, ssA, &ssal)) return 2;
    printf("honest session agrees: %s\n", (ssal == ssbl && !memcmp(ssA, ssB, ssal)) ? "yes" : "no");
    /* ---- wPFS attacker: transcript (m1,m2), pkA, pkB, and post-session skA, skB ---- */
    const unsigned char *tpk = m1, *ct_j = m1 + TPK_LEN, *c1 = m2, *c2 = m2 + ZEN_CIPHERTEXT_LEN_BYTES;
    unsigned char k_j[64], K1[64]; ull kl;
    kem_dec(skB, SKR_LEN, (unsigned char *)ct_j, C_R_LEN, k_j, &kl);
    kem_dec(skA, SKI_LEN, (unsigned char *)c1, ZEN_CIPHERTEXT_LEN_BYTES, K1, &kl);
    unsigned char x[8]; memcpy(x, x_true, 8);
    /* hint: attacker is given all but HIDE_BITS low-order bits of the 64-bit effective input */
    ull found = (ull)-1, tries = 0;
    for (ull g = 0; g < (1ULL << HIDE_BITS); g++) {
        for (int b = 0; b < HIDE_BITS; b++) { int byte = 7 - b / 8, bit = b % 8; if ((g >> b) & 1) x[byte] |= (1u << bit); else x[byte] &= ~(1u << bit); }
        unsigned char buf2[MSG_LEN_BYTES + SEED_BYTES], ctg[ZEN_INDCPA_CIPHERTEXT_LEN_BYTES], tp[TPK_LEN];
        pseudoXOF((MSG_LEN_BYTES + SEED_BYTES) * 8, x, 64, buf2);
        memcpy(tp, tpk, TPK_LEN);
        pke_enc(tp, buf2, buf2 + MSG_LEN_BYTES, ctg);
        tries++;
        if (!memcmp(ctg, c2, sizeof ctg)) { found = g; break; }
    }
    if (found == (ull)-1) { printf("search failed\n"); return 1; }
    unsigned char buf2[MSG_LEN_BYTES + SEED_BYTES]; pseudoXOF((MSG_LEN_BYTES + SEED_BYTES) * 8, x, 64, buf2);
    unsigned char b3[PKI_LEN + KEM_K_LEN + MSG_LEN_BYTES], k_i[64];
    memcpy(b3, pkA, PKI_LEN); memcpy(b3 + PKI_LEN, K1, KEM_K_LEN); memcpy(b3 + PKI_LEN + KEM_K_LEN, buf2, MSG_LEN_BYTES);
    pseudoXOF(KEM_K_LEN * 8, b3, sizeof b3 * 8, k_i);
    static unsigned char in[TPK_LEN + PKI_LEN + PKR_LEN + C_I_LEN + C_R_LEN + 2 * KEM_K_LEN]; ull off = 0;
    memcpy(in + off, tpk, TPK_LEN); off += TPK_LEN; memcpy(in + off, pkA, PKI_LEN); off += PKI_LEN;
    memcpy(in + off, pkB, PKR_LEN); off += PKR_LEN; memcpy(in + off, m2, C_I_LEN); off += C_I_LEN;
    memcpy(in + off, ct_j, C_R_LEN); off += C_R_LEN; memcpy(in + off, k_i, KEM_K_LEN); off += KEM_K_LEN;
    memcpy(in + off, k_j, KEM_K_LEN); off += KEM_K_LEN;
    unsigned char Katt[256]; pseudoXOF(K_LEN * 8, in, off * 8, Katt);
    printf("wPFS search: hidden=%d bits, tries=%llu, recovered key == session key: %s\n", HIDE_BITS, tries, !memcmp(Katt, ssA, ssal) ? "yes" : "no");
    /* control: without the correct 64-bit input (flip a hinted bit) no candidate in the window matches */
    x_true[0] ^= 0x80; memcpy(x, x_true, 8); ull hits = 0;
    for (ull g = 0; g < (1ULL << HIDE_BITS); g++) {
        for (int b = 0; b < HIDE_BITS; b++) { int byte = 7 - b / 8, bit = b % 8; if ((g >> b) & 1) x[byte] |= (1u << bit); else x[byte] &= ~(1u << bit); }
        unsigned char bb[MSG_LEN_BYTES + SEED_BYTES], ctg[ZEN_INDCPA_CIPHERTEXT_LEN_BYTES], tp[TPK_LEN];
        pseudoXOF((MSG_LEN_BYTES + SEED_BYTES) * 8, x, 64, bb); memcpy(tp, tpk, TPK_LEN); pke_enc(tp, bb, bb + MSG_LEN_BYTES, ctg);
        hits += !memcmp(ctg, c2, sizeof ctg);
    }
    printf("control (wrong hint bit): matches=%llu\n", hits);
    return 0;
}
