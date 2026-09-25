/* Independent repeated-opening recovery for the submitted Sigurd reference API.
 *
 * Build with the six submitted reference .c files for the chosen level, plus
 * this driver. Sigurd-256 additionally needs -DSIGURD_LEVEL_256 because its
 * Prover() interface carries the expanded witness explicitly.
 *
 * The signer key is passed only to the queried sig_sign() calls. Recovery and
 * the fresh-message Prover() call receive the public key and signatures only.
 */
#include "SIG_AlgorithmInstance.h"
#include "sig_core.h"
#include "drng.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DRNG_ctx drng_algorithm;

enum { FIELD_ORDER = 65535, FIELD_SIZE = 65536, MAX_CHUNKS = 32, MAX_K = 64 };
static uint16_t field_exp[2 * FIELD_ORDER], field_log[FIELD_SIZE];

static void field_init(void)
{
    uint32_t a = 1;
    for (unsigned i = 0; i < FIELD_ORDER; ++i) {
        field_exp[i] = (uint16_t)a;
        field_log[a] = (uint16_t)i;
        a <<= 1;
        if (a & 0x10000u) a ^= 0x1002du;
    }
    for (unsigned i = FIELD_ORDER; i < 2 * FIELD_ORDER; ++i)
        field_exp[i] = field_exp[i - FIELD_ORDER];
}

static uint16_t field_mul(uint16_t a, uint16_t b)
{
    return a && b ? field_exp[(unsigned)field_log[a] + field_log[b]] : 0;
}

static uint16_t field_inv(uint16_t a)
{
    if (!a) { fprintf(stderr, "singular field matrix\n"); exit(2); }
    return field_exp[FIELD_ORDER - field_log[a]];
}

static int recover_chunk(unsigned chunk, unsigned k, unsigned s,
                         const unsigned char *seen, const RawGF2EX *values,
                         unsigned long *recovered)
{
    uint16_t mat[MAX_K][MAX_K], rhs[MAX_K][6];
    unsigned count = 0;
    if (k > MAX_K) return -1;
    for (unsigned j = 0; j < s && count < k; ++j) {
        if (!seen[chunk * s + j]) continue;
        uint16_t x = field_exp[j + 1], power = 1;
        for (unsigned col = 0; col < k; ++col) {
            mat[count][col] = power;
            power = field_mul(power, x);
        }
        for (unsigned lane = 0; lane < 6; ++lane)
            rhs[count][lane] = values[chunk * s + j].coeffs[lane];
        ++count;
    }
    if (count != k) return -1;

    for (unsigned col = 0; col < k; ++col) {
        unsigned pivot = col;
        while (pivot < k && !mat[pivot][col]) ++pivot;
        if (pivot == k) return -1;
        if (pivot != col) {
            for (unsigned c = 0; c < k; ++c) {
                uint16_t t = mat[col][c]; mat[col][c] = mat[pivot][c]; mat[pivot][c] = t;
            }
            for (unsigned lane = 0; lane < 6; ++lane) {
                uint16_t t = rhs[col][lane]; rhs[col][lane] = rhs[pivot][lane]; rhs[pivot][lane] = t;
            }
        }
        uint16_t inv = field_inv(mat[col][col]);
        for (unsigned c = col; c < k; ++c) mat[col][c] = field_mul(mat[col][c], inv);
        for (unsigned lane = 0; lane < 6; ++lane)
            rhs[col][lane] = field_mul(rhs[col][lane], inv);
        for (unsigned row = 0; row < k; ++row) {
            if (row == col) continue;
            uint16_t scale = mat[row][col];
            if (!scale) continue;
            for (unsigned c = col; c < k; ++c)
                mat[row][c] ^= field_mul(scale, mat[col][c]);
            for (unsigned lane = 0; lane < 6; ++lane)
                rhs[row][lane] ^= field_mul(scale, rhs[col][lane]);
        }
    }
    for (unsigned block = 0; block < k; ++block) {
        unsigned ones = 0;
        for (unsigned lane = 0; lane < 6; ++lane) {
            if (rhs[block][lane] > 1) return -1;
            recovered[6 * (chunk * k + block) + lane] = rhs[block][lane];
            ones += rhs[block][lane];
        }
        if (ones != 1) return -1;
    }
    return 0;
}

