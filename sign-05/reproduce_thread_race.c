/* sign-05-7: Chinith verification keeps per-call scratch buffers in file-scope
 * caches, so honest verifications running in different threads overwrite each
 * other's intermediate values and fail.
 *
 * bavc.c declares `static bavc_scratch_cache_t g_bavc_scratch` and vole.c
 * `static vole_scratch_cache_t g_vole_scratch`. reconstruct_keys and
 * bavc_reconstruct_em take the opening-path map, the reconstructed tree keys
 * and the leaf commitments from the first cache (zeroing the key buffer on
 * every call), and vole_reconstruct takes its row workspace from the second.
 * Every call in the process shares these buffers without a lock.
 *
 * Honest inputs only: THREADS key pairs from sig_keygen and one signature from
 * sig_sign under each, all made before any thread starts. The argument is the
 * number of rounds per thread. Prints failed/total honest verifications for:
 * one thread taking the keys in turn (control); THREADS threads, one per key,
 * released together and then free-running; and one thread again afterwards
 * (control), with a check that keys and signatures are unchanged. */
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drng.h"

DRNG_ctx drng_algorithm;

unsigned long long sig_get_pk_len_bytes(void);
unsigned long long sig_get_sk_len_bytes(void);
unsigned long long sig_get_sn_len_bytes(void);
int sig_keygen(unsigned char *pk, unsigned long long *pk_len_bytes,
               unsigned char *sk, unsigned long long *sk_len_bytes);
int sig_sign(unsigned char *sk, unsigned long long sk_len_bytes,
             unsigned char *m, unsigned long long m_len_bytes,
             unsigned char *sn, unsigned long long *sn_len_bytes);
int sig_verify(unsigned char *pk, unsigned long long pk_len_bytes,
               unsigned char *sn, unsigned long long sn_len_bytes,
               unsigned char *m, unsigned long long m_len_bytes);

#define THREADS 4
#define MLEN 32

static unsigned char *pk[THREADS], *sn[THREADS], *pk0[THREADS], *sn0[THREADS];
static unsigned char msg[THREADS][MLEN];
static unsigned long long pkl[THREADS], snl[THREADS];
static pthread_barrier_t gate;
static int rounds = 25;

struct job { int key, fail; };

static int honest_verify(int k)
{
    return sig_verify(pk[k], pkl[k], sn[k], snl[k], msg[k], MLEN);
}

static void *worker(void *arg)
{
    struct job *j = arg;
    pthread_barrier_wait(&gate);
    for (int r = 0; r < rounds; r++)
        j->fail += honest_verify(j->key) != 0;
    return NULL;
}

/* THREADS threads; thread t verifies the honest signature under key t */
static int concurrent(void)
{
    pthread_t th[THREADS];
    struct job job[THREADS];
    int fail = 0;

    if (pthread_barrier_init(&gate, NULL, THREADS) != 0) exit(2);
    for (int t = 0; t < THREADS; t++) {
        job[t].key = t;
        job[t].fail = 0;
        if (pthread_create(&th[t], NULL, worker, &job[t]) != 0) exit(2);
    }
    for (int t = 0; t < THREADS; t++) {
        pthread_join(th[t], NULL);
        fail += job[t].fail;
    }
    pthread_barrier_destroy(&gate);
    return fail;
}

int main(int argc, char **argv)
{
    unsigned char seed[48];
    unsigned long long pkmax = sig_get_pk_len_bytes(), skmax = sig_get_sk_len_bytes();
    unsigned long long snmax = sig_get_sn_len_bytes();
    unsigned char *sk = malloc(skmax);
    int before = 0, during, after = 0, same = 1;

    if (argc > 1) rounds = atoi(argv[1]);
    if (rounds < 1 || !sk) return 2;
    for (size_t i = 0; i < sizeof seed; i++) seed[i] = (unsigned char)(7 * i + 5);
    if (init_random_number(&drng_algorithm, seed, sizeof seed) != 0) return 2;

    for (int k = 0; k < THREADS; k++) {
        unsigned long long skl = skmax;
        pk[k] = malloc(pkmax); sn[k] = malloc(snmax);
        pk0[k] = malloc(pkmax); sn0[k] = malloc(snmax);
        if (!pk[k] || !sn[k] || !pk0[k] || !sn0[k]) return 2;
        pkl[k] = pkmax; snl[k] = snmax;
        for (int i = 0; i < MLEN; i++) msg[k][i] = (unsigned char)(41 * k + 3 * i + 1);
        if (sig_keygen(pk[k], &pkl[k], sk, &skl) != 0) return 2;
        if (sig_sign(sk, skl, msg[k], MLEN, sn[k], &snl[k]) != 0) return 2;
        memcpy(pk0[k], pk[k], pkl[k]);
        memcpy(sn0[k], sn[k], snl[k]);
    }

    for (int r = 0; r < rounds; r++)
        for (int k = 0; k < THREADS; k++) before += honest_verify(k) != 0;
    during = concurrent();
    for (int k = 0; k < THREADS; k++) {
        after += honest_verify(k) != 0;
        same &= memcmp(pk0[k], pk[k], pkl[k]) == 0 && memcmp(sn0[k], sn[k], snl[k]) == 0;
    }

    printf("one thread %d/%d fail; four threads %d/%d fail; "
           "one thread afterwards %d/%d fail, inputs unchanged=%d\n",
           before, THREADS * rounds, during, THREADS * rounds, after, THREADS, same);
    return 0;
}
