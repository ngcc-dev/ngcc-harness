#include "packing.h"
#include "parameters.h"
#include "polyvec.h"
#include <stdint.h>
#include <string.h>

void DKE_packpk(uint8_t bytes[DKE_PKBYTES],
                const polyvec *pk,
                const uint8_t seed[DKE_SEEDBYTES]) {
    DKE_polyvec_tobytes(bytes, pk);
    memcpy(bytes + DKE_PACOMPRESSEDBYTES, seed, DKE_SEEDBYTES);
}

void DKE_unpackpk(polyvec *pk,
                  uint8_t seed[DKE_SEEDBYTES],
                  const uint8_t bytes[DKE_PKBYTES]) {
    DKE_polyvec_frombytes(pk, bytes);
    memcpy(seed, bytes + DKE_PACOMPRESSEDBYTES, DKE_SEEDBYTES);
}

void DKE_CPA_unpacksk(polyvec *sk, const uint8_t bytes[DKE_SKBYTES]) {
    DKE_polyvec_frombytes(sk, bytes);
}

void DKE_CPA_packsk(uint8_t bytes[DKE_CPA_SKABYTES], polyvec *sk) {
    DKE_polyvec_tobytes(bytes, sk);
}

void DKE_CPA_packciphertext(uint8_t bytes[DKE_CPA_CTBYTES],
                            polyvec *pb,
                            uint8_t sig[DKE_SIGNALBYTES]) {
    DKE_polyvec_compressB(bytes, pb);
    memcpy(bytes + DKE_PBCOMPRESSEDBYTES, sig, DKE_SIGNALBYTES);
}

void DKE_CPA_unpackciphertext(polyvec *pb,
                              uint8_t sig[DKE_SIGNALBYTES],
                              const uint8_t bytes[DKE_CPA_CTBYTES]) {
    DKE_polyvec_decompressB(pb, bytes);
    memcpy(sig, bytes + DKE_PBCOMPRESSEDBYTES, DKE_SIGNALBYTES);
}
