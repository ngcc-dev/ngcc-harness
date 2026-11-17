# hash-16 LLH — algorithm summary

Ultra-low-latency hash family built as a **sponge variant with a capacity
feed-forward**. The state is a 24 x w bit matrix (24 "words" of w bits);
the permutation `P` is fully bitsliced: a 24-bit "super S-box" `S24` applied to
each of the w columns, a per-word rotation (ShiftRows), and a round constant on
the last word. Instances differ only in the word size w ∈ {32, 64, 96, 128},
which scales state, rate, capacity, round count and digest together.

Specification: `hash-16-spec.pdf` (16 pages, English), Sect. 2.2–2.4 +
Appendix A.

## Parameters

| parameter | LLH-256 | LLH-512 | LLH-768 | LLH-1024 | meaning |
|---|---|---|---|---|---|
| w (word size) | 32 | 64 | 96 | 128 | bits per state row |
| b = 24w (state) | 768 | 1536 | 2304 | 3072 | Table 1-3 |
| r = 8w (rate) | 256 | 512 | 768 | 1024 | first 8 words |
| c = 16w (capacity) | 512 | 1024 | 1536 | 2048 | last 16 words |
| N_r (rounds) | 16 | 24 | 32 | 40 | `N_r = 8 + ceil(w/4)`, Sect. 1.4.2 |
| d (digest) | 256 | 512 | 768 | 1024 | `d = r` |
| claimed collision | 128 | 256 | 384 | 512 | `min(d/2, c/2) = d/2` |
| claimed preimage | 256 | 512 | 768 | 1024 | `d` bits, Sect. 1.1 |
| max message | 2^64 - 1 bits | " | " | " | Sect. 1.1 |

Digest length, specification vs the built reference library
(`OBSERVED/hash-16.txt`):

| instance | d spec | digest_bits impl | digest_bytes impl | match |
|---|---|---|---|---|
| LLH-256 | 256 | 256 | 32 | yes |
| LLH-512 | 512 | 512 | 64 | yes |
| LLH-768 | 768 | 768 | 96 | yes |
| LLH-1024 | 1024 | 1024 | 128 | yes |

No XOF instance. `d` is always exactly the rate `r`, so there is no squeezing
loop and no output-length parameter; `CryptHash()` rejects any
`digest_len_bits != DIGEST_BIT_LENGTH`.

## Pseudocode

### Round function `RoundFun` (Sect. 2.4), state = W[0..23], each w bits
```
# (1) SubColumn — the ONLY nonlinear layer, applied to each of the w columns
#     (bitsliced: every operation below is a full w-bit word operation)
#     24 bits v0..v23 laid out row-major as a 6 x 4 matrix; the four COLUMNS
#     (v_{4k+j})_{k<6} each go through the 6-bit S-box S6:
#        S6(x) = x ^ (x >>> 2) ^ ((x >>> 1) & (x >>> 3)) ^ 0x1B     (6-bit rotations)
for j = 0..3:                                    # the four columns
    x_k = W[4k + j],  k = 0..5
    T[4k+j] = x_k ^ x_{k+2 mod 6} ^ (x_{k+1 mod 6} & x_{k+3 mod 6}) ^ bit_k(0x1B)
    # 0x1B = 0b011011 -> complement rows k = 0,1,3,4 ; rows k = 2,5 uncomplemented

#     derangement (spec Table 2-3) then row x D:
#        D = [[0,1,1,1],[1,0,1,1],[1,1,0,1],[1,1,1,0]]  ->  R'_j = XOR_{i != j} R_i
DER = [ (4,9,18,23), (8,1,22,15), (12,17,6,3),
        (20,5,10,19), (0,21,14,11), (16,13,2,7) ]     # one 4-tuple per output row
for row = 0..5:  (a,b,c,d) = DER[row]
    W[4*row+0] = T[b]^T[c]^T[d]
    W[4*row+1] = T[a]^T[c]^T[d]
    W[4*row+2] = T[a]^T[b]^T[d]
    W[4*row+3] = T[a]^T[b]^T[c]

# (2) ShiftRows
for i = 0..23:  W[i] = W[i] <<< sigma_i        # sigma table per w, spec Table 2-4

# (3) AddConstant — last word only
W[23] ^= RC_t                                  # RC_t = bits [t*w, (t+1)*w) of frac(e)
```
`P(S) = RoundFun applied N_r times.`

