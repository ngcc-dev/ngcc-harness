# Constant-time review: COMPASS-KEM

The CPA secret key, recovered prekey, rejection seed, and shared secret are secret. The public key, ciphertext, and matrix seed are public. This follows the COMPASS-KEM-128 reference implementation; the other parameter sets use the same sources with a different `-DSECURITY_LEVEL`.

`kem.c:142-169` calls `indcpa_dec`, re-encrypts, and selects the rejection or candidate secret through fixed-length `verify` and `cmov`. The control decision is not a source-level `if`. Public-seed matrix rejection sampling is not a secret leak simply because it branches. Compression uses shifts and modular reduction uses Montgomery/Barrett multiplication; the only source-level `/` and `%` in this path are between compile-time constants. No additional secret-dependent branch or lookup was established in the representative decapsulation chain.

Status: source-level screen plus a valgrind memcheck run (GCC 15.2 `-O3`, x86-64) of the reference and optimized builds of all four parameter sets, with the key-generation coins, encapsulation message, CPA secret key and rejection seed marked undefined. The only reports are in `rej_uniform` on the public matrix seed during key generation; encapsulation and decapsulation of a valid and of a modified ciphertext give none. Other compilers and targets are not covered. No report from this review.
