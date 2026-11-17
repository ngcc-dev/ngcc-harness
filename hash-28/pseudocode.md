# hash-28 XRH-1 — algorithm summary

XRH-1 is a **SPONGE-EDMc** hash family: a sponge whose *absorption* update is
not a permutation but the one-way Encrypted-Davies-Meyer function
`F(x) = h(g(x) ⊕ ⌊x⌋_(c))` with only the c-bit capacity fed forward, while
*squeezing* uses the ordinary permutation `f = h ∘ g`. The permutation
**XRH-p** is 1280 bits (a 20 × 64 bit matrix), 18 rounds, built in the AES wide-
trail style from a 5-bit Almost-Bent S-box, a 4 × 4 MDS matrix over
F₂[x]/(x⁵+x²+1), and a Super-Mixcolumn `SR⁻¹ ∘ M ∘ SR` with a rotating choice of
three shift-row patterns.

Specification: `hash-28-spec.pdf` (34 pages), §1.1 (mode), §1.2 (permutation),
§1.3 (instances), §3 (security).

## Parameters

| parameter | XRH-1-512 | XRH-1-768 | XRH-1-1024 | meaning |
|---|---|---|---|---|
| width b | 1280 | 1280 | 1280 | bits, 20 rows × 64 columns |
| capacity c | 576 | 832 | 1088 | bits, **c = n + 64** |
| rate r = r′ | 704 | 448 | 192 | bits, r = 1280 − n − 64 |
| digest n | 512 | 768 | 1024 | bits |
| rounds of XRH-p | 18 | 18 | 18 | §1.2.2, `R = 18` |
| g / h split | Π[0,9) / Π[9,18) | idem | idem | Eq. 1.7 |
| f (squeeze) | Π[0,18) | idem | idem | f = h ∘ g |
| S-box | 5-bit, AB, DU = 2, max linear bias 4 | idem | idem | §1.2.1 |
| S-boxes per round | 512 | 512 | 512 | 2 layers × 4 per column × 64 columns |
| MDS | 4 × 4 over F₂⁵, 44 XOR gates | idem | idem | §1.2.1 |
| squeeze blocks ⌈n/r⌉ | 1 | 2 | 6 | |
| f calls in squeezing | 0 | 1 | 5 | |
| collision | 2²⁵⁶ | 2³⁸⁴ | 2⁵¹² | spec Table 1.1 |
| preimage | 2⁵¹² | 2⁷⁶⁸ | 2¹⁰²⁴ | spec Table 1.1 |
| 2nd preimage | 2⁵¹² | 2⁷⁶⁸ | 2¹⁰²⁴ | spec Table 1.1 |

Note the design point: an ordinary sponge with these rates would cap preimage
resistance at `2^{c/2}` = 2²⁸⁸/2⁴¹⁶/2⁵⁴⁴, below the claimed `2^n`. The EDM
absorption is precisely what lets XRH-1 claim `2^n` preimage resistance at
`c = n + 64` instead of needing `c = 2n`, which is why its rate is larger than a
classical sponge at the same digest size.

Sizes (bytes), specification vs the built reference library:

| instance | digest spec | digest impl (OBSERVED) | match |
|---|---|---|---|
| XRH-1-512 | 64 | digest_bits=512, digest_bytes=64 | yes |
| XRH-1-768 | 96 | digest_bits=768, digest_bytes=96 | yes |
| XRH-1-1024 | 128 | digest_bits=1024, digest_bytes=128 | yes |

## Pseudocode

