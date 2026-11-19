# hash-32 ZC-EDMC — algorithm summary

ZC-EDMC is **not** a Merkle–Damgård construction. It is the **SPONGE-EDMc**
sponge variant: the absorption update is the *Encrypted* Davies–Meyer mapping
`F(X) = h(g(X) ⊕ (0^r ‖ X_c))`, where the 12-round permutation ZC-m is split as
`ZC-m = h ∘ g` into two 6-round halves and the capacity part of the input is fed
forward **in the middle**. Squeezing uses the full unmodified 12-round ZC-m at
rate `r′ = r`. `f` is the same "scaled-up Xoodoo" ZC-m as hash-30/31: widths
1280 (5 planes × 4 lanes, 5-bit χ) and 1536 (3 planes × 8 lanes, 3-bit χ).

Specification: `hash-32-spec.pdf` (37 pages), Chapter 1 §1.1–§1.3
(Algorithms 1–3, Tables 1.1/1.2). English.

## Parameters

| parameter | ZC-EDMC-1280-512 | -1280-768 | -1280-1024 | -1536-512 | -1536-768 | -1536-1024 | meaning |
|---|---|---|---|---|---|---|---|
| m (state width) | 1280 | 1280 | 1280 | 1536 | 1536 | 1536 | bits |
| planes × lanes | 5×4 | 5×4 | 5×4 | 3×8 | 3×8 | 3×8 | 64-bit lanes |
| χ width | 5-bit | 5-bit | 5-bit | 3-bit | 3-bit | 3-bit | |
| n (digest) | 512 | 768 | 1024 | 512 | 768 | 1024 | bits |
| c = n+64 | 576 | 832 | 1088 | 576 | 832 | 1088 | capacity |
| r = m−c | 704 | 448 | 192 | 960 | 704 | 448 | absorb rate |
| r′ | 704 | 448 | 192 | 960 | 704 | 448 | squeeze rate (= r) |
| a (feed-forward width) | c = 576 | 832 | 1088 | 576 | 832 | 1088 | special case a = c |
| n_r | 12 (6+6) | 12 | 12 | 12 | 12 | 12 | permutation rounds |
| squeeze blocks ⌈n/r′⌉ | 1 | 2 | 6 | 1 | 2 | 3 | |
| "Sec. level" (Table 1.2) | 512 | 768 | 1024 | 512 | 768 | 1024 | spec's own column |
| collision (§3.1.1) | 256 | 384 | 512 | 256 | 384 | 512 | min{c/2, n/2} |
| preimage | 512 | 768 | 1024 | 512 | 768 | 1024 | O(2^n), §3.1.1 |
| 2nd preimage | min{n, c−log₂α} | ← | ← | ← | ← | ← | |
| indifferentiability | c/2 = 288 | 416 | 544 | 288 | 416 | 544 | bits |

Sizes (bytes), specification vs the built reference library (OBSERVED):

| instance | digest spec | digest impl | match |
|---|---|---|---|
| ZC-EDMC-1280-512 / -1536-512 | 64 | 64 | yes |
| ZC-EDMC-1280-768 / -1536-768 | 96 | 96 | yes |
| ZC-EDMC-1280-1024 / -1536-1024 | 128 | 128 | yes |

## Pseudocode

### ZC-m permutation
Identical to hash-30; see `hash-30/pseudocode.md` for the full round function
(`ι` on lane A0[0], column `χ`, plane-shift `ρ`, column-parity mixer `θ`, `ρ`;
12 rounds, spec Algorithms 1 and 2, constants Table 1.1). Here it is additionally
split: `g` = the first six rounds (constants `c_{-11} … c_{-6}`), `h` = the last
six rounds (constants `c_{-5} … c_0`), so that `ZC-m = h ∘ g` (spec §1.3).

### Hash(M, n) — ZC-EDMC-m-n (spec Algorithm 3, §1.3)
```
ZC-EDMC-m-n(M):
  c <- n + 64;  r <- m - c;  r' <- r
  # Padding: identical to ZC-DM / ZC-DMC
  M <- M || 0 || 1                              # 2-bit domain suffix "01"
  j <- smallest j >= 0 with |M| + 2 + j == 0 (mod r)
  M <- M || 1 || 0^j || 1                       # multi-rate pad10*1
  parse M = M_0 || ... || M_{alpha-1},  |M_i| = r
  # Absorption: F(X) = h( g(X) XOR (0^r || X_c) )    <-- EDM, inner feed-forward
  S <- M_0 || 0^c
  for i = 1 .. alpha-1:
      S <- F(S)
      S <- S XOR (M_i || 0^c)
  S <- F(S)                                     # final EDM mapping
  # Squeezing: plain sponge with the FULL 12-round ZC-m
  Z <- leading r' bits of S
  while |Z| < n:
      S <- ZC-m(S)
      Z <- Z || leading r' bits of S
  return leading n bits of Z
```

## Implementation vs specification

Checked: `src/ZC-EDMC-*/CryptHash_AlgorithmInstance.c` (≈195 lines, one file for
all six instances) and the shared
`ZC-EDMC/Implementations/lib/low/ZuD-{1280,1536}/plain/ZuD*-plain.c`.
All six instances KAT-PASS (`results/summary.tsv`).

Agreements:

- `absorb_block_custom()`: `AddBytes(M)` → save `oldState` → `permute_half()` →
  `AddBytes(state, rateBytes + oldState, rateBytes, WIDTH_BYTES - rateBytes)` →
  `permute_half()`. Structurally this is the EDM mapping of §1.3: half-permute,
  XOR the **capacity part** of the input in the middle, half-permute.
