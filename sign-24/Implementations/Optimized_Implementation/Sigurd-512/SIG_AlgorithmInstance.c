/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "SIG_AlgorithmInstance.h"

#include "drng.h"
#include "sig_core.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Heap-backed buffers used while rebuilding signer state from the key seeds. */
typedef struct
{
    unsigned long *e_bits;
    unsigned long *y_bits;
    unsigned char *fs_commitment_input;
    RawGF2EX (*H_hat_raw)[COLS];
    RawNZTerms (*H_hat_terms)[COLS];
    RawGF2EX *e_hat_prime_raw;
    uint16_t *y_hat_prime_raw;
    RawGF2EX *ye_raw;
} SignBuffers;

/* Heap-backed buffers used while rebuilding verifier public state. */
typedef struct
{
    unsigned long (*H_bits)[M];
    unsigned long *y_bits;
    unsigned char *fs_commitment_input;
    RawGF2EX (*H_hat_raw)[COLS];
    RawNZTerms (*H_hat_terms)[COLS];
    uint16_t *y_hat_prime_raw;
} VerifyBuffers;

typedef struct
{
    int valid;
    unsigned char sk[SK_LEN_BYTES];
    unsigned char fs_commitment_input[FS_COMMITMENT_INPUT_BYTES];
    unsigned long e_bits[M];
    RawGF2EX H_hat_raw[ROWS][COLS];
    RawNZTerms H_hat_terms[ROWS][COLS];
    RawGF2EX e_hat_prime_raw[N_PRIME_LEN];
    uint16_t y_hat_prime_raw[ROWS];
    RawGF2EX ye_raw[ROWS];
} SignStateCache;

typedef struct
{
    int valid;
    unsigned char pk[PK_LEN_BYTES];
    unsigned char fs_commitment_input[FS_COMMITMENT_INPUT_BYTES];
    RawGF2EX H_hat_raw[ROWS][COLS];
    RawNZTerms H_hat_terms[ROWS][COLS];
    uint16_t y_hat_prime_raw[ROWS];
} VerifyStateCache;

static int g_instance_ready = 0;
static SignStateCache g_sign_state_cache;
static VerifyStateCache g_verify_state_cache;
static ProverWorkspace g_cached_prover_workspace;
static VerifierWorkspace g_cached_verifier_workspace;

extern DRNG_ctx drng_algorithm;

/* Initialize arithmetic helpers exactly once before API use. */
static int ensure_instance_ready(void)
{
    if (!g_instance_ready)
    {
        if (sig_run_self_test() != 0)
        {
            return -1;
        }
        g_instance_ready = 1;
    }
    return 0;
}

/* Read random bytes from the template-provided DRNG instance. */
static int sig_get_random_bytes(unsigned char *out, size_t len)
{
    return get_random_number(&drng_algorithm, out, (unsigned long long)len * 8ULL);
}

static void pack_u16_le(unsigned char out[2], uint16_t value)
{
    out[0] = (unsigned char)(value & 0xFFu);
    out[1] = (unsigned char)(value >> 8);
}

static uint16_t unpack_u16_le(const unsigned char in[2])
{
    return (uint16_t)(in[0] | ((uint16_t)in[1] << 8));
}

static void pack_u32_le(unsigned char out[4], uint32_t value)
{
    out[0] = (unsigned char)(value & 0xFFu);
    out[1] = (unsigned char)((value >> 8) & 0xFFu);
    out[2] = (unsigned char)((value >> 16) & 0xFFu);
    out[3] = (unsigned char)((value >> 24) & 0xFFu);
}

static uint32_t unpack_u32_le(const unsigned char in[4])
{
    return ((uint32_t)in[0]) |
           ((uint32_t)in[1] << 8) |
           ((uint32_t)in[2] << 16) |
           ((uint32_t)in[3] << 24);
}

static void pack_raw_poly(unsigned char out[RAW_POLY_BYTES], const RawGF2EX *poly)
{
    int i;
    for (i = 0; i < RAW_POLY_LEN; ++i)
    {
        pack_u16_le(out + (i * 2), poly->coeffs[i]);
    }
}

