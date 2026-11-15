# sign-18 Origami — algorithm summary

Origami is a multivariate-quadratic (MQ) signature in the Oil-and-Vinegar family, but
with the global oil space replaced by *d+1 local zones*, each with its own hidden local
matrix algebra `K_{j,i} = F_q[T_{j,i}]`, a private oil-coordinate partition and a private
sampled/solved split. Signing is sequential linear solving zone by zone; verification is a
plain hash-and-evaluate MQ check `P_pub(σ) = t` with no oil structure visible. Security
rests on hidden-structure indistinguishability plus multi-target MQ inversion (spec §4.1).
The public key is *compressed*: a 4-byte parameter id, a public expansion seed, and an
explicit residual coefficient vector; the secret key is *only* the master seed.

Specification: `sign-18-spec.pdf` (56 pages), §2 (protocol), Algorithms 1–5, §3.2 (sizes).

## Parameters

| parameter | Origami-128 | Origami-256 | Origami-384 | Origami-512 | meaning |
|---|---|---|---|---|---|
| q | 16 | 16 | 16 | 16 | base field F_q, 4 bits/element |
| d | 3 | 6 | 8 | 9 | main local zones (+1 tail zone) |
| k | 2 | 5 | 7 | 8 | local copies per main zone |
| N | 200 | 968 | 1800 | 2312 | public input dim, `N = 32dk + 8` |
| M | 104 | 488 | 904 | 1160 | public output dim, `M = 16dk + 8` |
| l_j | 2 | 2 | 2 | 2 | hidden local algebra matrix dim |
| µ_red | 49 | 81 | 114 | 146 | reduction parameter (spec Table 2) |
| κ_sk = κ_pk = λ_salt | 128 | 256 | 384 | 512 | bits (16/32/48/64 B) |
| \|I_res\| | 5952 | 29864 | 55776 | 71712 | residual coefficient positions |
| claimed security | 128 / 64 | 256 / 128 | 384 / 192 | 512 / 256 | classical / quantum bits |

Per-zone parameters for Origami-128 (spec §2.2 notation, impl `origami_params.h`):
main zones `(k,v,o,m,l,a,b,δ) = (2,3,5,8,2,8,32,0)`, tail `(1,0,2,2,2,0,8,0)`; both satisfy
the spec's constraints `a_j + b_j = k_j o_j l_j²` and `b_j ≥ M_j + δ_j` (eq. 6).

Sizes (bytes), specification (§3.2 Tables 3–4) vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | sig spec | sig impl | match |
|---|---|---|---|---|---|---|---|
| Origami-128 | 2996 | 2996 | 16 | 16 | 116 | 116 | yes |
| Origami-256 | 14968 | 14968 | 32 | 32 | 516 | 516 | yes |
| Origami-384 | 27940 | 27940 | 48 | 48 | 948 | 948 | yes |
| Origami-512 | 35924 | 35924 | 64 | 64 | 1220 | 1220 | yes |

`|pk| = 4 + κ_pk/8 + |I_res|/2`, `|sk| = κ_sk/8`, `|sig| = ⌈N/2⌉ + λ_salt/8`. The very small
secret key (16 B at Origami-128) is the whole story of the design: by spec eq. (52),
`sk = seed_sk` and nothing else — the hidden local algebras, the oil-coordinate
permutations, the sampled/solved split and the central map are all *re-derived* from that
seed on every signature (spec §2.6.2). All public data the verifier needs is the MQ map
`P_pub`, which is itself compressed to `seed_pk` plus the residual vector `R_pk`.

## Pseudocode

