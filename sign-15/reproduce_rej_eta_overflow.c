/* sign-15-16: key generation with a seed given in hex on the command line. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int ngcc_seed(const unsigned char *, unsigned long long);
unsigned long long sig_get_pk_len_bytes(void), sig_get_sk_len_bytes(void);
int sig_keygen(unsigned char *, unsigned long long *, unsigned char *, unsigned long long *);

int main(int argc, char **argv)
{
    unsigned char seed[48] = {0};
    unsigned long long pl = sig_get_pk_len_bytes(), sl = sig_get_sk_len_bytes();
    unsigned char *pk = malloc(pl), *sk = malloc(sl);
    unsigned long v = argc > 1 ? strtoul(argv[1], NULL, 10) : 0;
    memcpy(seed, &v, sizeof v);
    if (!pk || !sk || ngcc_seed(seed, sizeof seed))
        return 2;
    return sig_keygen(pk, &pl, sk, &sl) ? 3 : 0;
}
