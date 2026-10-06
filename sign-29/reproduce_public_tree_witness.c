/*
 * Tins256 one-signature public-tree witness recovery and fresh-message forgery.
 * The only attacker inputs after the honest signing call are pk and signature.
 * Linker --wrap substitutes the recovered witness for the two secret-expander
 * draws inside the submitted signer; the source signer and verifier are intact.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SIG_TINS256.h"
#include "drng.h"
#include "params.h"
#include "bavc_commit.h"

DRNG_ctx drng_algorithm;
static unsigned char recovered_alpha[N_TUPLE_SIZE];
static unsigned char recovered_beta[N_TUPLE_SIZE];
static unsigned char true_alpha[N_TUPLE_SIZE];
static unsigned char true_beta[N_TUPLE_SIZE];
static int substitute_witness;
static int substituted_draws;
static int record_witness;
static int recorded_draws;

int __real_get_random_number(DRNG_ctx *, unsigned char *, unsigned long long);
int __wrap_get_random_number(DRNG_ctx *ctx, unsigned char *out,
                             unsigned long long bits)
{
    if (substitute_witness && bits == N_TUPLE - 2 && substituted_draws < 2) {
        memcpy(out, substituted_draws ? recovered_beta : recovered_alpha,
               N_TUPLE_SIZE);
        substituted_draws++;
        return 0;
    }
    int rc = __real_get_random_number(ctx, out, bits);
    if (!rc && record_witness && bits == N_TUPLE - 2 && recorded_draws < 2) {
        memcpy(recorded_draws ? true_beta : true_alpha, out, N_TUPLE_SIZE);
        recorded_draws++;
    }
    return rc;
}

static void fail(const char *what)
{
    fprintf(stderr, "FAIL %s\n", what);
    exit(1);
}

int main(void)
{
    unsigned char seed[48], source_message[] = "Tins original witness message";
    unsigned char target_message[] = "Tins forged fresh message";
    unsigned char pk[PK_SIZE], sk[SK_SIZE], equivalent_sk[SK_SIZE] = {0};
    unsigned char *signature = calloc(1, SIG_SIZE + 4096);
    unsigned char *forgery = calloc(1, SIG_SIZE + 4096);
    unsigned long long pk_len = 0, sk_len = 0, sig_len = 0, forgery_len = 0;
    if (!signature || !forgery) fail("allocation");
    for (unsigned i = 0; i < sizeof seed; i++) seed[i] = (unsigned char)(7 * i + 1);
    if (init_random_number(&drng_algorithm, seed, sizeof seed)) fail("DRBG init");
    if (sig_keygen(pk, &pk_len, sk, &sk_len)) fail("keygen");
    record_witness = 1;
    if (sig_sign(sk, sk_len, source_message, sizeof source_message,
                 signature, &sig_len)) fail("honest signing");
    record_witness = 0;
    if (recorded_draws != 2) fail("true witness draw count");

    /* Decode only the public salt and packed auxiliary responses. */
    unsigned char salt[SALT_SIZE];
    memcpy(salt, signature, SALT_SIZE);
    const int fixed_len = SALT_SIZE + sizeof(long long) + sizeof(hash_t)
        + sizeof(commitment) * TAU + ((N_TUPLE - 2) * TAU * 2 + K * TAU + 7) / 8;
    if (sig_len < (unsigned)fixed_len) fail("signature length");
    const int path_size = (int)((sig_len - fixed_len) / NODE_SIZE);
    const size_t aux_pos = SALT_SIZE + sizeof(long long) + sizeof(hash_t)
        + NODE_SIZE * (size_t)path_size + sizeof(commitment) * TAU
        + (K * TAU + 7) / 8;
    const size_t packed_len = (TAU * 2 * (N_TUPLE - 2) + 7) / 8;
    if (aux_pos + packed_len - 1 > sig_len) fail("auxiliary offset");
    unsigned char *packed = calloc(packed_len, 1);
    if (!packed) fail("packed allocation");
    memcpy(packed, signature + aux_pos, packed_len);

    static unsigned char public_aux[TAU][N_TUPLE_SIZE * 2];
    static unsigned char zero_witness_aux[TAU][N_TUPLE_SIZE * 2];
    decompress_aux(packed, public_aux, TAU);
    unsigned char zero_alpha[N_TUPLE_SIZE] = {0};
    unsigned char zero_beta[N_TUPLE_SIZE] = {0};
    unsigned char arbitrary_root[RSEED_SIZE] = {0};
    ggm_tree *tree = malloc(sizeof *tree);
    commitment (*commitments)[N] = calloc(TAU, sizeof(commitment[N]));
    static ff12b base[TAU][2 * N_TUPLE - 3];
    static ff12b delta[TAU];
    hash_t commitment_hash;
    if (!tree || !commitments) fail("tree allocation");
    CommitPoly(salt, arbitrary_root, *tree, commitments, zero_alpha, zero_beta,
               zero_witness_aux, base, delta, commitment_hash);

    for (int e = 0; e < TAU; e++) {
        unsigned char alpha[N_TUPLE_SIZE], beta[N_TUPLE_SIZE];
        for (int j = 0; j < N_TUPLE_SIZE; j++) {
            alpha[j] = public_aux[e][j] ^ zero_witness_aux[e][j];
            beta[j] = public_aux[e][j + N_TUPLE_SIZE]
                    ^ zero_witness_aux[e][j + N_TUPLE_SIZE];
        }
        alpha[N_TUPLE_SIZE - 1] &= 0xc0;
        beta[N_TUPLE_SIZE - 1] &= 0xc0;
        if (e == 0) {
            memcpy(recovered_alpha, alpha, sizeof alpha);
            memcpy(recovered_beta, beta, sizeof beta);
        } else if (memcmp(recovered_alpha, alpha, sizeof alpha)
                || memcmp(recovered_beta, beta, sizeof beta)) {
            fail("repetition disagreement");
        }
    }
    printf("PASS: %d/%d repetitions recover one common witness\n", TAU, TAU);
    if (memcmp(recovered_alpha, true_alpha, sizeof true_alpha)
            || memcmp(recovered_beta, true_beta, sizeof true_beta))
        fail("recovered witness differs from true signing witness");
    printf("PASS: recovered witness matches the true signing witness\n");

    /* The signing seed is replaced by zeros; the public seed comes from pk. */
    memcpy(equivalent_sk + SK_SEEDLEN, pk, PK_SEEDLEN);
    memset(sk, 0, sizeof sk);
    substitute_witness = 1;
    substituted_draws = 0;
    if (sig_sign(equivalent_sk, sizeof equivalent_sk,
                 target_message, sizeof target_message, forgery, &forgery_len))
        fail("attacker signing");
    substitute_witness = 0;
    if (substituted_draws != 2) fail("witness substitution count");
    unsigned char *wrong_message_signature = malloc(forgery_len);
    if (!wrong_message_signature) fail("negative-control allocation");
    memcpy(wrong_message_signature, forgery, forgery_len);
    target_message[0] ^= 1;
    if (sig_verify(pk, pk_len, wrong_message_signature, forgery_len,
                   target_message, sizeof target_message) == 0)
        fail("wrong-message negative control");
    target_message[0] ^= 1;
    free(wrong_message_signature);
    if (sig_verify(pk, pk_len, forgery, forgery_len,
                   target_message, sizeof target_message))
        fail("fresh-message verifier acceptance");
    printf("PASS: fresh-message signature accepted; wrong message rejected\n");

    free(tree);
    free(commitments);
    free(packed);
    free(signature);
    free(forgery);
    return 0;
}
