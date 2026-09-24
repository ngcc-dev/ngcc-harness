#ifndef RNG_H
#define RNG_H

void init_randombytes(const unsigned char *seed, unsigned long long seed_len_bytes);
void get_randombytes(unsigned char *x, unsigned long long xlen);

#endif /* ! RNG_H */
