#!/usr/bin/env python3
"""Static certificate for kem-32-3's bundled-RNG key-generation seed cap."""

from pathlib import Path


root = Path(__file__).resolve().parent / "Implementations/Reference_Implementation/QCTM512"
optimized = Path(__file__).resolve().parent / "Implementations/Optimized_Implementation/QCTM512"


def check(name: str, *needles: str) -> str:
    source = (root / name).read_text()
    for needle in needles:
        if needle not in source:
            raise SystemExit(f"NOT REPRODUCED kem-32-3: {name} lacks {needle!r}")
    return source


makefile = check(
    "Makefile",
    "all: $(KAT_TOOL)",
    "$(KAT_TOOL): $(OBJECTS) rng.o qctm512_kat.o",
    "-DAPI_PKC_KAT_DRNG_BRIDGE",
    "KEM_AlgorithmInstance.c",
)
api = check(
    "KEM_AlgorithmInstance.c",
    "unsigned char entropy_input[48]",
    "get_random_number(&drng_algorithm, entropy_input,",
    "randombytes_init(entropy_input, NULL, 256)",
    "status = init_local_rng_from_api_pkc_drng()",
    "status = crypto_kem_keypair(pk, sk)",
)
rng = check(
    "rng.c",
    "memcpy(seed_material, entropy_input, 48)",
    "AES256_CTR_DRBG_Update(seed_material, DRBG_ctx.Key, DRBG_ctx.V)",
    "AES256_ECB(DRBG_ctx.Key, DRBG_ctx.V, block)",
)
check("qctm512_kat.c", "randombytes_init(seed, NULL, 256)")
check("rng.h", "unsigned char   Key[32]", "unsigned char   V[16]")
kem = check(
    "kem.c",
    "unsigned char keygen_seed[KEYGEN_SEED_BYTES]",
    "randombytes(keygen_seed, sizeof(keygen_seed))",
    "gamma = scheme_keygen_seeded(",
    "serialize_gamma_prime(sk, gamma->g, gamma->L)",
)
check("seeded_keygen.h", "#define KEYGEN_SEED_BYTES 64")
seeded = check(
    "seeded_keygen.c",
    "seed_len != KEYGEN_SEED_BYTES",
    "memcpy(delta, seed, seed_len)",
    "FIPS202_SHAKE256(delta, (unsigned int)seed_len,",
)
if "randombytes(" in seeded:
    raise SystemExit("NOT REPRODUCED kem-32-3: seeded keygen takes extra randomness")
if "#ifdef LOCALLY_QUASI_CYCLIC_TWISTED_MCELIECE_BASELINE" not in kem:
    raise SystemExit("NOT REPRODUCED kem-32-3: seeded/default branch not identified")

optimized_api = (optimized / "KEM_AlgorithmInstance.c").read_text()
optimized_rng = (optimized / "rng.c").read_text()
for needle in ("unsigned char entropy_input[48]", "randombytes_init(entropy_input, NULL, 256)",
               "status = init_local_rng_from_api_pkc_drng()"):
    if needle not in optimized_api:
        raise SystemExit(f"NOT REPRODUCED kem-32-3: optimized adapter lacks {needle!r}")
if optimized_rng != rng:
    raise SystemExit("NOT REPRODUCED kem-32-3: optimized RNG differs from checked reference RNG")

print("CONFIRMED kem-32-3: bundled reference and optimized RNGs use 48-byte Key||V state")
print("CONFIRMED kem-32-3: bundled RNG determines the 64-byte deterministic keygen seed")
print("BOUND kem-32-3: at most 2^384 RNG states, each testable against the public key")
