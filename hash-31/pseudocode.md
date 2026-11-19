# hash-31 ZC-DMC — algorithm summary

ZC-DMC is **not** a Merkle–Damgård construction. It is the **SPONGE-DMc** sponge
variant: an ordinary sponge whose absorption update is
`F(X) = ZC-m(X) ⊕ (0^r ‖ X_c)` — a Davies–Meyer feed-forward **restricted to the
capacity (inner) part** — while squeezing uses the bare permutation. The
squeezing rate is decoupled from the absorption rate and fixed at **r′ = 64**.
`f` is **ZC-m**, the same "scaled-up Xoodoo" as hash-30/hash-32: 12 rounds,
`p = ρ ∘ θ ∘ ρ ∘ χ ∘ ι`, widths 1280 (5 planes × 4 lanes, 5-bit χ) and 1536
(3 planes × 8 lanes, 3-bit χ).

Specification: `hash-31-spec.pdf` (37 pages), Chapter 1 §1.1–§1.3
(Algorithms 1–3, Tables 1.1/1.2). English.

## Parameters

| parameter | ZC-DMC-1280-512 | -1280-768 | -1280-1024 | -1536-512 | -1536-768 | -1536-1024 | meaning |
|---|---|---|---|---|---|---|---|
| m (state width) | 1280 | 1280 | 1280 | 1536 | 1536 | 1536 | bits |
| planes × lanes | 5×4 | 5×4 | 5×4 | 3×8 | 3×8 | 3×8 | 64-bit lanes |
| χ width | 5-bit | 5-bit | 5-bit | 3-bit | 3-bit | 3-bit | |
| n (digest) | 512 | 768 | 1024 | 512 | 768 | 1024 | bits |
| c = n+64 | 576 | 832 | 1088 | 576 | 832 | 1088 | capacity |
| r = m−c | 704 | 448 | 192 | 960 | 704 | 448 | absorb rate |
| **r′** | **64** | **64** | **64** | **64** | **64** | **64** | squeeze rate (Table 1.2) |
| feed-forward width | c = 576 | 832 | 1088 | 576 | 832 | 1088 | inner part only |
| n_r | 12 | 12 | 12 | 12 | 12 | 12 | permutation rounds |
| squeeze permutation calls t = ⌈n/r′⌉−1 | 7 | 11 | 15 | 7 | 11 | 15 | |
| "Sec. level" (Table 1.2) | 512 | 768 | 1024 | 512 | 768 | 1024 | spec's own column |
| collision (§3.1.1) | 256 | 384 | 512 | 256 | 384 | 512 | min{c/2, n/2} |
| preimage | 512 | 768 | 1024 | 512 | 768 | 1024 | n bits |
| 2nd preimage | min{n, c−log₂α} | ← | ← | ← | ← | ← | |
| indifferentiability | c/2 = 288 | 416 | 544 | 288 | 416 | 544 | bits |

Sizes (bytes), specification vs the built reference library (OBSERVED):

| instance | digest spec | digest impl | match |
|---|---|---|---|
| ZC-DMC-1280-512 / -1536-512 | 64 | 64 | yes |
| ZC-DMC-1280-768 / -1536-768 | 96 | 96 | yes |
| ZC-DMC-1280-1024 / -1536-1024 | 128 | 128 | yes |

## Pseudocode

### ZC-m permutation
Identical to hash-30; see `hash-30/pseudocode.md` for the full round function
(`ι` on lane A0[0], column `χ`, plane-shift `ρ`, column-parity mixer `θ`, `ρ`;
12 rounds, spec Algorithms 1 and 2, constants Table 1.1).

### Hash(M, n) — ZC-DMC-m-n (spec Algorithm 3, §1.3)
```
ZC-DMC-m-n(M):
  c <- n + 64;  r <- m - c;  r' <- 64
  # Padding: identical to ZC-DM
  M <- M || 0 || 1                              # 2-bit domain suffix "01"
  j <- smallest j >= 0 with |M| + 2 + j == 0 (mod r)
  M <- M || 1 || 0^j || 1                       # multi-rate pad10*1
  parse M = M_0 || ... || M_{alpha-1},  |M_i| = r
  # Absorption: F(X) = ZC-m(X) XOR (0^r || X_c)     <-- INNER feed-forward only
  S <- M_0 || 0^c
  for i = 1 .. alpha-1:
      S <- F(S)
      S <- S XOR (M_i || 0^c)
  S <- F(S)                                     # final DM mapping
  # Squeezing: plain sponge at the SMALL rate r' = 64
  Z <- leading 64 bits of S
  while |Z| < n:
      S <- ZC-m(S)
      Z <- Z || leading 64 bits of S
  return leading n bits of Z
```

Rationale given in §1.3: `r′ = 64` "matches the security gap `c − n = 64`". The
spec's Fig. 1.5 works out the best generic preimage attack for SPONGE-DMc as
`2^{c/2}` when `min{r, r′} ≥ n` and `2^{(c + max{n−r,0})/2}` otherwise, which is
why `r′` must be kept small rather than equal to `r`.

