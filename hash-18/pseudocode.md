# hash-18 Megascon — algorithm summary

Sponge hash family over a 2048-bit bitsliced permutation (8 rows x 256 bits;
ChiLow-style 8-bit `chichi` S-box applied columnwise, a row permutation, an
Ascon-like rotation-XOR diffusion per row, and a 64-bit round constant).
Two modes: **Megascon-384/512 (+ both XOFs)** use a plain sponge with 15
rounds; **Megascon-768/1024** use a *feed-forward sponge* (F-Sponge) with 16
rounds, a capacity domain-separation bit on the last block, and an output taken
from the capacity end.

Specification: `hash-18-spec.pdf` (31 pages, English), Sect. 2.2–2.4,
Algorithms 1–2, Tables 1–3.

## Parameters

| parameter | -384 | -512 | -768 | -1024 | -384-XOF | -512-XOF | meaning |
|---|---|---|---|---|---|---|---|
| mode | SPONGE | SPONGE | F-SPONGE | F-SPONGE | SPONGE | SPONGE | Table 1 |
| state | 2048 | 2048 | 2048 | 2048 | 2048 | 2048 | bits |
| rate r | 1280 | 1024 | 1216 | 960 | 1280 | 1024 | bits |
| capacity c | 768 | 1024 | 832 | 1088 | 768 | 1024 | bits |
| rounds | 15 | 15 | 16 | 16 | 15 | 15 | Table 1 |
| digest h | 384 | 512 | 768 | 1024 | 0 (arbitrary ℓ) | 0 (arbitrary ℓ) | Table 1 |
| output taken from | MSB (rate) | MSB (rate) | LSB (capacity) | LSB (capacity) | MSB (rate) | MSB (rate) | Alg. 1 / Alg. 2 |
| claimed collision | 192 | 256 | 384 | 512 | min(384, ℓ/2) | min(512, ℓ/2) | Table 2 |
| claimed preimage | 384 | 512 | 768 | 1024 | min(384, ℓ) | min(512, ℓ) | Table 2 |
| claimed 2nd preimage | 384 | 512 | 768 | 1024 | min(384, ℓ) | min(512, ℓ) | Table 2 |

Digest length, specification vs the built reference libraries
(`OBSERVED/hash-18.txt`). The six XOF libraries are the *same two source
directories* built three times each, with the output length supplied through
`-DNGCC_DIGEST_BITS` (the shipped header names only the 2048-bit variant):

| instance | source dir | h/ℓ spec | digest_bits impl | digest_bytes impl | match |
|---|---|---|---|---|---|
| MEGASCON-384 | MEGASCON-384 | 384 | 384 | 48 | yes |
| MEGASCON-512 | MEGASCON-512 | 512 | 512 | 64 | yes |
| MEGASCON-768 | MEGASCON-768 | 768 | 768 | 96 | yes |
| MEGASCON-1024 | MEGASCON-1024 | 1024 | 1024 | 128 | yes |
| MEGASCON-384-XOF-256/1024/2048 | MEGASCON-384-XOF | arbitrary ℓ | 256 / 1024 / 2048 | 32 / 128 / 256 | yes |
| MEGASCON-512-XOF-256/1024/2048 | MEGASCON-512-XOF | arbitrary ℓ | 256 / 1024 / 2048 | 32 / 128 / 256 | yes |

## Pseudocode

### Permutation `p = pC ∘ pL ∘ pπ ∘ pS` (Sect. 2.4), state `x[0..7][0..3]` of 64-bit words
```
repeat `rounds` times (r = 0, 1, ...):
  pS (substitution, 256 parallel 8-bit chi-chi S-boxes down the columns):
     v0 = u0 ^ (~u1 & u2)      v4 = u2 ^ (~u5 & u6)
     v1 = u4 ^ (~u2 & u0)      v5 = u5 ^ (~u6 & u7)
     v2 = u3 ^ (~u0 & u1)      v6 = u6 ^ (~u7 & u3)
     v3 = ~u1 ^ (~u4 & ~u5)    v7 = u7 ^ (~u3 & u4)
  pπ (row permutation):  x_{π(i)} <- x_i ,  π = (1,7,5,2,4,6,0,3)
  pL (per row i, words mod 4):
     y[i][j] = x[i][j] ^ (x[i][j+1] <<< a_i) ^ (x[i][j+2] <<< b_i)
                       ^ (x[i][j+3] <<< c_i) ^ (x[i][j]   <<< d_i)
     (a,b,c,d)_i = (1,3,14,36) (2,5,23,60) (4,15,17,24) (7,9,12,40)
                   (26,46,13,37) (49,6,50,25) (55,19,56,57) (58,47,61,28)
  pC:  x[0][0] ^= c_r
       c_r = splitmix64 finalizer of (0x9e3779b97f4a7c15 ^ r)   # spec Listing 5
```

