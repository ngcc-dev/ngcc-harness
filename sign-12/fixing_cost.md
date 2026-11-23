No: both Critical findings are departures of the reference code from a sound specification, repaired by seeding key generation from the API generator and by passing the full 64-bit message length to the message hash, and neither changes a size or an operation count in any parameter set.

Impact: none
Scope: Function (small), code
Assessed: 2026-11-23

## Attacks considered

- sign-12-1: Publicly reproducible signing keys (Implementation, Confirmed)
- sign-12-2: 32-bit message-length truncation enables long-message forgeries (Implementation, Confirmed)

## Remediation

- **sign-12-1.** Root cause: `sig_keygen` builds a private generator and seeds it with 32 zero bytes unless a KAT helper has set a seed; it never reads the API generator. The costed repair is to take the key-generation seed from the initialized API generator and to drop the zero default, as the finding states. Scope: Single line, code. The specification samples k and x uniformly (Algorithm 19, physical p. 29) and names the NGCC drng as the randomness source of the reference code (Appendix A.2, physical p. 44); it stays as it is. Impact: none. The code skips no specified work: each key-generation attempt draws the same 2λ bits as before, and no key or signature changes. The report records no proposed fix. Not checked: the optimized tree, which is a separate code base.
- **sign-12-2.** Root cause: signing and verification cast `m_len_bytes` to `unsigned` before the message hash, and `galas_hash_mu` takes an `unsigned` length. The costed repair is the reporter's first proposal: hash the whole message with its full length, by widening that one parameter and removing the two casts. The hash layer below already takes `size_t` lengths. Scope: Function, code. The specification absorbs pk and then the whole message M in Hmsg (Table 5, physical p. 13; Algorithm 21 line 3, physical p. 30) and stays as it is. Impact: none. For a message under 2^32 bytes the same bytes are hashed as before; above that the code hashes the bytes that the specification already requires. The reporter's alternative, rejecting lengths that the code cannot process, is not costed, because it would not keep the required message range of 2^63 bits. Not checked: the reference hash layer copies the whole message into one buffer before hashing, and its behaviour on very long messages was not examined; the optimized implementation was not assessed.

## Evidence

- Baseline: Table 9 (physical p. 32) gives public keys of 40 to 128 bytes and signatures of 4,812 to 59,416 bytes. The formula of Section 3.1.3 (physical p. 8), (τ − 1)ℓ̂ + (λ + B) + ℓ + 2λ + (2τ + Topen)λ + λ + 160 + 32 bits with ℓ = 5λ/2, ℓ̂ = ℓ + 3λ + 16 and B = 16, reproduces all eight signature sizes; for Galas-160S, 13·896 + 176 + 400 + 320 + 160·160 + 160 + 160 + 32 = 38,496 bits = 4,812 bytes. Neither repair changes a term.
- Table 9 gives secret keys of λ/8 bytes (sk = k). The reference library stores x ∥ k, 2λ/8 bytes (`ngcc/SIG_AlgorithmInstance.c:95-97`). The secret key is not transmitted and neither repair touches it.
- sign-12-1, source under `sign-12/Implementations/Reference_Implementation/Galas-160F/`: `ngcc/SIG_AlgorithmInstance.c:69-73` (zero default seed and private generator), `:81-82` (the two draws per attempt), `ngcc/KAT_SIG.c:116` (the test harness seeds `drng_algorithm`, which key generation does not use).
- sign-12-2, same tree: `ngcc/SIG_AlgorithmInstance.c:291,413` (the casts), `galas/sign_helpers.c:8-15` (`unsigned msg_len`), `galas/random_oracle.h:42` and `galas/xof.h:59` (`size_t` lengths below), `galas/xof.c:33-38` (the message is buffered whole). `SIG_AlgorithmInstance.c` and `sign_helpers.c` are byte-identical in the eight reference trees.
