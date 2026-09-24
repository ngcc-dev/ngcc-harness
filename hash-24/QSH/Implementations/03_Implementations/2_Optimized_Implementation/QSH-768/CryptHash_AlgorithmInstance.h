/* QuantaSylva Hash (QSH) -- instance QSH-768 (x86-64 AVX2, WITHIN-PERMUTATION).
 * Low-latency variant: SIMD across the 16 G's of each permutation; faster on
 * short messages and competitive on long. Requires AVX2. */
#ifndef CRYPTHASH_ALGORITHM_INSTANCE_H
#define CRYPTHASH_ALGORITHM_INSTANCE_H
#define OUTPUT_BLANK_TEST_VECTORS 0
#define ALGORITHM_INSTANCE "QSH-768"
#define DIGEST_BIT_LENGTH 768
#ifdef __cplusplus
extern "C" {
#endif
    int CryptHash(int digest_len_bits, const unsigned char *msg,
                  unsigned long long msg_len_bits, unsigned char *digest);
#ifdef __cplusplus
}
#endif
#endif
