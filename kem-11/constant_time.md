# Constant-time review: COMPASS-KEM

The CPA secret key, recovered prekey, rejection seed, and shared secret are secret. The public key, ciphertext, and matrix seed are public. This follows the COMPASS-KEM-128 reference implementation; higher parameter sets and generated machine code are not independently certified.

`kem.c:142-169` calls `indcpa_dec`, re-encrypts, and selects the rejection or candidate secret through fixed-length `verify` and `cmov`. The control decision is not a source-level `if`. Public-seed matrix rejection sampling is not a secret leak simply because it branches. Compression/modular reductions use compile-time constants in this path; source-level `/` or `%` by a constant should not be equated with a hardware division instruction. No additional secret-dependent branch or lookup was established in the representative decapsulation chain.

Status: limited source-level screen; inspect optimized builds and emitted assembly before claiming end-to-end constant time. No report from this review.
