# Constant-time review — kem-33 QUBE

Scope: representative reference instance `Implementations/Reference_Implementation/qube-256/src/ref/vector.c`; other levels, optimized copies and compiled machine code are not certified by this source review.

Secret-bearing values: private decapsulation key, recovered message/error state, rejection secret and shared secret. Public values: public key, ciphertext and encoded lengths. A decoder result is not automatically public merely because its ciphertext input is public.

- Source trace: `Implementations/Reference_Implementation/qube-256/src/ref/vector.c:96` — The fixed-weight sampler called from `qube_pke_decrypt` repeats until enough distinct positions are drawn; its reject/duplicate branch at line 102 depends on the persistent secret seed.
- Branches, indexed accesses and division/remainder: this first pass classifies only the traced operand above. Public-seed rejection and fixed-divisor arithmetic are not treated as leaks; absence of other leaks has not been proved.

Assessment: Confirmed secret-seed-dependent loop: `reproduce_ct_sampler.c` observes 254 versus 231 XOF draws for two seeds in the unmodified reference sampler. This does not prove full-key recovery.

Reproduction is source/dataflow inspection at the cited location. A remote timing claim needs repeated same-public-input tests with changed secret state and an independent public-value control.

Two further decapsulation paths were missed in the first pass. `src/common/kem.c:155-160` re-encrypts secret-key-recovered `m_prime`; `src/ref/qube.c:124-136` then rejection-samples encryption supports from coins derived from that message. The [Guo et al. rejection-sampling attack](https://eprint.iacr.org/2021/1485.pdf) motivates testing this path, but its HQC/BIKE key-recovery equations have not been transferred to QUBE. Separately, `src/ref/qube.c:182-192` expands the private seed into `y_support` and multiplies by public ciphertext `u` through `src/ref/gf2x.c:14-25,44-63`; branch outcomes and output addresses depend directly on each private support position. This direct support trace is distinct from the seed-sampler draw count. A concrete observation channel and full support recovery remain to be demonstrated before calling it shared-secret extraction.
