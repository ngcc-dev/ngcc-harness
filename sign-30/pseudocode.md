# sign-30 TRINE — algorithm summary

TRINE is a Fiat–Shamir signature from a group action on trilinear forms: the
hardness assumption is the (promised search) Trilinear Form Equivalence problem
psTFE / ℓ-psTFE, the same tensor-isomorphism class as MEDS (matrix-code
equivalence) and ALTEQ (alternating TFE). A GMW identification protocol is made
non-interactive by Fiat–Shamir; the novelty versus MEDS/ALTEQ is that the
response is a *corank-one projective point* (an n-vector) instead of a full
GL(n,q) matrix, with the commitment being the corank-one **canonical form**
CF(φ, û) of the challenged public form.

Specification: `sign-30-spec.pdf` (23 pages), §2.4 (canonical form, Alg. 1–4),
§3 (scheme, Alg. 5–8), §3.5 (size formulas), §4.1 (parameter tables).

## Parameters

| parameter | 128-Bal | 256-Bal | 512-Bal | 128-SS | 256-SS | 512-SS | meaning |
|---|---|---|---|---|---|---|---|
| n | 22 | 40 | 81 | 22 | 40 | 81 | tensor dimension in all 3 modes |
| q | 4093 | 4093 | 4093 | 4093 | 4093 | 4093 | prime field order (⌈log2 q⌉ = 12) |
| r | 138 | 271 | 534 | 61 | 116 | 232 | parallel FS rounds |
| K | 52 | 104 | 211 | 36 | 76 | 149 | number of non-base challenges |
| X | 1 | 1 | 1 | 4 | 4 | 4 | non-base public forms (X+1 forms total) |
| λ | 128 | 256 | 512 | 128 | 256 | 512 | claimed classical security (bits) |

Spec §3.5: PubKey = X·n³·⌈log q⌉ + 2λ bits, PriKey = 2λ bits,
Sig = (r−K+2)·λ + K·n·⌈log q⌉ bits.

Sizes (bytes), specification (Tables 1–2) vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | sig spec | sig impl | match |
|---|---|---|---|---|---|---|---|
| TRINE-128-Balanced | 16004 | 16004 | 32 | 32 | 3124 | 3156 | sig **+32** |
| TRINE-256-Balanced | 96064 | 96064 | 64 | 64 | 11648 | 11712 | sig **+64** |
| TRINE-512-Balanced | 797290 | 797290 | 128 | 128 | 46437 | 46565 | sig **+128** |
| TRINE-128-ShortSig | 63920 | 63920 | 32 | 32 | 1620 | 1652 | sig **+32** |
| TRINE-256-ShortSig | 384064 | 384064 | 64 | 64 | 5904 | 5968 | sig **+64** |
| TRINE-512-ShortSig | 3188774 | 3188776 | 128 | 128 | 23544 | 23672 | pk **+2**, sig **+128** |

## Pseudocode

### Canonical form (spec Alg. 1–4)
```
Corank1Cal(φ):                                   # Alg. 1
  repeat  u <-$ F_q^n  until rank(Φ_U(u)) = n-1;  return u

BuildUVW(M, u1):                                 # Alg. 2
  if rank(Φ_U(u1)) != n-1: return ⊥
  L_U <- (u1); L_V, L_W <- ()
  for i = 1..n:
     v_i <- LKer(Φ_U(u_i));  if v_i in span(L_V): return ⊥;  append
     if rank(Φ_V(v_i)) != n-1: return ⊥
     w_i <- RKer(Φ_V(v_i));  if w_i in span(L_W): return ⊥;  append
     if i = n: break
     if rank(M(w_i)) != n-1: return ⊥
     u_{i+1} <- LKer(M(w_i))
     if rank(Φ_U(u_{i+1})) != n-1 or u_{i+1} in span(L_U): return ⊥;  append
  return (U,V,W) = (cols u_i | v_i | w_i)   if det U·det V·det W != 0

DiagonalNormalize(M,U,V,W):                      # Alg. 3
  M~_k <- U^T M(w_k) V,  k = 1..n
  a_i = (M~_1)_{i,2}, b_j = (M~_1)_{1,j}, c_k = (M~_k)_{1,2}   (3<=i,j,k<=n)
  d1=(M~_1)_{1,2}, d2=(M~_5)_{2,3}, d3=(M~_2)_{1,3}, d4=(M~_2)_{2,1}
  if any of a,b,c,d is 0: return ⊥
  f1=1, g2=1, h1=d1^-1; f_i=d1/a_i; g_j=d1/b_j; h_k=h1·d1/c_k
  f2=(g3 h5 d2)^-1, h2=(f1 g3 d3)^-1, g1=(f2 h2 d4)^-1
  return M̄_k = h_k · F · M~_k · G      (F=diag f, G=diag g, H=diag h)

CF(M, u) = DiagonalNormalize(M, BuildUVW(M,u))   # Alg. 4, ⊥-propagating
```

