/* hash-11: Garnet truncates 64-bit byte counts to 32-bit int.
 *
 * Linked against the reference CryptHash_Garnet.c dispatcher, one
 * Garnet_512*.c rate file, Garnet_768.c and Garnet_1024.c.
 *
 *   ./t hash <digest_bits> <msg_len_bits> [offset=value ...]
 *        hashes a message whose bytes 0..15 are a0..af and whose other
 *        bytes are zero except for the listed ones; the message is a sparse
 *        MAP_NORESERVE mapping. Prints "rc=<rc> <digest hex>".
 *   ./t crash <digest_bits> <msg_len_bits>
 *        runs the same hash on an all-zero message in a child process and
 *        reports how the child ended.
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest);

static unsigned char *message(unsigned long long len)
{
    unsigned long long n = (len + 7) / 8 + 4096;
    void *p = mmap(NULL, (size_t)n, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    return p == MAP_FAILED ? NULL : (unsigned char *)p;
}

int main(int argc, char **argv)
{
    unsigned char d[128];
    unsigned char *m;
    unsigned long long len;
    int bits, i, rc, st;
    pid_t pid;

    if (argc < 4) return 2;
    bits = atoi(argv[2]);
    len = strtoull(argv[3], NULL, 0);
    m = message(len);
    if (m == NULL) { printf("mmap failed\n"); return 77; }

    if (strcmp(argv[1], "crash") == 0) {
        fflush(stdout);
        pid = fork();
        if (pid < 0) return 2;
        if (pid == 0) {
            alarm(120);
            _exit(CryptHash(bits, m, len, d) == 0 ? 0 : 4);
        }
        if (waitpid(pid, &st, 0) != pid) return 2;
        if (WIFSIGNALED(st))
            printf("killed by %s\n", WTERMSIG(st) == SIGSEGV ? "SIGSEGV" :
                   WTERMSIG(st) == SIGALRM ? "timeout" : "another signal");
        else
            printf("exited with status %d\n", WEXITSTATUS(st));
        return 0;
    }

    for (i = 0; i < 16; i++) m[i] = (unsigned char)(0xa0 + i);
    for (i = 4; i < argc; i++) {
        char *eq = strchr(argv[i], '=');
        if (eq == NULL) return 2;
        m[strtoull(argv[i], NULL, 0)] = (unsigned char)strtoul(eq + 1, NULL, 0);
    }
    memset(d, 0xcc, sizeof d);
    rc = CryptHash(bits, m, len, d);
    printf("rc=%d ", rc);
    for (i = 0; i < bits / 8; i++) printf("%02x", d[i]);
    printf("\n");
    return 0;
}
