/* Driver for reproduce_reduced_round_rk.sh (hash-06, Cuishen).
 * Linked against the submitters' reference CryptHash with the Octarx round
 * loops limited to NR rounds. Hashes random message pairs that differ only in
 * the given bits of one 1024-bit block and counts full-digest collisions.
 * usage: t <variant> <trials>
 *   variant 0: Cuishen-512 trail  m12 ^= 2^56, m5 ^= 2^43      (MSB cancel, p = 1)
 *   variant 1: Cuishen-512 trail  m12 ^= 2^(k-7), m5 ^= 2^(k-20), k = 40 (p = 1/2)
 *   variant 2: Cuishen-768/1024 trail m6 ^= 2^45, m7 ^= 2^30, m2 ^= 2^63, m3 ^= 2^63 (p = 1)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
int CryptHash(int, const unsigned char *, unsigned long long, unsigned char *);
static uint64_t rs = 0x243f6a8885a308d3ULL;
static uint64_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return rs; }
static void flip(unsigned char *blk, int word, int bit) {
  blk[8 * word + (7 - bit / 8)] ^= (unsigned char)(1u << (bit % 8));
}
int main(int argc, char **argv) {
  int variant = atoi(argv[1]), trials = atoi(argv[2]), hits = 0, diffmsg = 0;
  static unsigned char m1[1024], m2[1024];
  unsigned char d1[128], d2[128];
  for (int t = 0; t < trials; t++) {
    unsigned long long nbits = 1024 + rnd() % 3072;       /* 1..4 blocks + tail */
    size_t nb = (size_t)((nbits + 7) / 8);
    unsigned long long blocks = nbits / 1024;
    unsigned char *blk;
    for (size_t i = 0; i < nb; i++) m1[i] = (unsigned char)rnd();
    memcpy(m2, m1, nb);
    blk = m2 + 128 * (rnd() % blocks);                    /* any full block */
    if (variant == 0) { flip(blk, 12, 56); flip(blk, 5, 43); }
    else if (variant == 1) { flip(blk, 12, 40 - 7); flip(blk, 5, 40 - 20); }
    else { flip(blk, 6, 45); flip(blk, 7, 30); flip(blk, 2, 63); flip(blk, 3, 63); }
    diffmsg += memcmp(m1, m2, nb) != 0;
    if (CryptHash(DLEN, m1, nbits, d1) || CryptHash(DLEN, m2, nbits, d2)) return 2;
    hits += memcmp(d1, d2, DLEN / 8) == 0;
  }
  printf("collisions=%d distinct=%d trials=%d\n", hits, diffmsg, trials);
  return 0;
}
