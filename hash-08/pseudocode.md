# hash-08 Duet — algorithm summary

Permutation-based **dual-branch alternating sponge with capacity feed-forward**.
The persistent state is two r-bit rate branches `A`, `B` plus one shared c-bit
capacity branch `C`. Each message block costs two serial permutation calls:
`f1` absorbs `M_i` into `A`, `f2` absorbs the word-permuted `sigma(M_i)` into
`B` using the capacity `f1` just produced. After every call the capacity output
is XORed with the capacity input (feed-forward). The digest is squeezed from
the **capacity**, not the rate, which is what blocks length extension.
Security is claimed in the ideal-permutation model.

Specification: `hash-08-spec.pdf` (29 pages), §2 (Algorithm 1, §2.3–§2.6,
Tables 1–5), claims in §3.3 / §4.1. English-language spec.

## Parameters

| parameter | Duet-512 | Duet-768 | Duet-1024 | meaning |
|---|---|---|---|---|
| digest h | 512 | 768 | 1024 | spec Table 5 |
| rate r | 384 | 1088 | 768 | bits absorbed per block |
| capacity c | 576 | 832 | 1152 | shared branch, source of output |
| width b = r + c | 960 | 1920 | 1920 | per permutation call |
| persistent state 2r + c | 1344 | 3008 | 2688 | A, B and C together |
| rounds N_b | 12 | 18 | 18 | spec §2.6 |
| state array | 5 x 3 words | 5 x 6 | 5 x 6 | 64-bit words |
| squeeze rate r' | 512 (= h) | 768 (= h) | 1024 (= h) | spec §2.1, §2.5 |
| sigma word order | (3,0,5,1,4,2) | (9,2,15,0,6,12,4,16,1,10,7,14,3,11,5,13,8) | (8,2,11,0,5,9,3,10,1,7,4,6) | 6 / 17 / 12 words |
| claimed collision | 256 | 384 | 512 | h/2, classical (§4.1) |
| claimed preimage | 512 | 768 | 1024 | h, classical |
| claimed 2nd-preimage | 512 | 768 | 1024 | h, classical |
| claimed quantum coll. | 171 | 256 | 341 | h/3 |
| claimed quantum preimage | 256 | 384 | 512 | h/2 |

Digest sizes, specification vs the built reference library:

| instance | digest spec (bits) | digest impl (bits/bytes) | match |
|---|---|---|---|
| Duet-512 | 512 | 512 / 64 | yes |
| Duet-768 | 768 | 768 / 96 | yes |
| Duet-1024 | 1024 | 1024 / 128 | yes |

`c >= h` holds for all three, and `c - h` (64 / 64 / 128 bits) covers the
`log2(2 N_max) < 57` long-message term the spec requires for h-bit
second-preimage resistance at `L_max = 2^64 - 1` (§3.3).

## Pseudocode

### Padding (spec §2.3)

```
Pad10*(M, r):  M* = M || 1 || 0^k,  k = smallest >= 0 with (|M| + 1 + k) mod r == 0
               # applied unconditionally; if |M| is a multiple of r the extra
               # block is exactly 1 || 0^(r-1)
M* = M_0 || M_1 || ... || M_l,   |M_i| = r
```

There is no length encoding and no final-block domain constant; separation
between absorbing and squeezing comes from the fact that squeezing injects
nothing and reads the capacity.

### Hash / XOF (spec Algorithm 1)

```
Duet_h(M, nu):
    r' = h
    M_0..M_l  <- Pad10*(M, r)
    A <- 0^r ; B <- 0^r ; C <- 0^c                  # all-zero initial state, no IV

    # --- absorbing ---
    for i = 0 .. l:
        (A, D1) <- f1( (A XOR M_i)        || C ) ;  C <- D1 XOR C
        (B, D2) <- f2( (B XOR sigma(M_i)) || C ) ;  C <- D2 XOR C

    # --- squeezing, from the CAPACITY ---
    Z <- trunc_{r'}(C)                              # z_0 ; the h-bit digest is exactly this
    while |Z| < nu:
        (A, C) <- F1(A, C) ;  Z <- Z || trunc_{r'}(C)
        if |Z| < nu:
            (B, C) <- F2(B, C) ;  Z <- Z || trunc_{r'}(C)
    return Z[0 : nu-1]

F_s(R, C):                                          # capacity feed-forward call
    (R', D) = f_s(R || C)
    return (R', D XOR C)
```

