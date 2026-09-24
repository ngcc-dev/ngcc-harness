#include <stdio.h>
#include <stdlib.h>

#include "../src/api_frostcc128.h"
enum { PK128 = CRYPTO_PUBLICKEYBYTES, CT128 = CRYPTO_CIPHERTEXTBYTES, SK128 = CRYPTO_SECRETKEYBYTES, SS128 = CRYPTO_BYTES };
#undef CRYPTO_SECRETKEYBYTES
#undef CRYPTO_PUBLICKEYBYTES
#undef CRYPTO_BYTES
#undef CRYPTO_CIPHERTEXTBYTES
#undef CRYPTO_ALGNAME
#include "../src/api_frostcc192.h"
enum { PK192 = CRYPTO_PUBLICKEYBYTES, CT192 = CRYPTO_CIPHERTEXTBYTES, SK192 = CRYPTO_SECRETKEYBYTES, SS192 = CRYPTO_BYTES };
#undef CRYPTO_SECRETKEYBYTES
#undef CRYPTO_PUBLICKEYBYTES
#undef CRYPTO_BYTES
#undef CRYPTO_CIPHERTEXTBYTES
#undef CRYPTO_ALGNAME
#include "../src/api_frostcc256.h"
enum { PK256 = CRYPTO_PUBLICKEYBYTES, CT256 = CRYPTO_CIPHERTEXTBYTES, SK256 = CRYPTO_SECRETKEYBYTES, SS256 = CRYPTO_BYTES };
#undef CRYPTO_SECRETKEYBYTES
#undef CRYPTO_PUBLICKEYBYTES
#undef CRYPTO_BYTES
#undef CRYPTO_CIPHERTEXTBYTES
#undef CRYPTO_ALGNAME
#include "../src/api_frostcc384.h"
enum { PK384 = CRYPTO_PUBLICKEYBYTES, CT384 = CRYPTO_CIPHERTEXTBYTES, SK384 = CRYPTO_SECRETKEYBYTES, SS384 = CRYPTO_BYTES };
#undef CRYPTO_SECRETKEYBYTES
#undef CRYPTO_PUBLICKEYBYTES
#undef CRYPTO_BYTES
#undef CRYPTO_CIPHERTEXTBYTES
#undef CRYPTO_ALGNAME
#include "../src/api_frostcc512.h"
enum { PK512 = CRYPTO_PUBLICKEYBYTES, CT512 = CRYPTO_CIPHERTEXTBYTES, SK512 = CRYPTO_SECRETKEYBYTES, SS512 = CRYPTO_BYTES };

typedef struct {
    const char *name;
    unsigned n, m, ell_r, ell_s, qbits, eta_s, eta_r, b_msg, t_pk, t_u, t_v;
    unsigned macro_pk, macro_ct, macro_sk, macro_ss;
    unsigned paper_pk, paper_ct, paper_sk, paper_ss;
} profile_t;

static unsigned ceil_log2_u(unsigned x)
{
    unsigned bits = 0;
    unsigned v = x - 1u;
    while (v != 0u) {
        bits++;
        v >>= 1;
    }
    return bits;
}

static unsigned calc_pk(const profile_t *p) { return 32u + p->m * p->ell_r * p->t_pk / 8u; }
static unsigned calc_ct(const profile_t *p) { return 32u + p->n * p->ell_s * p->t_u / 8u + p->ell_r * p->ell_s * p->t_v / 8u; }
static unsigned calc_skpke(const profile_t *p) { return p->n * p->ell_r * ceil_log2_u(2u * p->eta_s + 1u) / 8u; }
static unsigned calc_sk(const profile_t *p) { return calc_skpke(p) + calc_pk(p) + 32u + p->macro_ss; }
static unsigned calc_ss(const profile_t *p) { return p->b_msg * p->ell_r * p->ell_s / 8u; }

int main(void)
{
    const profile_t profiles[] = {
        { "MAMBA-Frost-CC-128", 512, 512, 8, 8, 15, 2, 2, 2, 10, 10, 5, PK128, CT128, SK128, SS128, 5152, 5192, 6736, 16 },
        { "MAMBA-Frost-CC-192", 880, 880, 8, 8, 16, 1, 1, 3, 11, 11, 6, PK192, CT192, SK192, SS192, 9712, 9760, 11528, 24 },
        { "MAMBA-Frost-CC-256", 1288, 1288, 8, 8, 16, 1, 1, 4, 13, 12, 8, PK256, CT256, SK256, SS256, 16776, 15552, 19416, 32 },
        { "MAMBA-Frost-CC-384", 1928, 1928, 12, 8, 16, 1, 1, 4, 13, 13, 9, PK384, CT384, SK384, SS384, 37628, 25204, 43492, 48 },
        { "MAMBA-Frost-CC-512", 2600, 2600, 16, 8, 16, 1, 1, 4, 14, 14, 7, PK512, CT512, SK512, SS512, 72832, 36544, 83328, 64 },
    };
    int ok = 1;
    for (size_t i = 0; i < sizeof(profiles)/sizeof(profiles[0]); i++) {
        const profile_t *p = &profiles[i];
        unsigned epk = calc_pk(p), ect = calc_ct(p), eskpke = calc_skpke(p), esk = calc_sk(p), ess = calc_ss(p);
        printf("%s n=%u m=%u ell_r=%u ell_s=%u q=2^%u eta_s=%u eta_r=%u b_msg=%u t_pk=%u t_u=%u t_v=%u | formula pk=%u ct=%u sk_PKE=%u sk_KEM=%u ss=%u | macros pk=%u ct=%u sk_KEM=%u ss=%u\n",
               p->name, p->n, p->m, p->ell_r, p->ell_s, p->qbits, p->eta_s, p->eta_r, p->b_msg, p->t_pk, p->t_u, p->t_v,
               epk, ect, eskpke, esk, ess, p->macro_pk, p->macro_ct, p->macro_sk, p->macro_ss);
        if (p->paper_pk && (epk != p->paper_pk || ect != p->paper_ct || esk != p->paper_sk || ess != p->paper_ss)) ok = 0;
        if (p->macro_pk != epk || p->macro_ct != ect || p->macro_sk != esk || p->macro_ss != ess) ok = 0;
    }
    if (!ok) {
        fprintf(stderr, "Frost-CC size test FAILED\n");
        return EXIT_FAILURE;
    }
    puts("Frost-CC size test PASSED");
    return EXIT_SUCCESS;
}
