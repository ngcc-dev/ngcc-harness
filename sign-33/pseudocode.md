# sign-33 VDOO (Vinegar–Diagonal–Oil–Oil) — algorithm summary

VDOO is a hash-and-sign multivariate signature of the classical bipolar
(UOV/Rainbow) shape P = S ∘ F ∘ T, whose novelty is a three-layer central map
F: a *diagonal* layer (each polynomial introduces exactly one new variable
linearly, so it inverts without Gaussian elimination) followed by two UOV
layers. Security rests on MQ / the rank and simple attacks against the
layered oil structure; inverting F costs only two Gaussian eliminations
(sizes o1 and o2).

Specification: `sign-33-spec.pdf` (29 pages), §4 (Algorithms 1–5:
VDOOCentPoly, VDOOCentPoly_Inversion, VDOOKeyGen, VDOOSign, VDOOVerif), §4.7
(key sizes), §6 Table 1 (parameters), §7 (implementation notes, which describe
this very reference code).

## Parameters

| parameter | VDOO-128 | VDOO-256 | VDOO-512 | meaning |
|---|---|---|---|---|
| q | 16 | 256 | 256 | field size (tower F16 / F256, §7.1) |
| v | 67 | 205 | 260 | vinegar variables |
| d | 16 | 10 | 10 | diagonal variables/equations |
| o1 | 14 | 17 | 10 | first oil layer |
| o2 | 40 | 68 | 175 | second oil layer |
| n = v+d+o1+o2 | 137 | 300 | 455 | total variables |
| m = d+o1+o2 | 70 | 95 | 195 | total equations |
| salt | 16 B | 16 B | 16 B | `SALT_BYTES` |
| claimed security | 128 | 256 | 512 | bits; (SA, RA) = (166,162)/(278,281)/(526,642) |

Sizes (bytes), specification Table 1 vs the built reference library
(Table 1 quotes KB; converted here at 1 KB = 1024 B):

| instance | pk spec | pk impl | sk spec | sk impl | sig spec | sig impl | match |
|---|---|---|---|---|---|---|---|
| VDOO-128 | 323.1 KB ≈ 330 854 | 330 855 | 226.7 KB ≈ 232 141 | **342 790** | 85 | 85 | sk **+47.7 %** |
| VDOO-256 | 4188.7 KB ≈ 4 289 229 | 4 289 250 | 3529.0 KB ≈ 3 613 696 | **4 388 307** | 316 | 316 | sk **+21.4 %** |
| VDOO-512 | 19755.2 KB ≈ 20 229 325 | 20 229 300 | 15882.9 KB ≈ 16 264 090 | **20 474 382** | 471 | 471 | sk **+25.9 %** |

pk and signature agree to rounding; only the secret key differs (see below).

## Pseudocode

### Central map and its inversion (spec Algorithms 1–2)
```
DiagPoly(q,k)      f_{k-v}(x) = Σ_{i<k} α^{(k)}_{i,k} x_i x_k + Σ_{i≤j<k} β^{(k)}_{i,j} x_i x_j
OVPoly(q,v',o)     f(x) = Σ_{i,j≤v'} α_{ij} x_i x_j + Σ_{i≤v'} Σ_{j=v'+1}^{v'+o} β_{ij} x_i x_j
                   (no oil×oil terms)

VDOOCentPoly(params):                                    # Alg. 1
  for 1≤i≤d          : f_i <- DiagPoly(q, v+i)
  for d+1≤i≤d+o1     : f_i <- OVPoly(q, v+d, o1)
  for d+o1+1≤i≤m     : f_i <- OVPoly(q, v+d+o1, o2)

VDOOCentPoly_Inversion(F, y):                            # Alg. 2
  x_1..x_v <-$ F_q                                       # vinegar
  for 1≤i≤d: solve the single unknown x_{v+i} from y_i and x_1..x_{v+i-1}
  (f~) <- ST(f_{d+1}..f_{d+o1} with x_1..x_{v+d} fixed)
  (x_{v+d+1}..x_{v+d+o1}) <- GE(q,o1) on (f~_i = y_i)
  (f~) <- ST(f_{d+o1+1}..f_m with x_1..x_{n-o2} fixed)
  (x_{v+d+o1+1}..x_n) <- GE(q,o2)
  return x    (failure if any GE is rank-deficient -> resample vinegar)
```

