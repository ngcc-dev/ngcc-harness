# sign-05 Chinith (SM4th / uBlockith / Vistrutith) — algorithm summary

VOLE-in-the-Head (FAEST-style) signature: a QuickSilver-variant zero-knowledge
proof of knowledge of a block-cipher OWF preimage, made non-interactive by a
3-challenge Fiat–Shamir transform with grinding. No number-theoretic assumption:
security rests on the PRP/one-wayness of SM4, uBlock and Vistrutah, and on SM3 as
a random oracle (spec §3.1). Three OWF backends (SM4th 128-bit, uBlockith 256-bit,
Vistrutith 512-bit), each in a standard form `y = E_k(x)` and — for SM4/uBlock — an
Even–Mansour form `y = E_x(k) ⊕ k`; `k` is always the secret component.

Specification: `sign-05-spec.pdf` (125 pages), §2 (overview), §3.2 (parameters),
§5.1 (BAVC/VOLE), §6.6 / §7.6 / §8.5 (KeyGen/Sign/Verify).

## Parameters

`param = (λ, λ_Q, λ_F, τ, w_grind, T_open, d, B, OWF, ℓ, λ_iv, N_block)` (spec §3.2).
Sampling: 5 of 14 instances shown (one per OWF/EM/loose-tight family); the other 9
differ only in the f/s split of (τ, w_grind, T_open).

| parameter | SM4th-d3-128s-lo | SM4th-EM-d2-128f-lo | SM4th-d3-128s-ti | uBlockith-EM-d3-256s | Vistrutith-d3-512f | meaning |
|---|---|---|---|---|---|---|
| OWF | SM4_k(x) | SM4_x(k)⊕k | SM4_k(x) | uBlock_x(k)⊕k | Vistrutah_k(x) | one-way function (Tab. 1) |
| λ / λ_Q | 128 / 80 | 128 / 80 | 128 / 80 | 256 / 128 | 512 / 256 | classical / quantum sec. param. |
| λ_F | 128 | 128 | 160 | 256 | 512 | VOLE/QuickSilver field GF(2^λ_F) |
| ℓ | 1808 | 1024 | 1808 | 3072 | 6912 | extended-witness bits (Tab. 2) |
| ℓ̂ = ℓ+dλ_F+B | 2208 | 1296 | 2304 | 3856 | 8464 | padded witness (Tab. 3) |
| τ (=τ₁+τ₀) | 11 (0+11) | 16 (8+8) | 14 (13+1) | 22 (8+14) | 64 (56+8) | GGM sub-trees |
| k | 12 | 8 | 11 | 12 | 8 | depth of the τ₁ larger trees |
| w_grind | 7 | 8 | 7 | 6 | 8 | grinding (PoW) bits |
| T_open | 102 | 112 | 131 | 218 | 481 | max seeds in a BAVC opening |
| d | 3 | 2 | 3 | 3 | 3 | max constraint degree |
| B | 16 | 16 | 16 | 16 | 16 | VOLE-check padding (fixed) |
| C | 453 | 257 | 453 | 3073 | 2881 | # OWF constraints (Tab. 2) |
| λ_iv / N_block | 128 / 128 | 128 / 128 | 256 / 256 | 256 / 256 | 256 / 512 | IV len / PRG block len |
| claimed security | 128 | 128 | 128 | 256 | 512 | bits, classical (spec Tab. 1, 15) |

"lo"/"ti" = loose/tight: loose keeps the native SM4 PRG (λ_iv = 128) and claims the
ICCS Q_sig = 2^80 budget only in a *multi-key* reading (2^16 keys × 2^64 sigs); tight
uses Ballet-256 as PRG with λ_iv = 256 and λ_F = 160 so a *single* key supports 2^80
queries with a tight QROM bound (spec §3.2, pp. 10–11).

Sizes (bytes), specification (Tab. 16) vs the built reference library (OBSERVED):

