# sign-31 TSUOV — algorithm summary

TSUOV ("Tensor-Structured UOV") is a hash-and-sign multivariate signature in the
Unbalanced Oil-and-Vinegar family; security rests on the MQ problem plus the
Oil–Vinegar (key-recovery) problem. It unifies QR-UOV (block structure over
F_{q^ℓ} via a symmetric Φ with irreducible characteristic polynomial), MAYO
(whipping, here written as a tensor with matrices E^{sa}) and SNOVA: the oil
space is enlarged from dimension o to oℓk by two tensor products, and the
number of equations used for key generation (m1) is decoupled from the number
used for signing (m2). Petzoldt-style compression puts everything but P̄_{i,3}
behind seeds.

Specification: `sign-31-spec.pdf` (41 pages), §4 "Algorithm Specification"
(§4.1 description, §4.2 key/signature representation, §4.3 PRG/Hash,
Algorithms 1–3, §4.4 Table 1).

## Parameters

| parameter | TSUOV 128 | TSUOV 256 | TSUOV 512 | meaning |
|---|---|---|---|---|
| λ | 128 | 256 | 512 | ICCS security level (bits); seed/salt = λ bits |
| q | 31 | 31 | 31 | prime field order, ⌈log2 q⌉ = 5 |
| ℓ | 2 | 2 | 2 | extension degree, Φ = [[1,8],[8,−1]], f(x)=x²−3 |
| n | 64 | 119 | 230 | variables (v = n − o) |
| o | 4 | 5 | 12 | oil variables per block |
| m1 | 61 | 114 | 218 | equations used in KeyGen |
| m2 | 88 | 160 | 240 | equations used in Sign/Verify |
| k | 11 | 16 | 10 | tensor/whipping factor (oil dim = oℓk) |
| g(z) | z⁸⁸−z²−3z−4 | z¹⁶⁰−z²−4z−6 | z²⁴⁰−z²−12z−15 | irreducible, defines E^{sa} |

Sizes (bytes), specification Table 1 vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | sig spec | sig impl | match |
|---|---|---|---|---|---|---|---|
| TSUOV_128 | 778 | 779 | 32 | 32 | 896 | 896 | pk **+1** |
| TSUOV_256 | 2170 | 2170 | 64 | 64 | 2412 | 2412 | yes |
| TSUOV_512 | 21319 | 21319 | 128 | 128 | 2939 | 2939 | yes |

§4.2 formulas: pk = ⌈log q⌉·m1·ℓ·o(o+1)/2 + λ bits, sk = 2λ bits,
sig = ⌈log q⌉·nℓk + λ bits.

## Pseudocode

### Setup (§4.1)
```
Φ ∈ Mat_{ℓ×ℓ}(F_q) symmetric with irreducible char. poly; F_q[Φ] ≅ F_{q^ℓ}, image λ
v = n − o;  E^{sa} ∈ Mat_{k×k}(F_q) (s∈[m2], a∈[m1]) from z^{j+i(k-1)} mod g(z)
F*_s = Σ_a E^{sa} ⊗ F_a ,  P*_s = Σ_a E^{sa} ⊗ P_a ,  P*_s = (I_k⊗S^T) F*_s (I_k⊗S)
```

### KeyGen (spec Algorithm 1, CompactKeyGen)
```
1  seed_sk <-$ {0,1}^λ
2  S' := PRG(seed_sk) ∈ F_{q^ℓ}^{v×o}          # S = [[I_v, S'],[0, I_o]]
3  seed_pk <-$ {0,1}^λ
4  for i = 0 .. m1-1:
5      P_{i,1} := PRG(seed_pk || 2i)            # symmetric, v×v over F_{q^ℓ}
6      P_{i,2} := PRG(seed_pk || 2i+1)          # v×o
7      P_{i,3} := (−S'^T P_{i,1} + P_{i,2}^T) S' + S'^T P_{i,2}    # o×o, symmetric
8  cpk = seed_pk || Pack_{⌈log q⌉}({P_{i,3}})   # upper triangle only
9  csk = seed_sk || seed_pk
```

