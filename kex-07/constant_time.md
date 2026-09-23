# NEV-AKE constant-time review

Scope: `NEV-AKE-C1` representative reference implementation; other C/R and compressed variants were not exhaustively traced. Long-term PKE keys, rejection secrets, ephemeral KEM messages and derived session key are secret. Identities, public keys, ciphertexts and their fixed lengths are public.

- `NEV-AKE-C1/ake.c:96-106,169-174,205-211` decrypts, re-encrypts and selects good/rejection session keys. `NEV-AKE-C1/verify.c:15-24,39-46` uses fixed-length XOR accumulation and masked conditional move; the secret-derived validity bit does not control an early return in the inspected AKE path.
- `NEV-AKE-C1/ake.c:21-93` hashes fixed-sized transcript and secret components; the cited loops and input sizes are parameter-fixed. Public-message validation and fixed variant selection were not treated as secret leakage.

No new secret-dependent branch, indexed lookup or variable-divisor site was confirmed on this path. The many other NEV-AKE variants and lower-level NTRU arithmetic require further review; this is not a whole-candidate constant-time proof.