static void unpack_raw_poly(RawGF2EX *poly, const unsigned char in[RAW_POLY_BYTES])
{
    int i;
    for (i = 0; i < RAW_POLY_LEN; ++i)
    {
        poly->coeffs[i] = unpack_u16_le(in + (i * 2));
    }
}

/* Serialize the secret key as H_seed || e_seed. */
void sig_pack_secret_key(unsigned char sk[SK_LEN_BYTES], const unsigned char H_seed[SEED_BYTES], const unsigned char e_seed[SEED_BYTES])
{
    memcpy(sk, H_seed, SEED_BYTES);
    memcpy(sk + SEED_BYTES, e_seed, SEED_BYTES);
}

/* Deserialize H_seed and e_seed from the compact secret key format. */
void sig_unpack_secret_key(const unsigned char sk[SK_LEN_BYTES], unsigned char H_seed[SEED_BYTES], unsigned char e_seed[SEED_BYTES])
{
    memcpy(H_seed, sk, SEED_BYTES);
    memcpy(e_seed, sk + SEED_BYTES, SEED_BYTES);
}

static void sig_pack_y_bits(unsigned char packed[Y_BITS_PACKED_BYTES], const unsigned long y_bits[embded_num])
{
    int i;

    memset(packed, 0, Y_BITS_PACKED_BYTES);
    for (i = 0; i < embded_num; ++i)
    {
        if (y_bits[i] != 0u)
        {
            packed[i / 8] |= (unsigned char)(1u << (7 - (i % 8)));
        }
    }
}

static void sig_unpack_y_bits(unsigned long y_bits[embded_num], const unsigned char packed[Y_BITS_PACKED_BYTES])
{
    int i;

    memset(y_bits, 0, sizeof(unsigned long) * embded_num);
    for (i = 0; i < embded_num; ++i)
    {
        y_bits[i] = (unsigned long)((packed[i / 8] >> (7 - (i % 8))) & 1u);
    }
}

static void build_fs_commitment_input(
    unsigned char out[FS_COMMITMENT_INPUT_BYTES],
    const unsigned char pk[PK_LEN_BYTES])
{
    memset(out, 0, HASH_DIGEST_LENGTH);
    memcpy(out + HASH_DIGEST_LENGTH, pk, PK_LEN_BYTES);
}

/* Serialize the public key as H_seed || packed y bits. */
void sig_pack_public_key(unsigned char pk[PK_LEN_BYTES], const unsigned char H_seed[SEED_BYTES], const unsigned long y_bits[embded_num])
{
    unsigned char packed_y[Y_BITS_PACKED_BYTES];

    sig_pack_y_bits(packed_y, y_bits);
    memcpy(pk, H_seed, SEED_BYTES);
    memcpy(pk + SEED_BYTES, packed_y, Y_BITS_PACKED_BYTES);
}

/* Deserialize the compact public key into H_seed and y bits. */
void sig_unpack_public_key(const unsigned char pk[PK_LEN_BYTES], unsigned char H_seed[SEED_BYTES], unsigned long y_bits[embded_num])
{
    memcpy(H_seed, pk, SEED_BYTES);
    sig_unpack_y_bits(y_bits, pk + SEED_BYTES);
}

/* Allocate large signer-side working arrays on the heap. */
static int alloc_sign_buffers(SignBuffers *bufs)
{
    memset(bufs, 0, sizeof(*bufs));

    bufs->e_bits = (unsigned long *)malloc(sizeof(unsigned long) * M);
    bufs->y_bits = (unsigned long *)malloc(sizeof(unsigned long) * embded_num);
    bufs->fs_commitment_input = (unsigned char *)malloc(FS_COMMITMENT_INPUT_BYTES);
    bufs->H_hat_raw = (RawGF2EX (*)[COLS])malloc(sizeof(RawGF2EX) * ROWS * COLS);
    bufs->H_hat_terms = (RawNZTerms (*)[COLS])malloc(sizeof(RawNZTerms) * ROWS * COLS);
    bufs->e_hat_prime_raw = (RawGF2EX *)malloc(sizeof(RawGF2EX) * N_PRIME_LEN);
    bufs->y_hat_prime_raw = (uint16_t *)malloc(sizeof(uint16_t) * ROWS);
    bufs->ye_raw = (RawGF2EX *)malloc(sizeof(RawGF2EX) * ROWS);

    if (bufs->e_bits == NULL || bufs->y_bits == NULL ||
        bufs->fs_commitment_input == NULL ||
        bufs->H_hat_raw == NULL || bufs->H_hat_terms == NULL || bufs->e_hat_prime_raw == NULL ||
        bufs->y_hat_prime_raw == NULL || bufs->ye_raw == NULL)
    {
        return -1;
    }
    return 0;
}

