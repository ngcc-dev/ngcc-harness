#ifndef API_H
#define API_H

#include <stddef.h>
#include <stdint.h>

#define pqcrystals_COMPASS_SIG2_PUBLICKEYBYTES 1312
#define pqcrystals_COMPASS_SIG2_SECRETKEYBYTES 2560
#define pqcrystals_COMPASS_SIG2_BYTES 2420

#define pqcrystals_COMPASS_SIG2_ref_PUBLICKEYBYTES pqcrystals_COMPASS_SIG2_PUBLICKEYBYTES
#define pqcrystals_COMPASS_SIG2_ref_SECRETKEYBYTES pqcrystals_COMPASS_SIG2_SECRETKEYBYTES
#define pqcrystals_COMPASS_SIG2_ref_BYTES pqcrystals_COMPASS_SIG2_BYTES

int pqcrystals_COMPASS_SIG2_ref_keypair(uint8_t *pk, uint8_t *sk);

int pqcrystals_COMPASS_SIG2_ref_signature(uint8_t *sig, size_t *siglen,
                                        const uint8_t *m, size_t mlen,
                                        const uint8_t *ctx, size_t ctxlen,
                                        const uint8_t *sk);

int pqcrystals_COMPASS_SIG2_ref(uint8_t *sm, size_t *smlen,
                              const uint8_t *m, size_t mlen,
                              const uint8_t *ctx, size_t ctxlen,
                              const uint8_t *sk);

int pqcrystals_COMPASS_SIG2_ref_verify(const uint8_t *sig, size_t siglen,
                                     const uint8_t *m, size_t mlen,
                                     const uint8_t *ctx, size_t ctxlen,
                                     const uint8_t *pk);

int pqcrystals_COMPASS_SIG2_ref_open(uint8_t *m, size_t *mlen,
                                   const uint8_t *sm, size_t smlen,
                                   const uint8_t *ctx, size_t ctxlen,
                                   const uint8_t *pk);

#define pqcrystals_COMPASS_SIG3_PUBLICKEYBYTES 1952
#define pqcrystals_COMPASS_SIG3_SECRETKEYBYTES 4032
#define pqcrystals_COMPASS_SIG3_BYTES 3309

#define pqcrystals_COMPASS_SIG3_ref_PUBLICKEYBYTES pqcrystals_COMPASS_SIG3_PUBLICKEYBYTES
#define pqcrystals_COMPASS_SIG3_ref_SECRETKEYBYTES pqcrystals_COMPASS_SIG3_SECRETKEYBYTES
#define pqcrystals_COMPASS_SIG3_ref_BYTES pqcrystals_COMPASS_SIG3_BYTES

int pqcrystals_COMPASS_SIG3_ref_keypair(uint8_t *pk, uint8_t *sk);

int pqcrystals_COMPASS_SIG3_ref_signature(uint8_t *sig, size_t *siglen,
                                        const uint8_t *m, size_t mlen,
                                        const uint8_t *ctx, size_t ctxlen,
                                        const uint8_t *sk);

int pqcrystals_COMPASS_SIG3_ref(uint8_t *sm, size_t *smlen,
                              const uint8_t *m, size_t mlen,
                              const uint8_t *ctx, size_t ctxlen,
                              const uint8_t *sk);

int pqcrystals_COMPASS_SIG3_ref_verify(const uint8_t *sig, size_t siglen,
                                     const uint8_t *m, size_t mlen,
                                     const uint8_t *ctx, size_t ctxlen,
                                     const uint8_t *pk);

int pqcrystals_COMPASS_SIG3_ref_open(uint8_t *m, size_t *mlen,
                                   const uint8_t *sm, size_t smlen,
                                   const uint8_t *ctx, size_t ctxlen,
                                   const uint8_t *pk);

#define pqcrystals_COMPASS_SIG5_PUBLICKEYBYTES 2592
#define pqcrystals_COMPASS_SIG5_SECRETKEYBYTES 4896
#define pqcrystals_COMPASS_SIG5_BYTES 4627

#define pqcrystals_COMPASS_SIG5_ref_PUBLICKEYBYTES pqcrystals_COMPASS_SIG5_PUBLICKEYBYTES
#define pqcrystals_COMPASS_SIG5_ref_SECRETKEYBYTES pqcrystals_COMPASS_SIG5_SECRETKEYBYTES
#define pqcrystals_COMPASS_SIG5_ref_BYTES pqcrystals_COMPASS_SIG5_BYTES

int pqcrystals_COMPASS_SIG5_ref_keypair(uint8_t *pk, uint8_t *sk);

int pqcrystals_COMPASS_SIG5_ref_signature(uint8_t *sig, size_t *siglen,
                                        const uint8_t *m, size_t mlen,
                                        const uint8_t *ctx, size_t ctxlen,
                                        const uint8_t *sk);

int pqcrystals_COMPASS_SIG5_ref(uint8_t *sm, size_t *smlen,
                              const uint8_t *m, size_t mlen,
                              const uint8_t *ctx, size_t ctxlen,
                              const uint8_t *sk);

int pqcrystals_COMPASS_SIG5_ref_verify(const uint8_t *sig, size_t siglen,
                                     const uint8_t *m, size_t mlen,
                                     const uint8_t *ctx, size_t ctxlen,
                                     const uint8_t *pk);

int pqcrystals_COMPASS_SIG5_ref_open(uint8_t *m, size_t *mlen,
                                   const uint8_t *sm, size_t smlen,
                                   const uint8_t *ctx, size_t ctxlen,
                                   const uint8_t *pk);


#endif
