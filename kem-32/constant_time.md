# Constant-time review — kem-32 QCTM

Scope: representative reference instance `Implementations/Reference_Implementation/QCTM128/kem.c`; other levels, optimized copies and compiled machine code are not certified by this source review.

Secret-bearing values: private decapsulation key, recovered message/error state, rejection secret and shared secret. Public values: public key, ciphertext and encoded lengths. A decoder result is not automatically public merely because its ciphertext input is public.

- Source trace: `Implementations/Reference_Implementation/QCTM128/kem.c:673` — The secret-key Goppa decoder returns a success bit that selects either decoded error or fallback vector through a branch at lines 673–679; the two paths execute different work.
- Branches, indexed accesses and division/remainder: this first pass classifies only the traced operand above, with one addition in encapsulation. `encrypt_nied` (same file, lines 426–440) branches on whether each secret error position is below `SYSTEMATIC_QC_ROWS`, doing one XOR in that case and a loop over all `CODIMENSION` rows otherwise, and the duplicate check in `m2e.c:39-45` indexes `seen[]` by error position. Public-seed rejection and fixed-divisor arithmetic are not treated as leaks; absence of other leaks has not been proved.

Assessment: Source-confirmed secret-dependent validity branch; candidate side-channel report requires a controlled differential decapsulation witness.

Reproduction is source/dataflow inspection at the cited location. A remote timing claim needs repeated same-public-input tests with changed secret state and an independent public-value control.