/* Release signer-side heap buffers. */
static void free_sign_buffers(SignBuffers *bufs)
{
    free(bufs->e_bits);
    free(bufs->y_bits);
    free(bufs->fs_commitment_input);
    free(bufs->H_hat_raw);
    free(bufs->H_hat_terms);
    free(bufs->e_hat_prime_raw);
    free(bufs->y_hat_prime_raw);
    free(bufs->ye_raw);
    memset(bufs, 0, sizeof(*bufs));
}

/* Allocate verifier-side working arrays on the heap. */
static int alloc_verify_buffers(VerifyBuffers *bufs)
{
    memset(bufs, 0, sizeof(*bufs));

    bufs->H_bits = (unsigned long (*)[M])malloc(sizeof(unsigned long) * embded_num * M);
    bufs->y_bits = (unsigned long *)malloc(sizeof(unsigned long) * embded_num);
    bufs->fs_commitment_input = (unsigned char *)malloc(FS_COMMITMENT_INPUT_BYTES);
    bufs->H_hat_raw = (RawGF2EX (*)[COLS])malloc(sizeof(RawGF2EX) * ROWS * COLS);
    bufs->H_hat_terms = (RawNZTerms (*)[COLS])malloc(sizeof(RawNZTerms) * ROWS * COLS);
    bufs->y_hat_prime_raw = (uint16_t *)malloc(sizeof(uint16_t) * ROWS);

    if (bufs->H_bits == NULL || bufs->y_bits == NULL || bufs->fs_commitment_input == NULL ||
        bufs->H_hat_raw == NULL || bufs->H_hat_terms == NULL || bufs->y_hat_prime_raw == NULL)
    {
        return -1;
    }
    return 0;
}

/* Release verifier-side heap buffers. */
static void free_verify_buffers(VerifyBuffers *bufs)
{
    free(bufs->H_bits);
    free(bufs->y_bits);
    free(bufs->fs_commitment_input);
    free(bufs->H_hat_raw);
    free(bufs->H_hat_terms);
    free(bufs->y_hat_prime_raw);
    memset(bufs, 0, sizeof(*bufs));
}

static void build_verify_public_state_opt(
    const unsigned char H_seed[SEED_BYTES],
    unsigned long y_bits[embded_num],
    unsigned long H_bits[embded_num][M],
    RawGF2EX H_hat_raw[ROWS][COLS],
    RawNZTerms H_hat_terms[ROWS][COLS],
    uint16_t y_hat_prime_raw[ROWS])
{
    random_H(H_bits, H_seed);
    embed_H_raw(H_hat_raw, H_bits);
    precompute_H_hat_terms(H_hat_terms, H_hat_raw);
    embed_y_raw(y_hat_prime_raw, y_bits);
}

static void sign_cache_store(const unsigned char sk[SK_LEN_BYTES], const SignBuffers *bufs)
{
    memcpy(g_sign_state_cache.sk, sk, SK_LEN_BYTES);
    memcpy(g_sign_state_cache.fs_commitment_input, bufs->fs_commitment_input, sizeof(g_sign_state_cache.fs_commitment_input));
    memcpy(g_sign_state_cache.e_bits, bufs->e_bits, sizeof(g_sign_state_cache.e_bits));
    memcpy(g_sign_state_cache.H_hat_raw, bufs->H_hat_raw, sizeof(g_sign_state_cache.H_hat_raw));
    memcpy(g_sign_state_cache.H_hat_terms, bufs->H_hat_terms, sizeof(g_sign_state_cache.H_hat_terms));
    memcpy(g_sign_state_cache.e_hat_prime_raw, bufs->e_hat_prime_raw, sizeof(g_sign_state_cache.e_hat_prime_raw));
    memcpy(g_sign_state_cache.y_hat_prime_raw, bufs->y_hat_prime_raw, sizeof(g_sign_state_cache.y_hat_prime_raw));
    memcpy(g_sign_state_cache.ye_raw, bufs->ye_raw, sizeof(g_sign_state_cache.ye_raw));
    g_sign_state_cache.valid = 1;
}