### Hash(M, L)  (Sect. 2.2)
```
r = 8w ;  blocks are loaded LITTLE-ENDIAN into the first 8 words

# padding (Sect. 2.2.1) — all-ONES padding, no length field in the message
if L mod r == 0:  M' = M                       # NO padding block at all
else:             M' = M ‖ 1^(r - (L mod r))
M' = M_1 ‖ .. ‖ M_k,  |M_i| = r
# within a partial byte the valid message bits occupy the LOW bit positions
# (little-endian bit order), the high bits are the '1' padding.

# initialization (Sect. 2.2.2 (1)) — HAIFA-style, L is the domain/length constant
S^(0) = 0^(b-64) ‖ L                           # message bit length in the last 64 bits

# absorbing (Sect. 2.2.2 (2))
for i = 1..k:
    S^(i) = P( S^(i-1) )  XOR  ( M_i ‖ S^(i-1)_{r..b-1} )
            #                     ^ rate injection   ^ capacity feed-forward

# squeezing (Sect. 2.2.2 (3)) — a single extra permutation, no loop
S^(k+1) = P( S^(k) )
return Z = S^(k+1)_{0..d-1}                    # d = r = the first 8 words
```
Injectivity of the encoding does **not** come from the padding (`1*` padding is
not suffix-free, and a message whose length is a multiple of `r` gets no
padding at all) — it comes from `L` being embedded in the initial state.

## Implementation vs specification

Checked: `src/LLH-*/llh_core.c` (the algorithm) and
`src/LLH-*/CryptHash_AlgorithmInstance.c` (the API wrapper, which `#include`s
`llh_core.c` after `#define W ...`; `hash-16/Makefile` correctly compiles only
`CryptHash_AlgorithmInstance.c` + `drng.c`). The LLH_ASIC tree was ignored as
instructed. The four `llh_core.c` copies differ only in `word_t`/`llh_rotl`
(32 / 64 / 96-as-{u32,u64} / `__uint128_t`), the `SIGMA` table and the `RC`
table.

Verified agreements:

- **Rounds:** `NR_DEFAULT = 8 + (W/32)*8` gives 16 / 24 / 32 / 40, identical to
  the spec rule `N_r = 8 + ceil(w/4)` and to Table 1-3 for all four w. **No
  round reduction.**
- **Rate/capacity:** `RATE_ROWS 8`, `CAP_ROWS 16`, `ROWS 24` ⇒ r = 8w,
  c = 16w, b = 24w — matches Table 1-3 for every instance.
- **S6:** `llh_round`'s six `T[]` equations are exactly
  `x_k ^ x_{k+2} ^ (x_{k+1} & x_{k+3})` over each of the four columns
  {0,4,8,12,16,20}, {1,5,…}, {2,6,…}, {3,7,…}, with `WORD_NOT` applied on rows
  k = 0,1,3,4 and not on k = 2,5 — i.e. XOR with `0x1B = 0b011011` in the
  spec's LSB-first 6-bit convention. Exact match.
- **Derangement:** I extracted the image of spec Table 2-3 (page 9 of the PDF,
  it is a figure and not extractable as text) and compared it to the six
  `LLH_MIX_FAMILY` index tuples. The spec's output matrix is
  `4 9 18 23 / 8 1 22 15 / 12 17 6 3 / 20 5 10 19 / 0 21 14 11 / 16 13 2 7`,
  identical to the code's tuples. It is a genuine derangement (no fixed point)
  and a bijection on 0..23.
- **D matrix:** `LLH_MIX_FAMILY` computes `R'_j = XOR_{i != j} R_i`, which is
  right-multiplication by the spec's `D` over GF(2). Match.
