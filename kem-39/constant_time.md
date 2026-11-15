# Constant-time review — kem-39 Weaver

Scope: representative reference instance `Implementations/Reference_Implementation/WeaverKEM-256/indcpa.c`; other levels, optimized copies and compiled machine code are not certified by this source review.

Secret-bearing values: private decapsulation key, recovered message/error state, rejection secret and shared secret. Public values: public key, ciphertext and encoded lengths. A decoder result is not automatically public merely because its ciphertext input is public.

- Source trace: `Implementations/Reference_Implementation/WeaverKEM-256/indcpa.c:195-203` — The XOF rejection loop shown here (the Lemire variant compiled for q = 7681; the variant at lines 161–183 is compiled only for q = 3329, WeaverKEM-128) expands a public matrix seed. It is not a secret-value leak even though its iteration count varies.
- Branches, indexed accesses and division/remainder: this first pass classifies only the traced operand above. Public-seed rejection and fixed-divisor arithmetic are not treated as leaks; absence of other leaks has not been proved.

Assessment: decapsulation has secret-dependent branches and addresses at source level, filed as kem-39-7. `poly_tomsg` runs the BCH decoder on the noisy message before the re-encryption check: `bch_low.c:80,90,98` index the log/antilog tables with syndrome-derived values, `:191` branches on `tmp > elp->deg`, and `:438` (`decode_bch_low_nibbles`; `:320` in `decode_bch_low`, used by WeaverKEM-128/-512) stores each correction at the error position. The lifting sampler accepts under `t[j] < BUCKET_SZ[r[ctr]]` (`poly_invq.c:76`) on PRF output keyed by the encryption coins, and `popcount_u32` (`cbd.c:24-31`) loops `while(x)` on the secret CBD halves in WeaverKEM-256/-512. `bch_low.c` and `poly_invq.c` are identical in the optimized tree, whose `cbd.c` still calls `popcount_u32` because no build file defines `WEAVER_USE_AVX_CBD`. No key recovery is demonstrated; compiler output remains open audit work.

Reproduction is source/dataflow inspection at the cited location. A remote timing claim needs repeated same-public-input tests with changed secret state and an independent public-value control.

Size-build follow-up: `poly_tomsg` (`msgenc.c:143`) decodes a secret-key-derived polynomial; `poly_compress` (`msgenc.c:237`) and `polyvec_compress` (`polyvec.c:258`) can process FO re-encryption of the recovered message. GCC `-Os` emits hardware division at these sites, though the reviewed `-O2` build does not. The KyberSlash-like placement is a lead, not a demonstrated Weaver key-recovery oracle.
