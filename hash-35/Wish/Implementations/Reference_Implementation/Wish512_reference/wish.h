/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#ifndef WISH_H
#define WISH_H

/*
 * Unified Wish permutation-based hash.
 *
 *   block_len_bits  - rate, in bits (e.g. 512 for Wish512, 1024 for Wish1024)
 *   state_len_bits  - full state size, in bits (rate + capacity; must be a
 *                     multiple of 128)
 *   num_steps       - number of permutation steps (e.g. 9 for Wish512,
 *                     12 for Wish1024)
 *   msg             - input message bytes
 *   msg_len_bits    - input length, in bits
 *   digest          - output buffer; receives state_len_bits/2 bits
 */
void Wish_hash(
    const unsigned digest_len_bits,
    const unsigned state_len_bits,
    const unsigned char *msg,
    unsigned long long msg_len_bits,
    unsigned char *digest
);

#endif /* WISH_H */