static unsigned row_bit(const uint64_t *row, unsigned col)
{
    return (unsigned)((row[col / 64] >> (col % 64)) & 1u);
}

static void row_set(uint64_t *row, unsigned col)
{
    row[col / 64] |= (uint64_t)1 << (col % 64);
}

static int recover_suffix(unsigned known_blocks, unsigned long *e,
                          const unsigned long *y, const unsigned long (*H)[M])
{
    unsigned vars = 6u * (COLS - known_blocks), words = (vars + 1u + 63u) / 64u;
    uint64_t *rows = calloc((size_t)embded_num * words, sizeof(uint64_t));
    if (!rows) return -1;
    for (unsigned r = 0; r < embded_num; ++r) {
        uint64_t *row = rows + (size_t)r * words;
        unsigned bit = (unsigned)y[r];
        for (unsigned col = 0; col < 6u * known_blocks; ++col)
            bit ^= (unsigned)(H[r][col] & e[col]);
        for (unsigned col = 0; col < vars; ++col)
            if (H[r][6u * known_blocks + col] & 1u) row_set(row, col);
        if (bit & 1u) row_set(row, vars);
    }
    for (unsigned col = 0; col < vars; ++col) {
        unsigned pivot = col;
        while (pivot < embded_num && !row_bit(rows + (size_t)pivot * words, col)) ++pivot;
        if (pivot == embded_num) { free(rows); return -1; }
        if (pivot != col) {
            for (unsigned w = 0; w < words; ++w) {
                uint64_t t = rows[(size_t)col * words + w];
                rows[(size_t)col * words + w] = rows[(size_t)pivot * words + w];
                rows[(size_t)pivot * words + w] = t;
            }
        }
        for (unsigned r = col + 1; r < embded_num; ++r) {
            uint64_t *row = rows + (size_t)r * words;
            if (!row_bit(row, col)) continue;
            for (unsigned w = 0; w < words; ++w)
                row[w] ^= rows[(size_t)col * words + w];
        }
    }
    for (unsigned r = vars; r < embded_num; ++r)
        if (row_bit(rows + (size_t)r * words, vars)) { free(rows); return -1; }
    for (int col = (int)vars - 1; col >= 0; --col) {
        const uint64_t *row = rows + (size_t)col * words;
        unsigned bit = row_bit(row, vars);
        for (unsigned c = (unsigned)col + 1; c < vars; ++c)
            bit ^= row_bit(row, c) & (unsigned)e[6u * known_blocks + c];
        e[6u * known_blocks + (unsigned)col] = bit;
    }
    free(rows);
    for (unsigned block = known_blocks; block < COLS; ++block) {
        unsigned ones = 0;
        for (unsigned j = 0; j < 6; ++j) ones += (unsigned)e[block * 6u + j];
        if (ones != 1) return -1;
    }
    return 0;
}

static void require(int condition, const char *why)
{
    if (!condition) { fprintf(stderr, "FAIL: %s\n", why); exit(1); }
}

