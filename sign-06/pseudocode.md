# sign-06 COMPASS-SIG — algorithm summary

Module-lattice Fiat–Shamir-with-aborts signature (Dilithium/ML-DSA lineage), part
of the "COMpact Post-quAntum Security Suite". Key recovery reduces to MLWE over
`R_q = Z_q[X]/(X^n+1)`; forgery to MSIS and to a variant `SelfTargetMSIS^R`
(§3.1, Defs. 1–3) in which the ML-DSA `t` is replaced by `Decomp_d(Comp_d(t))`.
Two deliberate departures from ML-DSA: **no hint vector** (the public key carries
t1 uncompressed enough that the verifier needs no hints, shrinking the signature
and halving the MISIS norm bound), and an **extra Euclidean lower-bound check**
`‖δz‖² + ‖w0'‖² ≥ B·γ2²` that the designers call Shell-MISIS / Shell-CVP, forcing
a forger to find a vector in a shell between a cube (ℓ∞) and a ball (ℓ2).

Specification: `sign-06-spec.pdf` (31 pages), English, §2.3 (Algorithms 1–3);
parameters in §3.3 Table 1, sizes in Table 2.

## Parameters

| parameter | 128 | 256 | 384 | 512 | meaning |
|---|---|---|---|---|---|
| q | 2081281 | 2081281 | 8380417 | 8380417 | modulus |
| n | 256 | 256 | 512 | 512 | ring degree (X^n+1) |
| (k, l) | (3, 4) | (7, 7) | (5, 6) | (7, 7) | MLWE samples / secret dim |
| η_s = η_e | 1 | 1 | 1 | 1 | secret/error ℓ∞ bound |
| d | 4 | 5 | 5 | **5 (spec) / 6 (impl)** | pk compression bits |
| γ1 | 2^15 | 2^17 | 2^18 | 2^19 | mask y ℓ∞ bound |
| γ2 | 130080 | 520320 | 1047552 | 2095104 | split bound = (q−1)/16, /4, /8, /4 |
| δ | 4 | 4 | 4 | 4 | rescale ⌊γ2/γ1⌉ |
| τ | 30 | 60 | 78 | 120 | non-zero entries of c' |
| B | 560 | 1120 | 1760 | 2240 | Euclidean (shell) bound |
| c̃ bytes | 32 | 48 | 48 | 64 | challenge-hash length (impl `CTILDEBYTES`) |
| claimed security | 128 | 256 | 384 | 512 | bits classical (80/128/192/256 quantum) |

Sizes (bytes), specification (Table 2) vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | sig spec | sig impl | match |
|---|---|---|---|---|---|---|---|
| COMPASS-SIG-128 | 1664 | 1664 | 960 | 960 | 2080 | 2080 | yes |
| COMPASS-SIG-256 | 3616 | 3616 | 2144 | 2144 | 4080 | 4080 | yes |
| COMPASS-SIG-384 | 5792 | 5792 | 3136 | 3136 | 7344 | 7344 | yes |
| COMPASS-SIG-512 | 7648 | 7648 | 4608 | 4608 | 9024 | 9024 | yes |

All eight size claims reproduce exactly. Note that the 512 public-key size 7648
is only reachable with d = 6 (32 + 7·512·17/8), not with the d = 5 printed in
Table 1 — Table 1 and Table 2 of the spec contradict each other and the
implementation follows Table 2.

## Pseudocode

### KeyGen (Algorithm 1, §2.3.1)
```
Input: seed <- {0,1}^n
1. (rho, sigma) := G(seed)                       # pk.seed, sk.seed
2. A_hat := ExpandA(rho)          in T_q^{k x l} # NTT domain
3. (s, e) := Sample(S_{eta_s}^l x S_{eta_e}^k, sigma)
4. t := INTT(A_hat o NTT(s)) + e   mod q
5. t1 := Comp_d(t);  t0 := t - Decomp_d(t1)
6. pk := (rho, t1);  sk := (seed, s, e, t0)
```
with (§2.2.4) `Comp_d(x) = ⌊(x−1)/2^d⌋` for x>0 else `⌊x/2^d⌋`, and
`Decomp_d(x) = x·2^d + 2^{d−1}`.

