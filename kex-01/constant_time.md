# ADKEX constant-time review

Scope: ADKEX-128 representative reference implementation; 256/512 share the same protocol structure but were not machine-code audited. Persistent responder KEM `sk_B`, initiator ephemeral decapsulation key, provisional KEM messages, PRNG coins and final shared secret are secret. Public keys, `m1/m2` ciphertexts, their fixed lengths and transcript labels are public.

- `ADKEX-128/KEX_AlgorithmInstance.c:55-68,76-92` generates secret long-term and ephemeral coins, then calls the deterministic core. `ADKEX-128/adkex_derand.c:68-99` passes secret keys through decapsulation and the KDF.
- `ADKEX-128/dkecca.c:95-114` decrypts and re-encrypts, then uses `DKE_verify` and `DKE_cmov` for implicit rejection. Both are fixed-length, mask-based loops in `ADKEX-128/verify.c:16-37`; no secret-dependent early-exit comparison was found in that path.
- `ADKEX-128/dke_utils.c:85-92` converts private random bits with `DKE_cmov_int16`, avoiding secret-bit table indices. Hash-length conditions in `ADKEX-128/dke_hash.c:96,120,137` are public configuration/length tests.

No new secret-dependent branch, lookup, or variable-divisor site was confirmed in these traced paths. This is not a proof of constant time: AVX2/AArch64 implementations and all compiler output remain outside this review.