int main(int argc, char **argv)
{
    const unsigned k = (N_PRIME_LEN + 31u) / 32u;
    const unsigned s = (N / N_PRIME_LEN) * k;
    const unsigned fixed_chunks = COLS / k;
    require(k <= MAX_K && fixed_chunks <= MAX_CHUNKS, "unexpected parameters");
    field_init();

    require(argc <= 2, "usage: reproduce_recovery_<level> [seed_id 0..255]");
    char *end = NULL;
    unsigned long seed_id = argc == 2 ? strtoul(argv[1], &end, 0) : 0;
    require(argc == 1 || (argv[1][0] && end && !*end && seed_id <= 255),
            "seed_id must be an integer in 0..255");
    unsigned char seed[SEEDLEN], pk[PK_LEN_BYTES], sk[SK_LEN_BYTES];
    for (unsigned i = 0; i < sizeof(seed); ++i)
        seed[i] = (unsigned char)(17u + 73u * i + 29u * seed_id);
    require(init_random_number(&drng_algorithm, seed, sizeof(seed)) == 0, "DRNG init");
    unsigned long long pk_len = 0, sk_len = 0;
    require(sig_keygen(pk, &pk_len, sk, &sk_len) == 0, "keygen");
    require(pk_len == PK_LEN_BYTES && sk_len == SK_LEN_BYTES, "key lengths");

    unsigned long (*H)[M] = malloc(sizeof(*H) * embded_num);
    RawGF2EX (*H_hat)[COLS] = malloc(sizeof(*H_hat) * ROWS);
    uint16_t *y_hat = malloc(sizeof(*y_hat) * ROWS);
    unsigned long y[M_K + 12];
    unsigned char H_seed[SEED_BYTES];
    require(H && H_hat && y_hat, "public-state allocations");
    sig_unpack_public_key(pk, H_seed, y);
    random_H(H, H_seed);
    embed_H_raw(H_hat, H);
    embed_y_raw(y_hat, y);

    RawGF2EX *open_values = calloc((size_t)fixed_chunks * s, sizeof(*open_values));
    unsigned char *seen = calloc((size_t)fixed_chunks * s, 1);
    unsigned *counts = calloc(fixed_chunks, sizeof(*counts));
    unsigned long *recovered = calloc(M, sizeof(*recovered));
    unsigned char *signature = malloc(MAX_SIGNATURE_BYTES);
    VerifierWorkspace *vw = malloc(sizeof(*vw));
    ProverWorkspace *pw = malloc(sizeof(*pw));
    require(open_values && seen && counts && recovered && signature && vw && pw,
            "attack allocations");

    RawGF2EX e_eval, v_eval, p_poly[SIGMA_REPETITIONS], rv[SIGMA_REPETITIONS];
    RawGF2EX G[6], w[N_PRIME_LEN], openings[opennum];
    unsigned char roots[total_groups][HASH_DIGEST_LENGTH];
    unsigned char hashes[MERKLE_MAX_PROOF_HASHES][HASH_DIGEST_LENGTH];
    unsigned char fs_input[FS_COMMITMENT_INPUT_BYTES];
    RawGF2EX z, delta, check1, check2;
    unsigned queries = 0;

    for (; queries < 100; ++queries) {
        unsigned done = 1;
        for (unsigned c = 0; c < fixed_chunks; ++c) done &= counts[c] >= k;
        if (done) break;
        unsigned char message[32];
        memset(message, 0, sizeof(message));
        message[0] = (unsigned char)queries;
        message[1] = (unsigned char)(queries >> 8);
        unsigned long long sig_len = 0;
        require(sig_sign(sk, sk_len, message, sizeof(message), signature, &sig_len) == 0,
                "oracle signing");
        require(sig_verify(pk, pk_len, signature, sig_len, message, sizeof(message)) == 0,
                "honest signature rejected");

        size_t hash_count = 0;
        require(sig_deserialize_signature(signature, sig_len, &e_eval, &v_eval, roots,
                  p_poly, rv, G, w, hashes, &hash_count, openings) == 0,
                "signature parsing");
        sig_init_verifier_workspace(vw);
        memset(fs_input, 0, HASH_DIGEST_LENGTH);
        memcpy(fs_input + HASH_DIGEST_LENGTH, pk, PK_LEN_BYTES);
        require(Verifier(vw, message, sizeof(message), fs_input, H_hat, y_hat,
                  &e_eval, &v_eval, roots, p_poly, rv, G, w, hashes, hash_count,
                  openings, &z, &delta, &check1, &check2), "verification transcript");
        for (unsigned i = 0; i < opennum; ++i) {
            unsigned global = (unsigned)vw->derived_trees[i] * group_size
                            + (unsigned)(vw->derived_choices[i] - MERKLE_LEAF_OFFSET);
            unsigned chunk = global / s, local = global % s;
            if (chunk >= fixed_chunks || seen[chunk * s + local]) continue;
            open_values[chunk * s + local] = openings[i];
            seen[chunk * s + local] = 1;
            ++counts[chunk];
        }
    }
    require(queries < 100, "opening coverage within 100 signatures");
    for (unsigned c = 0; c < fixed_chunks; ++c)
        require(recover_chunk(c, k, s, seen, open_values, recovered) == 0,
                "chunk interpolation and one-hot check");
    require(recover_suffix(fixed_chunks * k, recovered, y, H) == 0,
            "public syndrome suffix solve");
    for (unsigned r = 0; r < embded_num; ++r) {
        unsigned parity = 0;
        for (unsigned col = 0; col < M; ++col)
            parity ^= (unsigned)(H[r][col] & recovered[col]);
        require(parity == (unsigned)y[r], "full public syndrome check");
    }

    RawGF2EX *e_hat = malloc(sizeof(*e_hat) * N_PRIME_LEN);
    RawGF2EX *ye = malloc(sizeof(*ye) * ROWS);
    require(e_hat && ye, "forgery allocations");
    embed_e_raw(e_hat, recovered);
    Compute_YE_Raw(ye, H_hat, e_hat);
    const unsigned char fresh_message[] = "independent Sigurd recovered-witness forgery";
    sig_init_prover_workspace(pw);
    memset(fs_input, 0, HASH_DIGEST_LENGTH);
    memcpy(fs_input + HASH_DIGEST_LENGTH, pk, PK_LEN_BYTES);
    ProverOutputs output = {0};
    Prover(pw, fresh_message, sizeof(fresh_message) - 1, fs_input,
#ifdef SIGURD_LEVEL_256
           recovered,
#endif
           H_hat, y_hat,
           e_hat, ye, &z, &e_eval, &v_eval, &delta, &output);
    unsigned long long forged_len = 0;
    require(sig_serialize_signature(signature, &forged_len, &output) == 0,
            "forged signature serialization");
    require(sig_verify(pk, pk_len, signature, forged_len,
              (unsigned char *)fresh_message, sizeof(fresh_message) - 1) == 0,
            "fresh-message forgery rejected");

    /* Control: a different regular witness does not satisfy the public key. */
    unsigned original_choice = 0;
    while (original_choice < 6 && !recovered[original_choice]) ++original_choice;
    require(original_choice < 6, "missing first one-hot block");
    recovered[original_choice] = 0;
    recovered[(original_choice + 1) % 6] = 1;
    embed_e_raw(e_hat, recovered);
    Compute_YE_Raw(ye, H_hat, e_hat);
    sig_init_prover_workspace(pw);
    memset(fs_input, 0, HASH_DIGEST_LENGTH);
    memcpy(fs_input + HASH_DIGEST_LENGTH, pk, PK_LEN_BYTES);
    Prover(pw, fresh_message, sizeof(fresh_message) - 1, fs_input,
#ifdef SIGURD_LEVEL_256
           recovered,
#endif
           H_hat, y_hat, e_hat, ye, &z, &e_eval, &v_eval, &delta, &output);
    require(sig_serialize_signature(signature, &forged_len, &output) == 0,
            "control serialization");
    require(sig_verify(pk, pk_len, signature, forged_len,
              (unsigned char *)fresh_message, sizeof(fresh_message) - 1) != 0,
            "incorrect-witness control accepted");
    printf("SECURITY\tsign-24\tSigurd-%u\trepeated-openings-recovery\tCONFIRMED\t"
           "seed=%lu; %u signatures; %u/%u witness blocks recovered; new-message forgery accepted; incorrect-witness control rejected\n",
           (unsigned)(M == 1302 ? 128 : M == 2748 ? 256 : 512), seed_id, queries,
           (unsigned)COLS, (unsigned)COLS);
    free(ye); free(e_hat); free(pw); free(vw); free(signature);
    free(recovered); free(counts); free(seen); free(open_values);
    free(y_hat); free(H_hat); free(H);
    return 0;
}
