/* sign-24-6: Sigurd verification uses one static Reed-Solomon workspace, so
 * honest verifications of different signatures in different threads overwrite
 * each other's codeword and fail.
 *
 * The reference sig_core.c declares `static RSWorkspace g_rs_workspace`
 * (Sigurd-128 :55). RS_encode (:640-745) copies its input into that
 * workspace, encodes in place and copies the result out; the verifier calls it
 * on the signature's w polynomial (Commitment_Open_Verifier_Raw, :1071). Every
 * call in the process shares the workspace without a lock.
 *
 * Honest inputs only: THREADS key pairs from sig_keygen and one signature from
 * sig_sign under each, all made before any thread starts. The argument is the
 * number of rounds per thread. Prints failed/total honest verifications for:
 * one thread taking the keys in turn (control); THREADS threads, one per key,
 * released together and then free-running; and one thread again afterwards
 * (control), with a check that keys and signatures are unchanged.
 *
 * With "accept PROCS ROUNDS PAR" (optimized trees, whose sig_verify runs the
 * whole verifier on the static g_cached_verifier_workspace): one key pair and
 * one honest signature on msg, and alt = msg with one bit flipped. Control,
 * one thread: CROUNDS rounds of three verifications of the genuine message and
 * one of alt. Test: PROCS forked child processes, at most PAR at a time, each
 * with THREADS threads that hold private copies of pk and the signature; three
 * verify msg and one verifies alt, all released together by a barrier in each
 * of ROUNDS rounds. Counters live in shared memory, so a child that aborts or
 * segfaults is counted as an abnormal exit and the calls it completed still
 * count. Then one thread again, with a check that every input is unchanged. */
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
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

/* ---- accept mode ---- */

#define ALT (THREADS - 1)   /* the thread that verifies the altered message */
#define CROUNDS 50

struct slot {               /* per child, in shared memory */
    volatile long calls[THREADS], bad[THREADS];   /* bad: rejected (genuine) or accepted (alt) */
    volatile int finished, unchanged;
};

static unsigned char amsg[MLEN], aalt[MLEN];

struct ajob {
    int t;
    struct slot *s;
    unsigned char *pk, *sn, m[MLEN];
};

static void *accept_worker(void *arg)
{
    struct ajob *j = arg;
    for (int r = 0; r < rounds; r++) {
        pthread_barrier_wait(&gate);
        int rc = sig_verify(j->pk, pkl[0], j->sn, snl[0], j->m, MLEN);
        if (j->t == ALT ? rc == 0 : rc != 0) j->s->bad[j->t]++;
        j->s->calls[j->t]++;
    }
    return NULL;
}

static void accept_child(struct slot *s)
{
    pthread_t th[THREADS];
    struct ajob job[THREADS];
    int same = 1, devnull = open("/dev/null", O_WRONLY);

    if (devnull >= 0) { dup2(devnull, 1); dup2(devnull, 2); }
    alarm(60);
    if (pthread_barrier_init(&gate, NULL, THREADS) != 0) _exit(2);
    for (int t = 0; t < THREADS; t++) {
        job[t].t = t;
        job[t].s = s;
        job[t].pk = malloc(pkl[0]);
        job[t].sn = malloc(snl[0]);
        if (!job[t].pk || !job[t].sn) _exit(2);
        memcpy(job[t].pk, pk[0], pkl[0]);
        memcpy(job[t].sn, sn[0], snl[0]);
        memcpy(job[t].m, t == ALT ? aalt : amsg, MLEN);
    }
    for (int t = 0; t < THREADS; t++)
        if (pthread_create(&th[t], NULL, accept_worker, &job[t]) != 0) _exit(2);
    for (int t = 0; t < THREADS; t++) pthread_join(th[t], NULL);
    for (int t = 0; t < THREADS; t++)
        same &= memcmp(job[t].pk, pk[0], pkl[0]) == 0 && memcmp(job[t].sn, sn[0], snl[0]) == 0 &&
                memcmp(job[t].m, t == ALT ? aalt : amsg, MLEN) == 0;
    s->unchanged = same;
    s->finished = 1;
    _exit(0);
}