### KeyGen (spec Alg. 5, §3.2)
```
1  φ_X  <-$ TF(n,q)                        # base form, the (X+1)-th form
2  (A_i, B_i, C_i) <-$ GL(n,q)^3,  i = 0..X-1
3  φ_i  <- φ_X^{A_i,B_i,C_i}   for i = 0..X-1
4  pk = (φ_0, ..., φ_X),  sk = {(A_i,B_i,C_i)}
   §3.5: in practice sk is a 2λ-bit seed generating the 3X matrices, and
   φ_X in pk is a 2λ-bit seed; pk transmits only φ_0..φ_{X-1} explicitly.
```

### Sign (spec Alg. 6 + §3.4 unbalanced challenges)
```
1  for i = 0..r-1:  repeat  a_i <- Corank1Cal(φ_X);  ψ_i <- CF(φ_X, a_i)
                    until  ψ_i != ⊥
2  cha  <- H(M || ψ_0 || ... || ψ_{r-1})  in {0,1}^{2λ}
3  (b_0,...,b_{r-1}) <- ParseHash(cha)     # exactly K indices have b_i != X
4  d_i <- A_{b_i}^{-1} a_i                 # A_X = B_X = C_X = I
5  S = (cha, d_0, ..., d_{r-1})
   §3.4: for the r-K rounds with b_i = X, d_i = a_i is independent of sk,
   so a λ-bit seed generating a_i is transmitted instead of an n-vector.
```

### Verify (spec Alg. 7)
```
1  for i = 0..r-1:  ψ'_i <- CF(φ_{b_i}, d_i)     (b from ParseHash(cha))
2  cha' <- H(M || ψ'_0 || ... || ψ'_{r-1})
3  (b'_0,...,b'_{r-1}) <- ParseHash(cha')
4  accept iff b'_i = b_i for all i         # spec compares challenges, not digests
```

### ParseHash (spec Alg. 8)
```
x_0,x_1,... <- XOF(digest)
b_i <- 0 for all i
repeat K times:
   sample α from BitLen(r) bits; reject if α >= r or b_α already set
   sample b_α from BitLen(X+1) bits; reject if b_α = 0 or b_α >= X+1
output (X - b_0, ..., X - b_{r-1})
```

Hash: the spec only postulates `H : {0,1}* -> {0,1}^{2λ}` and an XOF; it names
no concrete primitive. The ICCS build (`-DUSE_ICCS`, `hashkdf.c`) uses
`sm3hash(256,…)` for 2λ=256, `pseudohash(512/1024,…)` for 2λ=512/1024 and
`pseudoXOF` for the XOF; the `-DUSE_SHA3` build uses SHAKE256 instead.

## Implementation vs specification

Checked (reference tree `Implementations/Reference_Implementation/TRINE-*`,
read at the `TRINE-128-balanced` instance; the six instance directories are
identical except for the default `PARAMS`):
`trine.c` (`crypto_sign_keypair`/`crypto_sign`/`crypto_sign_verify`),
`corank1.c` (Alg. 1), `canonical.c` (Alg. 2–4), `triform.c` (group action),
`util.c:178 trine_parse_hash` (Alg. 8), `trine_codec.c` (encodings),
`hashkdf.c` (H/XOF), `params.h` (constants). **Not run** (per instruction —
signing is minutes to hours per record).

Agreements:
- `params.h` carries (n,q,r,K,X,λ) exactly as spec Tables 1–2 for all six sets,
  with `TRINE_q_bits 12`; spot-checked all six sets, all match.