`sigma` is a fixed permutation of the r/64 64-bit words of the message block
(orders in the parameter table).

### Permutation f-b (spec §2.6, Algorithm 2)

```
state X = (X[i][j]),  0 <= i < 5 (rows), 0 <= j < m (columns),  m = 3 or 6
linear word index  t = 5j + i

for gamma = 0 .. N_b - 1:
    X <- SC(X)                    # 5-bit S-box on each (column, bit-position) slice:
                                  #   u = x[0][j][k] || ... || x[4][j][k], x[0] the MSB
    X <- MS(X)                    # Y = M_{5m} . X  (right-circulant block matrix)
                                  #   f-960  : offsets {0,3,4,5,7,8,13}          mod 15
                                  #   f-1920 : offsets {0,3,10,13,14,15,16,18,21,26,28} mod 30
    X[0][0] <- X[0][0] XOR C_{alpha,b,gamma}        # alpha = 1 or 2: f1 and f2 differ
                                                    # ONLY in these round constants
    X <- ML(X)                    # per-word: X <- (X <<< t) ^ (X <<< t+21) ^ (X <<< t+43)
```

Round constants: `f-960` from frac(pi) (Table 3), `f-1920` from frac(e)
(Table 4); the first N_b words instantiate `f1`, the next N_b `f2`.

## Implementation vs specification

Source reviewed: `hash-08/src/Duet-{512,768,1024}/CryptHash_Duet-<inst>.c`
(454 / ~470 / ~460 lines, three self-contained C99 files; line numbers below
are from `CryptHash_Duet-512.c`).

Verified agreements:

- `r`, `c`, `ROUNDS` enums (ll. 17–32) are 384/576/12, 1088/832/18 and
  768/1152/18 — match spec Table 5 and §2.6 for all three instances.
- The S-box is implemented as the spec's Boolean circuit (ll. 60–131, `t0`…
  `t51` with outputs `y0 = t14`, `y1 = t24`, `y2 = t34`, `y3 = t43`,
  `y4 = t51`). I **evaluated the spec's Table 2 circuit over all 32 inputs and
  it reproduces the Table 1 S-box exactly**, and the C code is a
  statement-for-statement transcription of Table 2. Both tables are therefore
  mutually consistent and correctly implemented.
- MS layer: `permutation960_l15()` (ll. 134–162) computes
  `out[i] = in[i]^in[i+3]^in[i+4]^in[i+5]^in[i+7]^in[i+8]^in[i+13] (mod 15)`.
  The spec prints `M15` as an explicit 15x15 matrix; I checked its first three
  rows and it *is* right-circulant with exactly that support, so the code
  matches. `permutation1920_l30()` uses offsets
  `{0,3,10,13,14,15,16,18,21,26,28} (mod 30)`, which is exactly the spec's
  first-row vector `v`.
- ML layer (ll. 164–173): `shift = x*5 + y` with `x` the column and `y` the
  row, i.e. the spec's `t = 5j + i`, and the three rotations `t`, `t+21`,
  `t+43` (reduced mod 64 inside `rotl64`) match.
- Round order `SC -> MS -> AC -> ML` (ll. 175–182) matches
  `phi_gamma = ML o AC o MS o SC`; AC hits `state[0][0] = X_{0,0}`.
- Round constants: `PI[24]` (Duet-512, ll. 42–51) equals Table 3 column-wise
  (first 12 = `C_{1,960,*}`, next 12 = `C_{2,960,*}`); `E[36]` (Duet-1024,
  ll. 42–55) equals Table 4. `f1`/`f2` are selected purely by the constant
  offset (`base = (flag == 2) ? ROUNDS : 0`, l. 205), exactly as the spec
  defines them.
- `sigma_permutation()` reproduces all three spec word orders verbatim.
- Capacity feed-forward `apply_f1`/`apply_f2` (ll. 310–336): permute
  `rate||C`, keep the rate output, and set `C_out = D XOR C_in` — matches
  `F_s`. `absorb_block()` (ll. 338–361) chains `C1` from `f1` into the `f2`
  call, matching Algorithm 1 lines 10–13.
- Initial state is all zero (`A`, `B`, `C` zero-initialised, l. 390–392) —
  matches §2.4; there is no IV.
