# Constant-time review: FLIT

The CPA secret polynomial/vector, decrypted prekey, fallback secret, and final shared secret are secret. The ciphertext, public key, and fixed lengths are public. This follows FLIT128 reference C; 256/512 and compiler output are not independently certified.

`kem.c:95-128` decrypts with `indcpa_dec`, re-encrypts, calls fixed-length `verify`, applies `cmov` to the prekey, and derives the final key. `indcpa.c:117-132` multiplies the secret polynomial by the ciphertext and decodes the result without an explicit branch in that routine. Early `if (r1 == 0)`/`if (r2 != 0)` decisions in `indcpa.c:39-45` are in key generation, not this decapsulation path; their effect on key-generation entropy/timing would require separate measurement. No secret-indexed lookup or variable-divisor operation was established in the traced decapsulation chain.

Status: representative source-level screen only; no new report from this review.

Size-build correction: `poly_compress_and_pack` (`poly.c:215,220-227`) runs on FO re-encryption of the secret-derived message, and GCC `-Os` emits hardware division there (including FLIT512). The source modulus is public, but the numerator is not necessarily public; a timing oracle and attack transfer remain untested. Thus the earlier absence statement applies only to the narrow default-build trace, not to size-optimized builds.
