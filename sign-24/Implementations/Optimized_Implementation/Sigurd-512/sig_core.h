#ifndef SIG_CORE_H
#define SIG_CORE_H

#include <stddef.h>
#include <stdint.h>

#include "drng.h"

#ifdef SIG_PROFILE_STEPS
#include <time.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define SIG512_OPT_PARAMETER_NOTE "Uses the current 512-bit parameter set as the submitted SIG512_OPT instance."

#ifndef SIG512_RS_RATE_FACTOR
#define SIG512_RS_RATE_FACTOR 16
#endif

#ifndef SIG512_TOTAL_GROUPS
#define SIG512_TOTAL_GROUPS 8
#endif

#if !((SIG512_RS_RATE_FACTOR == 4 && SIG512_TOTAL_GROUPS == 4) || \
      (SIG512_RS_RATE_FACTOR == 8 && (SIG512_TOTAL_GROUPS == 4 || SIG512_TOTAL_GROUPS == 8)) || \
      (SIG512_RS_RATE_FACTOR == 16 && (SIG512_TOTAL_GROUPS == 8 || SIG512_TOTAL_GROUPS == 16)))
#error "Unsupported SIG512_REF parameter sweep point"
#endif

enum
{
    M = 5676,
    K = 3230,
    M_K = 2446,
    TEST_ROUNDS = 100,
    opennum = (SIG512_RS_RATE_FACTOR == 4) ? 512 : ((SIG512_RS_RATE_FACTOR == 8) ? 342 : 256),
    SIGMA_REPETITIONS = 32,
    TAIL_POLY_LEN = 5,
    EXTRA_RANDOM_TAIL = opennum,
    TAIL_LEN = SIGMA_REPETITIONS + TAIL_POLY_LEN + EXTRA_RANDOM_TAIL,
    N_PRIME_LEN = (M / 6) + TAIL_LEN,
    N = SIG512_RS_RATE_FACTOR * N_PRIME_LEN,
    total_groups = SIG512_TOTAL_GROUPS,
    group_size = N / total_groups,
    embded_num = 2456,
    ROWS = embded_num / 16,
    COLS = M / 6,
    TAIL_INTERACTIVE_OFFSET = COLS,
    TAIL_POLY_OFFSET = TAIL_INTERACTIVE_OFFSET + SIGMA_REPETITIONS,
    RAW_POLY_LEN = 32,
    RAW_PRODUCT_LEN = 63,
    RAW_POLY_BYTES = RAW_POLY_LEN * 2,
    HASH_DIGEST_LENGTH = 128,
    MERKLE_LEAF_COUNT = (group_size <= 2048) ? 2048 : 4096,
    MERKLE_NODE_COUNT = (2 * MERKLE_LEAF_COUNT) - 2,
    MERKLE_LEAF_OFFSET = MERKLE_LEAF_COUNT - 2,
    MERKLE_TREE_HEIGHT = (MERKLE_LEAF_COUNT == 2048) ? 11 : 12,
    MERKLE_MAX_PROOF_HASHES = opennum * MERKLE_TREE_HEIGHT,
    Y_BITS_PACKED_BYTES = embded_num / 8,
    SEED_BYTES = 128,
    PK_LEN_BYTES = SEED_BYTES + Y_BITS_PACKED_BYTES,
    SK_LEN_BYTES = 2 * SEED_BYTES,
    FS_COMMITMENT_INPUT_BYTES = HASH_DIGEST_LENGTH + PK_LEN_BYTES,
    HASH_CTX_CAPACITY = 131072,
    MAX_SIGNATURE_BYTES =
        (2 * RAW_POLY_BYTES) +
        (total_groups * HASH_DIGEST_LENGTH) +
        (SIGMA_REPETITIONS * RAW_POLY_BYTES) +
        (SIGMA_REPETITIONS * RAW_POLY_BYTES) +
        (6 * RAW_POLY_BYTES) +
        (N_PRIME_LEN * RAW_POLY_BYTES) +
        4 +
        (MERKLE_MAX_PROOF_HASHES * HASH_DIGEST_LENGTH) +
        (opennum * RAW_POLY_BYTES)
};

