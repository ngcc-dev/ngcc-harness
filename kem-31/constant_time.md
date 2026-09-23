# Constant-time review — kem-31 QIMEN-PIKE

Scope: representative reference instance `Implementations/Implementations/src/ngcc/common/KEM_AlgorithmInstance.c`; other levels, optimized copies and compiled machine code are not certified by this source review.

Secret-bearing values: private decapsulation key, recovered message/error state, rejection secret and shared secret. Public values: public key, ciphertext and encoded lengths. A decoder result is not automatically public merely because its ciphertext input is public.

- Source trace: `Implementations/Implementations/src/ngcc/common/KEM_AlgorithmInstance.c:227` — The decapsulation wrapper branches on the result of private-key decryption and later on `is_valid` at line 250. Existing `kem-31-1` already records a stronger invalid-ciphertext abort.
- Branches, indexed accesses and division/remainder: this first pass classifies only the traced operand above. Public-seed rejection and fixed-divisor arithmetic are not treated as leaks; absence of other leaks has not been proved.

Assessment: No new side-channel report is promoted from this representative path. Lower-level decoding, sampling and compiler output remain open audit work.

Reproduction is source/dataflow inspection at the cited location. A remote timing claim needs repeated same-public-input tests with changed secret state and an independent public-value control.

Arithmetic follow-up: the `decrypt` routine in `src/pike/ref/pikex_compressed/pike_compressed.c:832-875` loads long-term private `deg`, `alpha`, `beta`, and `iota`, then calls `ibz_invmod`/`ibz_mod` on products of those values. The reference `intbig` backend delegates to GMP `mpz_invert`/`mpz_mod` (`src/intbig/ref/generic/intbig.c:317,928`). These are not constant-time operations on secret operands. Their work may be swamped by isogeny computations, and no recoverable timing predicate or shared-secret extraction is shown; this remains an unpromoted lead, separate from the already reported invalid-ciphertext assertion.
