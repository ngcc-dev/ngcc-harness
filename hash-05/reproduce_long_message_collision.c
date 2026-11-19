/* hash-05: uHash truncates 64-bit lengths to 32-bit int.
 *
 * Build with -DDIG=512, 768 or 1024 against one CryptHash_AlgorithmInstance.c.
 *
 *   ./t collide   hashes two different messages of the same long length (they
 *                 agree on bytes 0..15 only), the 16-byte message with the same
 *                 prefix, and two different 1 KiB messages as a control
 *   ./t crash     hashes an all-zero 2^34-bit (2 GiB) message in a child
 *                 process and reports how the child ended
 *
 * The long messages are sparse anonymous mappings (MAP_NORESERVE): untouched
 * pages read as zero and cost no memory.
 */
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest);

#define NB (DIG / 8)

static unsigned char *map(unsigned long long n)
{
    void *p = mmap(NULL, (size_t)n, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    return p == MAP_FAILED ? NULL : (unsigned char *)p;
}

static void hex(const char *tag, const unsigned char *d)
{
    int i;
    printf("%s ", tag);
    for (i = 0; i < NB; i++) printf("%02x", d[i]);
    printf("\n");
}

static int collide(void)
{
    /* uHash-512 absorbs 768-bit chunks, uHash-768 512-bit chunks and
     * uHash-1024 256-bit chunks; the long length is chosen so that both
     * (int)(len / 8) and (int)(len / chunk) + 1 wrap to their 128-bit values. */
    unsigned long long len = (DIG == 512 ? 3ULL : 1ULL) * (1ULL << 35) + 128;
    unsigned long long n = len / 8, i;
    unsigned char *m = map(n), s[16], c1[1024], c2[1024];
    unsigned char dA[NB], dB[NB], dS[NB], dC1[NB], dC2[NB];
    if (m == NULL) { printf("mmap of %llu bytes failed\n", n); return 77; }
    for (i = 0; i < 16; i++) s[i] = m[i] = (unsigned char)(0xa0 + i);

    memset(dA, 0x11, NB);
    if (CryptHash(DIG, m, len, dA) != 0) return 2;          /* message A */
    memset(m + 16, 0x5a, 64);                               /* message B: */
    m[n / 2] = 0xff;                                        /* bytes 16..79, */
    m[n - 1] = 0x01;                                        /* middle and last differ */
    memset(dB, 0x22, NB);
    if (CryptHash(DIG, m, len, dB) != 0) return 2;
    memset(dS, 0x33, NB);
    if (CryptHash(DIG, s, 128, dS) != 0) return 2;          /* 16-byte message */

    /* control: two 1 KiB messages that differ in the same bytes 16..79 */
    memset(c1, 0, sizeof c1);
    memcpy(c1, s, 16);
    memcpy(c2, c1, sizeof c2);
    memset(c2 + 16, 0x5a, 64);
    memset(dC1, 0x44, NB);
    memset(dC2, 0x55, NB);
    if (CryptHash(DIG, c1, 8 * sizeof c1, dC1) != 0) return 2;
    if (CryptHash(DIG, c2, 8 * sizeof c2, dC2) != 0) return 2;

    printf("uHash-%d long length %llu bits (%llu bytes)\n", DIG, len, n);
    hex("H(A, long) =", dA);
    hex("H(B, long) =", dB);
    hex("H(prefix, 128) =", dS);
    printf("same_length_collision=%d cross_length_collision=%d control_1KiB_differ=%d\n",
           memcmp(dA, dB, NB) == 0, memcmp(dA, dS, NB) == 0, memcmp(dC1, dC2, NB) != 0);
    munmap(m, (size_t)n);
    return 0;
}

static int crash(void)
{
    unsigned long long len = 1ULL << 34, n = len / 8;
    unsigned char d[NB];
    int st;
    pid_t pid;
    fflush(stdout);
    pid = fork();
    if (pid < 0) return 2;
    if (pid == 0) {
        unsigned char *m = map(n);
        if (m == NULL) _exit(3);
        alarm(90);
        _exit(CryptHash(DIG, m, len, d) == 0 ? 0 : 4);
    }
    if (waitpid(pid, &st, 0) != pid) return 2;
    if (WIFSIGNALED(st))
        printf("uHash-%d 2^34-bit message: child killed by signal %d (%s)\n", DIG, WTERMSIG(st),
               WTERMSIG(st) == SIGSEGV ? "SIGSEGV" : WTERMSIG(st) == SIGALRM ? "timeout" : "other");
    else
        printf("uHash-%d 2^34-bit message: child exited with status %d\n", DIG, WEXITSTATUS(st));
    return 0;
}

int main(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "crash") == 0) return crash();
    return collide();
}