### KeyGen (spec Algorithm 3)
```
1  m <- d+o1+o2 ; n <- m+v
2  seed <- PRNG(1^λ)
3  while det(S) != 0 && det(T) != 0:          # (spec's loop, as printed)
4      S <- randomMatrix(q, m, seed)          # S ∈_U F_q^{m×m}
5      T <- randomMatrix(q, n, seed)          # T ∈_U F_q^{n×n}
7  a ∈_U F_q^m ,  b ∈_U F_q^n
8  invS <- invMat(q,m,S) ;  invT <- invMat(q,n,T)
9  S <- Affine(S,a) ;  T <- Affine(T,b)
10 F <- VDOOCentPoly(params)
11 P <- S ∘ F ∘ T
12 pk = P ;  sk = (invS, a, invT, b, F)
```

### Sign (spec Algorithm 4)
```
1  salt <- PRNG
2  d <- H( H(msg) || salt ) ∈ F_q^m
3  t <- invS × (d − a)                        # t = S^{-1}(d)
4  y <- F^{-1}(t)   via Algorithm 2           # on GE failure: new salt, restart
5  s <- invT × (y − b)                        # s = T^{-1}(y)
6  σ = (s, salt)
```

### Verify (spec Algorithm 5)
```
1  d  <- H( H(msg) || salt )
2  d' <- P(s)
3  accept iff d = d'
```

### Hash / PRNG (spec §7.2)
All symmetric primitives are the ICCS reference ones: SM3 with its HMAC,
`pseudohash` and `pseudoXOF`; deterministic byte generation is the ICCS DRNG
"initialized from a seed and subsequently used to generate the pseudorandom
values required during key generation".

## Implementation vs specification

Checked, in `Implementation/Reference_Implementation/vdoo_{128,256,512}`:
`vdoo_config.h` (parameters, sizes), `api.c`/`SIG_AlgorithmInstance.c` (NGCC
API), `vdoo_keypair.c` (Alg. 1 + 3), `vdoo_sign.c` (Alg. 2 + 4),
`vdoo_verif.c` (Alg. 5), `rng.c`, `utils.c` (`hash_msg`). Not executed.

Agreements:
- `vdoo_config.h` holds exactly Table 1's tuples: (16,67,16,14,40),
  (256,205,10,17,68), (256,260,10,10,175), with n and m derived as the spec
  defines them. `CRYPTO_BYTES = ⌈n/elems_per_byte⌉ + 16` gives 85/316/471,
  matching Table 1's signature column exactly.
- `PK_BYTES = m·n(n+1)/2 / elems_per_byte` reproduces Table 1's public keys.
- `vdoo_sign.c` implements Algorithm 2 layer by layer (`solve_diagonal_eq`,
  `substitute_fixed_vars` + `gauss_elimination` twice). On a layer failure
  `vdoo_inverse_F` resamples the vinegar variables for the same salt, up to
  100 times (`vdoo_sign.c:177-224`); only when all of these fail does
  `vdoo_sign` draw a fresh salt, itself capped at 100 attempts
  (`vdoo_sign.c:274-300`) — matching §7.4.
- `vdoo_verify` implements Algorithm 5 with a real comparison
  (`diff |= computed[i] ^ correct[i]; return diff == 0 ? 0 : -1`) and the
  double hash `H(H(msg) || salt)`; `hash_msg` = `pseudoXOF` (ICCS).
  `sig_verify` propagates the result. No skipped or inverted check.

Discrepancies:
- **(a) CRITICAL — none of the randomness comes from the seeded DRNG.**
  `SIG_AlgorithmInstance.c:8` declares `extern DRNG_ctx drng_algorithm` and
  never reads from it. Every random byte in KeyGen and Sign goes through
  `get_randombytes` (`rng.c`), which draws from a *second*, file-local global
  `DRNG_ctx drng` (`rng.c:4`). The only callers of `init_randombytes` are the
  standalone tools `vdoo-keygen.c:30` and `test/test_{sizes,memory}.c` — none
  of which is part of the library build. So the DRNG state is the
  zero-initialised BSS object, advanced only by its internal counter.
  Consequences: (i) the per-record `Seed` supplied by the KAT driver has no
  effect on the output at all; (ii) every fresh process produces the *same*
  first key pair and the same salts. The spec is explicit that this should not
  be so — Algorithm 3 line 2 (`seed ← PRNG(1^λ)`) and lines 4–5
  (`randomMatrix(q,·,seed)`) derive S and T from a seed, and §7.2 states that
  "all deterministic pseudorandom byte generation is performed using the ICCS
  DRNG, which is initialized from a seed". KATs still reproduce because they
  were generated the same way.
