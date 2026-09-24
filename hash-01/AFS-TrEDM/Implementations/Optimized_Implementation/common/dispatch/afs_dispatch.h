/*
 * Optional runtime-dispatch API for optimized single-message builds.
 *
 * This API is intentionally separate from the ICCS CryptHash() entry point:
 * CryptHash() remains the conservative submission interface, while
 * CryptHash_dispatch() is an explicit optimized helper for evaluators who want
 * one binary that can choose portable or AVX2 code at runtime.
 */
#ifndef AFS_TREDM_DISPATCH_H
#define AFS_TREDM_DISPATCH_H

#ifdef __cplusplus
extern "C" {
#endif

int CryptHash_dispatch(int digest_len_bits,
                       const unsigned char *msg,
                       unsigned long long msg_len_bits,
                       unsigned char *digest);

int CryptHash_dispatch_avx2_available(void);
const char *CryptHash_dispatch_selected_backend(void);

#ifdef __cplusplus
}
#endif

#endif /* AFS_TREDM_DISPATCH_H */