static void verify_cache_store(const unsigned char pk[PK_LEN_BYTES], const VerifyBuffers *bufs)
{
    memcpy(g_verify_state_cache.pk, pk, PK_LEN_BYTES);
    memcpy(g_verify_state_cache.fs_commitment_input, bufs->fs_commitment_input, sizeof(g_verify_state_cache.fs_commitment_input));
    memcpy(g_verify_state_cache.H_hat_raw, bufs->H_hat_raw, sizeof(g_verify_state_cache.H_hat_raw));
    memcpy(g_verify_state_cache.H_hat_terms, bufs->H_hat_terms, sizeof(g_verify_state_cache.H_hat_terms));
    memcpy(g_verify_state_cache.y_hat_prime_raw, bufs->y_hat_prime_raw, sizeof(g_verify_state_cache.y_hat_prime_raw));
    g_verify_state_cache.valid = 1;
}

unsigned long long sig_get_pk_len_bytes(void)
{
    return PK_LEN_BYTES;
}

unsigned long long sig_get_sk_len_bytes(void)
{
    return SK_LEN_BYTES;
}

unsigned long long sig_get_sn_len_bytes(void)
{
    return MAX_SIGNATURE_BYTES;
}

/* Serialize the prover transcript into the submission signature layout. */
int sig_serialize_signature(unsigned char *sn, unsigned long long *sn_len_bytes, const ProverOutputs *outputs)
{
    size_t offset = 0;
    size_t i;

    if (sn == NULL || sn_len_bytes == NULL || outputs == NULL)
    {
        return -2;
    }

    memcpy(sn + offset, outputs->root_hash_ptr, total_groups * HASH_DIGEST_LENGTH);
    offset += total_groups * HASH_DIGEST_LENGTH;
    pack_raw_poly(sn + offset, outputs->e_eval_commit_ptr);
    offset += RAW_POLY_BYTES;
    pack_raw_poly(sn + offset, outputs->v_eval_commit_ptr);
    offset += RAW_POLY_BYTES;
    for (i = 0; i < SIGMA_REPETITIONS; ++i)
    {
        pack_raw_poly(sn + offset, &outputs->p_poly_interactive_ptr[i]);
        offset += RAW_POLY_BYTES;
    }
    for (i = 0; i < SIGMA_REPETITIONS; ++i)
    {
        pack_raw_poly(sn + offset, &outputs->rv_out_interactive_ptr[i]);
        offset += RAW_POLY_BYTES;
    }
    for (i = 0; i < 6; ++i)
    {
        pack_raw_poly(sn + offset, &outputs->G_coeffs_raw_ptr[i]);
        offset += RAW_POLY_BYTES;
    }
    for (i = 0; i < N_PRIME_LEN; ++i)
    {
        pack_raw_poly(sn + offset, &outputs->w_hat_prime_raw_ptr[i]);
        offset += RAW_POLY_BYTES;
    }
    pack_u32_le(sn + offset, (uint32_t)outputs->commitment_open_hash_count);
    offset += 4;
    memcpy(sn + offset, outputs->commitment_open_hashes_ptr, outputs->commitment_open_hash_count * HASH_DIGEST_LENGTH);
    offset += outputs->commitment_open_hash_count * HASH_DIGEST_LENGTH;
    for (i = 0; i < opennum; ++i)
    {
        pack_raw_poly(sn + offset, &outputs->e_hat_encoded_commitment_open_output_ptr[i]);
        offset += RAW_POLY_BYTES;
    }

    *sn_len_bytes = (unsigned long long)offset;
    return 0;
}

