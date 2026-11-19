# hash-30 ZC-DM — algorithm summary

ZC-DM is **not** a Merkle–Damgård construction. It is a **sponge** (SPONGE-DM,
spec ref [1] "to appear at IACR CRYPTO 2026") in which the absorption update is
wrapped in a Davies–Meyer feed-forward `F(x) = f(x) ⊕ x` while squeezing uses
the bare permutation `f`. `f` is **ZC-m**, a "scaled-up Xoodoo" with a 3-D state,
12 rounds, round function `p = ρ ∘ θ ∘ ρ ∘ χ ∘ ι`. Two widths: ZC-1280
(5 planes × 4 lanes × 64 bits, 5-bit χ) and ZC-1536 (3 planes × 8 lanes × 64
bits, 3-bit χ). Same author team as hash-29 (XRH-2) and hash-31/32.

Specification: `hash-30-spec.pdf` (35 pages), Chapter 1 §1.1–§1.3
(Algorithms 1–3, Tables 1.1/1.2). English.

## Parameters

| parameter | ZC-DM-1280-512 | -1280-768 | -1280-1024 | -1536-512 | -1536-768 | -1536-1024 | meaning |
|---|---|---|---|---|---|---|---|
| m (state width) | 1280 | 1280 | 1280 | 1536 | 1536 | 1536 | bits |
| planes × lanes | 5×4 | 5×4 | 5×4 | 3×8 | 3×8 | 3×8 | 64-bit lanes |
| χ width | 5-bit | 5-bit | 5-bit | 3-bit | 3-bit | 3-bit | column χ |
| n (digest) | 512 | 768 | 1024 | 512 | 768 | 1024 | bits |
| c = n+64 | 576 | 832 | 1088 | 576 | 832 | 1088 | capacity |
| r = m−c | 704 | 448 | 192 | 960 | 704 | 448 | absorb rate (bits) |
| r′ | 704 | 448 | 192 | 960 | 704 | 448 | squeeze rate (= r) |
| n_r | 12 | 12 | 12 | 12 | 12 | 12 | permutation rounds |
| squeeze blocks ⌈n/r′⌉ | 1 | 2 | 6 | 1 | 2 | 3 | |
| "Sec. level" (Table 1.2) | 512 | 768 | 1024 | 512 | 768 | 1024 | spec's own column |
| collision (§3.1.1) | 256 | 384 | 512 | 256 | 384 | 512 | min{c/2, n/2} = n/2 |
| preimage (§3.1.1, Thm 2) | 512 | 768 | 1024 | 512 | 768 | 1024 | n bits |
| 2nd preimage | min{n, c−log₂α} | ← | ← | ← | ← | ← | |
| indifferentiability | c/2 = 288 | 416 | 544 | 288 | 416 | 544 | bits |

Sizes (bytes), specification vs the built reference library (OBSERVED):

| instance | digest spec | digest impl | match |
|---|---|---|---|
| ZC-DM-1280-512 / -1536-512 | 64 | 64 | yes |
| ZC-DM-1280-768 / -1536-768 | 96 | 96 | yes |
| ZC-DM-1280-1024 / -1536-1024 | 128 | 128 | yes |

## Pseudocode

### ZC-m permutation (spec Fig. 1.3, Algorithms 1 and 2)
```
ZC-1280: A[y][x], y=0..4 planes, x=0..3 lanes of 64 bits   (1280 bits)
ZC-1536: A[y][x], y=0..2 planes, x=0..7 lanes of 64 bits   (1536 bits)
A <<< (a,b) = rotate plane by a lane positions and each lane left by b bits.

for i = -11 .. 0:                                   # 12 rounds
    iota : A0[0] <- A0[0] XOR c_i                   # ONE lane only (x=0)
    chi  :  ZC-1280 (5-bit chi, on each 5-bit column):
              B_y <- ~A_{y+1} AND A_{y+2};  A_y <- A_y XOR B_y,  y=0..4
            ZC-1536 (3-bit chi, Xoodoo chi):
              B0 <- ~A1·A2; B1 <- ~A2·A0; B2 <- ~A0·A1; A_y <- A_y XOR B_y
    rho  :  ZC-1280: A1 <<< (1,5), A2 <<< (3,24), A3 <<< (0,1), A4 <<< (2,3)
            ZC-1536: A1 <<< (3,3),  A2 <<< (5,0)
    theta:  P <- XOR of all planes                  # one lane-vector
            ZC-1280: E <- P <<< (1,5)  XOR P <<< (1,14)
            ZC-1536: E <- P <<< (1,20) XOR P <<< (1,56)
            A_y <- A_y XOR E, for all y
    rho  :  same rho as above (applied a second time)
```

