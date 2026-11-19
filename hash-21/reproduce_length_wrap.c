/* hash-21: Neulaser total_bits wraps near 2^64, so CryptHash returns 0
 * without writing the digest.
 *
 * Build with -DDIG=512, 768 or 1024 and -DWRAP_K=960, 1216 or 1472 against one
 * Neulaser instance directory. The message pointer is a PROT_NONE page, so
 * any read of the message would fault, and the digest buffer is prefilled
 * with 0xcc. Five lengths across [2^64 - 64 - (2^64 mod WRAP_K), 2^64 - 65]
 * must each return 0 and leave all DIG/8 digest bytes at 0xcc. Control: the empty
 * message writes the digest.
 */
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest);

#define NB (DIG / 8)

static int untouched(const unsigned char *d)
{
    int i;
    for (i = 0; i < NB; i++) if (d[i] != 0xcc) return 0;
    return 1;
}

int main(void)
{
    unsigned char *na = mmap(NULL, (size_t)sysconf(_SC_PAGESIZE), PROT_NONE,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    unsigned long long top = 0ULL - 65ULL;                      /* 2^64 - 65 */
    unsigned long long span = (0ULL - (unsigned long long)WRAP_K) % (unsigned long long)WRAP_K; /* 2^64 mod WRAP_K */
    unsigned long long lo = top + 1ULL - span;                  /* 2^64 - 64 - span */
    unsigned long long len[5];
    unsigned char d[NB], e[NB];
    int i, ok = 1, rc;

    if (na == MAP_FAILED) return 2;
    len[0] = lo; len[1] = lo + span / 4; len[2] = lo + span / 2; len[3] = top - 7; len[4] = top;
    for (i = 0; i < 5; i++) {
        memset(d, 0xcc, NB);
        rc = CryptHash(DIG, na, len[i], d);
        printf("  Neulaser-%d len=2^64-%llu rc=%d digest_untouched=%d\n", DIG, 0ULL - len[i], rc, untouched(d));
        if (rc != 0 || !untouched(d)) ok = 0;
    }
    memset(e, 0xcc, NB);
    rc = CryptHash(DIG, (const unsigned char *)"", 0, e);
    printf("  Neulaser-%d control len=0 rc=%d digest_untouched=%d\n", DIG, rc, untouched(e));
    printf("result range=[2^64-%llu,2^64-65] unwritten_success=%d control_written=%d\n",
           0ULL - lo, ok, rc == 0 && !untouched(e));
    return 0;
}