### XRH-1-n(msg) — Algorithm 4, §1.3
```
m = msg || Pad10*1(r, |msg|)        # msg || 1 || 0^k || 1, k minimal, |m| = 0 mod r
alpha = |m| / r ;  parse m = (m_1 .. m_alpha), m_i in F_2^r
state = 0^1280

# --- absorption (EDM) ---
for i = 1 .. alpha:
    state  = state XOR (m_i || 0^c)
    state' = state
    state  = g(state)                          # rounds 0..8
    state  = h( state XOR floor(state')_(c) )  # rounds 9..17, c-bit feed-forward

# --- squeezing (ordinary sponge) ---
z = Trunc_r(state)
while |z| < n:
    state = f(state)                           # f = h o g = rounds 0..17
    z = z || Trunc_r(state)
return Trunc_n(z)
```
`⌊x⌋_(c)` is the c least-significant (capacity) bits of x; the feed-forward
touches only the capacity, never the rate.

**Variable digest length:** only n ∈ {512, 768, 1024}, each fixing
c = n + 64 and r = 1280 − c. Separation between the three is by rate alone —
the initial state is 0^1280 for all three and there is no suffix or domain byte.
There is no XOF mode, although the squeeze loop would support one.

### XRH-p round function — §1.2.2, R = 18 rounds
```
state S[0..19][0..63];  a column S[.][j] is four 5-bit nibbles

for r = 0 .. 17:
    # 1. Super S-box  S' = S^(x4) o M o S^(x4), applied to every column
    S^(x4)                                   # 4 parallel 5-bit S-boxes per column
    M                                        # MDS on the 4 nibbles of each column
    S^(x4)

    # 2. Super Mixcolumn  M' = SR^-1 o M o SR
    SR   = SR_X if r mod 3 == 0, SR_Y if r mod 3 == 1, SR_Z if r mod 3 == 2
    M                                        # MDS on each column again
    SR^-1                                    # same variant as chosen above

    # 3. Round constant
    S[0][.] ^= RC_r                          # 64-bit constant into row 0
```

### Components (§1.2.1)
```
S = [ 4,14,10, 5, 9,24,20, 0,18,28,30,21, 2,23,29,13,
     22,27,15, 7,26,12,16, 3, 8, 1,19,31,25,11,17, 6]      # 5-bit Almost Bent

M over F_2[x]/(x^5+x^2+1):        [b3]   [3 2 1 3] [a3]
                                  [b2] = [3 1 4 4] [a2]
                                  [b1]   [1 3 6 4] [a1]
                                  [b0]   [2 2 3 1] [a0]
   with a_i = S[5i][j] + 2*S[5i+1][j] + 4*S[5i+2][j] + 8*S[5i+3][j] + 16*S[5i+4][j]

SR_X : S[i][j] -> S[i][ 4*floor(j/4)  + ((j mod 4)  +    floor(i/5)) mod 4  ]
SR_Y : S[i][j] -> S[i][ 16*floor(j/16)+ ((j mod 16) +  4*floor(i/5)) mod 16 ]
SR_Z : S[i][j] -> S[i][ (j + 16*floor(i/5)) mod 64 ]

RC_0..RC_17 : successive 64-bit words of the binary expansion of frac(pi)
              (0x243f6a8885a308d3, 0x13198a2e03707344, ...)
```

## Implementation vs specification

Checked: `src/XRH-1-{512,768,1024}/CryptHash_AlgorithmInstance.c` — all three
are **byte-identical** (md5 `3582e52a…`); only the `.h` differs, and it derives
everything from `DIGEST_BIT_LENGTH`. `drng.c` is the unmodified official DRNG.
All 3 instances PASS KAT.

Verified:

- **Round count is 18**, split 0–8 / 9–17. `XRH_edmc_permute()` calls
  `XRH1280(0, 9, ...)` = g, then `XRH1280(9, 18, ...)` = h ✓ Eq. 1.7; the
  squeeze calls `XRH1280(0, 18, ...)` = f = Π[0,18) ✓ Algorithm 4 line 12. No
  round-count shortfall.
- **Shift-row schedule.** `phase = R0 % 3` and incremented per round, so round
  index `r` uses SR_X/SR_Y/SR_Z for `r mod 3 = 0/1/2` in **all three** call
  sites (g starts at 0, h starts at 9 ≡ 0 mod 3, f starts at 0) ✓ §1.2.2(2a).