### Sign (Algorithm 2, §2.3.2)
```
Input: pk=(rho,t1), sk=(seed,s,e,t0), message m, random seed rho_r
1. A_hat := ExpandA(rho)
2. mu := H(pk || m)
3. (y, rho_r) := Sample(S~_{gamma1}^l x {0,1}^n, rho_r)   # rho_r is updated
4. w  := INTT(A_hat o NTT(y))      in R_q^k               # only NTT use
5. (w1, w0) := Split(w, gamma2)
6. c := H(mu || w1);  c' := SampleInBall_tau(c)
7. z := y + c'·s;  (u1, u0) := Split(w - c'·e, gamma2)
8. if ‖z‖_inf >= gamma1 - eta_s·tau  or  ‖u0‖_inf >= gamma2 - eta_e·tau: goto 3
11. (w1', w0') := Split(w - c'·e + c'·t0, gamma2)
12. if w1' != u1: goto 3                                  # replaces ML-DSA hints
15. if ‖delta·z‖^2 + ‖w0'‖^2 < B·gamma2^2: goto 3         # shell / Shell-MISIS
Output: sigma = (z, c)
```

### Verify (Algorithm 3, §2.3.3)
```
Input: pk=(rho,t1), m, sigma=(z,c)
1. A_hat := ExpandA(rho)
2. mu := H(pk || m)
3. c' := SampleInBall_tau(c)
   (w1', w0') := Split( INTT(A_hat o NTT(z)) - c'·Decomp_d(t1), gamma2 )
4. if ‖z‖_inf < gamma1 - eta_s·tau
      and ‖delta·z‖^2 + ‖w0'‖^2 >= B·gamma2^2
      and H(mu || w1') = c:            output Pass
   else                                output Fail
```

### Hashing and compression (§2.2.3–2.2.4)
```
SampleInBall_tau(rho):                    # Fig. 1, inside-out Fisher-Yates
  c := 0^n
  for i := n-tau to n-1:
    j <- {0,...,i} ;  s <- {0,1}          # randomness from XOF(rho)
    c_i := c_j ;  c_j := (-1)^s
  return c                                # tau entries of +-1

Split(x, gamma): p := ⌈q/(2·gamma+1)⌉
                 x1 := ⌊(p/q)·x⌉ mod+ p ;  x0 := x - x1·2·gamma mod± q
ExpandA(rho) -> A_hat in T_q^{k x l} sampled uniformly directly in NTT domain
                (shared convention with COMPASS-KEM)
```
Signature encoding (implementation, §"packing"): `sig = c̃ (CTILDEBYTES) ||
z[0..l-1]`, each z polynomial packed as `γ1 − z_i` in exactly `Z_BITS` bits
(16/18/19/20 for the four levels, i.e. `Z_BITS = log2(2γ1)`), byte-aligned per
group with **no slack bits and no hint vector** — the encoding is bijective on
S̃_γ1 and admits no trailing-byte freedom.

## Implementation vs specification

Checked against `Implementations/Reference_Implementation/COMPASS-SIG-{128,256,384,512}/`
(the four dirs the Makefile builds with `-DCOMPASS_SIG_MODE=N`). All C sources
and headers, including `params.h`/`rounding.h`, are byte-identical across the four
instances (md5-verified), so the review below covers all four. Files: `sign.c` (KeyGen /
Sign / Verify), `rounding.c` (Split, power2round), `poly.c` (SampleInBall,
uniform_gamma1, packing), `polyvec.c` (`polyvec_check_L2_bound`), `packing.c`,
`params.h`. `review-o48.md` does not exist for sign-06; `security_findings.md`
reports KAT PASS 4/4 and no reproduced break.

**Agreements.** `params.h` reproduces Table 1 exactly for q, n, k, l, η, γ1, τ, B
at all four levels, and γ2 as `(Q-1)/16, /4, /8, /4` = 130080 / 520320 / 1047552
/ 2095104 (sampled all four levels for q, n, k, l, γ2, τ, B). The three rejection
conditions of Algorithm 2 are all present in `sign.c` in order, including the
hint-free `w1' != u1` re-derivation. `polyvec_check_L2_bound` (`polyvec.c`)
computes `‖z‖² + ⌈‖w0‖²/16⌉ < B·(γ2/4)²`, which is exactly the spec's
`‖δz‖² + ‖w0'‖² < B·γ2²` with δ = 4 hardcoded, and verify applies the reversed
inequality. `y` is drawn by unpacking the XOF stream with `polyz_unpack`, whose
range is precisely S̃_γ1 = (−γ1, γ1]. No hint vector exists anywhere, as claimed.

**Discrepancies.**

1. *(spec-internal contradiction, impl follows the sizes)* Table 1 gives d = 5 for
   COMPASS-SIG-512; `COMPASS-SIG-512/params.h:99` sets `#define D 6`. Only d = 6
   yields Table 2's pk = 7648 and sk = 4608, which the built library reports.

