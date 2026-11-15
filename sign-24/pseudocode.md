# sign-24 Sigurd — algorithm summary

Sigurd is a VOLE-in-the-Head (VOLEitH) signature whose one-way function is **Regular Syndrome
Decoding over F2** (not a block cipher): `pk = (seed_H, y=He)` with `e` block-regular, `B=6`.
It departs from FAEST/ReSolveD by being **GGM-tree-free**: instead of an all-but-one GGM seed
tree plus SoftSpokenOT, the signer *directly* commits to the sVOLE sender vectors `(ê', v̂')`
with a Reed–Solomon vector commitment (Merkle roots + DEEP-quotient linear-relation proof, à la
DEEP-FRI) and, after Fiat–Shamir challenge ∆, opens `ŵ' = ∆ê' + v̂'`. The RSD statement is lifted
to a tower field F2 ⊂ F2^ι ⊂ F2^(ιτ): each one-hot block becomes `ê_i = Y^{a_i}`, regularity is a
QuickSilver-style vanishing argument for `F(x)=∏_{j<B}(x−Y^j)`, and `He=y` is a masked Σ-protocol
reading the `Y^{B−1}` coefficient of `Ĥŵ'`.

Specification: `sign-24-spec.pdf` (57 pages), §4 (VC/compiler), §5 (RSD proof system), §6 (scheme).

## Parameters

| parameter | NGCC-128 | NGCC-256 | NGCC-512 | meaning (spec Table 2, §6.2) |
|---|---|---|---|---|
| (m, k) | (1302, 738) | (2748, 1564) | (5676, 3230) | RSD code dimensions |
| B, n=m/B | 6, 217 | 6, 458 | 6, 946 | block size / block count |
| (ι, τ) | (16, 10) | (16, 16) | (16, 32) | tower degrees, τ>B, ιτ≥λ |
| t_Σ | 10 | 16 | 32 | Σ-protocol repetitions (ε_Σ = 2^{-ι·t_Σ}+2^{-ιτ}) |
| Q_VC | 160 | 171 | 256 | RS/Merkle query count |
| ρ | 1/4 | 1/8 | 1/16 | Reed–Solomon rate |
| N_VC | 1568 | 5208 | 19824 | evaluation-domain size = N'/ρ |
| TAIL_LEN | 175 | 193 | 293 | = t_Σ + 5 + extra random tail |
| N' = n+TAIL_LEN | 392 | 651 | 1239 | committed vector length |
| hash suite | SM3-256 + pseudoXOF | pseudohash-512 | pseudohash-1024 | digest 32/64/128 B |
| seed_H, seed_e (bits) | 320 | 512 | 1024 | |
| claimed security | 128 (80 q) | 256 (128 q) | 512 (256 q) | bits, spec's own claim |

Sizes (bytes), specification vs the built reference library:

| instance | pk spec | pk impl | sk spec | sk impl | sig spec | sig impl | match |
|---|---|---|---|---|---|---|---|
| Sigurd-128 | 112 | 112 | 80 | 80 | ≈25108 | 62868 | pk/sk yes, **sig no** |
| Sigurd-256 | 212 | 212 | 128 | 128 | ≈74756 | 137412 | pk/sk yes, **sig no** |
| Sigurd-512 | 435 | 435 | 256 | 256 | ≈282692 | 494532 | pk/sk yes, **sig no** |

**Why the signature is so large, and why the two columns differ.** §6.3.4 gives the serialization;
with `|F| = ιτ/8` bytes per field element, `|H|` the digest and `h` the Merkle height it is

```
|σ| = g·|H|                       (Merkle roots, g = 2 / 6 / 8 groups)
    + (2 + 2t_Σ + 6 + N')·|F|     (e_eval,v_eval | p_poly,rv_out | G0..G5 | ŵ')
    + 4 + h_open·|H|              (LE32 count + Merkle authentication hashes)
    + Q_VC·|F|                    (opened RS witness symbols)
```
The library's `sig_get_sn_len_bytes()` returns the **worst case** `h_open = Q_VC·h`
(`MAX_SIGNATURE_BYTES`, sig_core.h:70-77). For Sigurd-128: 64 + (2+20+6+392)·20 + 4 +
160·10·32 + 160·20 = 62868 — exactly the OBSERVED value; 256 and 512 reproduce 137412 and 494532
the same way. The Merkle path block alone is 51200/109440/393216 B, i.e. **81 % / 80 % / 79 %** of
the signature: this is the price of Q_VC full authentication paths with no GGM tree to share seeds.
The spec's "sample encoded signature length" counts the *actual* deduplicated path set
(≈420/731/1417 hashes instead of 1600/1710/3072), which is what `sig_sign` writes back in
`*sn_len_bytes`. Both numbers are self-consistent; the ABI simply advertises the upper bound.