- **Round order** inside `XRH1280`: `S^⊗4, M, S^⊗4` (the Super S-box), then
  `SR, M, SR⁻¹` (the Super Mixcolumn), then `x0 ^= RC[i]` ✓ §1.2.2.
- **S-box.** I compiled the bitsliced `sbox_hw25_bitslice64_withtemp()` and
  evaluated it on all 32 inputs: it reproduces the spec's table
  `[4,14,10,5,9,24,20,0,18,28,30,21,2,23,29,13,22,27,15,7,26,12,16,3,8,1,19,31,25,11,17,6]`
  **exactly** (with bit 0 = row 5i, matching the `a_i` weighting of §1.2.1) ✓.
- **MDS.** I worked the `MDS(...)` macro through symbolically over
  F₂[x]/(x⁵+x²+1) (the `MUL` macro is multiplication by x:
  `(c0..c4) = (t4, t0, t1⊕t4, t2, t3)` ✓). Writing A,B,C,D for the four nibbles
  `a0,a1,a2,a3`, the nine steps produce
  `A ← A+3B+2C+2D`, `B ← 4A+6B+3C+1D`, `C ← 4A+4B+1C+3D`, `D ← 3A+1B+2C+3D`,
  which is **exactly** the matrix of §1.2.1 under
  `[b3,b2,b1,b0]ᵀ = M[a3,a2,a1,a0]ᵀ` ✓. All 16 coefficients check out.
- **Shift rows.** `SR_X` rotates rows 5–9 / 10–14 / 15–19 left by 1 / 2 / 3
  within each 4-bit nibble and leaves rows 0–4 alone; `SR_Y` by 4 / 8 / 12
  within each 16-bit group; `SR_Z` by 16 / 32 / 48 over the whole 64-bit word —
  exactly `⌊i/5⌋`, `4⌊i/5⌋`, `16⌊i/5⌋` on groups of 4, 16, 64 columns ✓, with
  `*_INV` the matching inverses ✓.
- **Round constants.** `ROUND_CONSTANTS[]` are the π fractional hex digits
  (0x243f6a8885a308d3, 0x13198a2e03707344, …) ✓ §1.2.1; index `i` = the true
  round index, so g uses RC₀..RC₈, h uses RC₉..RC₁₇ and f uses RC₀..RC₁₇ ✓.
- **EDM absorption.** `XRH_edmc_permute()` snapshots the state, runs g, XORs the
  snapshot's **last** `STATE_CAPACITY/64` lanes into the corresponding lanes,
  then runs h ✓ Eq. 1.1 / Algorithm 4 lines 7–9. Since `c` = 576/832/1088 is a
  multiple of 64, the capacity is lane-aligned and the feed-forward touches
  exactly c bits, never the rate ✓.
- **Parameters.** `STATE_CAPACITY (DIGEST_BIT_LENGTH+64)` and
  `STATE_RATE (STATE_LANES*64-DIGEST_BIT_LENGTH-64)` give (r, c) =
  (704,576), (448,832), (192,1088) ✓ spec Table 1.1 exactly.
- **Padding.** The final-block branch of `CryptHash()` XORs the message
  remainder, masks the partial byte, sets the first `1` at bit `|M| mod r`
  (`0x80 >> tail_bits`), inserts an extra all-zero block when
  `|M| mod r == r−1`, then sets the closing `1` at the last bit of the rate
  (`state_bytes[rate_bytes-1] ^= 0x01`) ✓ `Pad10*1`, §1.3 Eq. 1.9. I checked the
  boundaries `|M| ≡ 0`, `r−8`, `r−2`, `r−1 (mod r)` and the empty message: all
  give the correct `M‖1‖0^k‖1`, and for `|M| ≡ 0 (mod r)` the code never
  dereferences `msg` past its end.
- **Squeezing.** The first block is read *without* a permutation and `f` is
  applied only *between* blocks ✓ Algorithm 4 lines 10–13. Block counts are
  1 / 2 / 6, and `n − (β−1)r` is a whole number of bytes in every case
  (512, 320, 64 bits), so the byte-granular `memcpy` is exact ✓.

