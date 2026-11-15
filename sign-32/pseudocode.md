# sign-32 UVW signature — algorithm summary

UVW is a code-based hash-and-sign signature over F3 in the Wave family: the
trapdoor is a generalized (U+V, U+W) code, and a signature is a *large-weight*
syndrome-decoding solution (weight exactly w) for the public parity-check
matrix in systematic form. Hardness is the (large-weight) syndrome decoding
problem over F3 plus indistinguishability of the permuted generalized
(U+V,U+W) parity-check matrix from a random one. Signing uses a Prange
information-set step in the Z domain and a rejection loop in the XY domain so
that the output weight is exactly w and its distribution is independent of the
trapdoor.

Specification: `sign-32-spec.pdf` (38 pages), §1.3 (Algorithms 1–5: InfSet,
PrangeStep, Prange, Dec_Z, Dec_XY), §1.4 (overview, Figure 1), §1.5
(Algorithms 6–8: KeyGen/Sign/Verify), §1.6 Table 1 (parameters).
Document is in Chinese/English mixed typesetting; the algorithm boxes and
Table 1 are in English.

## Parameters

| parameter | UVW128 (L1) | UVW256 (L2) | UVW512 (L3) | meaning |
|---|---|---|---|---|
| q | 3 | 3 | 3 | field (everything over F3) |
| n | 9700 | 19200 | 39000 | code length |
| k | 4850 | 9600 | 19500 | dimension, k = k1 + k2 |
| k1 | 3250 | 6433 | 13066 | XY-domain dimension |
| k2 | 1600 | 3167 | 6434 | Z-domain dimension |
| w | 8633 | 17088 | 34710 | target signature weight (large weight) |
| λ | 128 | 256 | 512 | salt length r ∈ {0,1}^λ |
| l (seed) | 256 | 256 | 512 | master-seed bits (§1.5) |
| claimed security | 160 cl. / 80 q. | 256 / 128 | 512 / 256 | bits (spec §1.6) |

Sizes (bytes), specification Table 1 vs the built reference library. Table 1's
public-key and signature columns are declared to be *information-entropy lower
bounds*, not the submitted encoding:

| instance | pk spec | pk impl | sk spec | sk impl | sig spec | sig impl | match |
|---|---|---|---|---|---|---|---|
| UVW-128 | 4.44 MiB ≈ 4 660 278 | 5 897 612 | (40) | 40 | 977 | 1244 | **no** (+26.5 % / +267 B) |
| UVW-256 | 17.41 MiB ≈ 18 258 700 | 23 040 012 | (40) | 40 | 1934 | 2444 | **no** |
| UVW-512 | 71.85 MiB ≈ 75 336 000 | 95 160 012 | (72) | 72 | 3928 | 4956 | **no** |

The secret key is not tabulated in the spec, but §1.5 defines it as
(seed, nonce_x, nonce_y, nonce_z, nonce_d) = l/8 + 4·2 bytes = 40 / 40 / 72,
which the library matches exactly.

## Pseudocode

### Auxiliary (spec Algorithms 1–5)
```
InfSet(H):            return an information set I, |I| = k, whose complement
                      indexes n−k linearly independent columns of H

PrangeStep(H, s, I, x):                                   # Alg. 2
   pick permutation P moving I to the last k coordinates
   (A || B) <- H P ;  (0 || e'') <- x P
   e <- [ (s − e'' Bᵀ)(Aᵀ)^{-1} || e'' ] Pᵀ
   return e                                               # satisfies e Hᵀ = s

Prange(H, s, w):                                          # Alg. 3
   t <- min(k, max(0, w − ((q−1)/q)(n−k)))
   I <- InfSet(H);  x <-$ {x ∈ F_q^n : |x| = |x_I| = t}
   return PrangeStep(H, s, I, x)

Dec_Z(H_Z, s2):                                           # Alg. 4
   t <- D2                        # distribution over [0,k2]
   I <- InfSet(H_Z);  x <-$ {x ∈ F_3^{n/2} : |x| = |x_I| = t}
   return e2 = PrangeStep(H_Z, s2, I, x)

Dec_XY(H_XY, s1', e2):                                    # Alg. 5
   I <- InfSet(H_XY)
   repeat
      x <-$ {x ∈ F_3^{n/2} : |x| = |x_I| = |(x+e2)_I| = k1}
      e1 <- PrangeStep(H_XY, s1', I, x)
   until |(e1 || e1+e2)| = w                              # exact-weight rejection
   return e1
```

