#ifndef RHYME_CONFIG_H
#define RHYME_CONFIG_H
/* Hybrid mode: AES-256-CTR (AES-NI) for Agen + sampling streams.
 * Build with -DRHYME_NO_AES for a portable pure-SHAKE build. */
#ifndef RHYME_NO_AES
#define RHYME_USE_AES
#endif
#if RHYME_MODE == 128
#define CRYPTO_ALGNAME "rhyme128"
#define RHYME_NAMESPACETOP rhyme128
#define RHYME_NAMESPACE(s) rhyme128_##s
#elif RHYME_MODE == 256
#define CRYPTO_ALGNAME "rhyme256"
#define RHYME_NAMESPACETOP rhyme256
#define RHYME_NAMESPACE(s) rhyme256_##s
#elif RHYME_MODE == 384
#define CRYPTO_ALGNAME "rhyme384"
#define RHYME_NAMESPACETOP rhyme384
#define RHYME_NAMESPACE(s) rhyme384_##s
#elif RHYME_MODE == 512
#define CRYPTO_ALGNAME "rhyme512"
#define RHYME_NAMESPACETOP rhyme512
#define RHYME_NAMESPACE(s) rhyme512_##s
#else
#error "RHYME_MODE must be 128, 256, 384 or 512"
#endif
#endif
