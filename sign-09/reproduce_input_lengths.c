/* AddressSanitizer driver for the submitted DOVE-128 API. */
#include "SIG_AlgorithmInstance.h"
#include "drng.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DRNG_ctx drng_algorithm;

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    const char *mode = argv[1];
    size_t pk_size = strcmp(mode, "short-pk") == 0 ? 1 : sig_get_pk_len_bytes();
    size_t sk_size = strcmp(mode, "short-sk") == 0 ? 1 : sig_get_sk_len_bytes();
    size_t sn_size = strcmp(mode, "short-sn") == 0 ? 1 : sig_get_sn_len_bytes();
    unsigned char *pk = calloc(pk_size, 1);
    unsigned char *sk = calloc(sk_size, 1);
    unsigned char *sn = calloc(sn_size, 1);
    unsigned char message[1] = {0};
    if (!pk || !sk || !sn) return 2;

    int rc;
    if (strcmp(mode, "short-pk") == 0 || strcmp(mode, "short-sn") == 0) {
        rc = sig_verify(pk, pk_size, sn, sn_size, message, 0);
    } else if (strcmp(mode, "short-sk") == 0 || strcmp(mode, "seed-only-sk") == 0) {
        unsigned long long out_len = sn_size;
        rc = sig_sign(sk, sk_size, message, 0, sn, &out_len);
    } else {
        return 2;
    }
    fprintf(stderr, "NO ASAN ERROR: mode=%s rc=%d\n", mode, rc);
    free(pk);
    free(sk);
    free(sn);
    return 1;
}
