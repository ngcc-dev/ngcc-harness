#ifndef P512_PARAMS_H
#define P512_PARAMS_H
/* Independent namespace for the imported packed-domain DKE-512 AVX2 kernel. */
#define DKE_NAMESPACE(s) dke_p512_##s
#define DKE3_Q 7681
#define DKE3_N 512
#define DKE3_K 4
#endif
