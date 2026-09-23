# Constant-time review: BW-KEM

The long-term CPA key, decrypted prekey, fallback secret, and shared secret are secret. Ciphertext, public key, and public matrix seed are public. This follows the BW_KEM_C128 reference path; the other parameter directories use the same wrapper pattern.

`kem.c:155-170` decrypts, re-encrypts, calls `verify`, and selects the result with `cmov`. `verify.c:16-25,40-59` accumulates all ciphertext differences and masks the selected secret without short-circuiting. The rejection sampler in `indcpa.c:124-145` expands the public matrix seed, so that path does not leak a KEM secret. However, `indcpa.c:321-331` computes `mp = v - sᵀu` and passes it to `poly_tomsg`; `poly.c:174-193` branches on the sign of each centered coefficient. GCC `-O2` retains a `jns` in `bwkem128_poly_tomsg`. This is the same code pattern already filed for AFS-KEX C128 as `kex-02-2`. Fixed public divisors are not automatically safe: the numerator can be secret-derived, and a size-optimized build may emit hardware division.

Status: source-and-assembly-confirmed secret-dependent branch (`kem-08-1`); no timing oracle or key recovery established. This is not a compiler/CPU constant-time certification; other parameter sets and transitive arithmetic have not been exhaustively proved.

Size-build lead: GCC `-Os` emits division in C128 `polyvec_compress` (`polyvec.c:57`), C256/C512 `polyvec_compress` (`:27`), and C512 `poly_compress` (`poly.c:75`). FO re-encryption can feed secret-derived values to these sites. This was not promoted as a separate exploit without a timing or key-recovery argument.