/* Parse a serialized signature back into verifier-facing transcript objects. */
int sig_deserialize_signature(const unsigned char *sn, unsigned long long sn_len_bytes, RawGF2EX *e_eval_commit, RawGF2EX *v_eval_commit, unsigned char root_hash[total_groups][HASH_DIGEST_LENGTH], RawGF2EX p_poly_interactive[SIGMA_REPETITIONS], RawGF2EX rv_out_interactive[SIGMA_REPETITIONS], RawGF2EX G_coeffs_raw[6], RawGF2EX w_hat_prime_raw[N_PRIME_LEN], unsigned char commitment_open_hashes[MERKLE_MAX_PROOF_HASHES][HASH_DIGEST_LENGTH], size_t *commitment_open_hash_count, RawGF2EX e_hat_encoded_commitment_open_output[opennum])
{
    size_t offset = 0;
    size_t i;

    if (sn == NULL || commitment_open_hash_count == NULL)
    {
        return -2;
    }
    if (sn_len_bytes < (2ULL * RAW_POLY_BYTES) + (unsigned long long)(total_groups * HASH_DIGEST_LENGTH) + 4ULL)
    {
        return -3;
    }

    memcpy(root_hash, sn + offset, total_groups * HASH_DIGEST_LENGTH);
    offset += total_groups * HASH_DIGEST_LENGTH;
    unpack_raw_poly(e_eval_commit, sn + offset);
    offset += RAW_POLY_BYTES;
    unpack_raw_poly(v_eval_commit, sn + offset);
    offset += RAW_POLY_BYTES;
    for (i = 0; i < SIGMA_REPETITIONS; ++i)
    {
        unpack_raw_poly(&p_poly_interactive[i], sn + offset);
        offset += RAW_POLY_BYTES;
    }
    for (i = 0; i < SIGMA_REPETITIONS; ++i)
    {
        unpack_raw_poly(&rv_out_interactive[i], sn + offset);
        offset += RAW_POLY_BYTES;
    }
    for (i = 0; i < 6; ++i)
    {
        unpack_raw_poly(&G_coeffs_raw[i], sn + offset);
        offset += RAW_POLY_BYTES;
    }
    for (i = 0; i < N_PRIME_LEN; ++i)
    {
        unpack_raw_poly(&w_hat_prime_raw[i], sn + offset);
        offset += RAW_POLY_BYTES;
    }

    *commitment_open_hash_count = (size_t)unpack_u32_le(sn + offset);
    offset += 4;
    if (*commitment_open_hash_count > MERKLE_MAX_PROOF_HASHES)
    {
        return -4;
    }
    if (offset + (*commitment_open_hash_count * HASH_DIGEST_LENGTH) + ((size_t)opennum * RAW_POLY_BYTES) > (size_t)sn_len_bytes)
    {
        return -5;
    }

    memcpy(commitment_open_hashes, sn + offset, *commitment_open_hash_count * HASH_DIGEST_LENGTH);
    offset += *commitment_open_hash_count * HASH_DIGEST_LENGTH;
    for (i = 0; i < opennum; ++i)
    {
        unpack_raw_poly(&e_hat_encoded_commitment_open_output[i], sn + offset);
        offset += RAW_POLY_BYTES;
    }

    if (offset != (size_t)sn_len_bytes)
    {
        return -6;
    }
    return 0;
}

