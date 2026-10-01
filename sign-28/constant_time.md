# Constant-time review — 28 SYDO

Scope: all six submitted reference trees, with representative tracing in `sydo_160f`; the optimized implementation has not been exhaustively certified. This remains a source-level review, not a constant-time certification.

Secret: the long-term RSD witness and all values arithmetically derived from it during signing; the secret key and per-signature seed material remain secret until the protocol intentionally reveals the corresponding transcript values. Public: verification key, message, final signature and verifier-only intermediates.

- Secret branches: `src/quicksilver.c:810–923` emits the prover's membership constraints with branches on the witness-derived `cache->a0[]` and `cache->a1[]` bits. The branch pattern is not made public by the signature interface. Replacing these conditional copies/XORs with masks is a local constant-time repair.
- Secret comparisons/cache behavior: `src/primitives.c:92–99` and `126–133` use early-exit `memcmp` on PRG keys derived from secret tree-node seeds and branch on cache hits. Disabling the cache or comparing/selecting every entry with masks is a local repair. The global cache is also not thread-safe.
- Wrapper branch: `SIG_AlgorithmInstance.c:76` tests an RNG return code, not a secret bit.
- Division/remainder and table lookups: this review does not certify their absence throughout the deeper proof implementation.

No timing or cache observation channel, witness recovery, or forgery was demonstrated for these source-level branches, so no separate side-channel finding is promoted. The definite stack over-read in `universal_hashing_impl.inc` is a memory-safety issue rather than a timing finding and is tracked as `sign-28-4`.

Recheck the cited source lines and callers before reusing this result. A timing claim requires controlled same-public-input tests with changed secret state and a public-value control.