| instance | pk spec | pk impl | sk spec | sk impl | sig spec | sig impl | match |
|---|---|---|---|---|---|---|---|
| sm4th_d3_128s_loose | 32 | 32 | 32 | 32 | 5056 | 5056 | yes |
| sm4th_d3_128f_loose | 32 | 32 | 32 | 32 | 6724 | 6724 | yes |
| sm4th_em_d2_128s_loose | 32 | 32 | 32 | 32 | 3818 | 3818 | yes |
| sm4th_em_d2_128f_loose | 32 | 32 | 32 | 32 | 4932 | 4932 | yes |
| sm4th_d3_128s_tight | 32 | 32 | 32 | 32 | 9176 | 9176 | yes |
| sm4th_d3_128f_tight | 32 | 32 | 32 | 32 | 11864 | 11864 | yes |
| sm4th_em_d2_128s_tight | 32 | 32 | 32 | 32 | 7556 | 7556 | yes |
| sm4th_em_d2_128f_tight | 32 | 32 | 32 | 32 | 9450 | 9450 | yes |
| ublockith_d3_256s | 64 | 64 | 64 | 64 | 24144 | 24144 | yes |
| ublockith_d3_256f | 64 | 64 | 64 | 64 | 31556 | 31556 | yes |
| ublockith_em_d3_256s | 64 | 64 | 64 | 64 | 19056 | 19056 | yes |
| ublockith_em_d3_256f | 64 | 64 | 64 | 64 | 25028 | 25028 | yes |
| vistrutith_d3_512s | 128 | 128 | 128 | 128 | 83004 | 83004 | yes |
| vistrutith_d3_512f | 128 | 128 | 128 | 128 | 106788 | 106788 | yes |

All 14 match. Signature size (spec §10.1):
`τ·(ℓ + dλ_F + B) + (2τ + T_open)·N_block + λ_F + λ_iv + 32` bits.

## Pseudocode

Hashes (spec §3.2, "Hash functions"), all SM3-based XOFs with distinct domain tags:
`H1 → 2N_block` (vector commitments); `H2^0 → 2λ` (µ), `H2^1 → 5λ_F+64`,
`H2^2 → 3λ_F+64`, `H2^3 → λ_F` (Fiat–Shamir); `H3: {0,1}^{4λ+ξ} → N_block+λ_iv`
(root seed + pre-IV, PRF-modelled, ξ = λ+128); `H4: λ_iv → λ_iv`.

### KeyGen  (spec §6.6.1, `SM4th.KeyGen`; §7.6.1, §8.5.1 analogous)
```
if EM:   x ← {0,1}^128                       // public cipher key
         repeat k ← {0,1}^128 until k[0]∧k[1] = 0
         y ← SM4_x(k) ⊕ k
else:    repeat k ← {0,1}^128 until k[0]∧k[1] = 0   // secret cipher key
         x ← {0,1}^128 ; y ← SM4_k(x)
sk ← (x, k) ;  pk ← (x, y) ;  return (sk, pk)
```
(The `k[0]∧k[1] = 0` rejection makes the witness-extension S-box path well-defined.
Widths are 256 bits for uBlockith, 512 for Vistrutith.)

