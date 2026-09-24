#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../context.h"
#include "../hash.h"
#include "../tfors.h"
#include "../address.h"
#include "../randombytes.h"
#include "../params.h"
#include "../octopus.h"

#define SPX_TFORS_MAX_SIG_BYTES (SPX_TFORS_K * SPX_N + SPX_TFORS_K * SPX_TFORS_HEIGHT * SPX_N)

int main(void)
{
    setbuf(stdout, NULL);

    spx_ctx ctx;
    unsigned char pk1[SPX_TFORS_PK_BYTES];
    unsigned char pk2[SPX_TFORS_PK_BYTES];
    unsigned char sig[SPX_TFORS_BYTES];
    unsigned char m[SPX_TFORS_MSG_BYTES];
    uint32_t tfors_addr[8] = {0};
    uint32_t indices[SPX_TFORS_K];

    randombytes(ctx.sk_seed, SPX_N);
    randombytes(ctx.pub_seed, SPX_N);
    randombytes(m, SPX_TFORS_MSG_BYTES);
    randombytes((unsigned char *)tfors_addr, 8 * sizeof(uint32_t));

    printf("Testing TFORS signature and PK derivation.. ");
    printf("\n  SPX_N = %d", SPX_N);
    printf("\n  SPX_TFORS_K = %d", SPX_TFORS_K);
    printf("\n  SPX_TFORS_A = %d", SPX_TFORS_A);
    printf("\n  SPX_TFORS_HEIGHT = %d", SPX_TFORS_HEIGHT);
    printf("\n  SPX_TFORS_MSG_BYTES = %d", SPX_TFORS_MSG_BYTES);
    printf("\n  SPX_TFORS_MAX_SIG_BYTES = %d\n", SPX_TFORS_BYTES);

    initialize_hash_function(&ctx);

    message_to_indices(indices, m, &ctx);  
    tfors_sign(sig, pk1, indices, &ctx, tfors_addr);
    printf("Signature generated, now verifying...\n");

    tfors_pk_from_sig(pk2, sig, m, &ctx, tfors_addr);

    if (memcmp(pk1, pk2, SPX_TFORS_PK_BYTES)) {
        printf("failed!\n");
        return -1;
    }
    printf("successful.\n");

    return 0;
}