## Implementation vs specification

Checked: `src/ZC-DMC-*/CryptHash_AlgorithmInstance.c` (≈195 lines, one file for
all six instances) and the shared `ZuD{1280,1536}-plain.c`. All six instances
KAT-PASS (`results/summary.tsv`). The file is a one-line edit of the ZC-DM file
in two places, which is exactly where the construction differs.

Agreements:

- `absorb_block_custom()`: `AddBytes(M)` → save `oldState` → `Permute_12rounds`
  → `AddBytes(state, rateBytes + oldState, rateBytes, WIDTH_BYTES - rateBytes)`.
  That last call XORs `oldState[r..m)` into `state[r..m)`, i.e. exactly
  `F(X) = ZC-m(X) ⊕ (0^r ‖ X_c)` — the **inner-only** feed-forward of §1.3.
  (Contrast hash-30, which XORs the full `WIDTH_BYTES`.)
- `unsigned int rateOutBytes = 8u;` and `squeeze_bytes(&st, rateOutBytes, …)`
  implement `r′ = 64` bits, decoupled from `rateBytes`. This matches Table 1.2
  and is the second of the two differences from ZC-DM.
- Absorption loop, padding (`01` suffix then pad10*1, MSB-first), zero
  initialisation, `capacity = n + 64` restricted to {576, 832, 1088}, and the
  derived rates 704/448/192 (1280) and 960/704/448 (1536) all match Table 1.2.
- ZuD-1280/1536 `chi`, `rho`, `theta`, `iota` match Algorithms 1 and 2 (same
  verification as hash-30; ZuD-1536's in-place 3-bit χ is equivalent to the
  parallel form — checked over all 8 inputs).

Discrepancies:

- **(a) Real deviation — round-constant schedule (recorded as hash-31-5).** Inherited from the shared
  permutation and from the identical Table 1.1 printed in this spec. Spec
  Table 1.1: `c_{-11..0} = 58, 38, 3C0, D0, 60, 120, 14, 2C, F0, 1A0, 380, 12`;
  implementation (`ZuD1280.h` / `ZuD1536.h`, applied rc12 → rc1):
  `58, 38, 3C0, D0, 120, 14, 60, 2C, 380, F0, 1A0, 12` — the original Xoodoo
  sequence. Six of twelve positions differ. Code written from the spec text
  will not reproduce the submitted KATs. Same issue in hash-30 and hash-32.
- **(b) Spec presentation.** Table 1.2's "Sec. level" column equals `n`; that is
  the preimage level, while collision resistance per §3.1.1 is `n/2` bits.
- (cosmetic) `DIGEST_BIT_LENGTH` is unused by the `.c`; the digest length and
  hence the rate come from the caller's `digest_len_bits`. The six shipped
  libraries of a given width are the same function.

### Length extension — the key check

**ZC-DMC is not length-extendable; the "plain Davies–Meyer Merkle–Damgård chain"
premise does not apply.** It is a sponge:

- the trailing `c = n + 64` capacity bits are never written by the message
  (`AddBytes(state, block, 0, rateBytes)`) and never extracted
  (`ExtractBytes(state, out, 0, take)` with `take ≤ 8`);
- with `r′ = 64`, each squeeze step reveals only 64 of the `m` state bits, so
  after producing the `n`-bit digest at least `m − 64` bits of the final state
  are unknown; the small `r′` makes this *more* conservative than ZC-DM;
- the absorption mapping `F` is non-invertible (`c`-bit feed-forward), so even
  a guessed state cannot be rolled back cheaply.

The spec claims length-extension resistance explicitly (§3.5.1) and `c/2`
indifferentiability (§3.1.1). Both are consistent with the implementation —
**not a finding.** There is no finalization step and no counter, and none is
required by the sponge structure.

### What DM / DMC / EDMC change relative to each other

| | absorption mapping `F(X)`, `X = X_r‖X_c` | squeeze rate r′ | rounds per absorbed block |
|---|---|---|---|
| ZC-DM (hash-30) | `ZC-m(X) ⊕ X` (full-width feed-forward) | r | 12 |
| **ZC-DMC (hash-31)** | `ZC-m(X) ⊕ (0^r ‖ X_c)` (**capacity-only** feed-forward) | **64** | 12 |
| ZC-EDMC (hash-32) | `h(g(X) ⊕ (0^r ‖ X_c))` (feed-forward **in the middle**) | r | 12 (6+6) |

Padding, initialisation, `c = n+64` and the squeezing permutation are identical
in all three. DMC's two changes (inner-only feed-forward, `r′ = 64`) buy a
cheaper feed-forward — the design argument in §1.3 is that the feed-forward may
shrink to `c` bits provided `r > r′ = c − n` — at the cost of `n/64` squeezing
permutation calls instead of `⌈n/r⌉`.
