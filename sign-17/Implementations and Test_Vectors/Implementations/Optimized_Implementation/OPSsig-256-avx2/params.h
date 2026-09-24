#ifndef OPSSIG_PARAMS_H
#define OPSSIG_PARAMS_H

#include <stdint.h>

#define SEEDBYTES 64
#define CRHBYTES 128

#define N 512
#define Q 67104769
#define D 13

#define K 4
#define L 4

#define ETA 3
#define TAU 45
#define BETA (TAU*ETA)

#define GAMMA1 (1 << 20)
#define GAMMA2 ((Q-1)/96)

#define OMEGA 85
#define CTILDEBYTES 64

#define POLYT1_PACKEDBYTES  (N*13/8)
#define POLYT0_PACKEDBYTES  (N*13/8)
#define POLYETA_PACKEDBYTES (N*3/8)

#define POLYZ_PACKEDBYTES   (N*21/8)
#define POLYW1_PACKEDBYTES  (N*6/8)

#define POLYVECH_POSBITS 9
#define POLYVECH_POSBYTES 96
#define POLYVECH_PACKEDBYTES (POLYVECH_POSBYTES + K)

#define CRYPTO_PUBLICKEYBYTES (SEEDBYTES + K*POLYT1_PACKEDBYTES)
#define CRYPTO_SECRETKEYBYTES (SEEDBYTES \
                               + CRHBYTES \
                               + L*POLYETA_PACKEDBYTES \
                               + K*POLYETA_PACKEDBYTES \
                               + K*POLYT0_PACKEDBYTES)
#define CRYPTO_BYTES (CTILDEBYTES + L*POLYZ_PACKEDBYTES + POLYVECH_PACKEDBYTES)

#endif