### BAVC / VOLE commitment  (spec §5.1.1–§5.1.3)
```
BAVC.LeafCommit(r,iv,twk):  com ← PRG(r,iv,twk; 2N_block) ;  sd ← r      // Alg. p.32
BAVC.Commit(sd_tree,iv):                                                  // Alg. p.33
  k_0 ← sd_tree ; for α ∈ [0,L-1): (k_{2α+1},k_{2α+2}) ← PRG(k_α,iv,α; 2N_block)
  for i ∈ [0,τ), j ∈ [0,N_i):  α ← PosInTree(i,j)          // interleaved leaves
       (sd_ij, com_ij) ← LeafCommit(k_α, iv, i+L-1)
  h_i ← H1(com_i,0 ‖ … ‖ com_i,N_i-1) ;  com ← H1(h_0 ‖ … ‖ h_{τ-1})
BAVC.Open(decom, I=(Δ_0..Δ_{τ-1})):                                       // Alg. p.34
  mark the τ root-to-leaf paths; n_h ← #marked nodes
  if n_h - 2τ + 1 > T_open: return ⊥
  decom_I ← (com_{i,Δ_i})_{i<τ} ‖ every node whose sibling subtree is fully opened
  zero-append decom_I to exactly 2N_block·τ + T_open·N_block bits
ConvertToVOLE(sd_0..sd_{N-1}, iv, twk, ℓ̂):                                // Alg. p.38
  r_{0,i} ← PRG(sd_i,iv,twk; ℓ̂)  (0^ℓ̂ if sd_i = ⊥)
  for j ∈ [0,D): v_j ← ⊕_i r_{j,2i+1} ; r_{j+1,i} ← r_{j,2i} ⊕ r_{j,2i+1}
  u ← r_{D,0}; return (u, v_0..v_{D-1})
VOLECommit(sd_tree,iv,ℓ̂):                                                 // Alg. p.39
  (com,decom,{sd_ij}) ← BAVC.Commit;  per tree i: (u_i,V_i) ← ConvertToVOLE(…, i+2^31, ℓ̂)
  V ← [V_0 ‖…‖ V_{τ-1} ‖ 0^{ℓ̂×w_grind}] ;  u ← u_0 ;  c_i ← u ⊕ u_i  (i ≥ 1)
```

### Sign  (spec §6.6.2, `SM4th.Sign`; uBlockith §7.6.2, Vistrutith §8.5.2 identical in shape)
```
 1: ctr ← 0 (32-bit);  ρ ← {0,1}^ξ
 2: µ ← H2^0(pk ‖ msg; 2λ)                       // pk = (x,y)
 3: (r, iv_pre) ← H3(sk_2 ‖ µ ‖ ρ);  iv ← H4(iv_pre; λ_iv)
 4: ℓ̂ ← ℓ + dλ_F + B
 5: (com, decom, c_1..c_{τ-1}, u, V) ← VOLECommit(r, iv, ℓ̂)
 6: chall_1 ← H2^1(µ ‖ com ‖ c_1 ‖…‖ c_{τ-1} ‖ iv; 5λ_F+64)
 7: ũ ← VOLEHash(chall_1, u) ;  Ṽ ← VOLEHash(chall_1, V)     // VOLE consistency
 8: w ← [EM.]ExtendWitness(sk_2, sk_1) ∈ {0,1}^ℓ ;  d ← w ⊕ u[0..ℓ)
 9: chall_2 ← H2^2(chall_1 ‖ ũ ‖ Ṽ ‖ d; 3λ_F+64)
10: u_zk ← u[ℓ .. ℓ+(d-1)λ_F) ;  V_zk ← V[0 .. ℓ+2λ_F)
11: (ã_0,…,ã_{d-1}) ← OWFProve(w, u_zk, V_zk, pk, chall_2)   // QuickSilver, degree d
12: repeat                                                    // grinding loop
13:    chall_3 ← H2^3(chall_2 ‖ ã_0 ‖…‖ ã_{d-1} ‖ ctr; λ_F)
14:    if chall_3[λ_F-w_grind .. λ_F) ≠ 0^{w_grind}: ctr++ ; continue
15:    I ← DecodeChall(chall_3[0, λ_F-w_grind)) ;  decom_I ← BAVC.Open(decom, I)
16: until decom_I ≠ ⊥
17: return σ ← ({c_i}_{i∈[1,τ)}, ũ, d, ã_1,…,ã_{d-1}, decom_I, chall_3, iv_pre, ctr)
```
Note ã_0 is *not* transmitted: the verifier recomputes it and checks it via chall_3.

