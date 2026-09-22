# hash-17 MasterCube — algorithm summary

Sponge hash family whose inner function is a **transformation** (not a
permutation): `Cube-f` is a ZIP construction — 9 rounds of the `Cube-p` round
function XORed with 9 rounds of the *inverse* `Cube-p` round function applied to
the same input. `Cube-p` is an **AndRX** design: the only nonlinearity is a
6-round Feistel network (`MAndRX`) over 16-bit cells using AND + rotation + XOR;
everything else (ShiftRows, MixRows, SwapRows, CrossMixs, MixColumns) is linear
over GF(2). AndRX designs are the natural target for rotational-XOR and
differential-linear cryptanalysis, so the round function is given in full below.

Specification: `hash-17-spec.pdf` (32 pages, English), Sect. 2.1–2.3,
Algorithms 1–4, Appendix A (test vectors).

## Parameters

| parameter | MasterCube-512 | MasterCube-768 | MasterCube-1024 | meaning |
|---|---|---|---|---|
| state b | 1536 | 1536 | 1536 | 6 x 8 lanes x 32 bits = 12 rows x 8 cells x 16 bits |
| rate r | 960 | 704 | 448 | Table 1 |
| capacity c | 576 | 832 | 1088 | `c = d + 64`, Sect. 3.1 |
| digest d | 512 | 768 | 1024 | Table 1 |
| Cube-p rounds | 18 | 18 | 18 | Algorithm 3 (`for i = 0..17`) |
| Cube-f branch rounds | 9 + 9 | 9 + 9 | 9 + 9 | Algorithm 4 |
| squeeze blocks | 1 | 2 | 3 | `ceil(d/r)` |
| max message | 2^64 - 1 bits | " | " | Sect. 2.3 |
| claimed (2nd-)preimage | 512 | 768 | 1024 | Table 2 (uses `2^(c-1)`, the transformation bound) |
| claimed collision | 256 | 384 | 512 | Table 2 |

Digest length, specification vs the built reference library
(`OBSERVED/hash-17.txt`):

| instance | d spec | digest_bits impl | digest_bytes impl | match |
|---|---|---|---|---|
| MasterCube-512 | 512 | 512 | 64 | yes |
| MasterCube-768 | 768 | 768 | 96 | yes |
| MasterCube-1024 | 1024 | 1024 | 128 | yes |

No XOF instance; `CryptHash()` rejects a `digest_len_bits` other than its own.

## Pseudocode

State: 12 rows x 8 cells of 16 bits. Rows 0–5 are the slice `A0xy`, rows 6–11
the slice `A1xy`; lane `(x,y)` is the 32-bit pair `(row[y][x], row[6+y][x])`.
In the "sheet" view a column `x` is the 12 cells `v_0 .. v_11 = row[0..11][x]`.

### `MAndRX` — the AndRX layer (spec Sect. 2.1.1) — the only nonlinear step
```
# one AndRX round on a 32-bit lane X = L ‖ R (L,R are 16-bit cells):
#    L' = ((L >>> a) AND (L >>> b)) XOR (L >>> g) XOR R
#    R' = L                                    (>>> = 16-bit rotate RIGHT)
# MAndRX = 6 such rounds with these (a,b,g), in this order:
   (0,1,8), (14,5,0), (9,12,8), (14,5,0), (0,1,8), (8,9,0)
# applied in parallel to all 48 lanes.
# Implemented in place by alternating which half plays the role of L:
#   round 1,3,5: L = A0 cell, R = A1 cell ;  round 2,4,6: swapped.
```

### `Cube-p-r(A, RC)` — one Cube-p round (spec Algorithm 1)
```
A <- CrossMixs(A)    # per column x, to (v0,v2,v4,v6,v8,v10) and (v1,v3,..,v11):
                     #   M = circulant involution
                     #   [1 1 0 0 1 0; 0 1 1 0 0 1; 1 0 1 1 0 0;
                     #    0 1 0 1 1 0; 0 0 1 0 1 1; 1 0 0 1 0 1]
A <- SwapRows(A)     # swap v6<->v7, v8<->v9, v10<->v11 (i.e. rows 6..11 pairwise)
A <- MAndRX(A)       # as above
A <- CrossMixs(A)
A <- SwapRows(A)
A <- MixColumns(A)   # per column x: (v0,v2,v4) and (v1,v3,v5) x M1;
                     #               (v6,v8,v10) and (v7,v9,v11) x M2
                     #   M1 = [0 1 1; 1 0 1; 1 1 1]   M2 = [1 0 1; 0 1 1; 1 1 1]
A <- ShiftRows(A)    # row i of A0xy rotated LEFT by i cells;
                     # row i of A1xy rotated RIGHT by i cells   (i = 0..5)
A <- MixRows(A)      # per row, to (r0,r2,r4,r6) and (r1,r3,r5,r7):
                     #   [1 1 0 1; 1 1 1 0; 0 1 1 1; 1 0 1 1]
A <- A XOR RC        # RC[i] = [0x243F^i, 0x6A88^i, 0x85A3^i, 0x08D3^i,
                     #          0x1319^i, 0x8A2E^i, 0x0370^i, 0x7344^i]
                     # XORed into the first row of A0xy only (frac(pi))
return A
```
`Cube-p-ir` (Algorithm 2) is intended to be the exact inverse, with the steps
in reverse order. The published algorithm and the submitted implementation
both fail that identity: `MixColumns` is not itself an involution across both
slices, and the implementation retains an extra slice exchange after already
reversing the nonlinear calls. See `hash-17-2`.

