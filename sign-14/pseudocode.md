# sign-14 Lynxer — algorithm summary

Lynxer is a VOLE-in-the-Head (VOLEitH) signature built by Fiat–Shamir on a
QuickSilver zero-knowledge proof of knowledge of a preimage of **Lynx**, an
iv-keyed one-way function over `F_{2^λ}` made only of power-map S-boxes, field
multiplications/inversions and random `F_2`-linear layers. The hard relation is
therefore purely symmetric (no lattice/code/isogeny structure): security rests on
preimage resistance of Lynx plus the ideal-cipher/ROM assumptions on AES,
Rijndael-256, SHACAL-2 and the XOF. Transcript: BAVC (batch all-but-one vector
commitment) → VOLE commit/correct → QuickSilver → three Fiat–Shamir hashes with
proof-of-work grinding on `chall_3`.

Specification: `sign-14-spec.pdf` (134 pages) — §4 (Lynx), §5 (Lynxer: §5.2
top-level APIs, §5.3 VOLE, §5.4 BAVC, §5.5 Lynx OWF, §5.6 QuickSilver, §5.9
symmetric primitives), §3.7 + §6.4 (sizes and parameters).

## Parameters

| parameter | 160s | 160f | 256s | 256f | 384s | 384f | 512s | 512f | meaning |
|---|---|---|---|---|---|---|---|---|---|
| λ | 160 | 160 | 256 | 256 | 384 | 384 | 512 | 512 | security parameter, field `F_{2^λ}` |
| ℓ_wit | 480 | 480 | 768 | 768 | 1152 | 1152 | 1536 | 1536 | extended witness bits, fixed `3λ` (§5.1) |
| ℓ_vole | 816 | 816 | 1296 | 1296 | 1936 | 1936 | 2576 | 2576 | `ℓ_wit + dλ + B` bits (Table 5) |
| τ | 14 | 21 | 22 | 35 | 34 | 53 | 46 | 72 | number of VOLE instances |
| w_grind | 6 | 8 | 12 | 8 | 10 | 9 | 6 | 8 | proof-of-work level |
| T_open | 129 | 139 | 224 | 223 | 332 | 336 | 439 | 447 | BAVC opening-node threshold |
| k0 | 11 | 7 | 11 | 7 | 11 | 7 | 11 | 7 | `⌊(λ−w_grind)/τ⌋`, `k1=k0+1`, `t1=(λ−w_grind) mod τ` (§5.1, §6.2) |
| L | 28672 | 3328 | 49152 | 4864 | 69632 | 7296 | 94208 | 9216 | total BAVC leaves `Σ N_α` (derived) |
| d | 2 | 2 | 2 | 2 | 2 | 2 | 2 | 2 | QuickSilver constraint degree (§3.7) |
| B | 16 | 16 | 16 | 16 | 16 | 16 | 16 | 16 | universal-hash redundancy, bits (§5.1) |
| block cipher | AES-192 | AES-192 | AES-256 | AES-256 | Rijndael-256 | Rijndael-256 | SHACAL-2 | SHACAL-2 | TCCR/PRG (Table 7) |
| claimed security | 160c/80q | 160c/80q | 256c/128q | 256c/128q | 384c/192q | 384c/192q | 512c/256q | 512c/256q | bits, classical/quantum (§6.1) |

Sizes (bytes), spec Table 5 vs the built reference library (OBSERVED):

| instance | pk spec | pk impl | sk spec | sk impl | sig spec | sig impl | match |
|---|---|---|---|---|---|---|---|
| Lynxer-160s | 40 | 40 | 40 | 40 | 4607 | 4607 | yes |
| Lynxer-160f | 40 | 40 | 40 | 40 | 5801 | 5801 | yes |
| Lynxer-256s | 64 | 64 | 64 | 64 | 12191 | 12191 | yes |
| Lynxer-256f | 64 | 64 | 64 | 64 | 15097 | 15097 | yes |
| Lynxer-384s | 96 | 96 | 96 | 96 | 27495 | 27495 | yes |
| Lynxer-384f | 96 | 96 | 96 | 96 | 34109 | 34109 | yes |
| Lynxer-512s | 128 | 128 | 128 | 128 | 48879 | 48879 | yes |
| Lynxer-512f | 128 | 128 | 128 | 128 | 61091 | 61091 | yes |

**Odd signature sizes explained.** Eq.(4) of §3.7 is stated in *bits*:
`|σ| = ℓ_wit + (τ−1)(ℓ_wit+dλ+B) + (T_open+2τ)λ + (d−1)λ + (λ+B) + λ + 120 + 32`.
The `120` is `iv_sig` (15 bytes) and `32` the grinding counter; 15 is the only
odd byte-count in the layout, so every `|σ|` comes out odd. Worked example for
160s: `480 + 13·816 + 157·160 + 160 + 176 + 160 + 120 + 32 = 36856` bits = 4607 B.
I evaluated Eq.(4) for all eight sets: **all eight reproduce the observed sizes
exactly** (160f 46408 b = 5801 B; 256s 97528 b = 12191 B; 512f 488728 b = 61091 B; etc.).
pk = sk = `2λ/8` bytes (`ivowf‖t` and `ivowf‖k`), matching the observed 40/64/96/128.

