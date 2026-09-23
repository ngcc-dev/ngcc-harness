# Constant-time review: DKEM

The private CPA vector `sA`, recovered CPA secret, rejection `z`, and shared secret are secret. The ciphertext, public key, tag, and dimensions are public. This follows DKEM-128 reference C; other parameter sets and assembly paths are outside this note.

`dkecca.c:78-114` calls `DKE_CPA_dec`, re-encapsulates, compares the full ciphertext, derives the fallback, then uses `DKE_cmov`. `dkecpa.c:189-207` computes the private-vector/ciphertext product with fixed-size NTT and polynomial routines; it has no source-level conditional on the resulting secret. Public/compile-time `#if` branches select implementation variants, not secret values. No secret-indexed lookup, branch, or variable-divisor operation was established in this representative chain.

Status: source-level screen, not a compiler or transitive-arithmetic proof. No report from this review.
