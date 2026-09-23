# Constant-time review: Lore

The CPA secret polynomial, recovered message `mu`, fallback seed `z`, and shared secret are secret. The ciphertext and public key are public. This traces Lore-SHAKE/Lore-L2 reference C; Lore-SM3 and other levels are not independently certified.

`kem.c:184-236` decrypts under the CPA secret, re-encrypts, computes both candidate/fallback shared secrets, and selects with `cmov`. In the re-encryption path, `indcpa.c:240-252` branches while packing overflow bits of the regenerated ciphertext. On a valid ciphertext those bits match public ciphertext encoding; on an invalid ciphertext they may be derived from private-key decryption. The unpacking branch at `indcpa.c:302` reads the public ciphertext and is not a secret leak. The trace warrants follow-up, but no controlled leakage measurement or secret-distinguishing witness was established. Fixed public modulus arithmetic is not treated as a hardware divide without code-generation evidence.

Status: potential secret-derived branch in re-encryption, not promoted to a report without a differential witness.