typedef struct
{
    uint16_t coeffs[RAW_POLY_LEN];
} RawGF2EX;

typedef struct
{
    uint16_t coeffs[RAW_PRODUCT_LEN];
} RawGF2EX_Product;

typedef struct
{
    uint8_t index[RAW_POLY_LEN];
    uint8_t count;
    uint16_t log_value[RAW_POLY_LEN];
} RawNZTerms;

typedef struct
{
    size_t len;
    unsigned char buf[HASH_CTX_CAPACITY];
} SIG_HASH_CTX;

typedef struct
{
    RawGF2EX *e_eval_commit_ptr;
    RawGF2EX *v_eval_commit_ptr;
    unsigned char (*root_hash_ptr)[HASH_DIGEST_LENGTH];
    RawGF2EX *p_poly_interactive_ptr;
    RawGF2EX *rv_out_interactive_ptr;
    RawGF2EX *G_coeffs_raw_ptr;
    RawGF2EX *w_hat_prime_raw_ptr;
    unsigned char (*commitment_open_hashes_ptr)[HASH_DIGEST_LENGTH];
    size_t commitment_open_hash_count;
    RawGF2EX *e_hat_encoded_commitment_open_output_ptr;
} ProverOutputs;

typedef struct
{
    SIG_HASH_CTX sha_ctx;
    unsigned char commitment_hash_storage[total_groups][MERKLE_NODE_COUNT][HASH_DIGEST_LENGTH];
    unsigned char root_hash_storage[total_groups][HASH_DIGEST_LENGTH];
    RawGF2EX v_hat_prime_raw[N_PRIME_LEN];
    RawGF2EX v_encoded[N];
    RawGF2EX e_hat_encoded[N];
    RawGF2EX yv_raw[ROWS];
    RawGF2EX tau_vec_raw[COLS];
    RawGF2EX w_hat_prime_raw[N_PRIME_LEN];
    RawGF2EX G_coeffs_raw[6];
    RawGF2EX p_poly_interactive[SIGMA_REPETITIONS];
    RawGF2EX rv_out_interactive[SIGMA_REPETITIONS];
    int commitment_open_trees[opennum];
    int commitment_open_choices[opennum];
    unsigned char message_hash[HASH_DIGEST_LENGTH];
    unsigned char fs_value_commitment[HASH_DIGEST_LENGTH];
    unsigned char fs_value_intercheck[HASH_DIGEST_LENGTH];
    unsigned char fs_value_polyres[HASH_DIGEST_LENGTH];
    unsigned char fs_value_mask[HASH_DIGEST_LENGTH];
    unsigned char fs_value_open[HASH_DIGEST_LENGTH];
    uint16_t rand_challenges_flat[SIGMA_REPETITIONS * ROWS];
    uint16_t gamma_vec[SIGMA_REPETITIONS];
    unsigned char commitment_open_hashes[MERKLE_MAX_PROOF_HASHES][HASH_DIGEST_LENGTH];
    size_t commitment_open_hash_count;
    RawGF2EX e_hat_encoded_commitment_open_output[opennum];
    SIG_HASH_CTX message_prefix_ctx;
} ProverWorkspace;

typedef struct
{
    SIG_HASH_CTX sha_ctx;
    RawGF2EX yw_raw[ROWS];
    RawGF2EX tau_vec_raw[COLS];
    int derived_trees[opennum];
    int derived_choices[opennum];
    unsigned char message_hash[HASH_DIGEST_LENGTH];
    unsigned char fs_value_commitment[HASH_DIGEST_LENGTH];
    unsigned char fs_value_intercheck[HASH_DIGEST_LENGTH];
    unsigned char fs_value_polyres[HASH_DIGEST_LENGTH];
    unsigned char fs_value_mask[HASH_DIGEST_LENGTH];
    unsigned char fs_value_open[HASH_DIGEST_LENGTH];
    uint16_t rand_challenges_flat[SIGMA_REPETITIONS * ROWS];
    uint16_t gamma_vec[SIGMA_REPETITIONS];
    SIG_HASH_CTX message_prefix_ctx;
} VerifierWorkspace;