/* Template API: generate one compact public/secret key pair. */
int sig_keygen(unsigned char *pk, unsigned long long *pk_len_bytes, unsigned char *sk, unsigned long long *sk_len_bytes)
{
    unsigned char H_seed[SEED_BYTES];
    unsigned char e_seed[SEED_BYTES];
    SignBuffers bufs;
    int rc;

    if (pk == NULL || pk_len_bytes == NULL || sk == NULL || sk_len_bytes == NULL)
    {
        return -2;
    }
    if (ensure_instance_ready() != 0)
    {
        return -1;
    }
    if (alloc_sign_buffers(&bufs) != 0)
    {
        return -7;
    }

    sig_get_random_bytes(H_seed, sizeof(H_seed));
    sig_get_random_bytes(e_seed, sizeof(e_seed));

    sig_build_public_state_direct(H_seed, e_seed, bufs.e_bits, bufs.y_bits, bufs.H_hat_raw, bufs.H_hat_terms, bufs.e_hat_prime_raw, bufs.y_hat_prime_raw, bufs.ye_raw);
    sig_pack_public_key(pk, H_seed, bufs.y_bits);
    build_fs_commitment_input(bufs.fs_commitment_input, pk);
    sig_pack_secret_key(sk, H_seed, e_seed);
    sign_cache_store(sk, &bufs);

    {
        VerifyBuffers verify_bufs;
        memset(&verify_bufs, 0, sizeof(verify_bufs));
        verify_bufs.fs_commitment_input = bufs.fs_commitment_input;
        verify_bufs.H_hat_raw = bufs.H_hat_raw;
        verify_bufs.H_hat_terms = bufs.H_hat_terms;
        verify_bufs.y_hat_prime_raw = bufs.y_hat_prime_raw;
        verify_cache_store(pk, &verify_bufs);
    }

    *pk_len_bytes = PK_LEN_BYTES;
    *sk_len_bytes = SK_LEN_BYTES;
    rc = 0;
    free_sign_buffers(&bufs);
    return rc;
}

/* Template API: rebuild private state from SK and sign the input message. */
int sig_sign(unsigned char *sk, unsigned long long sk_len_bytes, unsigned char *m, unsigned long long m_len_bytes, unsigned char *sn, unsigned long long *sn_len_bytes)
{
#ifdef SIG_PROFILE_STEPS
    uint64_t sign_total_t0 = sig_profile_now_ns();
#endif
    unsigned char H_seed[SEED_BYTES];
    unsigned char e_seed[SEED_BYTES];
    unsigned char pk[PK_LEN_BYTES];
    SignBuffers bufs;
    unsigned char *fs_commitment_input_ptr;
    unsigned long *e_bits_ptr;
    RawGF2EX (*H_hat_raw_ptr)[COLS];
    RawNZTerms (*H_hat_terms_ptr)[COLS];
    RawGF2EX *e_hat_prime_raw_ptr;
    uint16_t *y_hat_prime_raw_ptr;
    RawGF2EX *ye_raw_ptr;
    int use_cache = 0;
    RawGF2EX z_commit;
    RawGF2EX e_eval_commit;
    RawGF2EX v_eval_commit;
    RawGF2EX delta_raw;
    ProverOutputs outputs;
    int rc;

    if (sk == NULL || m == NULL || sn == NULL || sn_len_bytes == NULL)
    {
        return -2;
    }
    if (sk_len_bytes != SK_LEN_BYTES)
    {
        return -3;
    }
    if (ensure_instance_ready() != 0)
    {
        return -1;
    }
    memset(&bufs, 0, sizeof(bufs));
    if (g_sign_state_cache.valid && memcmp(sk, g_sign_state_cache.sk, SK_LEN_BYTES) == 0)
    {
        use_cache = 1;
        fs_commitment_input_ptr = g_sign_state_cache.fs_commitment_input;
        e_bits_ptr = g_sign_state_cache.e_bits;
        H_hat_raw_ptr = g_sign_state_cache.H_hat_raw;
        H_hat_terms_ptr = g_sign_state_cache.H_hat_terms;
        e_hat_prime_raw_ptr = g_sign_state_cache.e_hat_prime_raw;
        y_hat_prime_raw_ptr = g_sign_state_cache.y_hat_prime_raw;
        ye_raw_ptr = g_sign_state_cache.ye_raw;
    }
    else if (alloc_sign_buffers(&bufs) != 0)
    {
        return -7;
    }
    else
    {
        fs_commitment_input_ptr = bufs.fs_commitment_input;
        e_bits_ptr = bufs.e_bits;
        H_hat_raw_ptr = bufs.H_hat_raw;
        H_hat_terms_ptr = bufs.H_hat_terms;
        e_hat_prime_raw_ptr = bufs.e_hat_prime_raw;
        y_hat_prime_raw_ptr = bufs.y_hat_prime_raw;
        ye_raw_ptr = bufs.ye_raw;
    }

    sig_init_prover_workspace(&g_cached_prover_workspace);
    sig_unpack_secret_key(sk, H_seed, e_seed);
    if (!use_cache)
    {
        sig_build_public_state_direct(H_seed, e_seed, e_bits_ptr, bufs.y_bits, H_hat_raw_ptr, H_hat_terms_ptr, e_hat_prime_raw_ptr, y_hat_prime_raw_ptr, ye_raw_ptr);
        sig_pack_public_key(pk, H_seed, bufs.y_bits);
        build_fs_commitment_input(fs_commitment_input_ptr, pk);
        sign_cache_store(sk, &bufs);
    }

    memset(&outputs, 0, sizeof(outputs));
    memset(&z_commit, 0, sizeof(z_commit));
    memset(&e_eval_commit, 0, sizeof(e_eval_commit));
    memset(&v_eval_commit, 0, sizeof(v_eval_commit));
    memset(&delta_raw, 0, sizeof(delta_raw));

    (void)H_hat_raw_ptr;
    Prover(&g_cached_prover_workspace, m, (size_t)m_len_bytes, fs_commitment_input_ptr, e_bits_ptr, H_hat_terms_ptr, y_hat_prime_raw_ptr, e_hat_prime_raw_ptr, ye_raw_ptr, &z_commit, &e_eval_commit, &v_eval_commit, &delta_raw, &outputs);
#ifdef SIG_PROFILE_STEPS
    {
        uint64_t t0 = sig_profile_now_ns();
        rc = sig_serialize_signature(sn, sn_len_bytes, &outputs);
        g_sig_step_profile.sign_serialize_ns += sig_profile_now_ns() - t0;
    }
#else
    rc = sig_serialize_signature(sn, sn_len_bytes, &outputs);
#endif
#ifdef SIG_PROFILE_STEPS
    g_sig_step_profile.sign_total_ns += sig_profile_now_ns() - sign_total_t0;
    ++g_sig_step_profile.sign_calls;
#endif
    if (!use_cache)
    {
        free_sign_buffers(&bufs);
    }
    return rc;
}

