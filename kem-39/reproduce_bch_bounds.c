/* kem-39: Weaver BCH decoder stack-array bounds, and kem_dec's unchecked
 * ct_len_bytes. Built by reproduce_bch_bounds.sh against one reference
 * WeaverKEM directory (WEAVER_MODE 1, 3 or 5).
 *
 * Modes (argv[1]):
 *   honest N    N honest keygen/enc/dec round trips on exactly sized heap
 *               buffers; prints how many shared secrets agree.
 *   crafted E   decapsulates a ciphertext with u = 0 and v chosen so that the
 *               noisy low-layer BCH word is the zero codeword with E data-bit
 *               errors (bits 0..E-1, never the last data bit).
 *   short L     decapsulates an honest ciphertext copied into a heap buffer
 *               of exactly L bytes, declared with length L.
 *   msgs        prints a digest of indcpa_dec outputs over a fixed corpus
 *               (honest, crafted E = 0..t, 400 random ciphertexts) so builds
 *               can be compared byte for byte.
 *   observe     (only with -DOBSERVE and the sed-instrumented decoder copies)
 *               runs the corpus plus direct decoder sweeps and reports every
 *               elp->c[j+k] access beyond 2t and every roots[t] store.
 *   enum        symbolic sweep of compute_error_locator_polynomial's integer
 *               state over all 2^t zero/nonzero discrepancy patterns.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "params.h"
#include "poly.h"
#include "msgenc.h"
#include "indcpa.h"
#include "bch.h"
#include "drng.h"

DRNG_ctx drng_algorithm;
unsigned long long kem_get_pk_len_bytes(void), kem_get_sk_len_bytes(void),
    kem_get_ct_len_bytes(void), kem_get_ss_len_bytes(void);
int kem_keygen(unsigned char *, unsigned long long *, unsigned char *, unsigned long long *);
int kem_enc(unsigned char *, unsigned long long, unsigned char *, unsigned long long *,
            unsigned char *, unsigned long long *);
int kem_dec(unsigned char *, unsigned long long, unsigned char *, unsigned long long,
            unsigned char *, unsigned long long *);

#if WEAVER_MODE == 1
#define T_LOW 2
#define DATA_BITS 16 /* ELL_DDOT_BYTES = 2 */
#define ECC_LOW 10
#elif WEAVER_MODE == 3
#define T_LOW 4
#define DATA_BITS 36 /* ELL_DDOT_NIBBLES = 9 */
#define ECC_LOW 24
#else
#define T_LOW 7
#define DATA_BITS 64 /* ELL_DDOT_BYTES = 8 */
#define ECC_LOW 49
#endif

#ifdef OBSERVE
/* called by the sed-instrumented copies right before the original statement */
static long elp_acc, elp_oob, elp_oob_nonzero, elp_max_idx;
static long root_oob[2];
static unsigned root_last[2], root_final_min[2] = {~0u, ~0u}, root_final_max[2];
static int root_flag[2];
void obs_elp(int idx, unsigned val, int t)
{
    elp_acc++;
    if (idx > elp_max_idx) elp_max_idx = idx;
    if (idx > 2 * t) { elp_oob++; if (val) elp_oob_nonzero++; }
}
void obs_root(int tag, unsigned count, unsigned val, unsigned t)
{
    if (count >= t) { root_flag[tag] = 1; root_last[tag] = val; }
}
static void obs_close(void)
{
    for (int g = 0; g < 2; g++)
        if (root_flag[g]) {
            root_oob[g]++;
            if (root_last[g] < root_final_min[g]) root_final_min[g] = root_last[g];
            if (root_last[g] > root_final_max[g]) root_final_max[g] = root_last[g];
            root_flag[g] = 0;
        }
}
#else
static void obs_close(void) {}
#endif

static unsigned char *pk, *sk, *ct, *ss1, *ss2;
static unsigned long long PL, SL, CL, XL;