#ifdef SIG_PROFILE_STEPS
typedef struct
{
    uint64_t sign_message_hash_and_fs_commitment_ns;
    uint64_t sign_commitment_launch_ns;
    uint64_t sign_commitment_random_fill_ns;
    uint64_t sign_commitment_rs_prepare_ns;
    uint64_t sign_commitment_rs_v_launch_ns;
    uint64_t sign_commitment_rs_v_encode_ns;
    uint64_t sign_commitment_rs_e_encode_ns;
    uint64_t sign_commitment_rs_v_join_ns;
    uint64_t sign_commitment_merkle_launch_ns;
    uint64_t sign_commitment_merkle_leaf_hash_ns;
    uint64_t sign_commitment_merkle_tree_hash_ns;
    uint64_t sign_commitment_merkle_thread_launch_ns;
    uint64_t sign_commitment_merkle_join_ns;
    uint64_t sign_projection_and_merkle_join_ns;
    uint64_t sign_fs_intercheck_ns;
    uint64_t sign_interactive_check_ns;
    uint64_t sign_fs_polyres_ns;
    uint64_t sign_poly_response_ns;
    uint64_t sign_fs_mask_and_masking_ns;
    uint64_t sign_fs_open_and_commitment_open_ns;
    uint64_t sign_serialize_ns;
    uint64_t sign_total_ns;
    uint64_t verify_deserialize_ns;
    uint64_t verify_message_hash_and_fs_commitment_ns;
    uint64_t verify_fs_intercheck_ns;
    uint64_t verify_fs_polyres_ns;
    uint64_t verify_fs_mask_and_fs_open_ns;
    uint64_t verify_commitment_open_with_overlap_ns;
    uint64_t verify_initial_computation_ns;
    uint64_t verify_interactive_check_ns;
    uint64_t verify_final_poly_check_ns;
    uint64_t verify_total_ns;
    uint64_t sign_calls;
    uint64_t verify_calls;
} SigStepProfile;

extern SigStepProfile g_sig_step_profile;

static inline uint64_t sig_profile_now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint64_t)ts.tv_sec * 1000000000ULL) + (uint64_t)ts.tv_nsec;
}

void sig_profile_reset(void);
void sig_profile_snapshot(SigStepProfile *out);
#endif

void sig_init_prover_workspace(ProverWorkspace *ws);
void sig_init_verifier_workspace(VerifierWorkspace *ws);

void random_H(unsigned long H[embded_num][M], const unsigned char seed[SEED_BYTES]);
void random_e(unsigned long e[M], const unsigned char seed[SEED_BYTES]);
void generate_y(unsigned long y[embded_num], unsigned long e[M], unsigned long H[embded_num][M]);
void embed_H_raw(RawGF2EX H_hat_raw[ROWS][COLS], const unsigned long H[embded_num][M]);
void embed_e_raw(RawGF2EX e_hat_prime_raw[N_PRIME_LEN], const unsigned long e[M]);
void embed_y_raw(uint16_t y_hat_scalars[ROWS], const unsigned long y[embded_num]);
void precompute_H_hat_terms(RawNZTerms H_hat_terms[ROWS][COLS], const RawGF2EX H_hat_raw[ROWS][COLS]);
void Compute_YE_Raw(RawGF2EX ye_hat[ROWS], const RawNZTerms H_hat_terms[ROWS][COLS], const RawGF2EX e_hat_prime_raw[N_PRIME_LEN]);
void sig_build_public_state(
    const unsigned char H_seed[SEED_BYTES],
    const unsigned char e_seed[SEED_BYTES],
    unsigned long H_bits[embded_num][M],
    unsigned long e_bits[M],
    unsigned long y_bits[embded_num],
    RawGF2EX H_hat_raw[ROWS][COLS],
    RawNZTerms H_hat_terms[ROWS][COLS],
    RawGF2EX e_hat_prime_raw[N_PRIME_LEN],
    uint16_t y_hat_prime_raw[ROWS],
    RawGF2EX ye_raw[ROWS]);
