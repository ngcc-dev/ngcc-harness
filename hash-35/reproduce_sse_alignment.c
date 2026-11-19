/* hash-35: Wish x86 implementations use the aligned SSE load _mm_load_si128 on
 * message and padding buffers that need not be 16-byte aligned.
 *
 * Build with -DDIG=512 or 1024 against one Wish directory (with that
 * directory's Makefile flags). Each case runs CryptHash in a child process on
 * a message whose buffer starts at a 64-byte aligned address plus an offset,
 * and prints the digest prefix or the signal that killed the child:
 *
 *   ./t <offset> <msg_len_bits>
 *
 * The message bytes are i*7+1 for i = 0, 1, ...; the digest of a run that
 * completes is comparable across directories.
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest);

int main(int argc, char **argv)
{
    static unsigned char buf[4096] __attribute__((aligned(64)));
    unsigned char d[DIG / 8];
    int fd[2], st, i;
    size_t off;
    unsigned long long len;
    char line[64] = "";
    pid_t pid;

    if (argc != 3) return 2;
    off = (size_t)strtoul(argv[1], NULL, 10);
    len = strtoull(argv[2], NULL, 10);
    if (off + (len + 7) / 8 > sizeof buf) return 2;
    for (i = 0; i < (int)sizeof buf - 64; i++) buf[off + (size_t)i] = (unsigned char)(i * 7 + 1);
    if (pipe(fd) != 0) return 2;
    fflush(stdout);
    pid = fork();
    if (pid < 0) return 2;
    if (pid == 0) {
        int rc;
        close(fd[0]);
        rc = CryptHash(DIG, buf + off, len, d);
        for (i = 0; i < 8; i++) sprintf(line + 2 * i, "%02x", d[i]);
        sprintf(line + 16, " rc=%d", rc);
        if (write(fd[1], line, strlen(line)) < 0) _exit(3);
        _exit(0);
    }
    close(fd[1]);
    i = (int)read(fd[0], line, sizeof line - 1);
    line[i > 0 ? i : 0] = 0;
    if (waitpid(pid, &st, 0) != pid) return 2;
    if (WIFSIGNALED(st))
        printf("offset=%zu len=%llu: killed by %s\n", off, len,
               WTERMSIG(st) == SIGSEGV ? "SIGSEGV" : WTERMSIG(st) == SIGBUS ? "SIGBUS" : "signal");
    else
        printf("offset=%zu len=%llu: digest %s\n", off, len, line);
    return 0;
}
