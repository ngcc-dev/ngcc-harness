/* kem-40-6: YuanYang's encapsulation ignores a failure of the hash at
 * kem.c:163. tmphash then still holds msg || Hash(pk) || 42, so the
 * encryption coins are Hash(pk) || 45 (public) and the pre-key is msg itself.
 *
 * Built with kem.c included (so the attacker can call the scheme's own static
 * yy_encode_message) and -Wl,--wrap=malloc. When armed, the wrapper fails
 * exactly one allocation of the size pseudohash_1024 first requests for the
 * 163 hash ((2*YY_SEC + 8)/8 + 2 bytes). The attacker, holding only pk and
 * ct, derives the public coins, strips the mask from the ciphertext tail to
 * get msg, re-encrypts to check the whole ciphertext, and hashes
 * msg || ct || 43 as the encapsulator does. Prints rc and the comparisons. */
#include "kem.c"
#include <stdlib.h>
#include "KEM_AlgorithmInstance.h"

DRNG_ctx drng_algorithm;
void *__real_malloc(size_t);
static size_t fail_size, n_alloc, fail_at;

void *__wrap_malloc(size_t n)
{
    n_alloc++;
    if (fail_size && n == fail_size) {
        fail_size = 0;
        fail_at = n_alloc;
        return NULL;
    }
    return __real_malloc(n);
}

/* Attacker: recompute the shared secret from (pk, ct) under public coins. */
static int attack(unsigned char *ss_out, const unsigned char *pk, unsigned long long pkl,
                  const unsigned char *ct, unsigned long long ctl)
{
    uint8_t buf[3000] = {0}, seed[YY_SEC / 8 + 1], zero[YY_SEC / 8] = {0}, msg[YY_SEC / 8];
    uint8_t ct0[3000], ct1[3000];
    unsigned long long l0, l1;
    uint16_t h[YUANYANG_D];

    yuanyang_decode_uniform(h, YUANYANG_D, YUANYANG_Q, 4, (unsigned char *)pk);
    pseudohash(512, pk, pkl * 8, buf + YY_SEC / 8);          /* Hash(pk) as placed in tmphash */
    buf[2 * YY_SEC / 8] = 45;                                 /* kem.c:164 */
    memcpy(seed, buf + YY_SEC / 8, sizeof seed);
    yy_encode_message(ct0, &l0, seed, zero, h);               /* tail = mask */
    for (unsigned i = 0; i < YY_SEC / 8; i++) msg[i] = ct[ctl - YY_SEC / 8 + i] ^ ct0[l0 - YY_SEC / 8 + i];
    yy_encode_message(ct1, &l1, seed, msg, h);
    if (l1 != ctl || memcmp(ct1, ct, ctl) != 0) return 0;     /* coins were not the public ones */
    memcpy(buf, msg, YY_SEC / 8);                             /* pre-key K = msg (hash 163 skipped) */
    memcpy(buf + YY_SEC / 8, ct, ctl);
    buf[YY_SEC / 8 + ctl] = 43;
    pseudohash(512, buf, YY_SEC + ctl * 8 + 8, buf);
    memcpy(ss_out, buf, YY_SEC / 8);
    return 1;
}

int main(void)
{
    unsigned char seed[48];
    unsigned long long pkl = kem_get_pk_len_bytes(), skl = kem_get_sk_len_bytes();
    unsigned long long ssl = kem_get_ss_len_bytes(), ctl = kem_get_ct_len_bytes();
    unsigned char *pk = malloc(pkl), *sk = malloc(skl), *ct = malloc(ctl);
    unsigned char *ss = malloc(ssl), *ss_dec = malloc(ssl), *ss_att = malloc(ssl);
    size_t hsz = (2 * YY_SEC + 8) / 8 + 2;
    int rc, rd, ok;

    memset(seed, 5, sizeof seed);
    init_random_number(&drng_algorithm, seed, 48);
    if (kem_keygen(pk, &pkl, sk, &skl) != 0) return 2;

    rc = kem_enc(pk, pkl, ss, &ssl, ct, &ctl);
    rd = kem_dec(sk, skl, ct, ctl, ss_dec, &ssl);
    ok = attack(ss_att, pk, pkl, ct, ctl);
    printf("control: rc=%d rd=%d dec_agrees=%d attacker_ok=%d\n", rc, rd, !memcmp(ss, ss_dec, ssl), ok);

    memset(ss, 0xEE, ssl);
    n_alloc = 0; fail_at = 0; fail_size = hsz;
    rc = kem_enc(pk, pkl, ss, &ssl, ct, &ctl);
    fail_size = 0;
    ok = attack(ss_att, pk, pkl, ct, ctl);
    rd = kem_dec(sk, skl, ct, ctl, ss_dec, &ssl);
    printf("faulted: alloc=%zu size=%zu rc=%d attacker_ok=%d attacker_matches=%d dec_agrees=%d\n", fail_at, hsz,
           rc, ok, ok && !memcmp(ss, ss_att, ssl), !memcmp(ss, ss_dec, ssl));
    return 0;
}