### `Cube-f(A)` — the ZIP transformation (spec Algorithm 4)
```
A' <- A
for i = 0..8:   A  <- Cube-p-r (A , RC[i])       # forward branch, RC[0..8]
for i = 8..0:   A' <- Cube-p-ir(A', RC[i+9])     # inverse branch, RC[17..9]
return A XOR A'
```
(The spec's Algorithm 4 writes `RC[i]`, i = 0..8 ascending, for the inverse
branch — see the discrepancy section; the line above is what the reference code
and the spec's own test vectors do.)

### Hash(M, |M|)  (spec Sect. 2.3, Figure 1)
```
S <- 0^1536
Mpad = pad10*1(M, r)                # Definition 1, Keccak multi-rate padding
for each r-bit block B of Mpad:
    S[0..r-1] ^= B ;  S <- Cube-f(S)
Z <- ""
repeat ceil(d/r) times:
    Z <- Z ‖ S[0..r-1] ;  S <- Cube-f(S)
return first d bits of Z
```
Variable digest lengths are handled purely by `ceil(d/r)` sponge squeeze
iterations and a truncation; there is **no** output-length domain separation and
no IV difference between instances — the three instances are separated only by
their different rates (960 / 704 / 448), which do differ, so no digest is a
truncation of another.

## Implementation vs specification

Checked: `src/MasterCube-{512,768,1024}/MasterCube-*.c` (they differ *only* in
`digest_bits` and `block_bits`), `mastercube.h` (padding + dispatch), and
`CryptHash_AlgorithmInstance.c`.

Verified agreements (all checked against the spec text, layer by layer):

- **Rounds:** `N_ROUNDS = 18` = Algorithm 3; `zip_prf` runs
  `_core(state, 9)` and `_core_inverse(copy, 9)` = Algorithm 4's 9 + 9.
- **MAndRX:** `MAndRXs(l, r, a, b, g)` computes
  `r = (ror16(x,a) & ror16(x,b)) ^ ror16(x,g) ^ y` and the six calls use
  `(0,1,8) (14,5,0) (9,12,8) (14,5,0) (0,1,8) (8,9,0)` — identical to
  Sect. 2.1.1, including the rotate-**right** direction.
- **CrossMixs / MixColumns / MixRows / ShiftRows / SwapRows:** I expanded each
  and matched it to the spec's matrices; `M_CrossMixs`, `M1`, `M2` and
  `M_MixRows` all reproduce exactly, and ShiftRows rotates `A0xy` left and
  `A1xy` right by the row index.
- **Rate/capacity:** `block_bits` = 960 / 704 / 448 and the implied capacities
  576 / 832 / 1088 match Table 1 exactly.
- **Round constants:** the 16 bytes in `AddConstant` are frac(pi)
  (`243F6A8885A308D3 13198A2E03707344`) and `c_16[j] ^ rn` implements
  `RC[i] = base_j ^ i`; only row 0 of `A0xy` is touched, per Sect. 2.1.4. (The
  byte array is read through a `uint16_t*` cast, so the eight cells land in the
  reverse of the spec's written order; since the spec's own test vectors
  reproduce, this is the spec's cell-indexing convention rather than an error.)
- **End-to-end:** I compiled the three reference files standalone in a scratch
  directory (nothing in the submission tree was built or run) and reproduced
  **all six Appendix A test vectors byte for byte** (512/768/1024 x 16-bit and
  256-bit messages). So the implementation matches the designers' intent.

### Discrepancy 1 — **(a) real defect: broken `pad10*1` gives trivial collisions**

`pad10star1()` in `mastercube.h` writes the opening `1` bit at offset
`|M| mod r` and then unconditionally ORs `0x01` into the **last byte of the same
block**. When `|M| mod r == r-1` there is only one free bit, so the two padding
`1` bits land on the *same* bit position and no second block is produced. The
spec's Definition 1 (Keccak multi-rate padding) requires `1 ‖ 0* ‖ 1`, i.e. at
least two padding bits, which for `|M| mod r == r-1` forces an **extra block**.

Consequence: a message of bit length `L` with `L mod r == r-1` whose last bit is
`1` hashes **identically** to the same message with that last bit removed
(`L-1`, with `L-1 mod r == r-2`). Verified experimentally on all three
instances (scratch build, reference sources unmodified):

| instance | r | colliding lengths | result |
|---|---|---|---|
| MasterCube-512 | 960 | 959 vs 958 | identical 512-bit digest |
| MasterCube-768 | 704 | 703 vs 702 | identical 768-bit digest |
| MasterCube-1024 | 448 | 447 vs 446 | identical 1024-bit digest |

For MasterCube-512 both lengths give
`A5A7639CC666A9DE…9DF3FF85`. This is a genuine collision for the bit-oriented
function the spec defines (messages are bit strings, and the two inputs are
distinct bit strings), found with **zero** work. It also applies at every
block boundary, i.e. for any `L ≡ r-1 (mod r)`. This directly contradicts the
claimed 256/384/512-bit collision resistance of Table 2 for that (sparse but
easily hit) input class. Fixing it requires the spec's own rule: when fewer
than 2 bits remain, emit one more full padding block.

### Discrepancy 2 — **(b) spec text error: Algorithm 4's inverse-branch constants**

Spec Algorithm 4 line 7 reads `A' <- Cube-p-ir(A', RC[i])` for `i = 0..8`
ascending. The reference implementation (`_core_inverse`) iterates `i = 8..0`
**descending** and uses `AddConstant(state, i + 9)`, i.e. `RC[17] … RC[9]` — the
constants of the *second* half of the 18-round `Cube-p`, which is what the ZIP
construction `Cube-p_{0..8}(A) XOR Cube-p_{9..17}^{-1}(A)` actually needs. The
code even carries the comment `// for 18 rounds mcube`.

I built both variants and hashed the spec's own test input:

| variant | MasterCube-512("5D9B", 16 bits) |
|---|---|
| reference code (RC[17..9], descending) | `C6ADAAFA…336B3FF7` — **matches Appendix A** |
| spec Algorithm 4 as literally written (RC[0..8], ascending) | `A0EE2439…22F59A5E` |
| descending order but `RC[0..8]` | `7AFEA540…B304EDD6` |

So Algorithm 4 as printed does not generate the specification's own test
vectors. The implementation is right and the pseudocode is wrong; the spec needs
to state the inverse branch's constant schedule explicitly. Note also that the
hard-wired `+ 9` is only correct for `N_ROUNDS = 18` — changing the round count
would silently break the constant schedule.

### Other observations (not spec deviations)

- **Portability / build fragility.** The whole reference file is inside
  `#if (!defined __AVX512F__ …) && !defined(__AVX2__) && !defined(__SSE4_2__)`;
  compiled with `-msse4.2`, `-mavx2` or `-march=native`, `MasterCube*_plain`
  degenerates to `return 1` while `mastercube.h` redirects the public name to
  `MasterCube512_sse` / `_avx2` / `_avx512_256`, which do not exist in the
  Reference_Implementation directory — the build breaks. The NGCC build uses
  plain flags, so the KATs pass.
- **Endianness and alignment.** Message blocks are absorbed through
  `((uint16_t *)(message + …))[k]` (unaligned, and little-endian-dependent) and
  the round constant through `(uint16_t *)c`. The algorithm as implemented
  therefore only produces the specified digests on a little-endian machine, and
  the unaligned 16-bit loads are formally undefined behaviour in C.
- `_sponge_squeeze_helper` copies `n_m128_per_iter` 16-byte units into
  `buffer[i][96]`; for the 768 instance that is 96 bytes (exactly the buffer
  size) of which only 88 (the true rate) are used. No overflow, but the extra
  16 bytes read beyond the rate are capacity bits — they are discarded, so no
  leak, but the code is one constant away from leaking capacity.
- Not verified: any rotational-XOR, differential-linear or algebraic analysis of
  `MAndRX`/`Cube-p`, and the transformation-based sponge bound `2^(c-1)` of
  Sect. 3.1 that Table 2's preimage claims rest on.
