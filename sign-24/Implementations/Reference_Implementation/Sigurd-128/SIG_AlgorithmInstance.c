/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

/*
 * Submission-facing wrapper for the SIG128_REF reference implementation.
 *
 * Responsibilities:
 * - expose the ICCS template SIG API (`sig_keygen`, `sig_sign`, `sig_verify`);
 * - keep the external key format compact (`SK = H_seed || e_seed`,
 *   `PK = H_seed || packed_y`);
 * - serialize and parse the non-interactive Fiat-Shamir transcript;
 * - allocate the large reference work buffers on the heap so API calls do not
 *   place multi-megabyte protocol state on the C stack.
 *
 * The mathematical protocol is implemented in sig_core.c.  This wrapper should
 * not change transcript semantics except through explicit serialization changes.
 */

#include "SIG_AlgorithmInstance.h"

#include "drng.h"
#include "sig_core.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*
 * Heap-backed buffers used while rebuilding signer state from SK.
 * The secret key stores only seeds, so signing expands H, e, y, and their
 * embedded extension-field forms on every call.
 */
typedef struct
{
    unsigned long *e_bits;
    unsigned long *y_bits;
    RawGF2EX (*H_hat_raw)[COLS];
    RawGF2EX *e_hat_prime_raw;
    uint16_t *y_hat_prime_raw;
    RawGF2EX *ye_raw;
} SignBuffers;

/*
 * Heap-backed buffers used while rebuilding verifier public state from PK.
 * Verification only needs H and y-derived state; it never reconstructs e.
 */
typedef struct
{
    unsigned long (*H_bits)[M];
    unsigned long *y_bits;
    RawGF2EX (*H_hat_raw)[COLS];
    uint16_t *y_hat_prime_raw;
} VerifyBuffers;

static int g_instance_ready = 0;

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

/* Encode one extension-field element as little-endian 16-bit coefficients. */
static void pack_raw_poly(unsigned char out[RAW_POLY_BYTES], const RawGF2EX *poly)
{
    int i;
    for (i = 0; i < RAW_POLY_LEN; ++i)
    {
        pack_u16_le(out + (i * 2), poly->coeffs[i]);
    }
}

/* Decode one serialized extension-field element into the internal form. */
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

/* Pack public syndrome bits MSB-first to match the submitted compact PK format. */
static void sig_pack_y_bits(unsigned char packed[Y_BITS_PACKED_BYTES], const unsigned long y_bits[M_K + 12])
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

/* Unpack the compact public syndrome into the full binary y vector. */
static void sig_unpack_y_bits(unsigned long y_bits[M_K + 12], const unsigned char packed[Y_BITS_PACKED_BYTES])
{
    int i;

    memset(y_bits, 0, sizeof(unsigned long) * (M_K + 12));
    for (i = 0; i < embded_num; ++i)
    {
        y_bits[i] = (unsigned long)((packed[i / 8] >> (7 - (i % 8))) & 1u);
    }
}

/* Serialize the public key as H_seed || packed y bits. */
void sig_pack_public_key(unsigned char pk[PK_LEN_BYTES], const unsigned char H_seed[SEED_BYTES], const unsigned long y_bits[M_K + 12])
{
    unsigned char packed_y[Y_BITS_PACKED_BYTES];

    sig_pack_y_bits(packed_y, y_bits);
    memcpy(pk, H_seed, SEED_BYTES);
    memcpy(pk + SEED_BYTES, packed_y, Y_BITS_PACKED_BYTES);
}

/* Deserialize the compact public key into H_seed and y bits. */
void sig_unpack_public_key(const unsigned char pk[PK_LEN_BYTES], unsigned char H_seed[SEED_BYTES], unsigned long y_bits[M_K + 12])
{
    memcpy(H_seed, pk, SEED_BYTES);
    sig_unpack_y_bits(y_bits, pk + SEED_BYTES);
}

/* Build the first Fiat-Shamir preimage prefix: zero message hash || PK. */
static void build_fs_commitment_input(
    unsigned char out[FS_COMMITMENT_INPUT_BYTES],
    const unsigned char pk[PK_LEN_BYTES])
{
    memset(out, 0, HASH_DIGEST_LENGTH);//Clear the first 32 bytes.
    memcpy(out + HASH_DIGEST_LENGTH, pk, PK_LEN_BYTES);//put the publickey into it
}