2. **(a) — real deviation, affects levels 384 and 512.** `poly_challenge`
   (`COMPASS-SIG-512/poly.c:872,881-883,899-900`) loads only **64** sign bits into
   a single `uint64_t signs` and shifts it right once per non-zero coefficient.
   For τ = 78 (level 384) and τ = 120 (level 512), `signs` is exhausted after the
   64th coefficient, so the last τ−64 (14, resp. 56) non-zero challenge
   coefficients are deterministically `+1`. Spec Fig. 1 step 04 requires a fresh
   `s ← {0,1}` for every one of the τ positions. The position index
   `b = buf[pos++]` (`poly.c:895`) is also a single byte, so for n = 512 the
   test `b > i` never rejects and positions 256 … n−τ−1 are never non-zero.
   sign-06-5 bounds the resulting challenge image by 2^335.6 (level 384) and
   2^454.9 (level 512). Levels 128 (τ=30) and 256 (τ=60) are unaffected.

3. **(a) — real deviation from §2.3.1's stated anti-kleptography property.**
   Spec Algorithm 1 line 8 sets `sk := (seed, s, e, t0)` and the surrounding text
   explicitly justifies storing the *master seed* "so that the private key holder
   can verify that s, e are correct … avoid certain kleptographic attacks".
   `pack_sk` (`COMPASS-SIG-128/packing.c:101`) instead stores
   `rho || key || tr || s || e || t0` (ML-DSA layout); the master seed is never
   retained, so the claimed self-check is not implementable from sk.

4. **(c) — equivalent redefinition.** `power2round` (`rounding.c:17`) uses
   `t1 = (t + 2^{d−1} − 1) >> d`, `t0 = t − t1·2^d`, and verify reconstructs with
   `polyveck_shiftl` = `t1·2^d` (`sign.c`, `polyvec.c:287`) — i.e. **without** the
   `+2^{d−1}` of the spec's `Decomp_d`. Signer and verifier agree and the bound
   |t0| ≤ 2^{d−1} is the same, so this is an equivalent re-centering, but the
   literal `Comp_d`/`Decomp_d` of §2.2.4 (and hence of the `SelfTargetMSIS^R`
   definition, Def. 3) is not the function implemented.

5. **(b)/(a) — message binding is ML-DSA's, not the spec's.** Spec: `µ = H(pk‖m)`.
   Implementation (`sign.c`): `tr = SHAKE256(pk, 64)`, then
   `µ = SHAKE256(tr ‖ 0x00 ‖ ctxlen ‖ ctx ‖ m, 64)`. The pk binding is equivalent
   (tr is a CRH of pk), but the **context-string prefix is undocumented** in the
   specification. Likewise the signing randomness is ML-DSA's
   `ρ' = SHAKE256(key ‖ rnd ‖ µ)` plus a 16-bit nonce counter, not the spec's
   "ρ is updated" per-iteration rewrite, and the default build is **deterministic**
   (`rnd` all-zero unless `COMPASS_SIG_RANDOMIZED_SIGNING`) although the
   submission is filed as randomized. `SHAKE256` here denotes the implementation's
   `shake256_*` functions, which are a buffering wrapper around `pseudoXOF`
   (`symmetric-shake.c:11-30,151-167`) and not FIPS 202 SHAKE (sign-06-6).

6. **(a) — seed entropy is capped at 256 bits at every level.** Spec §2.3.1 takes
   an *n*-bit master seed (so 512 bits for the n = 512 parameter sets);
   `crypto_sign_keypair` draws `SEEDBYTES = 32` bytes from the DRNG
   (`params.h:6`, `sign.c`). Key generation therefore has 256 bits of entropy for
   the sets claiming 384- and 512-bit security. Similarly §2.2.3 says the first
   hash stage maps onto `{0,1}^n`, but `CTILDEBYTES` is 32/48/48/64, which equals
   n/8 only at levels 128 and 512.

**Not verified.** The NTT constants (`ROOT_OF_UNITY` 143389 for q = 2081281,
1718063 for q = 8380417 with n = 512), the `DECOMPOSE_M/A/S` magic-division
triples in `rounding.h`, and `ExpandA`'s rejection sampler were read but not
numerically re-derived. No estimator was rerun, so nothing here speaks to the
claimed bit-security levels; KAT agreement (4/4) is the only end-to-end evidence
that the deviations above are self-consistent.
