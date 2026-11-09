/* sign-15-16: print the range of the s1 coefficients of one generated key. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "params.h"
#include "polyvec.h"
#include "packing.h"

int ngcc_seed(const unsigned char *, unsigned long long);
int sig_keygen(unsigned char *, unsigned long long *, unsigned char *, unsigned long long *);
unsigned long long sig_get_pk_len_bytes(void), sig_get_sk_len_bytes(void);

int main(void)
{
    unsigned char seed[48] = {3};
    unsigned long long pl = sig_get_pk_len_bytes(), sl = sig_get_sk_len_bytes();
    unsigned char *pk = malloc(pl), *sk = malloc(sl);
    unsigned char rho[SEEDBYTES], key[SEEDBYTES], tr[CRHBYTES];
    polyvecl s1;
    polyveck t0;
    int i, j, lo = 1 << 30, hi = -(1 << 30);

    if (!pk || !sk || ngcc_seed(seed, sizeof seed) || sig_keygen(pk, &pl, sk, &sl))
        return 2;
    unpack_sk(rho, key, tr, &s1, &t0, sk);
    for (i = 0; i < L; i++)
        for (j = 0; j < N; j++) {
            int v = (int)s1.vec[i].coeffs[j];
            if (v > (int)(Q / 2))
                v -= Q;
            if (v < lo)
                lo = v;
            if (v > hi)
                hi = v;
        }
    printf("eta=%d s1 range [%d,%d] over %d coefficients\n", (int)ETA, lo, hi, L * N);
    return 0;
}