- **ShiftRows:** all 24 `SIGMA` values match spec Table 2-4 for each of
  w = 32, 64, 96, 128 (checked all 96 entries).
- **AddConstant:** `state[23] ^= rc` = `W_23 ^= RC_t` (last word only). All
  round constants match Appendix A tables A-1…A-4 (16 / 24 / 32 / 40 entries,
  the digits of frac(e)); the w = 64 and w = 128 tables are literally the
  w = 32 stream re-segmented, as the spec's derivation rule requires.
- **Mode:** `hash_process_block` = `permutation(); capacity ^= prev_capacity;
  rate ^= block` = `S^(i) = P(S^(i-1)) ⊕ (M_i ‖ S^(i-1)_cap)` of Sect. 2.2.2(2),
  and `hash_final` performs the final `P` and reads the first 8 words —
  Sect. 2.2.2(3). Match.
- **Initialization:** `hash_init` writes the 64-bit message length into the
  last 64 bits of the state: `row[23] = L` for w >= 64, and `row[22] = L_lo32,
  row[23] = L_hi32` for w = 32 (where a single word is only 32 bits). Under the
  little-endian word/byte convention of Sect. 2.2.1 this is the same
  `0^(b-64) ‖ L` for w = 32 and 64. For w = 96 and 128 the code stores L in
  bits 0..63 of word 23 and leaves the last w−64 state bits zero, which the
  formula does not determine; the C code, the RTL and the KATs agree with each
  other.
- **Padding:** `hash_final` appends a block only when `buf_len != 0`, filling
  with `0xFF` — i.e. `1*` padding, and no padding at all when `L mod r == 0`.
  Exactly Sect. 2.2.1 rules (1) and (2).
- **Bit order:** `load_word` is little-endian (`0A 01 03 04 -> 0x0403010A`, the
  spec's own example), and the non-byte-aligned tail handling in
  `CryptHash_AlgorithmInstance.c` reverses the API's MSB-first tail bits into
  LSB-first and ORs in `0xFF << rem_bits`. For the spec's example (2-bit
  message "10") this yields `0xFD`, exactly the value the spec states.

Discrepancies: **none found.** No round-count reduction, no altered padding
rule, no rate/capacity mismatch, and no constant hard-wired that the spec says
should vary (in particular `N_r`, `SIGMA` and `RC` all scale with `w`).

Notes / observations (not deviations):

- The padding is `1*` with no length field in the message stream; message
  encoding injectivity depends entirely on `L` in the initial state. The spec
  is explicit about this, and the API always supplies `L`, so it is sound for
  the one-shot interface — but any incremental/streaming API built on
  `hash_init/hash_absorb/hash_final` must know `L` up front. The shipped
  `hash_init(ctx, msg_len_bits)` signature reflects that constraint.
- `llh_rotl` is UB for a 0-bit rotation; no `SIGMA` entry is 0 for any w.
- The w = 96 instance emulates a 96-bit word as `{uint32 hi, uint64 lo}` with a
  hand-written branchy rotation. I reviewed the branches (n<32, n==32, …) and
  found no boundary error, but I did not exhaustively test them; the KAT for
  LLH-768 passes, which exercises the concrete `SIGMA` values used.
- `CryptHash()` correctly validates `digest_len_bits` and NULL pointers, unlike
  several sibling candidates.
- Not verified: the "ultra-low latency" depth claims, and any cryptanalysis of
  the super S-box `S24` or of the reduced-round permutation. The spec contains
  no differential/linear bound tables to check against.

## Review of 2026-11-17

S6, the derangement matrix, σ, the round counts and all 112 round constants were recomputed and match the code and Appendix A. The reference and optimized C trees agree for every bit length from 0 to 11 blocks. The submitted ASIC RTL does not: its padder discards a short final block; filed as `hash-16-1`. The optimized AVX-512 path uses aligned loads on unaligned table entries and crashes when enabled, but the shipped Makefiles force the AVX2 path.