### Hash(M, n) — ZC-DM-m-n (spec Algorithm 3, §1.3)
```
ZC-DM-m-n(M):
  c <- n + 64;  r <- m - c;  r' <- r
  # Padding: 2-bit domain suffix "01", then multi-rate pad10*1 to a multiple of r
  M <- M || 0 || 1
  j <- smallest j >= 0 with |M| + 2 + j == 0 (mod r)
  M <- M || 1 || 0^j || 1
  parse M = M_0 || ... || M_{alpha-1},  |M_i| = r
  # Absorption: F(X) = ZC-m(X) XOR X           (full-state Davies-Meyer)
  S <- M_0 || 0^c
  for i = 1 .. alpha-1:
      S <- ZC-m(S) XOR S                       # feed-forward
      S <- S XOR (M_i || 0^c)                  # absorb
  S <- ZC-m(S) XOR S                           # final feed-forward
  # Squeezing: plain sponge, no feed-forward
  Z <- leading r' bits of S
  while |Z| < n:
      S <- ZC-m(S)
      Z <- Z || leading r' bits of S
  return leading n bits of Z
```

## Implementation vs specification

Checked: `src/ZC-DM-*/CryptHash_AlgorithmInstance.c` (≈195 lines, essentially
one file for all six instances) and the shared permutation
`ZC-DM/Implementations/lib/low/ZuD-{1280,1536}/plain/ZuD{1280,1536}-plain.c`.
Note the implementation calls the permutation **ZuD**, the spec calls it **ZC-m**;
same object. All six instances KAT-PASS (`results/summary.tsv`).

Agreements:

- `absorb_block_custom()` = `AddBytes(M)` → save `oldState` → `Permute_12rounds`
  → XOR the **full** `oldState` back. That is exactly
  `S ← ZC-m(S ⊕ (M_i‖0^c)) ⊕ (S ⊕ (M_i‖0^c))`, i.e. spec Algorithm 3 lines 7–11
  with the loop re-associated (absorb-then-F each block, starting from 0^m).
- `squeeze_bytes()` uses `rateBytes` (r′ = r) and calls `permute()` = 12 rounds
  *between* extractions, never before the first — spec Alg. 3 lines 13–16.
- Padding: `append_bit_msb_custom(0)`, `(1)` = the `01` suffix; then `1`, zeros,
  and `xor_bit_msb(block, r-1, 1)` = pad10*1. MSB-first bit order. Matches
  Eq. in §1.3 and Alg. 3 lines 2–4, including the extra-block case.
- `capacity = digest_len_bits + 64` restricted to {576, 832, 1088}, so
  `r = m − (n+64)` — Table 1.2 values 704/448/192 (1280) and 960/704/448 (1536)
  are reproduced exactly.
- ZuD-1280 `chi()` uses saved `b0..b4` (true parallel χ) — matches Algorithm 1.
  ZuD-1280 `rho()` lane permutations correspond to `(1,5) (3,24) (0,1) (2,3)`,
  `theta()` to `E[x] = P[x−1]<<<5 ⊕ P[x−1]<<<14`; ZuD-1536 `rho()` to `(3,3)`
  and `(5,0)`, `theta()` to `P[x−1]<<<20 ⊕ P[x−1]<<<56`. All as specified.
- `iota()` XORs `A[0]` only (plane 0, lane x = 0) — matches the spec's own prose
  ("A0[0] denotes the lane at position x = 0 … all other lanes are unchanged").

Discrepancies:

- **(a) Real deviation — round-constant schedule (recorded as hash-30-4).** Spec Table 1.1 lists
  `c_{-11..0} = 58, 38, 3C0, D0, 60, 120, 14, 2C, F0, 1A0, 380, 12`.
  The implementation (`lib/low/ZuD-1280/ZuD1280.h` and `ZuD-1536/ZuD1536.h`,
  applied by `Permute_12rounds` in the order rc12 → rc1) uses
  `58, 38, 3C0, D0, 120, 14, 60, 2C, 380, F0, 1A0, 12`, i.e. the original
  **Xoodoo** constant sequence (the header says so: "derived from Xoodoo 32-bit
  constants, zero-extended"). Six of the twelve positions differ (rounds
  i = −7, −6, −5 and −3, −2, −1 are a cyclic rotation of the spec's entries).
  Consequence: an implementation written from the spec text alone will **not**
  reproduce the submitted KATs. The same Table 1.1 (and therefore the same
  discrepancy) appears in the hash-31 (ZC-DMC) and hash-32 (ZC-EDMC) specs.
  Cryptanalytic impact is expected to be negligible (constant order rarely
  matters), but the spec does not define the implemented function.
- **(c) Deliberate equivalent optimisation.** ZuD-1536 `chi()` updates
  `a0, a1, a2` in place and sequentially rather than from saved copies. I
  checked all 8 input patterns: the in-place 3-bit χ is *identical* to the
  parallel χ of Algorithm 2 (the standard Xoodoo in-place trick). Not a bug.
- **(b) Spec presentation.** Table 1.2's "Sec. level" column equals `n`
  (512/768/1024). That is the *preimage* level; the collision level stated in
  §3.1.1 is `min{2^{c/2}, 2^{n/2}}` = `n/2` bits (256/384/512). The column
  heading is easy to misread as a collision claim.
- (cosmetic) `DIGEST_BIT_LENGTH` is never read by the `.c` file; the digest
  length (and hence the rate) comes from the `digest_len_bits` argument. All six
  shipped libraries of a given width are therefore the same function; the six
  instance directories differ only in `ALGORITHM_INSTANCE`/`DIGEST_BIT_LENGTH`
  plus the 1280/1536 include. Stale comments remain ("SHA3-like rule: output =
  capacity/2", a dead `suffix = 0x06`, several commented-out blocks).

### Length extension — the key check

**ZC-DM is not length-extendable, and the premise of "a plain Davies–Meyer
Merkle–Damgård chain without finalization" does not apply to it.** The
construction is a sponge, verified in the code:

- the state is `m` bits (1280/1536) of which the trailing `c = n+64` capacity
  bits are never output and never directly injected — `absorb_block_custom()`
  XORs the message only over `rateBytes`, and `squeeze_bytes()` extracts only
  from offset 0 up to `rateBytes`;
- the digest is `n` bits while the rate is `r = m − n − 64`, so even for the
  single-squeeze cases (n=512) at least `c = 576` capacity bits plus `r − n`
  rate bits remain unknown to the attacker;
- absorption is additionally non-invertible because of the DM feed-forward.

The spec does claim length-extension resistance explicitly (§3.5.1 "Length
Extension Attack Resistance") and also claims `c/2` indifferentiability
(§3.1.1). Both claims are consistent with the implemented construction —
**this is not a finding.** No finalization or counter is needed or present.

### What DM / DMC / EDMC change relative to each other

| | absorption mapping `F(X)`, `X = X_r‖X_c` | squeeze rate r′ | rounds per absorbed block |
|---|---|---|---|
| ZC-DM (hash-30) | `ZC-m(X) ⊕ X` (feed-forward over the **full** m bits) | r | 12 |
| ZC-DMC (hash-31) | `ZC-m(X) ⊕ (0^r ‖ X_c)` (feed-forward over the **capacity only**) | **64** | 12 |
| ZC-EDMC (hash-32) | `h(g(X) ⊕ (0^r ‖ X_c))` (feed-forward **in the middle**, capacity only) | r | 12 (6+6) |

Padding, initialisation, capacity `c = n+64` and the squeezing permutation are
identical in all three. See `hash-31/pseudocode.md` and `hash-32/pseudocode.md`.