- `crypto_sign_keypair` (trine.c:178) is Alg. 5 in its seeded form: one secret
  seed -> public seed -> base form φ_X, then X pullbacks by (A_i,B_i,C_i)
  expanded from the secret seed. sk = the 2λ-bit seed, matching §3.5.
- `trine_parse_hash` (util.c:178–238) is Alg. 8 including the final `X − b_i`
  inversion and both rejection loops.
- `crypto_sign_verify` (trine.c:493) is Alg. 7: recompute all ψ'_i, re-hash,
  re-parse, and accept only if the parsed challenge vectors are equal
  (`trine_challenges_equal`), plus a weight check `trine_challenges_valid`
  (exactly K non-base) and an exact `siglen == TRINE_SIG_BYTES` check. No
  inverted or skipped check was found; failures all funnel to `result = -1`.

Discrepancies:
- **(a) Signature is 2λ bits larger than the spec's own formula.** `params.h`
  lays out the signature as `responses || base_seeds || digest(2λ) ||
  salt(2λ)` (`TRINE_digest_bytes`/`TRINE_salt_bytes` both `2*TRINE_lambda_bytes`,
  `TRINE_SIG_BYTES = TRINE_SALT_OFFSET + TRINE_salt_bytes`, params.h:170–188).
  §3.5's `SigSize = (r−K+2)·λ + K·n·⌈log q⌉` accounts for the digest but not the
  salt, hence exactly +2λ bits (+32/+64/+128 B) at every level, matching the
  built library. This is a deliberate strengthening (per-signature salt fed
  into the round-commitment XOF, `trine.c:44 trine_init_round_xof`) that the
  specification never mentions; the spec's Tables 1–2 sizes are therefore wrong
  for the submitted code.
- **(b) 512-ShortSig public key 3188774 (spec) vs 3188776 (impl), +2 bytes.**
  This is a spec arithmetic artifact, not a code defect: one trilinear form is
  n³·12 = 6377292 bits = 797161.5 bytes, which the code rounds up *per form*
  (`TRINE_EXPECTED_TRIFORM_BYTES 797162`), while §3.5 rounds the X-fold total
  once. All other pk sizes agree exactly.
- **(c) Salt is not bound into the Fiat–Shamir transcript.** `crypto_sign`
  /`crypto_sign_verify` absorb `M || ψ_0 || … || ψ_{r-1}` only. The signer feeds
  the salt into the commitment derivation of all r rounds (`trine.c:373-379`),
  but the verifier uses it only for the r−K base rounds (`trine.c:613-619`) and
  recomputes the K non-base ψ's from the responses without it
  (`trine.c:599-603`). It therefore is only indirectly committed (via the r−K
  base ψ's) and not at all through the K non-base rounds. Not a spec deviation (the spec has no salt), but the salt
  does not achieve the usual multi-target binding.
- **(d) Unbounded transcript buffering (resource, ICCS build).** With
  `-DUSE_ICCS` there is no streaming hash: `trine_hash_absorb` (hashkdf.c:266)
  appends into a `realloc`'d buffer and only hashes at finalize. The transcript
  is M plus r encoded forms of `TRINE_TRIFORM_BYTES`: ≈ 534 × 797162 B ≈ 425 MB
  for TRINE-512-Balanced and ≈ 185 MB for 512-ShortSig, per signature and per
  verification. Correctness is unaffected; the memory profile is not in the
  spec.

Also checked and agreeing: `canonical.c:83 compute_normalization_diagonals`
reproduces Alg. 3 in inverse-free form — it sets `f1=1, g2=1, h1=d1^-1`,
`f_i=d1/a_i`, `g_j=d1/b_j`, and computes `f2 = b3·c5/(d1·d2)`, `h2 = b3/(d1·d3)`,
`g1 = (f2·h2·d4)^-1`, which expand to exactly the spec's
`f2=(g3h5d2)^-1, h2=(f1g3d3)^-1, g1=(f2h2d4)^-1`; the anchors are read at the
spec's index positions (0-based `M1[1]`, `M5[…]`, `M2[…]`).

Not verified: `canonical.c`/`matrixelim.c` were not executed or differentially
tested against Alg. 2–3 for all n; the scheme was not run, per instruction. The
security-reduction claims of §5 were not audited.
