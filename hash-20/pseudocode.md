# hash-20 Mozi — algorithm summary

Sponge hash family over a 2048-bit permutation organised as a 4 x 4 array of
128-bit lanes (`I[x][y][z]`). Each round applies a 4-bit S-box across `x`, an
MDS MixColumn over GF(2^4) along `y`, a per-`y` rotation of the 128-bit lanes
(ShiftRow), and a one-byte round constant. Two modes: **Mozi-384/512 (+ both
XOFs)** use a plain sponge with 20 rounds; **Mozi-768/1024** use a feed-forward
sponge (F-Sponge) with 24 rounds, a capacity domain-separation bit on the last
block, and an output taken from the capacity end. The mode layer is the same
construction (and the same C skeleton) as hash-18 Megascon; only the
permutation differs.

Specification: `hash-20-spec.pdf` (34 pages, English), Sect. 2.2–2.4,
Algorithms 1–4, Tables 1–3, 5.

## Parameters

| parameter | -384 | -512 | -768 | -1024 | -384-XOF | -512-XOF | meaning |
|---|---|---|---|---|---|---|---|
| mode | SPONGE | SPONGE | F-SPONGE | F-SPONGE | SPONGE | SPONGE | Table 1 |
| state | 2048 | 2048 | 2048 | 2048 | 2048 | 2048 | 4 x 4 lanes of 128 bits |
| rate r | 1280 | 1024 | 1216 | 960 | 1280 | 1024 | bits (160/128/152/120 bytes) |
| capacity c | 768 | 1024 | 832 | 1088 | 768 | 1024 | bits |
| rounds | 20 (a) | 20 (a) | 24 (b) | 24 (b) | 20 (a) | 20 (a) | Table 1 |
| digest h | 384 | 512 | 768 | 1024 | 0 (arbitrary ℓ) | 0 (arbitrary ℓ) | Table 1 |
| output taken from | MSB (rate) | MSB (rate) | LSB (capacity) | LSB (capacity) | MSB (rate) | MSB (rate) | Alg. 1 / Alg. 2 |
| claimed collision | 192 | 256 | 384 | 512 | min(384, ℓ/2) | min(512, ℓ/2) | Table 5 |
| claimed preimage | 384 | 512 | 768 | 1024 | min(384, ℓ) | min(512, ℓ) | Table 5 |
| claimed 2nd preimage | 384 | 512 | 768 | 1024 | min(384, ℓ) | min(512, ℓ) | Table 5 |

Digest length, specification vs the built reference libraries
(`OBSERVED/hash-20.txt`). The six XOF libraries are two source directories
built three times each, with the output length supplied through
`-DNGCC_DIGEST_BITS` (the shipped header only names the 2048-bit variant):

| instance | source dir | h/ℓ spec | digest_bits impl | digest_bytes impl | match |
|---|---|---|---|---|---|
| MOZI-384 | MOZI-384 | 384 | 384 | 48 | yes |
| MOZI-512 | MOZI-512 | 512 | 512 | 64 | yes |
| MOZI-768 | MOZI-768 | 768 | 768 | 96 | yes |
| MOZI-1024 | MOZI-1024 | 1024 | 1024 | 128 | yes |
| MOZI-384-XOF-256/1024/2048 | MOZI-384-XOF | arbitrary ℓ | 256 / 1024 / 2048 | 32 / 128 / 256 | yes |
| MOZI-512-XOF-256/1024/2048 | MOZI-512-XOF | arbitrary ℓ | 256 / 1024 / 2048 | 32 / 128 / 256 | yes |

## Pseudocode

### Permutation `p_a` (spec Algorithm 3), state `A[x][y]`, 4x4 lanes of 128 bits
```
for i = 0 .. rounds-1:
  S-box (4-bit, across x, for each y; spec Alg. 3 lines 4-7 — note the
         SEQUENTIAL use of B[3,y] and B[1,y]):
      B[3,y] = (A[3,y] & A[2,y]) ^ A[1,y]
      B[1,y] = (A[2,y] | A[1,y]) ^ A[0,y]
      B[0,y] = (B[3,y] & A[0,y]) ^ A[3,y]
      B[2,y] = (B[1,y] & A[3,y]) ^ A[2,y]
  MixColumn: per bit-slice z, (C[·,0],..,C[·,3]) = M · (B[·,0],..,B[·,3])
      # MDS over GF(2^4), x^4+x+1, computed bit-sliced (spec Algorithm 4):
      #   MUL(x0,x1,x2,x3): (x0,x1,x2,x3) <- (x3, x0^x3, x1, x2)
      S2^=S3; S0^=S1; S1=MUL(S1); S3=MUL(S3); S1^=S2; S3^=S0;
      S0=MUL(MUL(S0)); S2=MUL(MUL(S2)); S2^=S3; S0^=S1; S1^=S2; S3^=S0
  ShiftRow:  D[x][y] = RotR_128( C[x][y], rho[i mod 4][y] )
      rho = [ [0,14,20,22], [0,13,68,91], [0,27,42,106], [0,32,48,80] ]
  AddConstant: RC[i] XORed into the least significant byte of D[3][3]
      RC = 0x24,0x3F,0x6A,0x88,… (hex digits of pi, 24 values)
A = D
```