## Pseudocode

### Lynx OWF (spec §4, Eqs. 5–7; §5.5)
```
Lynx.SampleMatVec(iv_owf):                       # Alg. §5.5.1
  buf := MatVecHash(iv_owf; 3(λ−1)λ + (2 or 3)λ)     # 3λ tail only if λ>160
  for i in 0..2: L_i := U'_i · L'_i                  # upper×lower triangular from buf
  c1,c2 [,c3 if λ∈{256,384,512}] := BitsToField(tail slices)
  # L3 is a fixed invertible matrix, irreducible char. poly (§4.2)
Lynx.ExtendWitness(...,k):                       # Alg. §5.5.2
  v1 := (k + c1)^{d1}     # λ=160: d1 = 2^λ−2 (inverse); else d1 = (2^i−1)^{-1} etc., Table 3
  v2 := (L0(k) + c2)^{d2} # Table 3
Lynx.EvalOutput(...):                            # Alg. §5.5.3, Eq.(7)
  t := δ_λ( L1(v1)·L2(v2), (k+c3)^{-1} ) + L3(k+v1+v2)
      where δ_λ(x,y) = x for λ=160, x·y otherwise
```

### KeyGen (spec §5.2.1)
```
loop:
  iv_owf <-$ {0,1}^λ ;  k <-$ F_{2^λ}
  (L0,L1,L2,c1,c2[,c3]) := Lynx.SampleMatVec(iv_owf)
  (v1,v2)               := Lynx.ExtendWitness(L0,L1,L2,c1,c2[,c3],k)
  if λ=160 and v1 = 0            : retry      # no zero input to an inversion box
  if λ∈{256,384,512} and k = c3  : retry
  t := Lynx.EvalOutput(...)
  return pk := (iv_owf, t), sk := (iv_owf, k)
```

### Sign (spec §5.2.2)
```
 2-5: recompute (L*,c*), (v1,v2), t, pk from sk
 6:   (mu, s)  := MsgHash(pk, msg)
 7-8: rho <-$ {0,1}^λ (or 0^λ, deterministic); (r, iv_sig) := CoinHash(sk, mu, rho)
 9:   (com, decom, {c_i}_{i∈[1,τ)}, u, V) := VOLE.Commit(r, iv_sig, mu, s)
10:   V := [V, 0^{ℓ_vole}, ..., 0^{ℓ_vole}]        # pad λ−w_grind columns up to λ
11:   chall_1 := FSHash1(com, {c_i})
12-13: ũ := VOLEHash(chall_1, u);  Ṽ := VOLEHash(chall_1, V)
14-15: w := FieldToBits(k)‖FieldToBits(v1)‖FieldToBits(v2);  d := w ⊕ u[0, ℓ_wit)
16:   chall_2 := FSHash2(chall_1, ũ, Ṽ, d)
17-19: u,V := first ℓ_wit+λ rows;  (ã1, ã0) := QS.LynxProve(w,u,V,L*,c*,t,chall_2)
20-30: ctr := 0; repeat
         chall_3 := FSHash3(chall_2, ã1, ã0, ctr)
         if chall_3[λ−w_grind, λ) != 0^{w_grind}: ctr++; continue     # grinding
         I := VOLE.ChalDecAll(chall_3);  decom_I := BAVC.Open(decom, I, iv_sig)
       until decom_I != ⊥                                            # |decom_I| ≤ T_open
31:   return σ := ({c_i}_{i∈[1,τ)}, ũ, d, ã1, decom_I, chall_3, iv_sig, ctr)
```

### Verify (spec §5.2.3)
```
 2-3: (L0,L1,L2,c1,c2[,c3]) := Lynx.SampleMatVec(iv_owf);  (mu,s) := MsgHash(pk,msg)
 4-5: (com, Q) := VOLE.Reconstruct(chall_3, decom_I, {c_i}, iv_sig, mu, s); pad Q to λ cols
 6-8: chall_1 := FSHash1(com,{c_i}); Q' := VOLEHash(chall_1,Q); Q̃ := VOLE.Fix(Q',ũ,chall_3)
 9-10: chall_2 := FSHash2(chall_1, ũ, Q̃, d);  Q := Q[0 .. ℓ_wit+λ−1]
11:   ã0 := QS.LynxVerify(d, Q, chall_2, chall_3, ã1, L*, c*, t)
12-13: chall_3' := FSHash3(chall_2, ã1, ã0, ctr)
       accept iff chall_3' = chall_3 AND chall_3[λ−w_grind, λ) = 0^{w_grind}
```