## Pseudocode

### KeyGen (spec Algorithm 6.6, §6.5)
```
1. seed_H, seed_e  <-$ {0,1}^{8·SEED_BYTES}          # 40 / 64 / 128 bytes
2. H := random_H(seed_H)      # bit stream under label "SIG<n>:H", row-major
   e := random_e(seed_e)      # one 16-bit word per block; a_i := rnd_i mod 6
   y := H·e    over F2
3. pk := seed_H || pack_y(y)  # MSB-first per byte, padded to the row-block grid
   sk := seed_H || seed_e     # both seeds stored, no master seed (§6.3.3)
```

### Sign(sk, M) (spec Algorithm 6.7, §6.6; internals Alg. 6.1-6.5)
```
 1. parse sk; rebuild (H,e,y); sig_build_public_state -> (Ĥ, ê', ȳ, ŷe=Ĥê)   # §6.3.1
 2. mh := Hash(M)
 3. Commitment_Raw:  append random TAIL_LEN tail to ê' (first t_Σ tail slots have
       coeff_{Y^{B-1}} forced to 0, they are the Σ masks r_Σ of Alg. 5.3 step 1);
       v̂' <-$ (F_{2^{ιτ}})^{N'};  RS-encode both to length N_VC;  Merkle trees -> root_0..root_{g-1}
 4. a_com := Hash(mh || pk || root);  z := hash_to_rand_commitment(a_com)
       (e_eval, v_eval) := (Ê(z), V̂(z))                      # DEEP evaluation point, z ∉ L
 5. ŷv := Ĥ·v̂'                                               # Initial_Computation_Prover_Raw
 6. a_ic := Hash(mh || a_com || e_eval || v_eval);  (r_s, γ_s)_{s<t_Σ} := hash_to_rand_InterCheck
 7. for s < t_Σ:   # Alg. 5.2 regularity + Alg. 5.3 syndrome check, fused
       aggregate Ĥê, Ĥv̂ row-blocks with r_s; place the binary form in the Y^{B-1} slot;
       mix in the tail element TAIL_INTERACTIVE_OFFSET+s with γ_s
       output (p_poly_s, rv_out_s)                            # p_poly_s has coeff_{Y^{B-1}} = 0
 8. a_poly := Hash(mh || a_ic || p_poly || rv_out);  (τ, ζ) := hash_to_rand_PolyRes
 9. PolyDef_Raw + ProverPolynomialResponse_Final_Raw -> G_0..G_5   # DEEP quotient response
10. a_mask := Hash(mh || a_poly || G);  ∆ := hash_to_rand_Mask(a_mask)   # ∆ ∈ F_{2^{ιτ}}
11. ŵ' := ∆·ê' + v̂'                                          # the deferred VOLE opening
12. a_open := Hash(mh || a_poly || ŵ');  Q_VC positions := hash_to_rand_Open(a_open)
13. Commitment_Open_Prover_Raw: copy out the queried encoded witness symbols e_open and the
       *minimal* set of Merkle authentication hashes (shared prefixes are emitted once)
14. sigma := sig_serialize_signature(root, e_eval, v_eval, p_poly, rv_out, G, ŵ', hash, e_open)
```
No rejection/abort loop and no grinding/proof-of-work counter: Remark 6.2 states Verify∘Sign
accepts with probability 1.

### Verify(pk, M, σ) (spec Algorithm 6.8, §6.7)
```
1. parse pk = seed_H||packed_y; rebuild (H, Ĥ, ȳ); deserialize σ (reject on any length mismatch)
2. replay mh, a_com, a_ic, a_poly, a_mask, a_open -> z, (r_s,γ_s), (τ,ζ), ∆, open positions
3. Commitment_Open_Verifier_Raw:  w_eval := Ŵ(z);  check  w_eval = ∆·e_eval + v_eval;
      RS-encode ŵ', derive the masking symbols from ŵ', ∆ and the opened witness symbols,
      recompute each Merkle root from leaves+paths and compare with root         [else reject]
4. ŷw := Ĥ·ŵ';  for each s < t_Σ check the response identity in (p_poly_s, rv_out_s, ∆, tail_s)
      and that coeff_{Y^{B-1}}(p_poly_s) = 0                                     [else reject]
5. accept iff  Polynomial_Challenge_Raw(τ,ζ,ŵ',∆) = VerifierCheck_Raw(G,∆)
```

Hash / expansion structure (spec §6.3.2): one fixed-width `Hash` for the transcript chain
(`Hash = SM3-256` at L1, the SM3/HMAC-SM3 cascade `pseudohash-512/1024` at L3/L5) plus
`derive_expand_bytes`/`pseudoXOF` keyed by the seven domain labels
`SIG<n>:{H,e,commitment,intercheck,polyres,mask,open}`. Merkle nodes use the same `Hash`.