### Verify  (spec §6.6.3, `SM4th.Verify`)
```
 1: parse σ ;  µ ← H2^0(pk ‖ msg; 2λ) ;  iv ← H4(iv_pre; λ_iv)
 2: if chall_3[λ_F-w_grind .. λ_F) ≠ 0^{w_grind}: reject      // grinding check
 3: (com, Q) ← VOLEReconstruct(decom_I, chall_3[0,λ_F-w_grind), c_1..c_{τ-1}, iv)
 4: if ⊥: reject
 5: chall_1 ← H2^1(µ ‖ com ‖ c_1 ‖…‖ c_{τ-1} ‖ iv; 5λ_F+64)
 6: Q̃ ← VOLEHash(chall_1, Q) ;  parse chall_3 = (δ_0..δ_{λ_F-1})
 7: Ṽ' ← Q̃ ⊕ [δ_0·ũ … δ_{λ_F-1}·ũ]
 8: chall_2 ← H2^2(chall_1 ‖ ũ ‖ Ṽ' ‖ d; 3λ_F+64)
 9: ã_0 ← OWFVerify(d, Q[0,ℓ+2λ_F), chall_2, chall_3, ã_1..ã_{d-1}, pk)
       // ⟨w[i]⟩ ← ToField(Q|_i) + d[i]·Δ ;  q* ← q*_0 + q*_1·Δ
       // ⟨z⟩ ← OWFConstraints(⟨w⟩,pk) ; q̃ ← ZKHash(chall_2, b ‖ q*)
       // return q̃ + Σ_{i∈[1,d)} ã_i·Δ^i
10: chall_3' ← H2^3(chall_2 ‖ ã_0 ‖…‖ ã_{d-1} ‖ ctr; λ_F)
11: accept iff chall_3' = chall_3
```

## Implementation vs specification

Checked: `src/<label>/params.{h,c}` (all constants), `sig_impl.c` +
`sig_impl_internal.h` (Sign/Verify flow, Fiat–Shamir absorption order, signature
layout), `bavc.c` (Commit/Open/Reconstruct), `vole.c`, `universal_hashing.c`,
`random_oracle.c` (domain separators). The uBlockith-EM witness layout and
Vistrutith aggregation paths were subsequently checked against §7.2–§7.5 and
§8.3–§8.4; the remaining OWF constraint code has not been reviewed line by line.

Agreements:

- Parameter constants sampled for 5 of 14 instances (`sm4th_d3_128s_loose`,
  `sm4th_d3_128s_tight`, `sm4th_em_d2_128f_loose`, `ublockith_em_d3_256s`,
  `vistrutith_d3_512f`): every one of λ, λ_Q, λ_F, λ_iv, ℓ, S_ke, S_enc, R, ℓ_ke,
  ℓ_enc, τ, w_grind, T_open, d matches Tables 1–2 exactly, and
  `ell_hat_bits = ℓ + d·λ_F + 16` reproduces Table 3's ℓ̂.
- `params.c` `sig_size()` implements the §10.1 formula term-for-term
  (A=(τ−1)ℓ̂, B=ũ, C=d, D=(d−1)λ_F, E=(2τ+T_open)·N_block, F=λ_F, G=λ_iv, H=4).
  Recomputed by hand for `sm4th_d3_128s_loose` (5056) and `vistrutith_d3_512f`
  (106788): matches both the spec table and OBSERVED.
- The tight variants correctly separate λ (=128) from N_block (=256): `params.h`
  adds `lambda_prg`/`prg_block_bits` = 256 and `sig_size()` uses
  `prg_lambda_bytes` for the BAVC term, which is what makes 9176/11864/7556/9450
  come out right. Using λ there would have given 6632 for `sm4th_d3_128s_tight`.
