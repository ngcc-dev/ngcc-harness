# Constant-time review: Aigis-Enc+

Secret inputs are the long-term PKE key in `sk`, the decrypted prekey, the rejection seed, and the shared secret. `pk`, `ct`, encoded lengths, and parameter constants are public. The trace below is for reference Aigis-Enc+-I; II/III use the same KEM structure. This is a source-level review, not a machine-code proof across compilers.

`kem.c:65` calls `owcpa_dec` on the secret key; `kem.c:72` re-encrypts the resulting secret prekey before the `verify`/`cmov` path at `:74-80`. `verify.c:15-27,41-54` uses fixed-length accumulation and a byte mask. The candidate already has a separate functional rejection failure in `report.md`; it must not be confused with a timing result. `pack.c:19-22` reaches `poly_compress` on the *re-encrypted*, secret-derived ciphertext, whose divisions by compile-time `PARAM_Q` occur at `poly.c:72,84-85,94`; the public input-ciphertext path is not the only caller. GCC `-Os` with the reference definitions emits `div` in `poly_compress` (two sites for Aigis-Enc+-I); GCC `-O2` did not in the reviewed build. Fixed divisors therefore cannot be cleared by source inspection alone. A timing oracle and key-recovery transfer have not been demonstrated.

Status: size-optimized secret-operand divide lead, not a demonstrated side-channel attack. Optimized variants and other compilers are not certified constant-time.