### KeyGen (spec Algorithm 6)
```
 1  params = (n,k,k1,k2,w,l);  k = k1 + k2
 2  seed <-$ {0,1}^l
 3  repeat nonce_x <-$ {0,1}^16 ; H_X = PRNG(seed || nonce_x) ∈ F_3^{(n/2−k1)×n/2}
    until H_X full rank
 4  likewise H_Y (nonce_y, same shape), H_Z (nonce_z, (n/2−k2)×n/2)
 5  repeat
 6     nonce_d <-$ {0,1}^16 ;  D = PRNG(seed || nonce_d) ∈ DP(n)   # monomial
 7     H_sk = [[H_X, H_Y], [−H_Z, H_Z]] · D
 8     split H_sk = (H_1 || H_2), H_1 square (n−k)×(n−k)
    until H_1 invertible
 9  row-reduce (H_1||H_2) to systematic form (I_{n−k} || R)
10  pk = R ;  sk = (seed, nonce_x, nonce_y, nonce_z, nonce_d)
```

### Sign (spec Algorithm 7)
```
1  r <-$ {0,1}^λ
2  s <- H(m, r) ∈ F_3^{n−k}
3  rebuild H_X, H_Y, H_Z, D from (seed, nonces); H_sk = [[H_X,H_Y],[−H_Z,H_Z]]·D;
   take its first n−k columns as H_1
4  (s1 || s2) <- s H_1ᵀ ,  s1 ∈ F_3^{n/2−k1}, s2 ∈ F_3^{n/2−k2}
5  e2 <- Dec_Z(H_Z, s2)
6  e1 <- Dec_XY(H_X + H_Y, s1 − e2 H_Yᵀ, e2)
7  (e' || e) <- (e1 || e1+e2)(Dᵀ)^{-1},  e' ∈ F_3^{n−k}, e ∈ F_3^k
8  σ = (e, r)
```