Discrepancies and notes:

- **No round-count, padding, rate/capacity or feed-forward deviation was
  found.** This is the cleanest of the eight candidates in this batch: 18
  rounds, the 9/9 g/h split, c = n + 64, the `10*1` pad, the c-bit EDM
  feed-forward and the π round constants are all implemented exactly as
  specified, and the S-box and MDS matrix were verified numerically/symbolically
  rather than by inspection.
- **(b) The byte encoding of the 1280-bit state is never specified.** The spec
  works purely in `F_2^1280` and Figure 1.3 numbers columns 63…0 right-to-left,
  but nothing says how a message byte maps to a (row, column) pair. The
  implementation simply aliases the 20-lane `uint64_t` array to bytes
  (`uint8_t *state_bytes = (uint8_t*)state.state`) and XORs message bytes into
  it, so the mapping is **host-endian-dependent**: byte j of the message lands
  in bits of lane j/8 in host order, and within a byte the message is MSB-first
  while columns are LSB-first. The KATs are therefore only reproducible on
  little-endian hosts, and an independent implementer cannot derive the mapping
  from the document.
- **(a) No validation of `digest_len_bits`.** `CryptHash()` never compares the
  argument to `DIGEST_BIT_LENGTH`; it uses the compile-time `STATE_RATE` for
  absorption but the caller's `digest_len_bits` for the squeeze length. Passing
  e.g. 1024 to the XRH-1-512 library produces 128 bytes squeezed at rate 704
  with capacity 576 — a function that is in no variant of the family — and
  passing a value larger than the caller's buffer overflows it. Robustness only;
  the KAT driver always passes `DIGEST_BIT_LENGTH`.
- **(a) Namespace pollution.** `ROUND_CONSTANTS`, `XRH1280` and
  `XRH_edmc_permute` have external linkage (no `static`), so two XRH-1 instances
  cannot be linked into one program. The uniform build gives each instance its
  own library, so this does not bite here.
- **(a) Dead constants.** `ROUND_CONSTANTS[24]` holds 24 words but only indices
  0–17 are ever read — six unused π words, presumably left over from a
  higher-round-count draft. Harmless, but it invites a future edit that silently
  changes the round count.
- **(a) Cosmetic:** `volatile uint64_t initial_state[STATE_LANES]` in
  `XRH_edmc_permute()` forces the feed-forward snapshot to memory on every
  absorption (a per-block performance cost) and serves no correctness or
  zeroization purpose — the buffer is never cleared. Several signed/unsigned
  comparisons (`size_t j < int rate_bytes`) will warn under `-Wall`.

Coverage: no independent differential, linear, impossible-differential or
zero-sum analysis of XRH-p was attempted; §3.1's bounds and the §3.2.4 MITM
analysis were read but not re-derived. The SPONGE-EDMc security proof (§3.2.1,
citing a CRYPTO 2026 paper not included in the submission) was not audited — the
`2^n` preimage claim at `c = n + 64` rests entirely on it, so it is the single
most load-bearing unverified statement in this candidate.

## Review of 2026-11-17

A model written from the specification equals `XRH1280` on random states and the full hash for every bit length from 0 to 2r+70. The S-box (almost bent, degree 2, inverse degree 3), the MDS matrix, the shift-row patterns and all 24 round constants were recomputed and match. Table 3.1's active S-box counts for 1 to 8 rounds were confirmed, the first five exactly. Two analysis statements do not hold: all single-cell impossible differentials exist over 5 rounds, and one round has degree 4 with inverse degree 9; filed as `hash-28-2`. The reference `CryptHash` block count wraps for `msg_len_bits ≥ 2^64 − r`, giving a constant digest without reading the message; filed as `hash-28-1`. The reference and AVX2 trees otherwise agree for all bit lengths from 0 to 4r+70.
