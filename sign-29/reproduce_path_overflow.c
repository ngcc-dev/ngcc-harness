#include "SIG_TINS128.h"
#include "drng.h"
#include "params.h"
#include <stdio.h>
#include <stdlib.h>

DRNG_ctx drng_algorithm;

int main(void) {
    unsigned long long pk_len = sig_get_pk_len_bytes();
    unsigned long long sn_len = sig_get_sn_len_bytes();
    if (getenv("OVERLONG")) sn_len += 32 * NODE_SIZE;
    unsigned char *pk = calloc(pk_len, 1);
    unsigned char *sn = calloc(sn_len, 1);
    unsigned char message = 0;
    if (!pk || !sn) return 2;
    int result = sig_verify(pk, pk_len, sn, sn_len, &message, 1);
    printf("length=%llu verify=%d\n", sn_len, result);
    free(pk);
    free(sn);
    return 0;
}