/* Allocate large signer-side working arrays on the heap. */
static int alloc_sign_buffers(SignBuffers *bufs)
{
    memset(bufs, 0, sizeof(*bufs));

    bufs->e_bits = (unsigned long *)malloc(sizeof(unsigned long) * M);
    bufs->y_bits = (unsigned long *)malloc(sizeof(unsigned long) * embded_num);
    bufs->H_hat_raw = (RawGF2EX (*)[COLS])malloc(sizeof(RawGF2EX) * ROWS * COLS);
    bufs->e_hat_prime_raw = (RawGF2EX *)malloc(sizeof(RawGF2EX) * N_PRIME_LEN);
    bufs->y_hat_prime_raw = (uint16_t *)malloc(sizeof(uint16_t) * ROWS);
    bufs->ye_raw = (RawGF2EX *)malloc(sizeof(RawGF2EX) * ROWS);

    if (bufs->e_bits == NULL || bufs->y_bits == NULL ||
        bufs->H_hat_raw == NULL || bufs->e_hat_prime_raw == NULL ||
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
    free(bufs->H_hat_raw);
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
    bufs->H_hat_raw = (RawGF2EX (*)[COLS])malloc(sizeof(RawGF2EX) * ROWS * COLS);
    bufs->y_hat_prime_raw = (uint16_t *)malloc(sizeof(uint16_t) * ROWS);

    if (bufs->H_bits == NULL || bufs->y_bits == NULL || bufs->H_hat_raw == NULL || bufs->y_hat_prime_raw == NULL)
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
    free(bufs->H_hat_raw);
    free(bufs->y_hat_prime_raw);
    memset(bufs, 0, sizeof(*bufs));
}

/* Rebuild the verifier's public-side embedded state from the public key. */
static void build_verify_public_state(const unsigned char H_seed[SEED_BYTES], unsigned long y_bits[M_K + 12], unsigned long H_bits[M_K + 12][M], RawGF2EX H_hat_raw[ROWS][COLS], uint16_t y_hat_prime_raw[ROWS])
{
    random_H(H_bits, H_seed);
    embed_H_raw(H_hat_raw, H_bits);
    embed_y_raw(y_hat_prime_raw, y_bits);
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

/*
 * Serialize the prover transcript into the submission signature layout.
 *
 * Field order is intentionally mirrored by sig_deserialize_signature():
 * all Merkle roots, e_eval_commit, v_eval_commit, interactive response
 * polynomials, final response coefficients, masked codeword seed blocks,
 * variable-length Merkle authentication hashes, and the opened e leaves.
 */
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

/*
 * Parse a serialized signature back into verifier-facing transcript objects.
 * The Merkle proof block is variable length, so the encoded hash_count is
 * bounded by MERKLE_MAX_PROOF_HASHES before any proof bytes are copied.
 */
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

    /*
     * The public key stores H_seed plus y, not expanded H.  Keeping y in PK
     * lets the verifier rebuild only the deterministic public matrix from
     * H_seed while avoiding secret e_seed exposure.
     */
    sig_build_public_state_direct(H_seed, e_seed, bufs.e_bits, bufs.y_bits, bufs.H_hat_raw, bufs.e_hat_prime_raw, bufs.y_hat_prime_raw, bufs.ye_raw);
    sig_pack_public_key(pk, H_seed, bufs.y_bits);
    sig_pack_secret_key(sk, H_seed, e_seed);

    *pk_len_bytes = PK_LEN_BYTES;
    *sk_len_bytes = SK_LEN_BYTES;
    rc = 0;
    free_sign_buffers(&bufs);
    return rc;
}

/* Template API: rebuild private state from SK and sign the input message. */
int sig_sign(unsigned char *sk, unsigned long long sk_len_bytes, unsigned char *m, unsigned long long m_len_bytes, unsigned char *sn, unsigned long long *sn_len_bytes)
{
    unsigned char H_seed[SEED_BYTES];
    unsigned char e_seed[SEED_BYTES];
    unsigned char pk[PK_LEN_BYTES];
    unsigned char fs_commitment_input[FS_COMMITMENT_INPUT_BYTES];
    SignBuffers bufs;
    ProverWorkspace *ws;
    RawGF2EX z_commit;
    RawGF2EX e_eval_commit;
    RawGF2EX v_eval_commit;
    RawGF2EX delta_raw;
    ProverOutputs outputs;
    int rc;//Record whether the serialization is successful.

    //check the format of inputdata
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
    if (alloc_sign_buffers(&bufs) != 0)
    {
        return -7;
    }

    ws = (ProverWorkspace *)malloc(sizeof(ProverWorkspace));
    if (ws == NULL)
    {
        free_sign_buffers(&bufs);
        return -7;
    }

    sig_init_prover_workspace(ws);
    sig_unpack_secret_key(sk, H_seed, e_seed);
    /*
     * Signing reconstructs the full private/public state from SK seeds, then
     * rebuilds the compact PK bytes so the first Fiat-Shamir commitment uses
     * exactly message_hash || PK, matching verifier reconstruction.
     */
    sig_build_public_state_direct(H_seed, e_seed, bufs.e_bits, bufs.y_bits, bufs.H_hat_raw, bufs.e_hat_prime_raw, bufs.y_hat_prime_raw, bufs.ye_raw);
    sig_pack_public_key(pk, H_seed, bufs.y_bits);
    build_fs_commitment_input(fs_commitment_input, pk);

    memset(&outputs, 0, sizeof(outputs));
    memset(&z_commit, 0, sizeof(z_commit));
    memset(&e_eval_commit, 0, sizeof(e_eval_commit));
    memset(&v_eval_commit, 0, sizeof(v_eval_commit));
    memset(&delta_raw, 0, sizeof(delta_raw));

    Prover(ws, m, (size_t)m_len_bytes, fs_commitment_input, bufs.H_hat_raw, bufs.y_hat_prime_raw, bufs.e_hat_prime_raw, bufs.ye_raw, &z_commit, &e_eval_commit, &v_eval_commit, &delta_raw, &outputs);
    rc = sig_serialize_signature(sn, sn_len_bytes, &outputs);

    free(ws);
    free_sign_buffers(&bufs);
    return rc;
}

/* Template API: rebuild public state from PK and verify the input signature. */
int sig_verify(unsigned char *pk, unsigned long long pk_len_bytes, unsigned char *sn, unsigned long long sn_len_bytes, unsigned char *m, unsigned long long m_len_bytes)
{
    unsigned char H_seed[SEED_BYTES];
    unsigned char fs_commitment_input[FS_COMMITMENT_INPUT_BYTES];
    VerifyBuffers bufs;
    VerifierWorkspace *ws;
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
    if (alloc_verify_buffers(&bufs) != 0)
    {
        return -7;
    }

    ws = (VerifierWorkspace *)malloc(sizeof(VerifierWorkspace));
    if (ws == NULL)
    {
        free_verify_buffers(&bufs);
        return -7;
    }

    rc = sig_deserialize_signature(sn, sn_len_bytes, &e_eval_commit, &v_eval_commit, root_hash, p_poly_interactive, rv_out_interactive, G_coeffs_raw, w_hat_prime_raw, commitment_open_hashes, &commitment_open_hash_count, e_hat_encoded_commitment_open_output);
    if (rc != 0)
    {
        free(ws);
        free_verify_buffers(&bufs);
        return -4;
    }

    sig_init_verifier_workspace(ws);
    sig_unpack_public_key(pk, H_seed, bufs.y_bits);
    /*
     * Verification trusts only the serialized PK and signature transcript.
     * H is deterministically expanded from H_seed; y is unpacked from PK and
     * embedded for the interactive consistency checks.
     */
    build_verify_public_state(H_seed, bufs.y_bits, bufs.H_bits, bufs.H_hat_raw, bufs.y_hat_prime_raw);
    build_fs_commitment_input(fs_commitment_input, pk);

    memset(&z_commit, 0, sizeof(z_commit));
    memset(&delta_raw, 0, sizeof(delta_raw));
    memset(&vc_poly_challenge_raw, 0, sizeof(vc_poly_challenge_raw));
    memset(&vc_verifier_check_raw, 0, sizeof(vc_verifier_check_raw));

    rc = Verifier(ws, m, (size_t)m_len_bytes, fs_commitment_input, bufs.H_hat_raw, bufs.y_hat_prime_raw, &e_eval_commit, &v_eval_commit, root_hash, p_poly_interactive, rv_out_interactive, G_coeffs_raw, w_hat_prime_raw, commitment_open_hashes, commitment_open_hash_count, e_hat_encoded_commitment_open_output, &z_commit, &delta_raw, &vc_poly_challenge_raw, &vc_verifier_check_raw);

    free(ws);
    free_verify_buffers(&bufs);
    return rc ? 0 : -1;
}