- **(a) The secret seed stored in the secret key is never used.**
  `crypto_sign_keypair` (api.c) draws `sk_seed` via `get_randombytes` and
  `generate_private_key` (vdoo_keypair.c:267) only `memcpy`s it into
  `sk->sk_seed`; S, T and F are then filled from further `get_randombytes`
  calls, not from the seed. The 32-byte field is dead weight, and the
  spec's seed-expansion design (§4.7: "These maps can be generated using a
  random seed") is not realised.
- **(a/size) Secret key is 21–48 % larger than spec Table 1.** `sk_t`
  (vdoo_keypair.h) = `sk_seed[32]` + dense `invS[m_byte·m]` + dense
  `invT[n_byte·n]` + **a dense `F[m_byte·SUM_K(n)]` of exactly the public-key
  size**, i.e. 32 + 2450 + 9453 + 330855 = 342790 at level 128 (and 4 388 307 /
  20 474 382). The central map is stored as a full dense quadratic form with
  the forbidden monomials merely zeroed (`generate_central_map` →
  `set_zero_forbidden_terms`), whereas §4.7 sizes F by its *structurally
  non-zero* coefficients only (diagonal layer Σ(v_i(v_i+1)/2 + v_i) plus two
  UOV layers ≈ 290 326 field elements ≈ 145 163 B at level 128). Note the
  spec's Table 1 does not agree with its own §4.7 formula either, so the
  spec side is internally inconsistent; the concrete fact is that the library
  reports 342 790 / 4 388 307 / 20 474 382 against 226.7 / 3529.0 / 15882.9 KB.
- **(a) The affine offsets a and b of Algorithm 3 are absent — S and T are
  linear, and the public map is purely homogeneous.** There is no vector `a`
  or `b` anywhere in `vdoo_keypair.c`/`vdoo_sign.c`; `apply_invS`/`apply_invT`
  are plain matrix-vector products, and `pk` holds only the n(n+1)/2
  homogeneous quadratic coefficients per polynomial, not the
  (n+1)(n+2)/2 of §4.7. Table 1's pk column agrees with the homogeneous
  count, so Table 1 and the code agree and §4.7/Algorithm 3 lines 7 and 9 are
  the outliers — but Algorithm 3 as written is not what is implemented.
- **(c, documented) S and T are not uniform over GL(q,·).** They are built as
  L·U with non-zero diagonals (`generate_M_invM`, vdoo_keypair.c:110), so only
  matrices all of whose leading principal minors are non-zero occur —
  a fraction (1−1/q)^n / ∏_{i≤n}(1−q^{−i}) of GL(n,q), ≈1.6·10⁻⁴ at level 128.
  This is exactly what §7.3 describes, so it is a documented deviation from
  Algorithm 3's "randomMatrix … ∈_U F_q^{n×n}" with a det ≠ 0 rejection loop,
  not an undocumented one; its effect on the rank/min-rank analyses of §5 is
  not addressed by the spec.
- **(b, spec) Algorithm 3's loop condition is printed as
  `while (det(S) ≠ 0 && det(T) ≠ 0)`**, i.e. it loops *while the matrices are
  invertible* — an obvious typo for `while (det(S) = 0 || det(T) = 0)`. The
  implementation sidesteps the question with the LU construction.
- **(minor) dead code:** `utils.c` retains a commented-out `hash_msg` that was
  a byte-wise XOR fold; `LEN_PKSEED` is defined and unused; `vdoo_naive_evaluation`
  zeroes `y` as raw bytes while `vdoo_optimized_evaluation` writes packed
  nibbles (the naive routine is not called from the API path).

Not verified: the monomial-index arithmetic of `solve_diagonal_eq` /
`substitute_fixed_vars` against the exact term ranges of §4.3 was read but not
re-derived exhaustively; `is_allowed_diag/oil1/oil2` were not checked
term-by-term against the three layer formulas. §5 security analysis and the
optimized key generation of §4.9 (Algorithm 6, not used by this build) were
not audited.