static int accept_test(int procs, int par)
{
    unsigned long long skl = sig_get_sk_len_bytes();
    unsigned char *sk = malloc(skl), *pk1, *sn1;
    struct rlimit nocore = { 0, 0 };
    struct slot *slot;
    long cgen = 0, calt = 0, cg = 0, ca = 0, ngen = 0, nalt = 0, fgen = 0, aalt_n = 0, aalt_crashed = 0;
    int running = 0, started = 0, abnormal = 0, nabrt = 0, nsegv = 0, nother = 0, same = 1, ag, aa;

    pk[0] = malloc(pkl[0] = sig_get_pk_len_bytes());
    sn[0] = malloc(snl[0] = sig_get_sn_len_bytes());
    if (!sk || !pk[0] || !sn[0]) return 2;
    for (int i = 0; i < MLEN; i++) amsg[i] = (unsigned char)(3 * i + 1);
    if (sig_keygen(pk[0], &pkl[0], sk, &skl) != 0) return 2;
    if (sig_sign(sk, skl, amsg, MLEN, sn[0], &snl[0]) != 0) return 2;
    memcpy(aalt, amsg, MLEN);
    aalt[0] ^= 1;
    pk1 = malloc(pkl[0]); sn1 = malloc(snl[0]);
    if (!pk1 || !sn1) return 2;
    memcpy(pk1, pk[0], pkl[0]);
    memcpy(sn1, sn[0], snl[0]);

    for (int r = 0; r < CROUNDS; r++) {
        for (int t = 0; t < THREADS - 1; t++, cg++)
            cgen += sig_verify(pk[0], pkl[0], sn[0], snl[0], amsg, MLEN) != 0;
        calt += sig_verify(pk[0], pkl[0], sn[0], snl[0], aalt, MLEN) == 0;
        ca++;
    }

    slot = mmap(NULL, (size_t)procs * sizeof *slot, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (slot == MAP_FAILED) return 2;
    memset(slot, 0, (size_t)procs * sizeof *slot);
    setrlimit(RLIMIT_CORE, &nocore);
    fflush(stdout);
    fflush(stderr);
    while (started < procs || running > 0) {
        if (started < procs && running < par) {
            pid_t p = fork();
            if (p < 0) return 2;
            if (p == 0) accept_child(&slot[started]);
            started++;
            running++;
            continue;
        }
        int st;
        pid_t p = wait(&st);
        if (p < 0) return 2;
        running--;
        if (!(WIFEXITED(st) && WEXITSTATUS(st) == 0)) {
            abnormal++;
            if (WIFSIGNALED(st) && WTERMSIG(st) == SIGABRT) nabrt++;
            else if (WIFSIGNALED(st) && WTERMSIG(st) == SIGSEGV) nsegv++;
            else nother++;
        }
    }
    for (int c = 0; c < procs; c++) {
        for (int t = 0; t < THREADS; t++) {
            if (t == ALT) {
                nalt += slot[c].calls[t];
                aalt_n += slot[c].bad[t];
                if (!slot[c].finished) aalt_crashed += slot[c].bad[t];
            } else {
                ngen += slot[c].calls[t];
                fgen += slot[c].bad[t];
            }
        }
        if (slot[c].finished) same &= slot[c].unchanged;
    }

    ag = sig_verify(pk[0], pkl[0], sn[0], snl[0], amsg, MLEN) != 0;
    aa = sig_verify(pk[0], pkl[0], sn[0], snl[0], aalt, MLEN) == 0;
    same &= memcmp(pk1, pk[0], pkl[0]) == 0 && memcmp(sn1, sn[0], snl[0]) == 0 && (amsg[0] ^ aalt[0]) == 1 &&
            memcmp(amsg + 1, aalt + 1, MLEN - 1) == 0;

    printf("one thread: genuine message rejected %ld/%ld, altered message accepted %ld/%ld\n", cgen, cg, calt, ca);
    printf("four threads, three on the genuine message and one on the altered message, %d processes of %d rounds: "
           "altered message accepted %ld/%ld (%ld in processes that then exited abnormally), genuine message "
           "rejected %ld/%ld; abnormal exits %d/%d (SIGABRT %d, SIGSEGV %d, other %d)\n",
           procs, rounds, aalt_n, nalt, aalt_crashed, fgen, ngen, abnormal, procs, nabrt, nsegv, nother);
    printf("one thread afterwards: genuine message rejected %d/1, altered message accepted %d/1, inputs unchanged=%d\n",
           ag, aa, same);
    return 0;
}

int main(int argc, char **argv)
{
    unsigned char seed[48];
    unsigned long long pkmax = sig_get_pk_len_bytes(), skmax = sig_get_sk_len_bytes();
    unsigned long long snmax = sig_get_sn_len_bytes();
    unsigned char *sk = malloc(skmax);
    int before = 0, during, after = 0, same = 1;

    int accept = argc > 4 && strcmp(argv[1], "accept") == 0;
    if (accept) rounds = atoi(argv[3]);
    else if (argc > 1) rounds = atoi(argv[1]);
    if (rounds < 1 || !sk) return 2;
    for (size_t i = 0; i < sizeof seed; i++) seed[i] = (unsigned char)(7 * i + 5);
    if (init_random_number(&drng_algorithm, seed, sizeof seed) != 0) return 2;
    if (accept) {
        int procs = atoi(argv[2]), par = atoi(argv[4]);
        if (procs < 1 || par < 1) return 2;
        free(sk);
        return accept_test(procs, par);
    }

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
