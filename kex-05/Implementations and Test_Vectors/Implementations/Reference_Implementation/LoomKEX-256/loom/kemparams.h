#ifndef KEM_PARAMS_H
#define KEM_PARAMS_H


#define PK_COMPRESS

#if   (WEAVER_MODE == 1)
  #define WEAVER_NAMESPACE(s) loom_kem128_ref##s
  /* Table 4: LOOM-KEM (n,k,q,eta1,eta2,dt,du,dv) = (128,5,3329,3,2,9,8,4) */
  #define WEAVER_INDCPA_MSGBYTES 16
  #define WEAVER_N  128
  #define WEAVER_K  5
  #define WEAVER_Q  3329
  #define WEAVER_QBITS 12
  #define WEAVER_ETA1 3
  #define WEAVER_ETA2 2
  #define LOOM_KEM_DT 9
  #define LOOM_KEM_DU 8
  #define LOOM_KEM_DV 4
  #define WEAVER_TAGBYTES 32
  #define WEAVER_SYMBYTES 32

#elif (WEAVER_MODE == 3)
  #define WEAVER_NAMESPACE(s) loom_kem256_ref##s
  /* Table 4: LOOM-KEM (n,k,q,eta1,eta2,dt,du,dv) = (256,4,7681,8,4,10,9,4) */
  #define WEAVER_INDCPA_MSGBYTES 32
  #define WEAVER_N  256
  #define WEAVER_K  4
  #define WEAVER_Q  7681
  #define WEAVER_QBITS 13
  #define WEAVER_ETA1 8
  #define WEAVER_ETA2 4
  #define LOOM_KEM_DT 10
  #define LOOM_KEM_DU 9
  #define LOOM_KEM_DV 4
  #define WEAVER_TAGBYTES 64
  #define WEAVER_SYMBYTES 32

#elif (WEAVER_MODE == 5)
  #define WEAVER_NAMESPACE(s) loom_kem512_ref##s
  /* Table 4: LOOM-KEM (n,k,q,eta1,eta2,dt,du,dv) = (512,4,7681,10,8,11,10,4) */
  #define WEAVER_INDCPA_MSGBYTES 64
  #define WEAVER_N  512
  #define WEAVER_K  4
  #define WEAVER_Q  7681
  #define WEAVER_QBITS 13
  #define WEAVER_ETA1 10
  #define WEAVER_ETA2 8
  #define LOOM_KEM_DT 11
  #define LOOM_KEM_DU 10
  #define LOOM_KEM_DV 4
  #define WEAVER_TAGBYTES 128
  #define WEAVER_SYMBYTES 64

#else
  #error "WEAVER_MODE must be in {1,3,5}"
#endif

#define WEAVER_DV LOOM_KEM_DV

#define WEAVER_PK_POLYVECBYTES         (WEAVER_K * ((WEAVER_N * LOOM_KEM_DT) / 8))
#define WEAVER_POLYVECCOMPRESSEDBYTES  (WEAVER_K * ((WEAVER_N * LOOM_KEM_DU) / 8))
#define WEAVER_POLYCOMPRESSEDBYTES     ((WEAVER_N * LOOM_KEM_DV) / 8)

#define WEAVER_POLYBYTES     ((WEAVER_N * WEAVER_QBITS) / 8)
#define WEAVER_POLYVECBYTES  (WEAVER_K * WEAVER_POLYBYTES)

#ifndef PK_COMPRESS
#undef WEAVER_PK_POLYVECBYTES
#define WEAVER_PK_POLYVECBYTES WEAVER_POLYVECBYTES
#endif

/* ====================================================================
 * Global Parameters: DO NOT modify.
 * ==================================================================== */
#define WEAVER_HALFQ ((WEAVER_Q + 1) / 2)

/* size in bytes of shared key */
#define WEAVER_SSBYTES  WEAVER_INDCPA_MSGBYTES

/* key pair and ciphertext sizes */
#define WEAVER_INDCPA_PUBLICKEYBYTES (WEAVER_PK_POLYVECBYTES + WEAVER_SYMBYTES)
#define WEAVER_INDCPA_SECRETKEYBYTES (WEAVER_POLYVECBYTES)
#define WEAVER_INDCPA_BYTES          (WEAVER_POLYVECCOMPRESSEDBYTES + WEAVER_POLYCOMPRESSEDBYTES)
#define WEAVER_CIPHERTEXTBODYBYTES   WEAVER_INDCPA_BYTES
/* size in bytes of hashes, seeds, and FO outputs */
#define WEAVER_KEM_DERAND_COINBYTES (WEAVER_INDCPA_MSGBYTES + WEAVER_SYMBYTES)

#define WEAVER_PUBLICKEYBYTES  (WEAVER_INDCPA_PUBLICKEYBYTES)
#define WEAVER_SECRETKEYBYTES  (WEAVER_INDCPA_SECRETKEYBYTES)
#define WEAVER_CIPHERTEXTBYTES (WEAVER_CIPHERTEXTBODYBYTES + WEAVER_TAGBYTES)

#endif /* KEM_PARAMS_H */