### Verify (spec Algorithm 8)
```
1  b <- [ |e| + |H(m,r) − e Rᵀ| == w ]      # single equation, exact weight w
2  return b
```
Correctness (spec eq. (1)–(4)): (I_{n−k}||R)(e'||e)ᵀ = H_1^{-1}H_sk D D^{-1}(e1||e1+e2)ᵀ
= H_1^{-1}(s1ᵀ,s2ᵀ)ᵀ = sᵀ, hence e' = H(m,r) − e Rᵀ and |e'|+|e| = w.

### Hash / PRNG (spec §1.5, §1.6)
```
H and PRNG are both instantiated with SM3. PRNG = SM3-DRNG(seed || nonce16);
64-bit output words are reduced mod 3^32 and expanded to 32 F3 elements
(bias ≈ 2^64/3^32 ≈ 9955:1, negligible); elements fill matrices row-major.
D: diagonal entries uniform in {1,2}, permutation by deterministic Fisher–Yates.
```

## Implementation vs specification

Checked, in `Implementations/Reference_Implementation/UVW-{128,256,512}`
(built with `-DUSE_API_PKC`): `SIG_AlgorithmInstance.c` (ICCS API, key/signature
serialization, key generation retry loops), `uvw.c` (`uvw_sign`, `uvw_verify`,
`dec_z`, `dec_xy`, `hash_to_vf3`, the `USE_API_PKC` hash/RNG shims),
`fq_arithmetic/{vf3,mf3,dp}.c`, `gauss.c`. Not executed (UVW-512 keygen alone
exceeds 30 minutes).

Agreements:
- `uvw_param_from_level` (uvw.c) holds exactly the spec's Table 1 tuples:
  L1 (128, 9700, 4850, 3250, 1600, 8633), L2 (256, 19200, 9600, 6433, 3167,
  17088), L3 (512, 39000, 19500, 13066, 6434, 34710). Each instance directory
  calls its own level (1/2/3) — no hard-wired level.
- Seed length follows the spec: `SEED_LEN_BYTES 32` for UVW-128/256 and `64`
  for UVW-512 (`SIG_AlgorithmInstance.h:20`), so sk = 40/40/72 bytes as §1.5.
- KeyGen implements Algorithm 6 faithfully: per-matrix nonce retry loops with a
  full-rank test (`mf3_is_row_full_rank`), the D retry loop keyed on
  invertibility of H_1 (`try_build_keypair` → `row_reduce_to_identity`), and
  the systematic-form correction R ← T·R.
- Sign implements Algorithm 7: s H_1ᵀ via the stored transpose `sk.sti`, then
  `dec_z`, then `dec_xy` with the exact-weight rejection loop against
  `param.w`, then the monomial product and split into (e', e). The retry
  loops in `dec_z` and `dec_xy` are `while (true)` (`uvw.c:152,251,291`);
  `counter` (`uvw.c:150,245`) is incremented but never tested, so the hard
  truncation threshold N of spec §2.3.2 (N = 6000 for Level 1, 12000 for
  Level 2) is not implemented.
- With `-DUSE_API_PKC` all randomness is drawn from the shared seeded DRNG
  (`uvw_drng = &drng_algorithm`, SIG_AlgorithmInstance.c:19; `vf3.c:37`,
  `dp.c:13`, `uvw.c:10` route `csprng_read` to `get_random_number`). Without
  that define the same code calls `getrandom()` (csprng.h) — the KAT build does
  set it.

Discrepancies:
- **(a) CRITICAL — verification result is discarded; every signature is
  accepted.** `sig_verify` computes
  `int result = uvw_verify(pk_obj, &ctx, sn_obj) ? 0 : -1;` and then
  `return 0;` unconditionally:
  `UVW-128/SIG_AlgorithmInstance.c:619,623`, `UVW-256/…:577,581`,
  `UVW-512/…:554,558`. `result` is never read (it does not even produce a
  warning at the project's flags). The spec's Algorithm 8 is the one-line test
  `|e| + |H(m,r) − e Rᵀ| = w`; `uvw_verify` in `uvw.c` *does* compute exactly
  that (`bool result = param.w == ew + ertw;`) and returns it correctly — the
  defect is purely the discarded return in the ICCS API wrapper. Effect through
  the submitted API: EUF-CMA is void (a flipped message bit verifies). The
  harness reports CRYPTOFAIL; with `return result;` the KAT text matches.
- **(a) Public key and signature carry a 12-byte self-describing parameter
  header that the spec does not define.** `serialize_param` writes
  (λ, n, k, k1, k2, w) as six little-endian `uint16` at the head of both pk and
  σ (`SIG_AlgorithmInstance.c:21-41`), and `deserialize_pk`/`deserialize_sn`
  take the working parameters — including the verification weight w — from the
  supplied buffers rather than from compile-time constants. `uvw_verify` does
  `memcmp(&pk.param, &sign.param, …)` first, so a σ-only attack fails, but the
  encoding is non-canonical versus §1.4/§1.5 (pk : R, σ = (e, r)) and it is the
  reason the sizes below do not match the spec's.
- **(a/size) Every pk and σ is ~26.5 % larger than the spec's Table 1.** The
  code stores F3 vectors bit-sliced as two 64-bit-word planes (r0, r1), i.e.
  2 bits per trit with per-row 64-bit padding:
  `pk = 12 + (n−k)·(1+⌈(k−1)/64⌉)·8·2`, `σ = 12 + λ/8 + (1+⌈(k−1)/64⌉)·8·2`
  (`pk_serialized_size`, `sn_serialized_size`, SIG_AlgorithmInstance.c:43-53).
  Table 1 quotes the entropy bound k·(n−k)·log₂3 bits. Both statements can be
  true, but the submitted artifact is 5.90 MB / 23.04 MB / 95.16 MB, not
  4.44 / 17.41 / 71.85 MiB, and σ is 1244 / 2444 / 4956 B, not 977 / 1934 /
  3928 B. No 5-trits-per-byte packing (as used inside `hash_to_vf3`) is applied
  to the key or signature.
- **(a, minor) `sig_verify` ignores `pk_len_bytes` and `sn_len_bytes`** and
  `deserialize_*` reads a fixed amount based on the header it just parsed, so a
  short buffer is over-read. `sig_sign` does check `sk_len_bytes`.
- **(a, performance/hygiene) Debug instrumentation left in the signing path.**
  `uvw_sign` (uvw.c) prints `DBG_SIGN:` lines to stderr on every signature and
  `uvw_verify` prints `DBG_VFY:` on every verification, leaking internal
  Hamming weights. Worse, the diagnostic block runs an O(n²) duplicate scan
  over the monomial permutation (`for i … for j>i … if (sk.d->p[i] == sk.d->p[j])`),
  i.e. ≈4.7·10⁷ iterations per signature at L1 and ≈7.6·10⁸ at L3, purely to
  print `p_dups`.
- **(c, equivalent) transcript buffering:** with `-DUSE_API_PKC`,
  `uvw_hash_update` accumulates the whole message in a `realloc`'d buffer and
  `uvw_hash_read` materialises at least 64 KiB of `pseudoXOF` output before the
  first byte is consumed (uvw.c:58-79). Output-equivalent to a streaming XOF,
  but memory-hungry.

Not verified: the D2 distribution of Algorithm 4 (the spec does not pin it
down; `dec_z` in uvw.c draws `t = csprng_get_size_t() % (h_z->n_col −
h_z->n_row + 1)`, i.e. uniform on [0, k2] — this appears to match the spec's
"distribution D2 over [0,k2]" but §2.3's distribution-matching analysis was not
re-derived, and it is the step on which the "signature indistinguishable from a
random weight-w vector" claim rests). The (Dᵀ)^{-1} versus D convention in step
7 was not re-derived. §3 security analysis not audited.
