# Loom constant-time review

Scope: LoomKEX-128 representative reference implementation; 256/512 and alternative compiled modes were not exhaustively compared. Secrets include KEM decapsulation keys, recovered messages, ephemeral KEM coins, signature keys and derived AKE secret; transcript bytes and wire lengths are public.

- `LoomKEX-128/kem/kem_cpaf.c:151-168` decrypts with the private key, recomputes a ciphertext tag, and explicitly returns `-1` on mismatch. The branch is not a *new timing-only* finding: this rigid API already exposes the same validity bit as its return value. Its broader protocol significance should be assessed with the existing Loom reports, not relabelled as a separate constant-time leak without an additional secret-value witness.
- `LoomKEX-128/kem/indcpa.c:171-201` has rejection tests when expanding a matrix from a public seed. Those branches concern public-derived coefficients. `LoomKEX-128/loom/state_serialize.c:149-174,254-281` tests public buffer sizes, role/stage and header fields; the state structure itself contains secrets, but these cited predicates do not inspect secret key bytes.

No separate constant-time report was established in the traced reference path. The embedded signature and alternate Loom modes still need a deeper compiled-code audit; this note does not certify them.
