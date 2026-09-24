#ifndef _KeccakHashInterface_h_
#define _KeccakHashInterface_h_

#include "config.h"
#ifdef XKCP_has_KeccakP1600

#include <stdint.h>
#include <string.h>
#include "KeccakSponge.h"

#ifndef _Keccak_BitTypes_
#define _Keccak_BitTypes_
typedef uint8_t BitSequence;

typedef size_t BitLength;
#endif

typedef enum { KECCAK_SUCCESS = 0, KECCAK_FAIL = 1, KECCAK_BAD_HASHLEN = 2 } HashReturn;

typedef struct {
    KeccakWidth1600_SpongeInstance sponge;
    unsigned int fixedOutputLength;
    unsigned char delimitedSuffix;
} Keccak_HashInstance;

#define Keccak_HashInitialize_SHAKE128(hashInstance)        Keccak_HashInitialize(hashInstance, 1344,  256,   0, 0x1F)

#define Keccak_HashInitialize_SHAKE256(hashInstance)        Keccak_HashInitialize(hashInstance, 1088,  512,   0, 0x1F)

#define Keccak_HashInitialize_SHA3_224(hashInstance)        Keccak_HashInitialize(hashInstance, 1152,  448, 224, 0x06)

#define Keccak_HashInitialize_SHA3_256(hashInstance)        Keccak_HashInitialize(hashInstance, 1088,  512, 256, 0x06)

#define Keccak_HashInitialize_SHA3_384(hashInstance)        Keccak_HashInitialize(hashInstance,  832,  768, 384, 0x06)

#define Keccak_HashInitialize_SHA3_512(hashInstance)        Keccak_HashInitialize(hashInstance,  576, 1024, 512, 0x06)

#else
#error This requires an implementation of Keccak-p[1600]
#endif

#endif
