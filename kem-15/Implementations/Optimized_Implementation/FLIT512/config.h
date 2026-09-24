#ifndef CONFIG_H
#define CONFIG_H

/*************************************************
 *This file is to be included in the implementation files to set the parameters for the KEM.
 * We provides three different parameter sets: 128, 256, and 512. The default is 128, which is the most secure and efficient one.
 * You can change the parameter set by defining the KEM_MODE macro before including this file.
 * For example, to use the 128-bit parameter set, you can write: # define KEM_MODE 128
 **************************************************/

#ifndef KEM_MODE
#define KEM_MODE 512
#endif

#if KEM_MODE == 128
#define CRYPTO_ALGNAME "FLIT128-AVX2"
#define KEM_NAMESPACETOP flit128_avx2
#define KEM_NAMESPACE(s) flit128_avx2_##s
#elif KEM_MODE == 256
#define CRYPTO_ALGNAME "FLIT256-AVX2"
#define KEM_NAMESPACETOP flit256_avx2
#define KEM_NAMESPACE(s) flit256_avx2_##s
#elif KEM_MODE == 512
#define CRYPTO_ALGNAME "FLIT512-AVX2"
#define KEM_NAMESPACETOP flit512_avx2
#define KEM_NAMESPACE(s) flit512_avx2_##s
#endif

#endif
