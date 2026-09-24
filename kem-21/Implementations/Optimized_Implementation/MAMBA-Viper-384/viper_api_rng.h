#ifndef VIPER_API_RNG_H
#define VIPER_API_RNG_H
int randombytes(unsigned char *x, unsigned long long xlen);
int viper_api_rng_status(void);
void viper_api_rng_clear_status(void);
#endif