## Implementation vs specification

Checked: `Implementations/Reference_Implementation/Sigurd-{128,256,512}/sig_core.h`
(all constants), `sig_core.c` (FS labels, `hash_to_rand_*`, serializer/deserializer, keygen),
`SIG_AlgorithmInstance.c` (`sig_get_*_len_bytes`), `auxfunc.c` (SM3 / pseudohash / pseudoXOF).
The Makefile builds exactly these five sources per instance.

Parameter sampling (6 constants × 3 instances, from `sig_core.h`): `M/K/M_K`, `opennum`,
`SIGMA_REPETITIONS`, `RAW_POLY_LEN` (=τ), `TAIL_LEN`, `N`(=N_VC) — **all agree** with spec Table 2:
1302/738, 2748/1564, 5676/3230; 160/171/256; 10/16/32; 10/16/32; 175/193/293; 1568/5208/19824
(the 512 defaults `SIG512_RS_RATE_FACTOR=16`, `SIG512_TOTAL_GROUPS=8` are the ones matching ρ=1/16).
ι=16 is implicit in the `uint16_t coeffs[RAW_POLY_LEN]` representation. Digest widths 32/64/128 B
match the stated hash suites. The FS chain in `sig_core.c` matches §6.3.2 label for label,
including `mh||pk||root` for α_com (`FS_COMMITMENT_INPUT_BYTES = HASH_DIGEST_LENGTH + PK_LEN_BYTES`).

- **Discrepancy (reporting, not crypto)** — spec Table 2 "sample encoded signature length"
  25108/74756/282692 vs OBSERVED 62868/137412/494532. `sig_get_sn_len_bytes()`
  (`SIG_AlgorithmInstance.c:281-283`) returns `MAX_SIGNATURE_BYTES` (`sig_core.h:70-77`), the
  worst-case Merkle-path count `Q_VC·height`; real signatures are ~40-57 % shorter because
  authentication paths are deduplicated. Both are reproducible from the §6.3.4 formula — the spec
  never states the *maximum*, which is the number an API consumer must allocate. Category (c)/(b).
- **Spec ambiguity** — §6.3.3 writes `pk := seed_H || packed_y` with no padding rule; a literal
  `⌈(m−k)/8⌉` gives 111/212/434, while the implementation pads the syndrome to the embedded
  row-block grid (`Y_BITS_PACKED_BYTES = 72/148/307`, from `embded_num = M_K+12 / M_K / 2456`),
  giving 112/212/435. Table 2 and OBSERVED agree on the padded values; only the formula is loose.
  At L5, `embded_num = 2456` is not a multiple of 16 while `ROWS = embded_num/16 = 153` truncates
  to 2448 ≥ M_K = 2446 bits — harmless, but the constant is not self-consistent.
- **Spec ambiguity** — §1.5 calls the construction "Fiat–Shamir **with aborts**", while Remark 6.2
  and the code have no abort/retry path at all. §2.3 also offers SHAKE128/256 as instantiations,
  whereas Table 2 and the code fix SM3/pseudohash.
- **Minor implementation note** — modulo bias: `hash_to_rand_Open` (`Sigurd-128/sig_core.c:1741`,
  reduction at `:1760`) draws 16-bit words and reduces `% remaining` (remaining ≤ N_VC), and
  `random_e` reduces a 16-bit word `% 6`. For the first draw the per-slot preimage counts are
  42:41 at N_VC = 1568, 13:12 at N_VC = 5208 and 4:3 at N_VC = 19824, so the favoured slots are
  about 0.5%, 3.3% and 21% above uniform (the 6064 favoured slots of Sigurd-512) and the others
  1.9%, 4.6% and 9.3% below; the `% 6` bias is ≲2^-14. Not obviously exploitable, but
  they mean the challenge/witness distributions are not exactly uniform as §6.3.1 claims.
- **Not verified (time-boxed ~20 min):** the arithmetic of `Interactive_Check_Raw_*`,
  `PolyDef_Raw`, `ProverPolynomialResponse_Final_Raw` and `VerifierCheck_Raw` was not traced
  against Algorithms 5.2/5.3 coefficient by coefficient; the tower-field reduction polynomials
  (Table 2 prints `Y^16+Y^11+Y+X` and `Y^32+Y^31+Y^5+X`, which mix Y and X indeterminates —
  presumably X-coefficient constants) were not checked against `rsencode.c`; no independent RSD
  estimator was run. Consistent with `security_findings.md`, which leaves FS transcript binding,
  domain separation and parameter selection `not_tested`.