- `squeeze_bytes(&st, rateBytes, …)` with `permute()` = `Permute_12rounds`:
  `r′ = r` and the full 12-round permutation during squeezing — Alg. 3 lines
  13–16 and Table 1.2.
- Padding (`01` suffix then pad10*1, MSB-first bit order), zero initialisation,
  `capacity = n + 64` restricted to {576, 832, 1088} and the derived rates
  704/448/192 (1280) and 960/704/448 (1536) match Table 1.2 exactly.
- ZuD `chi`, `rho`, `theta`, `iota` match Algorithms 1 and 2 (same verification
  as hash-30; ZuD-1536's in-place 3-bit χ is equivalent to the parallel form).

Discrepancies:

- **(a) Real deviation — the two halves are not `h ∘ g`.** Spec §1.3 states
  "ZC-m is decomposed into two 6-round halves `g` and `h` such that
  `ZC-m = h ∘ g`, where `g` uses round constants `c_{-11}, …, c_{-6}` and `h`
  uses `c_{-5}, …, c_0`", and Algorithm 3 line 9 annotates
  `F(X) = h(g(X) ⊕ (0^r ‖ X_c))`. The implementation's `permute_half()` is
  `ZuD{1280,1536}_plain_Permute_6rounds()`, which is hard-coded to the
  constants `rc6, rc5, rc4, rc3, rc2, rc1` — i.e. the **second** half `h` — and
  it is called **twice**. The implemented absorption is therefore
  `F(X) = h(h(X) ⊕ (0^r ‖ X_c))`, not `h(g(X) ⊕ …)`.
  Notably, the inline display formula on spec p. 13 literally reads
  `F(X) = h( h(X) ⊕ (0^r ‖ X_c) )` — so the spec is **internally inconsistent**:
  its prose sentence, its Algorithm 3 annotation and its §2.1 rationale
  (`F(x) = h(g(x) ⊕ 0^r‖⌊x⌋_c)`) all say `h ∘ g`, while one display equation and
  the reference code say `h ∘ h`. The implementation follows the typo.
  Consequences: (i) the composition used in absorption is *not* a decomposition
  of ZC-m, so the "reuse the same permutation" rationale of §2.1 does not hold
  as written; (ii) the same 6-round constant sequence is applied twice per
  absorbed block, giving the absorption mapping a self-similar structure that
  the spec's analysis does not cover; (iii) an implementation written from
  Algorithm 3 will not reproduce the submitted KATs.
- **(a) Real deviation — round-constant schedule (recorded as hash-32-4).** As in hash-30/31: spec
  Table 1.1 gives `c_{-11..0} = 58, 38, 3C0, D0, 60, 120, 14, 2C, F0, 1A0, 380,
  12`, the implementation uses the Xoodoo order `58, 38, 3C0, D0, 120, 14, 60,
  2C, 380, F0, 1A0, 12`. Six of twelve positions differ. This compounds with the
  previous item: under the spec's table, `h` would be `14, 2C, F0, 1A0, 380, 12`,
  whereas `Permute_6rounds` applies `60, 2C, 380, F0, 1A0, 12`.
- **(b) Spec presentation.** Table 1.2's "Sec. level" column equals `n`; that is
  the preimage level, while collision resistance per §3.1.1 is `n/2` bits. The
  `a ≥ …` values quoted in the §1.3 examples (e.g. `a ≥ 448` for
  ZC-1280-EDMc[576,512]) are lower bounds on the feed-forward width; the shipped
  instances all use `a = c`.
- (cosmetic) `DIGEST_BIT_LENGTH` is unused by the `.c`; digest length and rate
  come from the caller's `digest_len_bits`. The six shipped libraries of a given
  width are the same function.

### Length extension — the key check

**ZC-EDMC is not length-extendable; the "plain Davies–Meyer Merkle–Damgård chain"
premise does not apply.** It is a sponge:

- the trailing `c = n + 64` capacity bits are never written by the message
  (`AddBytes(state, block, 0, rateBytes)`) and never extracted
  (`ExtractBytes(state, out, 0, take)` with `take ≤ rateBytes`);
- the digest is `n` bits of an `m`-bit state, so `c = n + 64` capacity bits plus
  `r − (n mod r)` rate bits are never revealed;
- absorption is non-invertible: the middle feed-forward of `X_c` makes `F` a
  `c`-bit-compressing one-way map rather than a permutation.

The spec claims length-extension resistance explicitly (§3.5.1) and `c/2`
indifferentiability (§3.1.1). Both are consistent with the implemented sponge —
**not a finding.** No finalization step or counter exists, and none is needed.
(The `h ∘ h` deviation above does not affect this: it changes which 12-round
function is used inside absorption, not the rate/capacity separation.)

### What DM / DMC / EDMC change relative to each other

| | absorption mapping `F(X)`, `X = X_r‖X_c` | squeeze rate r′ | rounds per absorbed block |
|---|---|---|---|
| ZC-DM (hash-30) | `ZC-m(X) ⊕ X` (full-width feed-forward, after 12 rounds) | r | 12 |
| ZC-DMC (hash-31) | `ZC-m(X) ⊕ (0^r ‖ X_c)` (capacity-only, after 12 rounds) | 64 | 12 |
| **ZC-EDMC (hash-32)** | `h(g(X) ⊕ (0^r ‖ X_c))` (capacity-only, **after 6 rounds**, then 6 more) | r | 12 (6+6) |

Padding, initialisation, `c = n + 64` and the squeezing phase (full ZC-m) are
identical in all three. EDMC's point is that the feed-forward is *encrypted* by
the second half of the permutation, which the spec argues raises the preimage
bound to `O(2^n)` while keeping exactly one permutation's worth of work per
block; DMC instead buys a narrower feed-forward at the price of a 64-bit
squeezing rate; DM is the plainest of the three.