- Fiat–Shamir absorption order matches §6.6.2 byte-for-byte:
  `hash_mu` absorbs `x ‖ y ‖ msg` (= pk ‖ msg), `hash_challenge_1` absorbs
  `µ ‖ com ‖ c ‖ iv`, `..._2` absorbs `chall_1 ‖ ũ ‖ Ṽ ‖ d`, `..._3` absorbs
  `chall_2 ‖ ã_0..ã_{d-1} ‖ ctr` with `ctr` as little-endian `uint32`
  (`sig_impl.c:511`, `H2_3_final_u32_le`). Distinct `H2` domain separators are
  appended (e.g. `H2_DOMAIN_SEP_3 = 11`, `sig_impl.c:213`).
- Signature layout in `sig_impl_internal.h:27-65` is exactly the spec's σ tuple in
  order: `c_1..c_{τ-1} ‖ ũ ‖ d ‖ ã_1..ã_{d-1} ‖ decom_I ‖ chall_3 ‖ iv_pre ‖ ctr`,
  all fields fixed-length. `ã_0` is absent, as the spec requires.
- Grinding is enforced on *both* sides: `check_challenge_3(...)` at `sig_impl.c:482`
  (signer) and `sig_impl.c:543` (verifier, before any reconstruction).
- `bavc_open` enforces `n_h − 2τ + 1 > T_open ⇒ ⊥` at `bavc.c:487`, matching
  BAVC.Open steps 16–17, and `bavc_reconstruct` bounds its seed reads to
  `T_open·λ_bytes` (`bavc.c:546`).

Discrepancies / notes:

- **(a) uBlockith-EM constraint misalignment.** The EM witness already begins
  with `S0=in`, but `OWFConstraints` selects from `l_ke=0` before
  `EncCstrnts` prepends `in` again. The specified final transition therefore
  reads `S22`, not `out`; the code has the same duplicated start but reads
  `out` directly in its final-round branch. See `sign-05-4`.
- **(a) Vistrutith aggregation mismatch.** The prover passes tags of three
  different constraint families as the three coefficients of one polynomial,
  while the verifier evaluates that artificial polynomial at `delta`. Once the
  final challenge binds the reconstructed constant term, honest proofs fail.
  See `sign-05-5`.
- **(b) spec ambiguity, not verified.** `decom_I` is zero-padded to the fixed length
  `2N_block·τ + T_open·N_block` (BAVC.Open step 27). Neither the spec's
  `SM4th.Verify` nor `VOLEReconstruct` requires the verifier to check that the
  unused tail is zero, and `decom_I` influences `chall_3'` only indirectly (via
  `com → chall_1 → chall_2 → ã_0`), so unconsumed padding bytes plausibly do not
  affect acceptance. I did not confirm whether `bavc_reconstruct_em` rejects a
  non-zero tail; this is a canonical-encoding question left open, consistent with
  `security_findings.md` recording `signature-encoding-malleability: not_found`
  (sampled-bit testing only, not exhaustive). Worth a targeted test.
- **(b) naming, harmless.** `params.h:60` defines `is_em(params) ((params)->ske == 0)`.
  Vistrutith has `S_ke = 0` (Table 2) because Vistrutah's key schedule is *linear*,
  not because it is an EM construction — the spec explicitly says no EM variant of
  Vistrutith exists (§3.2, p. 12). The Vistrutith instance uses its own
  `utils_vistrutah/sig_impl_vistrutith.c` path, so the macro does not appear to
  mis-route, but the predicate is a misnomer.
- **(c) deliberate.** `use_em_bavc` is set (`true` everywhere except
  `vistrutith_*`, `params.c:26`) but is read nowhere in the Vistrutith instance;
  it is a dead configuration field. The spec gives only one `BAVC.LeafCommit`
  (the PRG-based, FAEST-EM-style one, §5.1.1), so there is nothing to select.
- **Build fragility (already recorded in `security_findings.md`, not rediscovered):**
  `vole.c` has an `assert()` on an undeclared variable; the build only succeeds
  with `-DNDEBUG`.
- No discrepancy found between spec Table 16 and OBSERVED for any of the 14 built
  instances; all 14 KATs pass per `RESULTS.md:194` ff.