### `SPONGE_{h,r,a}(M, ℓ)`  — Megascon-384 / -512 / both XOFs (spec Algorithm 1)
```
I <- 0^2048                                  # all-zero IV, no h- or r-dependent constant
M_1..M_s <- M ‖ 1 ‖ 0*                       # pad to a multiple of r; ALWAYS adds >= 1 bit
for i = 1..s:  I <- p_a( I XOR (M_i ‖ 0^(2048-r)) )
Z <- MSB_r(I)
for i = 1..floor(ℓ/r):  I <- p_a(I) ;  Z <- Z ‖ MSB_r(I)
return MSB_ℓ(Z)                              # ℓ = h for the fixed-length variants
```
Variable digest length is handled *only* by the squeeze-block count and the
final truncation. **`h` never enters the computation** — it only constrains `ℓ`.

### `F-SPONGE_{h,r,b}(M)` — Megascon-768 / -1024 (spec Algorithm 2)
```
I <- 0^2048
M_1..M_s <- M ‖ 1 ‖ 0*
for i = 1..s:
    if i != s:  I <- I XOR (M_i ‖ 0^(2048-r))
    else:       I <- I XOR (M_i ‖ 1 ‖ 0^(2048-r-1))    # domain-separation bit
    I <- (0^r ‖ LSB_(2048-r)(I)) XOR p_b(I)            # capacity feed-forward
return LSB_h(I)
```

## Implementation vs specification

Checked: all ten `src/*/CryptHash_AlgorithmInstance.c` files (each is
self-contained; the permutation source is duplicated verbatim in every one).

Verified agreements:

- **Permutation:** `chichi_scalar` reproduces the eight S-box equations of
  Sect. 2.4.1 exactly (including the double-negated `v3`); `BIT_PERM =
  {1,7,5,2,4,6,0,3}` with `t[BIT_PERM[i]] = x[i]` is `x_{π(i)} <- x_i`;
  `row_mix_one_scalar` is the Sect. 2.4.3 formula and the 32 rotation constants
  `A/B/C/D` match the spec's table row for row; `add_constant_scalar` XORs into
  `x[0][0]` only (Sect. 2.4.4); `round_constant()` is byte-identical to spec
  Listing 5. Layer order `chichi, bitperm, row_mix, add_constant` = `pC ∘ pL ∘
  pπ ∘ pS`. (b) Sect. 7.1.2 (p. 15) describes the row mix with source-row
  constants followed by π, the opposite order; that reading reproduces none of
  the KATs.
- **Rounds:** 15 for the sponge instances, 16 for the F-sponge instances —
  matches Table 1 (see discrepancy 3 about Sect. 2.1).
- **Rates:** 160 / 128 / 152 / 120 bytes = 1280 / 1024 / 1216 / 960 bits, and
  160 / 128 bytes for the two XOFs. All match Table 1.
- **Padding:** `0x80` after the last message bit, then zeros, with the length
  rounded up by `((L + r) / r) * r` so a padding block is always added even when
  `L` is a multiple of `r` — exactly `M ‖ 1 ‖ 0*` of Algorithms 1 and 2.
- **F-Sponge:** `state[rate] ^= 0x80` on the last block, `forward` snapshot of
  `state[rate..255]` XORed back after the permutation, output
  `state[256-hash .. 255]` = `LSB_h(I)`. Exactly Algorithm 2.
- **Squeeze:** the XOF loop emits `ceil(ℓ/r)` rate blocks with a permutation
  between them, which produces the same `MSB_ℓ` as Algorithm 1's
  `1 + floor(ℓ/r)` blocks (the spec simply generates one unused trailing block).
  The final partial byte is masked, so sub-byte `ℓ` is handled.

### Discrepancy 1 — **(a) real weakness: NO domain separation between the fixed-length hash and the XOF at the same rate**

