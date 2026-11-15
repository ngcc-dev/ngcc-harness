/* kem-21-4: build Viper ciphertexts whose mu field is arbitrary instead of
 * H32(Kbar), with everything else honest, and check that decapsulation
 * accepts them (returns the key of the valid path). The control flips one
 * bit of u and must not return that key. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "kem.c"      /* the archived KEM, for its static hash helpers */

static uint64_t st = 88172645463325252ULL;

int randombytes(unsigned char *x, unsigned long long n)   /* replaces viper_api_rng.c */
{
    for (unsigned long long i = 0; i < n; i++) {
        st ^= st << 13; st ^= st >> 7; st ^= st << 17;
        x[i] = (unsigned char)(st >> 24);
    }
    return 0;
}

int main(void)
{
    static unsigned char pk[VIPER_PUBLICKEYBYTES], sk[VIPER_SECRETKEYBYTES], ct[VIPER_CIPHERTEXTBYTES];
    unsigned char m[VIPER_MSGBYTES], hpk[32], buf[VIPER_MSGBYTES + 32];
    unsigned char kr[VIPER_SSBYTES + VIPER_FALLBACK_KEY_BYTES], hct[32];
    unsigned char omega[VIPER_FALLBACK_KEY_BYTES + VIPER_MU_BYTES], kdfin[VIPER_SSBYTES + 32];
    unsigned char ss[VIPER_SSBYTES], ss2[VIPER_SSBYTES];
    int accepted = 0, control = 0;

    crypto_kem_keypair(pk, sk);
    for (int t = 0; t < 50; t++) {
        randombytes(m, sizeof m);
        h32(hpk, pk, VIPER_PUBLICKEYBYTES);
        memcpy(buf, m, sizeof m);
        memcpy(buf + sizeof m, hpk, 32);
        shake128(kr, sizeof kr, buf, sizeof buf);
        memcpy(omega, kr + VIPER_SSBYTES, VIPER_FALLBACK_KEY_BYTES);
        randombytes(omega + VIPER_FALLBACK_KEY_BYTES, VIPER_MU_BYTES);   /* not H32(Kbar) */
        viper_pke_enc(ct, pk, m, omega);
        for (int pass = 0; pass < 2; pass++) {
            ct[0] ^= (unsigned char)pass;
            h32(hct, ct, VIPER_CIPHERTEXTBYTES);
            memcpy(kdfin, kr, VIPER_SSBYTES);
            memcpy(kdfin + VIPER_SSBYTES, hct, 32);
            hbytes(ss, VIPER_SSBYTES, kdfin, sizeof kdfin);
            crypto_kem_dec(ss2, ct, sk);
            *(pass ? &control : &accepted) += !memcmp(ss, ss2, VIPER_SSBYTES);
        }
    }
    printf("arbitrary_mu_accepted=%d control_accepted=%d\n", accepted, control);
    return 0;
}