### KeyGen — spec Algorithm 3 (§2.6.1)
```
seed_sk  <-$ {0,1}^κ_sk
seed_pk    <- Expand(seed_sk, "pk-seed"    ‖ param_id ‖ λ_sec ‖ κ_pk/8, κ_pk/8)   (55)
seed_coeff <- Expand(seed_sk, "coeff-seed" ‖ param_id ‖ λ_sec ‖ κ_sk/8, κ_sk/8)   (58)
for j = 1..d+1:
    for i = 1..k_j: (S,C,T,K)_{j,i} <- SampleHiddenLocalAlgebra(seed_sk, j, i, l_j)   [Alg 1]
    ρ_j : Z_j -> (U_j, W_j)  via  π_j^oil = Permute(seed_sk, "oil-partition"‖param_id‖j, k_j o_j l_j²)
for j = 1..d+1:                       # central map, affine in the solved part
    generate D_j, A_j  with  F^(j)(Y^(≤j)) = D_j(X_j) + A_j(X_j)·W_j,  no quadratic W_j term
I_all  <- ordered public coefficient-position universe;  I_elig ⊆ I_all (affine positions)
I_res  <- SelectRes(params, seed_pk) ⊆ I_elig ;  I_seed <- I_all \ I_res
R_pk   <- ExpandToField(seed_coeff, "Rpk-coeff" ‖ param_id ‖ λ_sec ‖ |I_res|, |I_res|)   (59)
P_pub  <- P_seed(seed_pk on I_seed) + Embed_{I_res}(R_pk)
return pk = LE32(param_id) ‖ seed_pk ‖ EncodeVec16(R_pk)  (51),   sk = seed_sk  (52)
```
Algorithm 1 (`SampleHiddenLocalAlgebra`): rejection-sample `S_{j,i}` from
`Expand(seed_sk,"embed-S"‖j‖i‖r_S)` until its characteristic polynomial is irreducible,
then `C_{j,i}` from `"embed-C"` until invertible; `T = C⁻¹SC`, `K = F_q[T]`.

### Sign — spec Algorithm 4 (§2.6.2)
```
parse sk = seed_sk; re-derive seed_pk, Π_pub = Permute(seed_pk,"secret-to-public",N),
       seed_coeff, I_res, R_coeff, and all hidden local data from seed_sk     (pk is NOT an input)
salt <-$ {0,1}^λ_salt ;  h_µ <- H_msg(µ) ∈ B^64
t    <- HashToFieldVector("target" ‖ h_µ ‖ salt, M, F_q) ;  split t = (u^(1),...,u^(d+1))
Y^(<1) <- ∅
for j = 1..d+1:
    repeat
        V_i^(j) <-$ K_{j,i}^{v_j}  (i = 1..k_j);   U_j <-$ F_q^{a_j}
        X_j <- (Y^(<j), V_1^(j),...,V_{k_j}^(j), U_j)
        W_j <- SolveLocalZone(D_j, A_j, X_j, u^(j))          [Alg 2]
    until W_j != ⊥
    Z_j <- ρ_j^{-1}(U_j, W_j);  assemble Y^(j);  Y^(<j+1) <- (Y^(<j), Y^(j))
y <- Flat(Y) ∈ F_q^N ;  σ <- Π_pub(y)
return sig = EncodeVec16(σ) ‖ salt
```
Algorithm 2 (`SolveLocalZone`): `d_j = D_j(X_j)`, `B_j = A_j(X_j)`, `rhs = u^(j) − d_j`;
if `rank(B_j) ≠ M_j` return ⊥, else take a particular solution `W_j⁰` of `B_j W = rhs`
and a random kernel element `z_j`, return `W_j = W_j⁰ + z_j`.

### Verify — spec Algorithm 5 (§2.6.3)
```
parse pk = (params, seed_pk, R_pk);  derive N, M, I_all, I_elig
I_res <- SelectRes(params, seed_pk);  I_seed <- I_all \ I_res
P_seed <- seed-defined coefficients from seed_pk on I_seed
Π_pub  <- Permute(seed_pk, "secret-to-public", N)
P_pub  <- P_seed + Embed_{I_res}(R_pk)          # in the coordinate order induced by Π_pub
h_µ <- H_msg(µ);  t <- HashToFieldVector("target" ‖ h_µ ‖ salt, M, F_q)
return [ P_pub(σ) == t ]
```

### Hashing (spec §2.4)
```
Expand(seed, ctx, L) = H(seed ‖ ctx ‖ enc(L))[1..8L]      (25)
H_λ(ctx, X)          = H(ctx ‖ enc(λ) ‖ X)[1..λ]          (24)
H = extendable-output function; spec §2.4.1: "H may be instantiated by SHAKE256".
```

## Implementation vs specification

- **Checked.** `src/<inst>/origami_ref.c` (867 lines) implements all of KeyGen
  (`genkeys`, line 699), Sign (`sign`, 753), Verify (`verify`, 850), Alg 1
  (`gen_hidden_algebra`, 340), Alg 2 (`solve_rect_random`, 611, Gaussian elimination with
  free variables filled from a PRF stream), `SelectRes` (rank/stride schedule, ~line 180–216),
  `Π_pub` (`derive_public_permutation`, 438). `SIG_AlgorithmInstance.c` is the NGCC ABI
  shim; it draws `seed_sk` and `salt` from the shared DRNG.