### `SPONGE_{h,r,a}(M, ℓ)` — Mozi-384 / -512 / both XOFs (spec Algorithm 1)
```
I <- 0^2048                                  # all-zero IV, no h- or r-dependent constant
M_1..M_s <- M ‖ 1 ‖ 0*                       # pad to a multiple of r; always adds >= 1 bit
for i = 1..s:  I <- p_a( I XOR (M_i ‖ 0^(2048-r)) )
Z <- MSB_r(I)
for i = 1..floor(ℓ/r):  I <- p_a(I) ;  Z <- Z ‖ MSB_r(I)
return MSB_ℓ(Z)
```
Variable digest lengths are handled only by the squeeze-block count and the
final truncation; **`h` never enters the computation**.

### `F-SPONGE_{h,r,b}(M)` — Mozi-768 / -1024 (spec Algorithm 2)
```
I <- 0^2048
M_1..M_s <- M ‖ 1 ‖ 0*
for i = 1..s:
    if i != s:  I <- I XOR (M_i ‖ 0^c)
    else:       I <- I XOR (M_i ‖ 0^(c-1) ‖ 1)      # <- spec; see Discrepancy 2
    I <- (0^r ‖ LSB_c(I)) XOR p_b(I)                # capacity feed-forward
return LSB_h(I)
```

## Implementation vs specification

Checked: all ten `src/MOZI-*/CryptHash_AlgorithmInstance.c` (each self-contained;
the permutation source is duplicated verbatim in every one).

Verified agreements:

- **Permutation:** the four S-box lines in `permutation()` are literally spec
  Algorithm 3 lines 4-7, and I checked the resulting 4-bit map against spec
  Table 2 (`SB = [0,8,6,13,5,15,7,12,4,14,2,3,9,1,11,10]`) on several inputs —
  it matches with `A[0]` as the most significant bit. (Unlike hash-19 MoFang,
  Mozi's algebraic S-box definition and its lookup table agree.)
- **MixColumn:** `bitSliceMDS()` performs exactly the 12 steps of spec
  Algorithm 4 in the same order, and `MUL()` is the spec's `MUL` LFSR. (b) MUL
  maps (x0..x3) to (x3, x0⊕x3, x1, x2), the transpose of the Sect. 2.4.2 α
  matrix (x1, x2, x3, x0⊕x1); with α no KAT is reproduced. Both have branch
  number 5.
- **ShiftRow:** `rhoY[4][4]` matches spec Table 3 entry for entry
  (`{0,14,20,22} {0,13,68,91} {0,27,42,106} {0,32,48,80}`), selected by
  `i mod 4`, and the 128-bit code path implements a rotate **right** by
  `rho` over the (high:low) = (`[1]`:`[0]`) word pair, as spec Eq. (4) requires,
  including the `= 64` and `> 64` cases.
- **AddConstant:** `D[3][3][0] ^= RCON[r]` XORs the one-byte constant into the
  least significant byte of lane `D[3,3]` — exactly Sect. 2.4.4 — and
  `RCON[0..23]` matches the spec's pi-digit table.
- **Rounds:** 20 for the sponge instances, 24 for the F-sponge instances,
  matching both Sect. 2.1 (`a = 20, b = 24`) and Table 1. **No reduction.**
  (Note: the sibling Megascon spec has a stale `a=20/b=24` in Sect. 2.1 that
  contradicts its Table 1; Mozi is internally consistent here.)
- **Rates:** 160 / 128 / 152 / 120 bytes = 1280 / 1024 / 1216 / 960 bits and
  160 / 128 for the XOFs — all match Table 1, as do the implied capacities.
- **Padding:** `0x80` after the last message bit then zeros, with
  `((L + r) / r) * r` rounding so a padding block is always added even for
  `L ≡ 0 (mod r)` — `M ‖ 1 ‖ 0*` of Algorithms 1 and 2.
- **State indexing:** the byte→lane mapping `state[(4x+y)*16 + k]` matches the
  spec's `i = (4x+y)·128 + z` lane order.
- **F-Sponge:** the `forward` snapshot of `state[rate..255]` XORed back after
  the permutation is `(0^r ‖ LSB_c(I)) ⊕ p_b(I)`, and `output[i] =
  state[256-hash+i]` is `LSB_h(I)` — both as in Algorithm 2.
- **Squeeze:** `ceil(ℓ/r)` rate blocks with a permutation between them produces
  the same `MSB_ℓ` as Algorithm 1's `1 + floor(ℓ/r)` blocks; the final partial
  byte is masked.

### Discrepancy 1 — **(a) real weakness: no domain separation at all between instances or output lengths**

Algorithm 1 uses an all-zero IV, `10*` padding, and never absorbs `h`, `r` or an
instance identifier. Two consequences, both verified by compiling the
unmodified reference sources in a scratch directory:

1. **The fixed-length hash is a prefix of the XOF at the same rate**, for every
   message:
   `Mozi-384(M) = MSB_384(Mozi-384-XOF(M, ℓ))` and
   `Mozi-512(M) = MSB_512(Mozi-512-XOF(M, ℓ))` — both **YES** in the test.
   XOF outputs of different lengths are likewise prefixes of one another
   (`XOF(256)` is a prefix of `XOF(2048)` — **YES**). The spec claims separate
   security levels for `Mozi-384` and `Mozi-384-XOF`, but they are the same
   function.
2. **Mozi-384 is a truncation of Mozi-512 for every single-block message.**
   Because the IV is zero and the padding is `10*` with no rate encoding,
   absorbing a message that fits in one block gives the *same* 2048-bit state
   whichever rate is used (the extra rate bytes are XORed with zeros).
   Measured:

   | `|M|` | `Mozi-384(M) == MSB_384(Mozi-512(M))` |
   |---|---|
   | 24 bits | **YES** |
   | 1000 bits | **YES** |
   | 3000 bits | no (512 needs 2 blocks, 384 does not) |

   Example for the 24-bit test message:
   `Mozi-384 = A037EBE9…FB111B1E` and
   `Mozi-512 = A037EBE9…FB111B1E 57CD8917EDD0BA17EB1FCE266810D3CC`.

   So for every message of at most 1023 bits — the normal case for signatures,
   KDFs and commitments — `Mozi-384`, `Mozi-512`, `Mozi-384-XOF` and
   `Mozi-512-XOF` emit **one and the same output stream**, differing only in the
   truncation point. A collision or preimage on one instance transfers
   immediately to the others, and Table 5's per-instance claims cannot hold
   independently. This is a *specification* flaw (Algorithm 1 as written),
   faithfully implemented; the fix is an instance-dependent IV or a
   domain-separation suffix (as SHA-3/SHAKE use). **Identical defect to hash-18
   Megascon.**

   The F-sponge pair is **not** affected: `Mozi-768` and `Mozi-1024` place their
   domain bit at different offsets and read different LSB windows; I verified
   `Mozi-768(M) != LSB_768(Mozi-1024(M))`.

### Discrepancy 2 — **(a) the F-sponge domain-separation bit is at the wrong end of the capacity relative to Algorithm 2**

Spec Algorithm 2 line 7 (and Sect. 4.6 item 2) both say the last block XORs
`0^(c-1) ‖ 1` into the inner part, i.e. the **last** bit of the capacity
(state bit 2047). The implementation does `state[rate] ^= 0x80`, i.e. the
**first** bit of the capacity (state bit `r`):

```
if (i == blocks - 1) { state[rate] ^= 0x80; }   // domain separation
```

The code's placement is the safer one: with `T = LSB_h(I)` and the capacity
feed-forward `(0^r ‖ LSB_c(I)) ⊕ p_b(I)`, the spec's bit 2047 sits *inside* the
output window and is XORed straight into the last output bit, whereas bit `r`
(1216 for Mozi-768, 960 for Mozi-1024) lies in the `c - h` bits of capacity that
are never output. The implementation matches the formulation used in the
sibling Megascon spec (`M_i ‖ 1 ‖ 0^(2048-r-1)`). Either way, Algorithm 2 and
the reference code disagree and the specification needs fixing.

### Other observations

- **Memory leak in every instance.** `hash_sponge`/`hash_Fsponge` `malloc()`
  the padded message and never `free()` it (0 `free` calls in all ten files),
  and the `malloc` result is not NULL-checked. Every call leaks
  `ceil((L+1)/r)*r/8` bytes.
- `hash_sponge` for the fixed-length instances validates `rate`/`hash` and
  `printf`s to stdout on failure — inappropriate for a library routine.
- `CryptHash()` derives the byte length as `digest_len_bits / 8` without
  checking that `digest_len_bits` is a multiple of 8 or equal to the instance's
  own `DIGEST_BIT_LENGTH` (the fixed-length instances catch a wrong value only
  indirectly via the `hash != 48/64/96/128` test).
- Spec Eq. (4) writes ShiftRow as `RotR(C, rho)` and also as
  `C[x,y,z-rho mod 128]`; the two readings are opposite conventions. The code
  implements a right rotation of the little-endian 128-bit lane. With no test
  vectors in the specification this could not be settled independently; the
  KATs are self-consistent (all 10 instances PASS).
- Not verified: the differential/linear active-S-box bounds of Sect. 4, the
  sponge indifferentiability argument, or any cryptanalysis of the permutation.
