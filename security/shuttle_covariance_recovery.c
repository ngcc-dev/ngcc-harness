/* Shuttle covariance key recovery using ordinary submitted signatures.
 * The attack phase uses only the public key and signatures. The original
 * secret key is held solely to answer sig_sign oracle queries.
 */
#include "SIG_AlgorithmInstance.h"
#include "params.h"
#include "packing.h"
#include "polyvec.h"
#include "rounding.h"
#include "reduce.h"
#include "xof.h"
#include "drng.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

DRNG_ctx drng_algorithm;

static int32_t nearest_div(int64_t num, int64_t den)
{
    return num < 0 ? -(int32_t)((-num + den/2) / den)
                   : (int32_t)((num + den/2) / den);
}

/* Public completion: the difference b-a_gen-A_gen*s must be short. */
static int complete_key(uint8_t sk_out[CRYPTO_SECRETKEYBYTES],
                        const uint8_t pk[CRYPTO_PUBLICKEYBYTES],
                        const poly s[ELL])
{
    uint8_t seedA[SEEDBYTES], masterK[CHALLENGESEEDBYTES] = {0};
    uint8_t tr[CHALLENGESEEDBYTES];
    poly b[EM], b0[EM], ep[EM], zero[EM] = {0}, full[KVEC], stretched[KVEC];
    poly16 agen[EM], hAgen[EM * ELL];
    if (unpack_pk(seedA, b, pk) != 0) return -1;
    expand_a(agen, hAgen, seedA);
    keygen_bproduct(b0, agen, hAgen, s, zero);
    for (unsigned i = 0; i < EM; ++i) {
        for (unsigned j = 0; j < N; ++j) {
            int32_t d = freeze(b[i].coeffs[j] - b0[i].coeffs[j]);
            if (d > Q/2) d -= Q;
            if (d < -BE_ENC || d > BE_ENC) return -1;
            ep[i].coeffs[j] = d;
        }
    }
    memset(full, 0, sizeof(full));
    full[0].coeffs[0] = 1;
    for (unsigned i = 0; i < ELL; ++i) full[1+i] = s[i];
    for (unsigned i = 0; i < EM; ++i) full[1+ELL+i] = ep[i];
    stretch_s(stretched, full);
    if (!keygen_norm_ok(stretched)) return -1;

    uint8_t pkhash_in[1 + CRYPTO_PUBLICKEYBYTES];
    xof_ctx ctx;
    pkhash_in[0] = DS_HASH_PK;
    memcpy(pkhash_in + 1, pk, CRYPTO_PUBLICKEYBYTES);
    xof256_init(&ctx, pkhash_in, sizeof(pkhash_in));
    xof256_squeeze(&ctx, tr, CHALLENGESEEDBYTES);
    pack_sk(sk_out, seedA, b, masterK, tr, s, ep);
    return 0;
}

static int collect_queries(uint8_t *oracle_sk, unsigned long long sk_len,
                           uint8_t *pk, unsigned long long pk_len,
                           unsigned first_query, unsigned count,
                           unsigned round, unsigned worker,
                           int64_t cov[ELL][N])
{
    uint8_t seed[SEEDLEN], *sig = malloc((size_t)sig_get_sn_len_bytes());
    unsigned long long sig_len = 0;
    poly z1[Z1LEN], hint[EM], c;
    uint8_t seedC[CHALLENGESEEDBYTES];
    if (!sig) return -1;
    for (unsigned i = 0; i < sizeof(seed); ++i)
        seed[i] = (uint8_t)(31u + 47u * i + 19u * worker + 43u * round);
    if (init_random_number(&drng_algorithm, seed, sizeof(seed)) != 0) return -1;
    for (unsigned query = first_query; query < first_query + count; ++query) {
        uint8_t message[32] = {0};
        for (unsigned b = 0; b < 4; ++b) message[b] = (uint8_t)((query >> (8*b)) & 255);
        if (sig_sign(oracle_sk, sk_len, message, sizeof(message), sig, &sig_len) != 0) {
            free(sig); return -1;
        }
        if (sig_verify(pk, pk_len, sig, sig_len, message, sizeof(message)) != 0) {
            free(sig); return -1;
        }
        if (unpack_sig(seedC, z1, hint, sig) != 0) {
            free(sig); return -1;
        }
        sample_c(&c, seedC);
        for (unsigned j = 0; j < N; ++j) {
            if (c.coeffs[j] != 1) continue;
            int64_t anchor = z1[0].coeffs[j];
            for (unsigned p = 0; p < ELL; ++p) {
                for (unsigned d = 0; d < N; ++d) {
                    unsigned at = j + d;
                    int32_t val = at < N ? z1[1+p].coeffs[at]
                                         : -z1[1+p].coeffs[at - N];
                    cov[p][d] += anchor * val;
                }
            }
        }
    }
    free(sig);
    return 0;
}

static int write_all(int fd, const void *buffer, size_t length)
{
    const uint8_t *p = buffer;
    while (length) {
        ssize_t n = write(fd, p, length);
        if (n <= 0) return -1;
        p += n; length -= (size_t)n;
    }
    return 0;
}

static int read_all(int fd, void *buffer, size_t length)
{
    uint8_t *p = buffer;
    while (length) {
        ssize_t n = read(fd, p, length);
        if (n <= 0) return -1;
        p += n; length -= (size_t)n;
    }
    return 0;
}