void sig_build_public_state_direct(
    const unsigned char H_seed[SEED_BYTES],
    const unsigned char e_seed[SEED_BYTES],
    unsigned long e_bits[M],
    unsigned long y_bits[embded_num],
    RawGF2EX H_hat_raw[ROWS][COLS],
    RawNZTerms H_hat_terms[ROWS][COLS],
    RawGF2EX e_hat_prime_raw[N_PRIME_LEN],
    uint16_t y_hat_prime_raw[ROWS],
    RawGF2EX ye_raw[ROWS]);

void Prover(
    ProverWorkspace *ws,
    const unsigned char *message,
    size_t message_len,
    unsigned char fs_commitment_input[FS_COMMITMENT_INPUT_BYTES],
    unsigned long e_bits[M],
    const RawNZTerms H_hat_terms[ROWS][COLS],
    uint16_t y_hat_prime_raw[ROWS],
    RawGF2EX e_hat_prime_raw[N_PRIME_LEN],
    const RawGF2EX ye_raw[ROWS],
    RawGF2EX *z_commit,
    RawGF2EX *e_eval_commit,
    RawGF2EX *v_eval_commit,
    RawGF2EX *delta_raw,
    ProverOutputs *outputs);

int Verifier(
    VerifierWorkspace *ws,
    const unsigned char *message,
    size_t message_len,
    unsigned char fs_commitment_input[FS_COMMITMENT_INPUT_BYTES],
    const RawNZTerms H_hat_terms[ROWS][COLS],
    uint16_t y_hat_prime_raw[ROWS],
    const RawGF2EX *e_eval_commit,
    const RawGF2EX *v_eval_commit,
    const unsigned char root_hash[total_groups][HASH_DIGEST_LENGTH],
    const RawGF2EX p_poly_interactive[SIGMA_REPETITIONS],
    const RawGF2EX rv_out_interactive[SIGMA_REPETITIONS],
    const RawGF2EX G_coeffs_raw[6],
    const RawGF2EX w_hat_prime_raw[N_PRIME_LEN],
    const unsigned char commitment_open_hashes[MERKLE_MAX_PROOF_HASHES][HASH_DIGEST_LENGTH],
    size_t commitment_open_hash_count,
    RawGF2EX e_hat_encoded_commitment_open_output[opennum],
    RawGF2EX *z_commit,
    RawGF2EX *delta_raw,
    RawGF2EX *vc_poly_challenge_raw,
    RawGF2EX *vc_verifier_check_raw);

void sig_pack_secret_key(unsigned char sk[SK_LEN_BYTES], const unsigned char H_seed[SEED_BYTES], const unsigned char e_seed[SEED_BYTES]);
void sig_unpack_secret_key(const unsigned char sk[SK_LEN_BYTES], unsigned char H_seed[SEED_BYTES], unsigned char e_seed[SEED_BYTES]);
void sig_pack_public_key(unsigned char pk[PK_LEN_BYTES], const unsigned char H_seed[SEED_BYTES], const unsigned long y_bits[embded_num]);
void sig_unpack_public_key(const unsigned char pk[PK_LEN_BYTES], unsigned char H_seed[SEED_BYTES], unsigned long y_bits[embded_num]);
int sig_serialize_signature(unsigned char *sn, unsigned long long *sn_len_bytes, const ProverOutputs *outputs);
int sig_deserialize_signature(
    const unsigned char *sn,
    unsigned long long sn_len_bytes,
    RawGF2EX *e_eval_commit,
    RawGF2EX *v_eval_commit,
    unsigned char root_hash[total_groups][HASH_DIGEST_LENGTH],
    RawGF2EX p_poly_interactive[SIGMA_REPETITIONS],
    RawGF2EX rv_out_interactive[SIGMA_REPETITIONS],
    RawGF2EX G_coeffs_raw[6],
    RawGF2EX w_hat_prime_raw[N_PRIME_LEN],
    unsigned char commitment_open_hashes[MERKLE_MAX_PROOF_HASHES][HASH_DIGEST_LENGTH],
    size_t *commitment_open_hash_count,
    RawGF2EX e_hat_encoded_commitment_open_output[opennum]);

int sig_run_self_test(void);

#ifdef __cplusplus
}
#endif

#endif
