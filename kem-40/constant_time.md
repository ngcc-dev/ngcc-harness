# Constant-time review — kem-40 YuanYang.KEM

Scope: representative reference instance `Implementations/Reference_Implementation/yuanyang-512/kem.c`; other levels, optimized copies and compiled machine code are not certified by this source review.

Secret-bearing values: private decapsulation key, recovered message/error state, rejection secret and shared secret. Public values: public key, ciphertext and encoded lengths. A decoder result is not automatically public merely because its ciphertext input is public.

- Source trace: `Implementations/Reference_Implementation/yuanyang-512/kem.c:224` — Decapsulation computes a private `overflow` from the key and ciphertext, then indexes `finvint[(i-overflow+h)%h]`; the memory address depends on that secret intermediate.
- Branches, indexed accesses and division/remainder: this first pass classifies only the traced operand above. Public-seed rejection and fixed-divisor arithmetic are not treated as leaks; absence of other leaks has not been proved.

Assessment: Source-confirmed secret-indexed address pattern in all three reference levels; `kem-40-2` records it as Medium/Probable because no measured cache trace or full-key recovery is yet available.

Reproduction is source/dataflow inspection at the cited location. A remote timing claim needs repeated same-public-input tests with changed secret state and an independent public-value control.

Key-generation follow-up: `kem-keygen.c:44` invokes `poly_inv_xn1_q` on secret `f`, reaching extended-Euclid `mod_inv` (`poly_inv_ntt.c:110-139`), whose secret-dependent divisions remain `idiv` in a GCC `-O2` build. This is one key-generation trace, not a repeated chosen-ciphertext oracle; no separate key-extraction result is inferred. The decapsulation `centered_mod`/`yy_encrypt` divisions at `kem.c:99-103,127` are size-build (`-Os`) leads and need an actual timing/security analysis.