static void setup(void)
{
    unsigned char seed[48];
    unsigned long long a, b;
    for (int i = 0; i < 48; i++) seed[i] = (unsigned char)(7 * i + 3);
    init_random_number(&drng_algorithm, seed, 48);
    PL = kem_get_pk_len_bytes(); SL = kem_get_sk_len_bytes();
    CL = kem_get_ct_len_bytes(); XL = kem_get_ss_len_bytes();
    pk = malloc(PL); sk = malloc(SL); ct = malloc(CL); ss1 = malloc(XL); ss2 = malloc(XL);
    if (kem_keygen(pk, &a, sk, &b)) exit(3);
}

/* u = 0, so the decrypted polynomial is v. low != 0: v = q/4 on the four D4
 * positions of low-codeword bits 0..e-1 (low word = zero codeword + e data
 * errors, high word all zero). low == 0: v = q/2 at coefficients 0..e-1,
 * which leaves every low bit 0 and sets high-codeword bits 0..e-1 (zero
 * codeword + e data errors). */
static void crafted2(unsigned char *c, int e, int low)
{
    poly v;
    memset(c, 0, CL);
    memset(&v, 0, sizeof v);
    for (int b = 0; b < e; b++) {
        if (!low) { v.coeffs[b] = WEAVER_Q / 2; continue; }
        for (int k = 0; k < 4; k++) v.coeffs[b + k * D4_STEP_LEN] = WEAVER_Q / 4;
    }
    poly_compress(c + WEAVER_POLYVECCOMPRESSEDBYTES, &v);
}
static void crafted(unsigned char *c, int e) { crafted2(c, e, 1); }

static uint64_t fnv(uint64_t h, const unsigned char *p, size_t n)
{
    while (n--) { h ^= *p++; h *= 0x100000001b3ULL; }
    return h;
}

static uint64_t corpus(void)
{
    unsigned char m[WEAVER_INDCPA_MSGBYTES];
    unsigned long long a, c;
    uint64_t h = 0xcbf29ce484222325ULL;
    for (int r = 0; r < 20; r++) {
        kem_enc(pk, PL, ss1, &a, ct, &c);
        indcpa_dec(m, ct, sk); obs_close(); h = fnv(h, m, sizeof m);
    }
    for (int e = 0; e <= T_LOW; e++)
        for (int low = 0; low < 2; low++) {
            crafted2(ct, e, low);
            indcpa_dec(m, ct, sk); obs_close(); h = fnv(h, m, sizeof m);
        }
    srand(1);
    for (int r = 0; r < 400; r++) {
        for (unsigned long long i = 0; i < CL; i++) ct[i] = (unsigned char)rand();
        indcpa_dec(m, ct, sk); obs_close(); h = fnv(h, m, sizeof m);
    }
    return h;
}

static void symbolic(void)
{
    /* integer state of bch_low.c:156-219 (deg, pelp.deg, pp) depends only on
     * which discrepancies d are zero; a store can change memory only when
     * d != 0, j <= pelp.deg and j <= i */
    int t = T_LOW, touch = 0, change = 0;
    for (int pat = 0; pat < (1 << t); pat++) {
        int L = 0, Lp = 0, pp = -1;
        for (int i = 0; i < t; i++) {
            int k = 2 * i - pp, tmp;
            if (i + k > touch) touch = i + k;
            if (pat >> i & 1) {
                int j = i < Lp ? i : Lp;
                if (j + k > change) change = j + k;
                tmp = Lp + k;
            } else tmp = Lp;
            if (tmp > L) { Lp = L; L = tmp; pp = 2 * i; }
        }
    }
    printf("t=%d valid=0..%d max_index_touched=%d max_index_changed=%d\n", t, 2 * t, touch, change);
}

