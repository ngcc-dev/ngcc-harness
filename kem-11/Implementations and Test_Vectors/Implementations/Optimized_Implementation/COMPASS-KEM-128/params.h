#ifndef PARAMS_H
#define PARAMS_H

#ifndef SECURITY_LEVEL
#define SECURITY_LEVEL 128  /* Default: 128-bit classical security */
#endif

#if (SECURITY_LEVEL == 128)
    #define COMPASS_KEM_K 2
    #define COMPASS_KEM_L 2
    #define COMPASS_KEM_N 256
    #define COMPASS_KEM_Q 3329
    #define COMPASS_KEM_ETA1 3
    #define COMPASS_KEM_ETA2 3
    #define COMPASS_KEM_D  2
    #define COMPASS_KEM_D1 2
    #define COMPASS_KEM_D2 8
    #define COMPASS_KEM_POLYBYTES 384  /* 256 * 12 bits / 8 = 384 */
    #define COMPASS_KEM_NAMESPACE(s) pqcrystals_COMPASS_KEM256_ref_##s

#elif (SECURITY_LEVEL == 256)
    #define COMPASS_KEM_K 4
    #define COMPASS_KEM_L 4
    #define COMPASS_KEM_N 256
    #define COMPASS_KEM_Q 3329
    #define COMPASS_KEM_ETA1 2
    #define COMPASS_KEM_ETA2 2
    #define COMPASS_KEM_D  2
    #define COMPASS_KEM_D1 2
    #define COMPASS_KEM_D2 6
    #define COMPASS_KEM_POLYBYTES 384
    #define COMPASS_KEM_NAMESPACE(s) pqcrystals_COMPASS_KEM512_ref_##s

#elif (SECURITY_LEVEL == 384)
    #define COMPASS_KEM_K 3
    #define COMPASS_KEM_L 3
    #define COMPASS_KEM_N 512
    #define COMPASS_KEM_Q 7681
    #define COMPASS_KEM_ETA1 3
    #define COMPASS_KEM_ETA2 3
    #define COMPASS_KEM_D  2
    #define COMPASS_KEM_D1 2
    #define COMPASS_KEM_D2 8
    #define COMPASS_KEM_POLYBYTES 832  /* q=7681 needs 13 bits, 512 * 13 bits / 8 = 832 */
    #define COMPASS_KEM_NAMESPACE(s) pqcrystals_COMPASS_KEM768_ref_##s

#elif (SECURITY_LEVEL == 512)
    #define COMPASS_KEM_K 4
    #define COMPASS_KEM_L 4
    #define COMPASS_KEM_N 512
    #define COMPASS_KEM_Q 7681
    #define COMPASS_KEM_ETA1 4
    #define COMPASS_KEM_ETA2 4
    #define COMPASS_KEM_D  2
    #define COMPASS_KEM_D1 2
    #define COMPASS_KEM_D2 6
    #define COMPASS_KEM_POLYBYTES 832
    #define COMPASS_KEM_NAMESPACE(s) pqcrystals_COMPASS_KEM1024_ref_##s

#else
    #error "SECURITY_LEVEL must be in {128, 256, 384, 512}"
#endif


#if (COMPASS_KEM_Q == 3329)
    /* n=256, 10 bits per coefficient -> 256 * 10 / 8 = 320 */
    #define COMPASS_KEM_POLYCOMPRESSEDBYTES_D1    320  
    /* d2=6: max value 3329/64 = 52, needs 6 bits -> 256 * 6 / 8 = 192 */
    #if (COMPASS_KEM_D2 == 6)
        #define COMPASS_KEM_POLYCOMPRESSEDBYTES_D2    192
    #else
        #define COMPASS_KEM_POLYCOMPRESSEDBYTES_D2    128
    #endif
#elif (COMPASS_KEM_Q == 7681)
    /* n=512, 11 bits per coefficient -> 512 * 11 / 8 = 704 */
    #define COMPASS_KEM_POLYCOMPRESSEDBYTES_D1    704  
    /* d2=6: max value 7681/64 = 120, needs 7 bits -> 512 * 7 / 8 = 448 */
    #if (COMPASS_KEM_D2 == 6)
        #define COMPASS_KEM_POLYCOMPRESSEDBYTES_D2    448
    #else
        #define COMPASS_KEM_POLYCOMPRESSEDBYTES_D2    320
    #endif
#endif

#define COMPASS_KEM_SYMBYTES 32   /* size in bytes of hashes, and seeds */
#define COMPASS_KEM_SSBYTES  32   /* size in bytes of shared key */

// #define COMPASS_KEM_POLYBYTES		384
// #define COMPASS_KEM_POLYVECBYTES	(COMPASS_KEM_K * COMPASS_KEM_POLYBYTES)
#define COMPASS_KEM_POLYVECBYTES           (COMPASS_KEM_K * COMPASS_KEM_POLYBYTES)
#define COMPASS_KEM_POLYVECCOMPRESSEDBYTES (COMPASS_KEM_K * COMPASS_KEM_POLYCOMPRESSEDBYTES_D1)

#define COMPASS_KEM_INDCPA_MSGBYTES       COMPASS_KEM_SYMBYTES
/* In spec: pk = (pk.seed, t), where t is compressed */
#define COMPASS_KEM_INDCPA_PUBLICKEYBYTES (COMPASS_KEM_POLYVECCOMPRESSEDBYTES + COMPASS_KEM_SYMBYTES) 
/* In spec: sk = (seed, s^), where s^ is the NTT-domain vector */
#define COMPASS_KEM_INDCPA_SECRETKEYBYTES (COMPASS_KEM_POLYVECBYTES) 
/* ct = (c1, c2), where c1 is compressed by d1, c2 compressed by d2 */
#define COMPASS_KEM_INDCPA_BYTES          (COMPASS_KEM_POLYVECCOMPRESSEDBYTES + COMPASS_KEM_POLYCOMPRESSEDBYTES_D2) 

/* KEM-level size definitions (CCA transform with FO structure, COMPASS_KEM settings) */
#define COMPASS_KEM_PUBLICKEYBYTES  (COMPASS_KEM_INDCPA_PUBLICKEYBYTES)
/* Extra space for storing H(pk) and pseudo-random z */
#define COMPASS_KEM_SECRETKEYBYTES  (COMPASS_KEM_INDCPA_SECRETKEYBYTES + COMPASS_KEM_INDCPA_PUBLICKEYBYTES + 2*COMPASS_KEM_SYMBYTES)
#define COMPASS_KEM_CIPHERTEXTBYTES (COMPASS_KEM_INDCPA_BYTES)

#endif
