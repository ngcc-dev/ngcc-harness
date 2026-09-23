# Constant-time review — kem-39 Weaver

Scope: representative reference instance `Implementations/Reference_Implementation/WeaverKEM-256/indcpa.c`; other levels, optimized copies and compiled machine code are not certified by this source review.

Secret-bearing values: private decapsulation key, recovered message/error state, rejection secret and shared secret. Public values: public key, ciphertext and encoded lengths. A decoder result is not automatically public merely because its ciphertext input is public.

- Source trace: `Implementations/Reference_Implementation/WeaverKEM-256/indcpa.c:171` — The XOF rejection loop shown here expands a public matrix seed. It is not a secret-value leak even though its iteration count varies.
- Branches, indexed accesses and division/remainder: this first pass classifies only the traced operand above. Public-seed rejection and fixed-divisor arithmetic are not treated as leaks; absence of other leaks has not been proved.

Assessment: No new side-channel report is promoted from this representative path. Lower-level decoding, sampling and compiler output remain open audit work.

Reproduction is source/dataflow inspection at the cited location. A remote timing claim needs repeated same-public-input tests with changed secret state and an independent public-value control.

Size-build follow-up: `poly_tomsg` (`msgenc.c:143`) decodes a secret-key-derived polynomial; `poly_compress` (`msgenc.c:237`) and `polyvec_compress` (`polyvec.c:258`) can process FO re-encryption of the recovered message. GCC `-Os` emits hardware division at these sites, though the reviewed `-O2` build does not. The KyberSlash-like placement is a lead, not a demonstrated Weaver key-recovery oracle.