int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "honest";
    unsigned long long a, c;
    if (!strcmp(mode, "enum")) { symbolic(); return 0; }
    setup();
    if (!strcmp(mode, "honest")) {
        int n = argc > 2 ? atoi(argv[2]) : 1, ok = 0;
        for (int r = 0; r < n; r++) {
            if (kem_enc(pk, PL, ss1, &a, ct, &c)) return 4;
            ok += kem_dec(sk, SL, ct, CL, ss2, &a) == 0 && !memcmp(ss1, ss2, XL);
        }
        printf("honest agree=%d/%d\n", ok, n);
    } else if (!strcmp(mode, "crafted") || !strcmp(mode, "crafted-high")) {
        int e = atoi(argv[2]), low = !strcmp(mode, "crafted");
        crafted2(ct, e, low);
        int rc = kem_dec(sk, SL, ct, CL, ss2, &a);
        printf("%s errors=%d rc=%d\n", mode, e, rc);
    } else if (!strcmp(mode, "short")) {
        unsigned long long L = argc > 2 ? strtoull(argv[2], 0, 10) : CL - 1;
        if (L > CL) return 2;
        kem_enc(pk, PL, ss1, &a, ct, &c);
        unsigned char *t = malloc(L ? L : 1);
        memcpy(t, ct, L);
        int rc = kem_dec(sk, SL, t, L, ss2, &a);
        printf("declared=%llu rc=%d agrees=%d\n", L, rc, !memcmp(ss1, ss2, XL));
        free(t);
    } else if (!strcmp(mode, "msgs")) {
        printf("corpus digest %016llx\n", (unsigned long long)corpus());
#ifdef OBSERVE
    } else if (!strcmp(mode, "observe")) {
        int ok = 0;
        for (int r = 0; r < 200; r++) {
            kem_enc(pk, PL, ss1, &a, ct, &c);
            ok += kem_dec(sk, SL, ct, CL, ss2, &a) == 0 && !memcmp(ss1, ss2, XL);
            obs_close();
        }
        long hon_oob = elp_oob, hon_nz = elp_oob_nonzero, hon_root = root_oob[0] + root_oob[1];
        long root_by_e[T_LOW + 1];
        for (int e = 0; e <= T_LOW; e++) {
            long before = root_oob[0];
            crafted(ct, e); kem_dec(sk, SL, ct, CL, ss2, &a); obs_close();
            root_by_e[e] = root_oob[0] - before;
        }
        corpus();
        /* direct decoder sweep over the low-layer remainder (data = 0) */
        uint8_t buf[32], ecc[16];
        long sweep = 0;
        int eb = (ECC_LOW + 7) / 8;
        uint64_t lim = ECC_LOW <= 24 ? (1ULL << ECC_LOW) : (1ULL << 22);
        uint64_t x = 0x9e3779b97f4a7c15ULL;
        for (uint64_t r = 0; r < lim; r++) {
            uint64_t v = r;
            if (ECC_LOW > 24) { x ^= x << 13; x ^= x >> 7; x ^= x << 17; v = x; }
            memset(buf, 0, sizeof buf);
            for (int i = 0; i < eb; i++) ecc[i] = (uint8_t)(v >> (8 * (eb - 1 - i)));
#if WEAVER_MODE == 3
            decode_bch_low_nibbles(buf, 9, ecc);
#else
            decode_bch_low(buf, DATA_BITS / 8, ecc);
#endif
            obs_close(); sweep++;
        }
        printf("honest: agree=%d/200 elp_oob_accesses=%ld nonzero_oob_xors=%ld roots[t]_stores=%ld\n",
               ok, hon_oob, hon_nz, hon_root);
        printf("crafted:");
        for (int e = 0; e <= T_LOW; e++) printf(" e=%d:roots[t]=%ld", e, root_by_e[e]);
        printf("\n");
        printf("all: elp accesses=%ld max_index=%ld (valid 0..%d) oob=%ld nonzero_oob_xors=%ld; "
               "decoder sweep=%ld (%s); roots[t]_stores low=%ld high=%ld final_value_low=%u..%u (ecc_bits=%d)\n",
               elp_acc, elp_max_idx, 2 * T_LOW, elp_oob, elp_oob_nonzero, sweep,
               ECC_LOW <= 24 ? "exhaustive" : "random", root_oob[0], root_oob[1],
               root_oob[0] ? root_final_min[0] : 0, root_final_max[0], ECC_LOW);
#endif
    } else return 2;
    return 0;
}