/* Template API: rebuild public state from PK and verify the input signature. */
int sig_verify(unsigned char *pk, unsigned long long pk_len_bytes, unsigned char *sn, unsigned long long sn_len_bytes, unsigned char *m, unsigned long long m_len_bytes)
{
#ifdef SIG_PROFILE_STEPS
    uint64_t verify_total_t0 = sig_profile_now_ns();
#endif
    unsigned char H_seed[SEED_BYTES];
    VerifyBuffers bufs;
    unsigned char *fs_commitment_input_ptr;
    RawGF2EX (*H_hat_raw_ptr)[COLS];
    RawNZTerms (*H_hat_terms_ptr)[COLS];
    uint16_t *y_hat_prime_raw_ptr;
    int use_cache = 0;
    RawGF2EX e_eval_commit;
    RawGF2EX v_eval_commit;
    unsigned char root_hash[total_groups][HASH_DIGEST_LENGTH];
    RawGF2EX p_poly_interactive[SIGMA_REPETITIONS];
    RawGF2EX rv_out_interactive[SIGMA_REPETITIONS];
    RawGF2EX G_coeffs_raw[6];
    RawGF2EX w_hat_prime_raw[N_PRIME_LEN];
    unsigned char commitment_open_hashes[MERKLE_MAX_PROOF_HASHES][HASH_DIGEST_LENGTH];
    size_t commitment_open_hash_count = 0;
    RawGF2EX e_hat_encoded_commitment_open_output[opennum];
    RawGF2EX z_commit;
    RawGF2EX delta_raw;
    RawGF2EX vc_poly_challenge_raw;
    RawGF2EX vc_verifier_check_raw;
    int rc;

    if (pk == NULL || sn == NULL || m == NULL)
    {
        return -2;
    }
    if (pk_len_bytes != PK_LEN_BYTES)
    {
        return -3;
    }
    if (ensure_instance_ready() != 0)
    {
        return -1;
    }
    memset(&bufs, 0, sizeof(bufs));
    if (g_verify_state_cache.valid && memcmp(pk, g_verify_state_cache.pk, PK_LEN_BYTES) == 0)
    {
        use_cache = 1;
        fs_commitment_input_ptr = g_verify_state_cache.fs_commitment_input;
        H_hat_raw_ptr = g_verify_state_cache.H_hat_raw;
        H_hat_terms_ptr = g_verify_state_cache.H_hat_terms;
        y_hat_prime_raw_ptr = g_verify_state_cache.y_hat_prime_raw;
    }
    else if (alloc_verify_buffers(&bufs) != 0)
    {
        return -7;
    }
    else
    {
        fs_commitment_input_ptr = bufs.fs_commitment_input;
        H_hat_raw_ptr = bufs.H_hat_raw;
        H_hat_terms_ptr = bufs.H_hat_terms;
        y_hat_prime_raw_ptr = bufs.y_hat_prime_raw;
    }

#ifdef SIG_PROFILE_STEPS
    {
        uint64_t t0 = sig_profile_now_ns();
        rc = sig_deserialize_signature(sn, sn_len_bytes, &e_eval_commit, &v_eval_commit, root_hash, p_poly_interactive, rv_out_interactive, G_coeffs_raw, w_hat_prime_raw, commitment_open_hashes, &commitment_open_hash_count, e_hat_encoded_commitment_open_output);
        g_sig_step_profile.verify_deserialize_ns += sig_profile_now_ns() - t0;
    }
#else
    rc = sig_deserialize_signature(sn, sn_len_bytes, &e_eval_commit, &v_eval_commit, root_hash, p_poly_interactive, rv_out_interactive, G_coeffs_raw, w_hat_prime_raw, commitment_open_hashes, &commitment_open_hash_count, e_hat_encoded_commitment_open_output);
#endif
    if (rc != 0)
    {
#ifdef SIG_PROFILE_STEPS
        g_sig_step_profile.verify_total_ns += sig_profile_now_ns() - verify_total_t0;
        ++g_sig_step_profile.verify_calls;
#endif
        if (!use_cache)
        {
            free_verify_buffers(&bufs);
        }
        return -4;
    }

    sig_init_verifier_workspace(&g_cached_verifier_workspace);
    if (!use_cache)
    {
        sig_unpack_public_key(pk, H_seed, bufs.y_bits);
        build_verify_public_state_opt(H_seed, bufs.y_bits, bufs.H_bits, H_hat_raw_ptr, H_hat_terms_ptr, y_hat_prime_raw_ptr);
        build_fs_commitment_input(fs_commitment_input_ptr, pk);
        verify_cache_store(pk, &bufs);
    }

    memset(&z_commit, 0, sizeof(z_commit));
    memset(&delta_raw, 0, sizeof(delta_raw));
    memset(&vc_poly_challenge_raw, 0, sizeof(vc_poly_challenge_raw));
    memset(&vc_verifier_check_raw, 0, sizeof(vc_verifier_check_raw));

    (void)H_hat_raw_ptr;
    rc = Verifier(&g_cached_verifier_workspace, m, (size_t)m_len_bytes, fs_commitment_input_ptr, H_hat_terms_ptr, y_hat_prime_raw_ptr, &e_eval_commit, &v_eval_commit, root_hash, p_poly_interactive, rv_out_interactive, G_coeffs_raw, w_hat_prime_raw, commitment_open_hashes, commitment_open_hash_count, e_hat_encoded_commitment_open_output, &z_commit, &delta_raw, &vc_poly_challenge_raw, &vc_verifier_check_raw);

    if (!use_cache)
    {
        free_verify_buffers(&bufs);
    }
#ifdef SIG_PROFILE_STEPS
    g_sig_step_profile.verify_total_ns += sig_profile_now_ns() - verify_total_t0;
    ++g_sig_step_profile.verify_calls;
#endif
    return rc ? 0 : -1;
}