`SPONGE_{384,1280,15}(M, 384)` and `SPONGE_{0,1280,15}(M, ℓ)` are, by the spec's
own Algorithm 1, *the identical computation*: same zero IV, same padding, same
rate, same round count; `h` is never absorbed anywhere. Confirmed
experimentally (scratch build of the unmodified reference sources, M = "abc"):

```
Megascon-384          12A06F59D0C531C2 … B76ACCC9966D7CF4        (48 bytes)
Megascon-384-XOF(2048)[0:48]  identical
Megascon-512          12A06F59 … 966D7CF4 907761161784FCCE7082995B14EE6036
Megascon-512-XOF(2048)[0:64]  identical
```
So `Megascon-384(M) = MSB_384(Megascon-384-XOF(M, ℓ))` for every `M` and every
`ℓ >= 384`, and likewise for the 512 pair. A party that releases an XOF output
also releases the corresponding fixed-length digest. SHA-3/SHAKE avoid exactly
this with the `01` vs `1111` domain suffix; Megascon has no such mechanism.

### Discrepancy 2 — **(a) real weakness: Megascon-384 is a truncation of Megascon-512 for all single-block messages**

Because the IV is all-zero and the padding is `10*` with no rate encoding,
absorbing a message that fits in one block produces the *same 2048-bit state*
whichever rate is used — the extra rate bytes are simply XORed with zeros.
Measured on the unmodified reference code:

| `|M|` | `Megascon-384(M) == MSB_384(Megascon-512(M))` |
|---|---|
| 24 bits | **YES** |
| 1000 bits | **YES** |
| 1100 bits | no (512 now needs 2 blocks) |
| 3000 bits | no |

i.e. for every message of at most 1023 bits — which covers essentially all
signature/KDF/commitment inputs — the four sponge instances
`Megascon-384`, `Megascon-512`, `Megascon-384-XOF`, `Megascon-512-XOF`
produce **one and the same output stream**, differing only in where it is
truncated. Table 2's separate per-instance claims cannot all hold
simultaneously in any multi-instance setting, and a collision or preimage on
one instance immediately transfers to the others. This is a *specification*
flaw (Algorithm 1 as written), faithfully implemented — the fix is an
instance-dependent IV or a domain-separation suffix.

The F-sponge pair is **not** affected: `Megascon-768` and `Megascon-1024` place
their domain-separation bit at different offsets (bit 1216 vs bit 960) and read
different LSB windows, and I verified `Megascon-768(M) != LSB_768(Megascon-1024(M))`.

### Discrepancy 3 — **(b) spec internal inconsistencies** (implementation follows the right one)

- Sect. 2.1 defines "`a/b` number of rounds of the permutation, **a = 20 and
  b = 24**", but Table 1 says **15** and **16**, Sect. 5.2.2 computes the
  security margins as `15-7=8` and `16-13=3`, and Table 3 lists exactly **16**
  round constants. The code uses 15 / 16, i.e. Table 1. Sect. 2.1 is stale and
  should be corrected — as printed it would suggest a 25%/50% larger round count
  than anything actually built or analysed.
- Sect. 5.5 item 2 says the F-sponge domain constant is `1 = 0^(c-1) ‖ 1`,
  i.e. the **last** bit of the capacity, while Algorithm 2 line 7 puts it at the
  **first** capacity bit (`M_i ‖ 1 ‖ 0^(2048-r-1)`). The code follows
  Algorithm 2. The other placement gives a different function (no KAT is
  reproduced with it) but leaves the length-extension argument intact, which
  only needs the bit in the capacity.

### Other observations

- **Memory leak in every instance.** `hash_sponge`/`hash_Fsponge` `malloc()`s
  `padded_input` and never `free()`s it (0 occurrences of `free` in all ten
  files), and the `malloc` return value is not NULL-checked. Every hash call
  leaks `ceil((L+1)/r)*r/8` bytes — unbounded for a long-running caller.
- `hash_sponge` for the fixed-length instances validates `rate` and `hash`
  and prints to `stdout` on failure (`printf` in a library routine).
- The state is filled by `memcpy` into an array of `uint64_t`, so the external
  bit-index mapping of Sect. 2.4 is realised with little-endian word loads; the
  spec does not state a byte order for that mapping and ships no test vectors,
  so this convention could not be checked against the specification. The KATs
  are self-consistent (all 10 instances PASS).
- Not verified: the differential/linear bounds of Sect. 5.2.1 (the spec itself
  reports no tight bound even for three rounds) and the MitM margins of
  Sect. 5.2.2.