### Sign (spec Algorithm 2)
```
 1  µ = Expand_µ(seed_pk || M)                       # pseudohash(512, ·)
 2  S' = PRG(seed_sk)
 3  seed_v <-$ {0,1}^λ ;  v_0||…||v_{k-1} = PRG(seed_v),  v_i ∈ F_{q^ℓ}^v
 4  for r = 0..m1-1:
 5      P_{r,1}, P_{r,2} := PRG(seed_pk||2r), PRG(seed_pk||2r+1)
 6      for i = 0..k-1:
 7          M_i[r,:] = π_ℓ(−v_i^T P_{r,1} S' + v_i^T P_{r,2})       # M_i ∈ F_q^{m1×ℓo}
 8          for j = i..k-1: U_ij[r] = π_1(v_i^T P_{r,1} v_j)        # U_ij ∈ F_q^{m1}
 9  ctr = 0; u = 0^{m2}; A_i = 0^{m2×ℓo}
10  for i = 0..k-1: for j = k-1 down to i:
11      u += E^{ctr} U_ij ;  A_i += E^{ctr} M_j ;  A_j += E^{ctr} M_i ;  ctr++
12  L := [A_0, …, A_{k-1}] ∈ F_q^{m2 × kℓo}
13  loop:
14      salt <-$ {0,1}^λ
15      t = Hash(µ || salt) ∈ F_q^{m2}
16      if L x = t − u is solvable:
17          solve for x ∈ F_q^{kℓo}
18          o_i = π_ℓ^{-1}(x[iℓo : (i+1)ℓo]) ;  s_i = (v_i − S' o_i , o_i)
19          return sig = salt || Pack_{⌈log q⌉}(s_0,…,s_{k-1})
```

### Verify (spec Algorithm 3)
```
1  parse sig = salt || s ;  parse cpk = seed_pk || {P_{i,3}}
2  µ = Expand_µ(seed_pk || M) ;  t = Hash(µ || salt) ∈ F_q^{m2}
3  for r = 1..m1:
4      P_{r,1}, P_{r,2} := PRG(seed_pk||2r), PRG(seed_pk||2r+1)   # P_r = [[P_{r,1},P_{r,2}],[P_{r,2}^T,P_{r,3}]]
5      for i = 0..k-1:  c = π_ℓ(s_i^T P_r)
6          for j = i..k-1:  U_ij[r] = c · π_ℓ(s_j)
7  y = 0 ; ctr = 0
8  for i = 0..k-1: for j = k-1 down to i:  y += E^{ctr} π_1(U_ij) ; ctr++
9  return (y == t)
```

### Hashes / PRG (spec §4.3)
```
PRG(seed||t)   = RejSamp(L, τ_L, pseudoXOF(τ, seed || be16(t)))        # → F_q elements
Expand_µ(x)    = pseudohash(512, seed_pk || M) ∈ {0,1}^512
Hash(µ||salt)  = RejSamp(m2, τ_{m2}, pseudoXOF(τ_{m2}, µ || salt)) ∈ F_q^{m2}
π_ℓ(x̄) = Φ_x̄[0,:] ,  π_1(x̄) = Φ_x̄[0,0]
```
(the PRG's underlying primitive in the submitted code is SM3-based MGF:
`mgf.c:12 sm3hash(256, …)`, with `pseudoXOF`/`pseudohash` from the ICCS
`auxfunc`.)

## Implementation vs specification

Checked, in
`Implementations/Digital_Signature-TSUOV-x86-Reference_Implementation/API_PKC/Implementations/Reference_Implementation/TSUOV_128`
(the 256/512 directories differ only in `TSUOV_VARIANT` and the rejection-
sampling lengths): `tsuov_params.h` (constants), `SIG_AlgorithmInstance.c`
(ICCS API), `tsuov_core.c` (`TSUOV_KeyGen`, `TSUOV_Sign`, `TSUOV_Verify`, the
E-multiplication tables, `Expand_µ`, `Hash`), `Fql.h` (bit packing), `mgf.c`.
The scheme was not executed.

Agreements:
- `tsuov_params.h` matches spec Table 1 for all three sets: q=31, ℓ=2, k=11/16/10,
  m1=61/114/218, m2=88/160/240, O=4/5/12 with V=60/114/218 so n=V+O=64/119/230,
  seed/salt length λ/8 = 16/32/64. `TSUOV_ceil_log_2_q 5`. The `F_COEF`
  comments give g(z) = z^{m2}+30z²+28z+27 etc., i.e. exactly the spec's
  z⁸⁸−z²−3z−4 / z¹⁶⁰−z²−4z−6 / z²⁴⁰−z²−12z−15 over F31.