### Hashes / symmetric layer (spec §5.9)
`MsgHash, CoinHash, BAVCHash, MatVecHash, FSHash1..3` are domain-separated calls
to one XOF; `LeafHash(x, iv) = (x, PRG(x, iv, 0^8; 2λ))` (§5.4.7); `PRG` and the
`TCCR` used in the GGM/BAVC tree are built from the block cipher of Table 7
(AES-192/AES-256/Rijndael-256/SHACAL-2). Table 6 offers two XOF backends:
`pseudoXOF` (NGCC SM3-based) and `Keccak[2λ]`.

## Implementation vs specification

Checked original reference source for each parameter label: `parameters.h`/`instances.c`
(parameter sets), `voleith_impl.c` (Sign/Verify and signature serialization),
`owf.c` + `lynx_matrices.c` (Lynx), `quicksilver.c` (QS constraints), `bavc.c`,
`vole.c`, `universal_hashing.c`, `xof.h`.

Agreements:
- Parameter spot-check (sampled: λ, ℓ_wit, τ, w_grind, T_open, |σ| for
  Lynxer-160s/160f/256s/384s, i.e. 4 of 8 sets): `Lynxer-160s/parameters.h`
  gives `CSP 160, LENWIT 480, TAU 14, POW_LEVEL 6, T_OPEN 129, SIG_SIZE 4607`
  and the 160f/256s/384s blocks give `(21,8,139,5801)`, `(22,12,224,12191)`,
  `(34,10,332,27495)` — all identical to spec Table 5.
- `instances.c:33-36` derives `k1 = ⌊(λ−w_grind)/τ⌋+1`, `t1 = (λ−w_grind) mod τ`,
  `L = t1·2^{k1} + t0·2^{k1−1}`, exactly §5.1/§6.2.
- `instances.h:18-21`: `UNIVERSAL_HASH_B_BITS 16`, `IV_SIZE 15` (=120 bits) —
  matches `B = 16` and `iv_sig ∈ {0,1}^120`.
- Signature layout in `voleith_impl.c:17-77` is field-for-field Eq.(4):
  `(τ−1)·ℓ_vole/8` correction vectors, then `ũ (λ/8+2)`, `d (ℓ_wit/8)`,
  `ã1 ((d−1)λ/8)`, `decom_I`, and the tail `chall_3 (λ/8) ‖ iv (15) ‖ ctr (4)`
  addressed backwards from `sig_size`. `quicksilver_degree()` returns the
  constant 2, matching `d = 2` in §3.7.
- Grinding and the `decom_I = ⊥` retry loop are present in the signer, and the
  verifier re-checks both `chall_3' = chall_3` and the `w_grind` zero suffix.

Discrepancies:
- **(a) Real deviation, λ=160 only — the second Lynx constant is not used.**
  Spec Eq.(6)/§5.5.2 sets `v2 := S2(L0(k) + c2)`, with `c1, c2` two independent
  constants expanded from `iv_owf`. The implementation uses `c1` (its `mats.C0`)
  in *both* S-boxes for the 160-bit sets:
  `owf.c:602-605` — `v1 = bf160_s1(k + c0); ... v2 = bf160_s2(L0k + c0);`
  and identically in `lynx_extend_witness`, `owf.c:765-766`. The 128/192/256/384/512
  paths correctly use `c0` then `c1` (`owf.c:574-577, 629-632, 658-661, ...`).
  `lynx_matrices.c:484-485` does squeeze both `C0` and `C1`, so `C1` is generated
  and then ignored at λ=160. The QuickSilver constraint generator agrees with the
  (deviant) evaluation — `quicksilver.c:335` loads only `mats.C0` for λ=160 — so
  the scheme is internally consistent and the KATs pass; but the built
  Lynxer-160s/160f do **not** implement the OWF as specified. Effect: one fewer
  independent random constant, and `k + c1` and `L0(k) + c1` become related
  inputs to the two S-boxes. I did not attempt to quantify any security impact.
- **(b) Spec ambiguity (minor).** §6.2 writes `k0 = ⌈(λ−w_grind)/τ⌉` while §5.1
  defines `k1 = k0 + 1` with `t1` instances of the larger type; those agree only
  when the division is exact. The implementation follows the §5.1 reading
  (`k0 = ⌊·⌋`), which is the self-consistent one.
- **(c) Deliberate build choice, not a deviation.** The local audit build
  compiles with `-DXOF_PSEUDO`, selecting the NGCC SM3-based `pseudoXOF` from the
  ICCS `auxfunc.c` (`xof.h:31ff`); the `sha3/` Keccak backend
  (`-DWITH_KECCAK_X4`, `hash_shake.h`) is not compiled. Spec Table 6 lists both
  as sanctioned backends, so what we measure is the `pseudoXOF` variant only; the
  standardized `Keccak[2λ]` variant is untested here.

Not verified (time-box): the internals of `bavc.c`, `vole.c`,
`universal_hashing.c` and the QuickSilver constraint algebra for λ≥256 were not
traced line-by-line against §5.3/§5.4/§5.6/§5.7; the domain-separation tags of
the seven hash functions in §5.9 were not audited. See
the original source review (KAT 8/8 PASS; Fiat–Shamir
transcript binding and hash domain separation recorded there as `not_tested`).
