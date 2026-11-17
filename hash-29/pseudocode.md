# hash-29 XRH-2 — algorithm summary

XRH-2 is a permutation-based hash family built on the **SPONGE-DM** construction
(a member of the SPONGE-EDM family, spec ref [1] "to appear at CRYPTO 2026"):
an ordinary sponge whose *absorption* update is replaced by the Davies–Meyer
one-way map `F(x) = f(x) ⊕ x`, while *squeezing* uses the bare permutation `f`.
`f` is **XRH-p**, a 1280-bit, 18-round wide-trail SPN over a 20×64 bit matrix
with a 5-bit almost-bent S-box and a 4×4 MDS matrix over GF(2^5).
Sibling of hash-28 (XRH-1); same submitter team (Sun et al., UCAS).

Specification: `hash-29-spec.pdf` (34 pages), Chapter 1 §1.1–§1.3
(Algorithms 1–4, Table 1.1). English.

## Parameters

| parameter | XRH-2-512 | XRH-2-768 | XRH-2-1024 | meaning |
|---|---|---|---|---|
| b | 1280 | 1280 | 1280 | permutation width (20 lanes × 64 bits) |
| n | 512 | 768 | 1024 | digest bits |
| c = n+64 | 576 | 832 | 1088 | capacity bits |
| r = b−c | 704 | 448 | 192 | absorption rate (= 88/56/24 bytes) |
| r′ | 704 | 448 | 192 | squeezing rate (spec: r′ = r) |
| R | 18 | 18 | 18 | XRH-p rounds |
| S-box | 5-bit AB, δ=2, lin.bias 4 | ← | ← | same for all |
| squeeze blocks β=⌈n/r′⌉ | 1 | 2 | 6 | |
| claimed collision | 256 | 384 | 512 | bits, Table 1.1 (= min{n/2, c/2}) |
| claimed preimage | 512 | 768 | 1024 | bits, Table 1.1 (= n, "no c/2 barrier") |
| claimed 2nd preimage | 512 | 768 | 1024 | bits (= min{n, c − log2 α}) |
| claimed indifferentiability | c/2 = 288 | 416 | 544 | §3.2.1 |

Sizes (bytes), specification vs the built reference library (OBSERVED):

| instance | digest spec | digest impl | match |
|---|---|---|---|
| XRH-2-512 | 64 | 64 | yes |
| XRH-2-768 | 96 | 96 | yes |
| XRH-2-1024 | 128 | 128 | yes |

## Pseudocode

### XRH-p permutation f (spec Algorithm 3, §1.2.2)
```
State: S[0..19][0..63] (20 rows × 64 columns = 1280 bits), row i = one 64-bit lane.
A "column" j is 20 bits = four 5-bit nibbles S[5i..5i+4][j], i = 0..3.

f(S):
  for r = 0 .. 17:
      # Super S-box layer S' = S^{⊗4} o M o S^{⊗4}, applied to all 64 columns
      for j = 0..63: for i = 0..3: S[5i:5i+4][j] <- Sbox(S[5i:5i+4][j])
      for j = 0..63: S[*][j] <- M · S[*][j]            # MDS over F_2[x]/(x^5+x^2+1)
      for j = 0..63: for i = 0..3: S[5i:5i+4][j] <- Sbox(S[5i:5i+4][j])
      # Super MixColumn M' = SR^{-1} o M o SR  (inter-column diffusion)
      SR <- SR_X if r mod 3 = 0;  SR_Y if r mod 3 = 1;  SR_Z otherwise
      S <- SR(S);  for j: S[*][j] <- M · S[*][j];  S <- SR^{-1}(S)
      # Round constant
      S[0][*] <- S[0][*] XOR RC_r          # 64 bits of frac(pi), row 0 only
  return S

Sbox = [4,14,10,5,9,24,20,0,18,28,30,21,2,23,29,13,
        22,27,15,7,26,12,16,3,8,1,19,31,25,11,17,6]   # 5-bit, almost bent
M    = [[3,2,1,3],[3,1,4,4],[1,3,6,4],[2,2,3,1]]      # applied as b3..b0 = M·(a3..a0)
SR_X: shift within groups of 4 columns by floor(i/5)
SR_Y: shift within groups of 16 columns by 4*floor(i/5)
SR_Z: global rotate of 64 columns by 16*floor(i/5)
```

### Hash(M, n) — XRH-2-n (spec Algorithm 4, §1.3)
```
XRH-2-n(msg):                                  # n in {512,768,1024}
  c <- n + 64;  r <- 1280 - c
  m <- msg || Pad10*1(r, |msg|)                # msg || 1 || 0^k || 1, k minimal
  parse m = (m_1,...,m_alpha), |m_i| = r
  state <- 0^1280
  for i = 1 .. alpha:                          # absorption, DM mode
      state  <- state XOR (m_i || 0^c)
      state' <- state
      state  <- f(state) XOR state'            # F(x) = f(x) XOR x, non-invertible
  z <- Trunc_r(state)                          # first squeeze block, no extra f
  while |z| < n:                               # squeezing, plain sponge
      state <- f(state)
      z <- z || Trunc_r(state)
  return Trunc_n(z)
```
Note the asymmetry: the DM feed-forward is applied **only** in absorption; the
squeezing permutation calls are plain `f` (spec Alg. 4 lines 12–13).

## Implementation vs specification

Checked: `src/XRH-2-512/CryptHash_AlgorithmInstance.{c,h}` (296 + 52 lines) and
the 768/1024 instances. The three instance directories are **byte-identical
except for `ALGORITHM_INSTANCE` and `DIGEST_BIT_LENGTH`**; rate and capacity are
derived by macro, so one code path serves all three.