- `TSUOV_Verify` really computes the spec's equation and returns the comparison
  (`verify &= (acc[i] == msg[i])`, tsuov_core.c ~:1040), and
  `SIG_AlgorithmInstance.c:100 sig_verify` propagates it (`return 0` only when
  `TSUOV_Verify` is true). No inverted or omitted check.
- All randomness comes from the shared seeded DRNG: `sig_random_seed` calls
  `get_random_number(&drng_algorithm, …)` (SIG_AlgorithmInstance.c:19) for
  seed_sk, seed_pk (KeyGen) and seed_v, seed_r, seed_sol (Sign). No OS-entropy
  path is compiled in.
- Public key layout is `seed_pk || upper-triangle P3` and secret key is
  `seed_sk || seed_pk` exactly as §4.2; `CRYPTO_PUBLICKEYBYTES` /
  `CRYPTO_BYTES` in `SIG_AlgorithmInstance.h:25-26` are the spec's formulas.

Discrepancies:
- **(b, spec arithmetic) TSUOV 128 public key: spec says 778 B, library reports
  779 B.** ⌈log q⌉·m1·ℓ·o(o+1)/2 = 5·61·2·10 = 6100 bits = 762.5 B, which the
  code rounds up per key (`CEIL_DIV(L·m1·O·(O+1)·5, 16) + SEED_LEN` = 763+16 =
  779) while Table 1 truncated to 762+16 = 778. The 256 and 512 rows were
  rounded up in the table, so Table 1 is internally inconsistent; the
  implementation is the self-consistent one. Not a code defect.
- **(a) Non-canonical signature/public-key encodings are accepted.**
  `Fql.h:91 restore_Fq` returns a raw 5-bit value in [0,31], but F31 elements
  are 0..30; nothing rejects the value 31. The E-multiplication tables
  (`tsuov_core.c:182-226`) are 32 entries wide with `T1[31]` holding the value
  for 31 ≡ 0 (mod 31), so there is no out-of-bounds read, but a coefficient
  encoded as 31 behaves as 0. Consequence: for every valid signature there are
  other distinct byte strings that also verify (strong-unforgeability /
  malleability), and public keys have multiple encodings. The spec's
  `Pack_{⌈log q⌉}` (§4.3) does not state a range check either, so this is at
  best a spec gap the implementation inherits.
- **(a, minor) `sig_verify` ignores `sn_len_bytes` and `pk_len_bytes`**
  (SIG_AlgorithmInstance.c:104-105 cast them to void), so a truncated signature
  is read past its end rather than rejected. TRINE (sign-30) does check this.
- **(b/c) Candidate salts come from one seed.** Algorithm 2 line 26 samples a
  fresh `salt` in every loop iteration. `TSUOV_Sign` (`tsuov_core.c:978-985`)
  instead takes successive `TSUOV_SALT_LEN`-byte blocks of the `mgf.c` stream
  keyed by the single DRNG-drawn `seed_r` (`PRG2_init(seed_r)`, `PRG2_yield`),
  so all candidate salts of one signature are determined by `seed_r`.
- **(minor, not a spec issue) `static TSUOV_P3 P3`** in `sig_keygen` and
  `sig_verify` — a multi-megabyte shared mutable buffer in the library; the
  API is not re-entrant.
- KAT packaging (already recorded in RESULTS.md): the top-level
  `Test_Vectors/KAT_SIG_TSUOV-<n>.txt` files are in a private format the ICCS
  driver cannot produce; the in-tree `API_PKC/Test_Vector/KAT_SIG_TSUOV_<n>.txt`
  vectors are the ones that reproduce.

Not verified: the E-matrix construction from g(z) was checked only against the
source comments, not recomputed; the tensor identity P*_s = (I_k⊗S^T)F*_s(I_k⊗S)
and the linearisation bookkeeping in `TSUOV_Sign` (the `ctr1/ctr2` walks) were
not re-derived. §6 security analysis was not audited.
