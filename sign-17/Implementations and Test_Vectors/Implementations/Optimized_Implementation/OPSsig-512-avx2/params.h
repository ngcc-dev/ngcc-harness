#ifndef OPSSIG_PARAMS_H
#define OPSSIG_PARAMS_H

#include <stdint.h>

#define SEEDBYTES 64
#define CRHBYTES 128

#define N 1024
#define Q 67104769
#define D 13

#define K 4
#define L 4

#define ETA 2
#define TAU 90
#define BETA (TAU*ETA)

#define GAMMA1 (1 << 22)
#define GAMMA2 ((Q-1)/48)

#define OMEGA 90
#define CTILDEBYTES 128

#define POLYT1_PACKEDBYTES  (N*13/8)
#define POLYT0_PACKEDBYTES  (N*13/8)
#define POLYETA_PACKEDBYTES (N*3/8)

#define POLYZ_PACKEDBYTES   (N*23/8)
#define POLYW1_PACKEDBYTES  (N*5/8)

#define POLYVECH_POSBITS 10
#define POLYVECH_POSBYTES 113
#define POLYVECH_PACKEDBYTES (POLYVECH_POSBYTES + K)

#define CRYPTO_PUBLICKEYBYTES (SEEDBYTES + K*POLYT1_PACKEDBYTES)
#define CRYPTO_SECRETKEYBYTES (SEEDBYTES \
                               + CRHBYTES \
                               + L*POLYETA_PACKEDBYTES \
                               + K*POLYETA_PACKEDBYTES \
                               + K*POLYT0_PACKEDBYTES)
#define CRYPTO_BYTES (CTILDEBYTES + L*POLYZ_PACKEDBYTES + POLYVECH_PACKEDBYTES)

#endif