Agreements:

- `STATE_LANES 20`, `STATE_CAPACITY (DIGEST_BIT_LENGTH+64)`,
  `STATE_RATE (20*64 - DIGEST_BIT_LENGTH - 64)` reproduce Table 1.1 exactly
  (704/448/192 bits; all byte-aligned, so the byte-wise absorb loop is exact).
- `XRH1280(0, 18, buf)` iterates the round function 18 times = spec R = 18;
  per-round order Sbox → MDS → Sbox → SR_{X|Y|Z} → MDS → SR^{-1} → `x0 ^= RC_r`
  matches Algorithm 3 line-for-line, including the `r mod 3` shift-row schedule
  and the round constant landing only in row 0 (lane `x0`).
- `ROUND_CONSTANTS[0] = 0x243f6a8885a308d3` is frac(π) as the spec states.
- `XRH_dm_permute()` = `f(x) ⊕ x` (copy, 18 rounds, XOR back) — spec Eq. 1.3;
  the squeeze loop calls `XRH1280(0,18,...)` **without** the feed-forward —
  spec Alg. 4 line 12. Correct DM/plain split.
- Padding: `state_bytes[length] ^= 0x80 >> tail_bits` places the leading `1`
  (MSB-first bit order), `state_bytes[rate_bytes-1] ^= 0x01` the trailing `1`
  — pad10*1 as in Eq. 1.8. The `msg_len_bits % rate == rate-1` branch correctly
  spills the trailing `1` into an extra block. A message that is an exact
  multiple of `r` gets a full extra `10^{r-2}1` block. Verified by reading, not
  by execution beyond the KAT PASS already recorded in RESULTS.md (3/3 PASS).
- First squeeze block is read from the post-absorption state with no extra
  permutation (Alg. 4 line 10). For n=512 one 704-bit block is truncated to 512;
  for n=1024 six 192-bit blocks (1152 bits) are truncated to 1024.

Observations / minor discrepancies (none security-affecting as built):

- (c, cosmetic) `ROUND_CONSTANTS[]` has **24** entries but only 18 are used;
  the extra six are dead. `XRH1280(R0,R1,...)` also exposes a round range,
  suggesting the `f = h ∘ g` split mentioned in §1.1 Eq. (1.6); §1.3 fixes
  `f` to the full 18 rounds and both call sites use `(0,18)`, so spec and code
  agree — but the spec's "f is typically instantiated as h ∘ g" wording in
  §1.1/Alg. 4 line 12 is a leftover from the generic mode and is **ambiguous**
  in isolation.
- (b, API ambiguity) `CryptHash()` honours its `digest_len_bits` argument for
  the squeeze length but takes the *rate* from the compile-time
  `DIGEST_BIT_LENGTH`. Calling the 512 library with `digest_len_bits = 768`
  therefore yields "768 bits squeezed at rate 704", which is not any specified
  XRH-2 variant. The NGCC harness only ever calls it with `DIGEST_BIT_LENGTH`,
  so this does not affect the KATs.
- Domain separation between digest lengths is **implicit**: each n uses a
  different rate/capacity split (c = n+64), so the three variants absorb
  differently from the first block onward. There is no explicit domain constant,
  which matches SHA-3 practice and is adequate here.
- Length extension: not applicable in the exploitable sense — the squeezed
  output never reveals the full state (for n=512, 192 rate bits plus 576
  capacity bits stay hidden; for n=1024 the 1088-bit capacity stays hidden), and
  absorption is additionally non-invertible thanks to the DM feed-forward. The
  spec's indifferentiability claim (c/2, §3.2.1) is the usual sponge bound and
  is not contradicted by the implementation.
- **Verified by exhaustive evaluation** (macros extracted into a standalone
  harness, no submission file touched):
  - `sbox_hw25_bitslice64_withtemp()` reproduces the spec's 32-entry S-box table
    exactly for all 32 inputs, with the bit order `x0` = LSB, i.e. the spec's
    `a_i = S[5i][j] + 2·S[5i+1][j] + 4·S[5i+2][j] + 8·S[5i+3][j] + 16·S[5i+4][j]`.
  - the `MDS()` macro equals `(b3,b2,b1,b0)^T = M · (a3,a2,a1,a0)^T` over
    `F_2[x]/(x^5+x^2+1)` with the spec's M, checked over all 2^20 column values.
    `MUL()` is multiplication by x in that field.
- Not verified line-by-line: the `SR_*` / `SR_*_INV` bit-permutation masks
  against the three shift-row definitions (they are self-consistent — each
  `SR_x_INV` undoes its `SR_x` — and the shift amounts 1/2/3 nibbles, 4/8/12
  bits and 16/32/48 bits match `floor(i/5) ∈ {1,2,3}` scaled by 1, 4 and 16
  columns respectively, as the spec requires).

## Review of 2026-11-17

A model written from the specification equals `XRH1280` on random states and the full hash for every bit length from 0 to 2r+70. The S-box (almost bent, degree 2, inverse degree 3), the MDS matrix, the shift-row patterns and all 24 round constants were recomputed and match. Table 3.1's active S-box counts for 1 to 8 rounds were confirmed, the first five exactly. Two analysis statements do not hold: all single-cell impossible differentials exist over 5 rounds, and one round has degree 4 with inverse degree 9; filed as `hash-29-2`. The reference `CryptHash` block count wraps for `msg_len_bits ≥ 2^64 − r`, giving a constant digest without reading the message; filed as `hash-29-1`. The reference and AVX2 trees otherwise agree for all bit lengths from 0 to 4r+70.
