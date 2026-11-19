/* hash-02: AXIS byte-aligned fast path wraps padded_bits to 0 near 2^64.
 *
 * Build with -DDIG=512, 768 or 1024 against one AXIS instance directory.
 * The message pointer is a PROT_NONE page, so any read of the message would
 * fault. For the eight byte-aligned lengths 2^64-128, 2^64-120, ..., 2^64-72
 * CryptHash must return 0 with one and the same digest. Control: the digest
 * of the empty message differs from it.
 */
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest);

#define NB (DIG / 8)

int main(void)
{
    unsigned char *na = mmap(NULL, (size_t)sysconf(_SC_PAGESIZE), PROT_NONE,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    unsigned char d[8][NB], e[NB];
    unsigned long long len;
    int k, rc = 0, same = 1, written = 1, i;

    if (na == MAP_FAILED) return 2;
    for (k = 0; k < 8; k++) {
        len = 0ULL - 128ULL + 8ULL * (unsigned long long)k;
        memset(d[k], 0xcc, NB);
        rc |= CryptHash(DIG, na, len, d[k]);
        if (k > 0 && memcmp(d[0], d[k], NB) != 0) same = 0;
    }
    for (i = 0; i < NB; i++) if (d[0][i] != 0xcc) break;
    if (i == NB) written = 0;
    memset(e, 0xcc, NB);
    rc |= CryptHash(DIG, (const unsigned char *)"", 0, e);
    printf("AXIS-%d digest(2^64-128 .. 2^64-72) = ", DIG);
    for (i = 0; i < 16; i++) printf("%02x", d[0][i]);
    printf("... rc=%d identical_for_8_lengths=%d differs_from_empty=%d\n",
           rc, same, memcmp(d[0], e, NB) != 0 && written);
    return 0;
}
