# Constant-time review — kem-37 TriQ-KEM

Scope: representative reference instance `Implementations/Reference_Implementation/TriQ-KEM-128/src/ref/vector.c`; other levels, optimized copies and compiled machine code are not certified by this source review.

Secret-bearing values: private decapsulation key, recovered message/error state, rejection secret and shared secret. Public values: public key, ciphertext and encoded lengths. A decoder result is not automatically public merely because its ciphertext input is public.

- Source trace: `Implementations/Reference_Implementation/TriQ-KEM-128/src/ref/vector.c:69` — The fixed-weight sampler re-expands the secret PKE seed during every decapsulation; reject/duplicate loops and XOF fetches depend on that seed, as in TriQ-KEX.
- Branches, indexed accesses and division/remainder: this first pass classifies only the traced operand above. Public-seed rejection and fixed-divisor arithmetic are not treated as leaks; absence of other leaks has not been proved.

Assessment: Confirmed secret-seed-dependent work: `reproduce_ct_sampler.c` observes one versus multiple XOF fetches for two seeds. This does not prove full-key recovery.

Reproduction is source/dataflow inspection at the cited location. A remote timing claim needs repeated same-public-input tests with changed secret state and an independent public-value control.

Decapsulation also re-encrypts the recovered message: `src/common/kem.c:192-200` derives `theta_prime` from secret-key-recovered `m_prime`, then `src/ref/triq_pke.c:113-115` calls secret-derived bounded-density and fixed-weight samplers (`src/ref/vector.c:244-253`). This is distinct from re-expanding the persistent private seed. The [Guo et al. HQC/BIKE mechanism](https://eprint.iacr.org/2021/1485.pdf) makes it a key-recovery lead, but no TriQ-specific oracle or extraction has been shown.