- **Parameter spot-check (sampling: `ORIGAMI_q`, `MU_RED`, `MAIN_ZONES`=d, `MAIN_K`=k,
  `SEED_SK/PK/SALT_BYTES`, `RPK_BYTES` on all four `origami_params.h`).** All agree with
  spec Tables 2–3: e.g. Origami-384 `d=8, k=7, µ_red=114, seeds 48 B, RPK 27888`. Derived
  `ORIGAMI_N/M` (`origami.h:51-52`) reproduce `N = 32dk+8`, `M = 16dk+8` exactly, and
  `NUMGF_RPK = 2·RPK_BYTES` equals the spec's `|I_res|` for every instance. **No size
  mismatch anywhere:** all 12 spec/impl size cells above match.
- **Deviation (a), hash primitive.** The spec names SHAKE256; the build uses the ICCS
  competition primitives. `symmetric_iccs.c` defines `shake256_init/absorb/squeeze` over
  `pseudoXOF()` (`auxfunc.h:44`); `shake_absorb` (`symmetric_iccs.c:42-51`) buffers the
  input in a fixed 208-byte area and discards anything beyond it. The message
  digest is `pseudohash(512 bits, …)` (`SIG_AlgorithmInstance.c:51,80`), not SHAKE. The
  64-byte digest length matches the spec's `h_µ ∈ B^64`; the function does not. The spec's
  "may be instantiated by" wording makes this arguably permitted, but the SHAKE-named
  symbols are misleading.
- **Deviation (b/c), signing randomness.** Spec Alg 4 lines 16–18 sample the vinegar
  variables and `U_j` uniformly at random, and Alg 2 line 8 samples a random kernel
  element. The implementation derives all of these deterministically from `seed_sk`:
  `sample_zone_known` (line 530) uses label `"sign-known" ‖ zone ‖ attempt ‖ digest ‖ salt`,
  and `solve_rect_random` uses `"kernel-sample" ‖ …` (line 678). Randomization comes only
  from the fresh `salt`. For Origami-512 the two streams absorb 218 and 221 bytes
  (seed ‖ label ‖ three u32 ‖ block counter ‖ digest ‖ salt), more than the shim's 208, so
  the last 10 and 13 salt bytes do not enter them; the target hash absorbs the whole salt,
  and Origami-128/256/384 stay below the limit (sign-18-7). This is a reasonable hedged/derandomized construction, but it is
  not what the spec text says, and the spec does not describe it.
- **Deviation (b), biased sampling.** `origami_ref.c:554` forces the first sampled oil
  coordinate to be non-zero (`if (i == 0 && u == 0) u = 1;`). `U_j` is therefore not
  uniform on `F_q^{a_j}`; the spec (eq. 61) says uniform. Unexplained in the spec.
- **Restriction not in the spec.** `validate_params` (`origami_ref.c:108`) rejects any
  `l_j != 2`, and `MAX_SIGN_ATTEMPTS = 8192` (line 12) caps the spec's unbounded
  `repeat … until W_j != ⊥` loop; the failure behaviour of that cap is not specified.
- **Transcript binding.** `hash_to_field` absorbs only `"target" ‖ digest ‖ salt`
  (line 515) — matching spec Alg 4 line 10, i.e. the public key is *not* bound into the
  target. Consistent with spec, but it leaves the multi-target/multi-user question open;
  see `security_findings.md` (`fiat-shamir-transcript-binding`, `not_tested`).
- **KAT coverage (known, not re-investigated).** The submitted
  `Implementations and Test_Vectors/Test_Vectors/KAT_SIG_Origami-*.txt` files contain a
  **single record** (`Count = 0`) each, so the uniform harness is run with `--count 1`
  (see `sign-18/Makefile`). All four instances PASS, but one record per parameter set is
  unusually weak conformance evidence.
- **Not verified.** The exact construction of `D_j`/`A_j` (spec Alg 3 line 11 is
  descriptive only — the spec never gives the concrete coefficient generation for the
  central map), the `SelectRes` rank/stride schedule against the spec's abstract
  `SelectRes`, and the correspondence between the impl's coefficient schedule and the spec's
  `I_elig` definition. These are genuine specification gaps: §2.3/§2.6.1 state the
  *constraints* on the central map rather than an algorithm to generate it.