int main(int argc, char **argv)
{
    const unsigned checkpoint = 25000, limit = 400000;
    unsigned workers = 8;
    if (argc > 2) return 2;
    if (argc == 2) {
        char *end = NULL;
        unsigned long parsed = strtoul(argv[1], &end, 10);
        if (!argv[1][0] || !end || *end || parsed < 1 || parsed > 32) return 2;
        workers = (unsigned)parsed;
    }
    uint8_t seed[SEEDLEN], *pk, *oracle_sk, *equivalent_sk, *sig;
    unsigned long long pk_len = 0, sk_len = 0, sig_len = 0;
    int64_t cov[ELL][N] = {{0}};
    poly estimate[ELL];
    for (unsigned i = 0; i < sizeof(seed); ++i)
        seed[i] = (uint8_t)(31u + 47u * i);
    if (init_random_number(&drng_algorithm, seed, sizeof(seed)) != 0) return 2;
    pk = malloc((size_t)sig_get_pk_len_bytes());
    oracle_sk = malloc((size_t)sig_get_sk_len_bytes());
    equivalent_sk = malloc((size_t)sig_get_sk_len_bytes());
    sig = malloc((size_t)sig_get_sn_len_bytes());
    if (!pk || !oracle_sk || !equivalent_sk || !sig) return 2;
    if (sig_keygen(pk, &pk_len, oracle_sk, &sk_len) != 0) return 2;

    for (unsigned query = checkpoint; query <= limit; query += checkpoint) {
        int pipes[32][2];
        pid_t children[32];
        unsigned base = query - checkpoint + 1;
        unsigned per_worker = checkpoint / workers;
        unsigned extra = checkpoint % workers;
        for (unsigned w = 0; w < workers; ++w) {
            unsigned count = per_worker + (w < extra);
            if (pipe(pipes[w]) != 0) return 2;
            children[w] = fork();
            if (children[w] < 0) return 2;
            if (children[w] == 0) {
                int64_t partial[ELL][N] = {{0}};
                close(pipes[w][0]);
                int rc = collect_queries(oracle_sk, sk_len, pk, pk_len,
                                         base, count, query/checkpoint, w, partial);
                if (rc == 0) rc = write_all(pipes[w][1], partial, sizeof(partial));
                close(pipes[w][1]);
                _exit(rc == 0 ? 0 : 1);
            }
            close(pipes[w][1]);
            base += count;
        }
        for (unsigned w = 0; w < workers; ++w) {
            int64_t partial[ELL][N];
            int status = 0;
            int ok = read_all(pipes[w][0], partial, sizeof(partial));
            close(pipes[w][0]);
            if (waitpid(children[w], &status, 0) < 0 || ok != 0 ||
                !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
                fprintf(stderr, "FAIL: worker %u at checkpoint %u\n", w, query);
                return 1;
            }
            for (unsigned p = 0; p < ELL; ++p)
                for (unsigned d = 0; d < N; ++d) cov[p][d] += partial[p][d];
        }
        int64_t denominator = 2LL * (int64_t)query * TAU;
        for (unsigned p = 0; p < ELL; ++p) {
            for (unsigned d = 0; d < N; ++d) {
                int32_t v = nearest_div(cov[p][d], denominator);
                if (v < -BS_ENC) v = -BS_ENC;
                if (v > BS_ENC) v = BS_ENC;
                estimate[p].coeffs[d] = v;
            }
        }
        if (complete_key(equivalent_sk, pk, estimate) != 0) {
            fprintf(stderr, "checkpoint %u: public completion rejected\n", query);
            continue;
        }
        const uint8_t fresh_message[] = "Shuttle equivalent-key forgery";
        sig_len = 0;
        if (sig_sign(equivalent_sk, sk_len, (uint8_t *)fresh_message,
                     sizeof(fresh_message)-1, sig, &sig_len) != 0) {
            fprintf(stderr, "FAIL: recovered-key signing\n");
            return 1;
        }
        if (sig_verify(pk, pk_len, sig, sig_len, (uint8_t *)fresh_message,
                       sizeof(fresh_message)-1) != 0) {
            fprintf(stderr, "FAIL: recovered-key verifier verdict\n");
            return 1;
        }
        uint8_t wrong_message[sizeof(fresh_message)];
        memcpy(wrong_message, fresh_message, sizeof(wrong_message));
        wrong_message[0] ^= 1;
        if (sig_verify(pk, pk_len, sig, sig_len, wrong_message,
                       sizeof(fresh_message)-1) == 0) {
            fprintf(stderr, "FAIL: changed-message control accepted\n");
            return 1;
        }
        printf("SECURITY\tsign-23\tShuttle-%u\tcovariance-key-recovery\tCONFIRMED\t"
               "%u ordinary signatures on %u workers; public key completion; fresh-message forgery accepted; changed-message control rejected\n",
               (unsigned)LAMBDA, query, workers);
        free(sig); free(equivalent_sk); free(oracle_sk); free(pk);
        return 0;
    }
    fprintf(stderr, "NOT-CONFIRMED: no public completion within %u signatures\n", limit);
    free(sig); free(equivalent_sk); free(oracle_sk); free(pk);
    return 1;
}
