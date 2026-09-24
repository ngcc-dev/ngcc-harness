#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "registry.h"
#include "crypthash_adapter.h"
#include "bench_local.h"

#define HASH_INPUT_SIZE 65536

#ifdef SINGLE_WORKER_HASH_FUNC

#ifndef SINGLE_WORKER_DIGEST_BITS
#error "SINGLE_WORKER_DIGEST_BITS is not defined"
#endif

extern int SINGLE_WORKER_HASH_FUNC(int digest_len_bits,const unsigned char *msg,unsigned long long msg_len_bits,unsigned char *digest);

int main(void) {
    uint8_t *input = NULL;
    uint8_t *digest = NULL;
    size_t digest_len = SINGLE_WORKER_DIGEST_BITS / 8;
    int ret = CRYPTO_SUCCESS;

    input = malloc(HASH_INPUT_SIZE);
    digest = malloc(digest_len);

    if(input == NULL || digest == NULL) {
        free(input);
        free(digest);
        return CRYPTO_FAILED;
    }

    memset(input, 0, HASH_INPUT_SIZE);

    if(SINGLE_WORKER_HASH_FUNC(SINGLE_WORKER_DIGEST_BITS, input, HASH_INPUT_SIZE << 3, digest)) {
        ret = CRYPTO_FAILED;
    }

    free(input);
    free(digest);
    return ret;
}

#else

static void register_all_alg(void) {
    register_taichi_512_ref();
    register_taichi_768_ref();
    register_taichi_1024_ref();

    register_taichi_512_op_per();
    register_taichi_768_op_per();
    register_taichi_1024_op_per();

    register_taichi_512_op_res();
    register_taichi_768_op_res();
    register_taichi_1024_op_res();
}

static int do_hash_operation(const HASH_METHOD *hash_alg) {
    uint8_t *input = NULL;
    uint8_t *md = NULL;
    size_t digest_len = hash_alg->get_digest_len();
    int ret = CRYPTO_SUCCESS;

    input = malloc(HASH_INPUT_SIZE);
    if(input == NULL) {
        return CRYPTO_FAILED;
    }

    md = malloc(digest_len);
    if(md == NULL) {
        free(input);
        return CRYPTO_FAILED;
    }

    memset(input, 0, HASH_INPUT_SIZE);

    if(hash_alg->do_hash((int)(digest_len << 3), input, HASH_INPUT_SIZE << 3, md)) {
        ret = CRYPTO_FAILED;
    }

    free(md);
    free(input);
    return ret;
}

int main(int argc, char *argv[]) {
    int alg_id = 0;
    const ALGORITHM *measured_alg = NULL;

    if(argc != 2) {
        fprintf(stderr, "Didn't specify the algorithm id to bench_worker.\n");
        return -1;
    }

    register_all_alg();

    alg_id = atoi(argv[1]);
    measured_alg = get_algorithm_by_id((uint32_t)alg_id);
    if(measured_alg == NULL) {
        fprintf(stderr, "Specified algorithm not found.\n");
        return -1;
    }

    if(measured_alg->type != ALG_HASH) {
        fprintf(stderr, "Unsupported algorithm type.\n");
        return -1;
    }

    return do_hash_operation((const HASH_METHOD *)measured_alg->method);
}

#endif