- **Partial-byte input is handled correctly.** `pad_message_block()`
  (ll. 273–295) masks the final byte with `0xff << (8 - rem_bits)` before
  OR-ing in the `1` bit, so bits past `msg_len_bits` cannot affect the digest.
  `clear_unused_digest_bits()` likewise zeroes the unused tail of the digest.
- `CryptHash()` validates `digest_len_bits`, `digest` and `msg` and returns
  distinct negative codes.

### Discrepancy 1 (real deviation, all instances): the squeezing rate is `c`, not `r' = h`

Spec §2.1 fixes `r' = h <= c` and §2.5 defines every squeezed fragment as
`z_j = trunc_h(C)`; Algorithm 1 lines 16/19/22 all use `trunc_{r'}`.

`squeeze_copy_capacity()` uses `c` instead (l. 372, and l. 407 / l. 396 in the
-768 / -1024 files):

```c
take_bits = *remaining_digest_bits > c ? c : *remaining_digest_bits;
```

For a request of exactly `h` bits the two rules coincide (`min(h, c) = h`), so
**the shipped fixed-length digests and the KATs are unaffected**. But for any
`nu > h` the implementation emits `c` bits per squeeze instead of `h`
(576 vs 512, 832 vs 768, 1152 vs 1024), producing an output stream that
disagrees with Algorithm 1 from bit `h` onward. The exported API accepts
`digest_len_bits` up to 8192, so this path is reachable: `Duet-512` asked for
1024 bits returns `C[0:575] || F1(C)[0:447]`, whereas the spec's XOF returns
`C[0:511] || F1(C)[0:511]`. This also leaks 64–128 capacity bits per squeeze
beyond what the security analysis of §4.2 (which assumes `r' = h <= c`)
covers.

### Discrepancy 2 (memory safety, Duet-768 only): one-word out-of-bounds stack read

`sigma_permutation()` in `CryptHash_Duet-768.c` declares
`uint64_t block[WORDS_r]` with `WORDS_r = 1088/64 = 17` (indices 0..16) but
reads

```c
const uint64_t w17 = block[17];
```

`block` is the caller's `uint64_t SigmaMi[WORDS_r]` stack array
(`absorb_block()`), so this is a one-word read past the end of a stack object.
The value `w17` is never used, so the digest is unaffected and a compiler may
elide the load, but it is undefined behaviour; AddressSanitizer reports it at `-O0`, UndefinedBehaviorSanitizer at `-O1` and `-O2`, and Valgrind does not. Filed as `hash-08-1`.
The corresponding `-512` and `-1024` files read exactly `WORDS_r` words and are
clean.

### Spec gap (class b): the mode-to-permutation word mapping is unspecified

§2.6 describes the permutation state as a 5 x m array ordered "column by
column" (`t = 5j + i`), but the specification never says how the `(r + c)`-bit
string `R || C` that the mode hands to `f_s` maps onto those words, nor how
message bytes map to 64-bit words. The implementation uses the obvious choice
(big-endian 64-bit words, rate at linear indices `0 .. r/64-1`, capacity at
`r/64 .. (r+c)/64-1`), which is almost certainly the intent, but a second
implementer has nothing normative to follow.

### Minor observations

- `f1` and `f2` differ only in their 12 (resp. 18) round constants, while
  §4.1 models them as "two independent ideal permutations". That is the
  spec's own instantiation, not an implementation deviation, but it is the
  weakest link between the proof model and the concrete design.
- No `params.h`/`config.h`; the parameter spot-check above covers `r`, `c`,
  `ROUNDS`, both circulant MS supports, the ML rotation rule, all round
  constants of two of the three instances, all three `sigma` orders and the
  S-box.

### Not verified

- The remaining 12 rows of the printed `M15` matrix (rows 0–2 were checked for
  circulancy) and the round constants of Duet-768.
- The advantage bounds of §4.2 and the permutation cryptanalysis of §4.4.

## Review of 2026-11-17

The S-box tables, the M15 and M30 branch numbers, the invertibility of ML, and the round constants were recomputed and match the specification. The two-round active S-box bounds (6 and 8) were confirmed and are tight; the three-round bound of 17 for f-960 was proven only down to 13. The reference, AVX2, AVX-512 and NEON trees agree on all tested lengths. Both discrepancies recorded above are filed: the stack over-read as `hash-08-1` (reference Duet-768 only) and the squeeze rate for outputs longer than h as `hash-08-2` (all four software trees).
