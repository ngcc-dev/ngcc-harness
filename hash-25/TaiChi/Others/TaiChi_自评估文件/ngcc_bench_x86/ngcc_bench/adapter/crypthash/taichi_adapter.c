#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "crypthash_adapter.h"
#include "registry.h"

#define TAICHI_512_DIGEST_LEN      64
#define TAICHI_768_DIGEST_LEN      96
#define TAICHI_1024_DIGEST_LEN     128

#define TAICHI_512_BLOCK_LEN       176
#define TAICHI_768_BLOCK_LEN       144
#define TAICHI_1024_BLOCK_LEN      112

extern int TaiChi_512_ref_CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest);
extern int TaiChi_768_ref_CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest);
extern int TaiChi_1024_ref_CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest);

extern int TaiChi_512_op_per_CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest);
extern int TaiChi_768_op_per_CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest);
extern int TaiChi_1024_op_per_CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest);

extern int TaiChi_512_op_res_CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest);
extern int TaiChi_768_op_res_CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest);
extern int TaiChi_1024_op_res_CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest);

#define DEFINE_TAICHI_ADAPTER(tag, alg_enum, alg_name_str, digest_bytes, block_bytes, lib_path, crypt_hash_fn) \
    static size_t tag##_get_digest_len(void) { \
        return digest_bytes; \
    } \
    static size_t tag##_get_block_len(void) { \
        return block_bytes; \
    } \
    static int tag##_do_hash(int digest_len_bits, const uint8_t *msg, size_t msg_len_bits, uint8_t *digest) { \
        return crypt_hash_fn(digest_len_bits, msg, (unsigned long long)msg_len_bits, digest); \
    } \
    static HASH_METHOD tag##_method = { \
        .get_digest_len = tag##_get_digest_len, \
        .get_block_len = tag##_get_block_len, \
        .do_hash = tag##_do_hash, \
    }; \
    static ALGORITHM tag##_alg = { \
        .alg_id = alg_enum, \
        .type = ALG_HASH, \
        .alg_name = alg_name_str, \
        .author_name = "TaiChi", \
        .lib_name = lib_path, \
        .method = &tag##_method, \
    }; \
    void register_##tag(void) { \
        if (registry_algorithm(&tag##_alg)) { \
            fprintf(stderr, "Failed to register %s algorithm.\n", alg_name_str); \
        } \
    }

DEFINE_TAICHI_ADAPTER(
    taichi_512_ref,
    TAICHI_512_REF,
    "TaiChi-512-ref",
    TAICHI_512_DIGEST_LEN,
    TAICHI_512_BLOCK_LEN,
    "./bench/bench_worker_taichi_512_ref",
    TaiChi_512_ref_CryptHash
)

DEFINE_TAICHI_ADAPTER(
    taichi_768_ref,
    TAICHI_768_REF,
    "TaiChi-768-ref",
    TAICHI_768_DIGEST_LEN,
    TAICHI_768_BLOCK_LEN,
    "./bench/bench_worker_taichi_768_ref",
    TaiChi_768_ref_CryptHash
)

DEFINE_TAICHI_ADAPTER(
    taichi_1024_ref,
    TAICHI_1024_REF,
    "TaiChi-1024-ref",
    TAICHI_1024_DIGEST_LEN,
    TAICHI_1024_BLOCK_LEN,
    "./bench/bench_worker_taichi_1024_ref",
    TaiChi_1024_ref_CryptHash
)

DEFINE_TAICHI_ADAPTER(
    taichi_512_op_per,
    TAICHI_512_OP_PER,
    "TaiChi-512-op-per",
    TAICHI_512_DIGEST_LEN,
    TAICHI_512_BLOCK_LEN,
    "./bench/bench_worker_taichi_512_op_per",
    TaiChi_512_op_per_CryptHash
)

DEFINE_TAICHI_ADAPTER(
    taichi_768_op_per,
    TAICHI_768_OP_PER,
    "TaiChi-768-op-per",
    TAICHI_768_DIGEST_LEN,
    TAICHI_768_BLOCK_LEN,
    "./bench/bench_worker_taichi_768_op_per",
    TaiChi_768_op_per_CryptHash
)

DEFINE_TAICHI_ADAPTER(
    taichi_1024_op_per,
    TAICHI_1024_OP_PER,
    "TaiChi-1024-op-per",
    TAICHI_1024_DIGEST_LEN,
    TAICHI_1024_BLOCK_LEN,
    "./bench/bench_worker_taichi_1024_op_per",
    TaiChi_1024_op_per_CryptHash
)

DEFINE_TAICHI_ADAPTER(
    taichi_512_op_res,
    TAICHI_512_OP_RES,
    "TaiChi-512-op-res",
    TAICHI_512_DIGEST_LEN,
    TAICHI_512_BLOCK_LEN,
    "./bench/bench_worker_taichi_512_op_res",
    TaiChi_512_op_res_CryptHash
)

DEFINE_TAICHI_ADAPTER(
    taichi_768_op_res,
    TAICHI_768_OP_RES,
    "TaiChi-768-op-res",
    TAICHI_768_DIGEST_LEN,
    TAICHI_768_BLOCK_LEN,
    "./bench/bench_worker_taichi_768_op_res",
    TaiChi_768_op_res_CryptHash
)

DEFINE_TAICHI_ADAPTER(
    taichi_1024_op_res,
    TAICHI_1024_OP_RES,
    "TaiChi-1024-op-res",
    TAICHI_1024_DIGEST_LEN,
    TAICHI_1024_BLOCK_LEN,
    "./bench/bench_worker_taichi_1024_op_res",
    TaiChi_1024_op_res_CryptHash
)