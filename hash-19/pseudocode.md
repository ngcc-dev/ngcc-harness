# hash-19 MoFang — algorithm summary

Block-cipher-based hash family (not a sponge). The primitive is **MoFang-BC**,
a 576-bit block cipher (3 x 3 array of 64-bit words, 16 rounds of
S-box / MixColumn / ShiftRow / AddRoundKey) whose key is the message block
(k = 2n = 1152 bits, or 3n = 1728 in the double-length mode). MoFang-256/512
use **Davies–Meyer + MDP** (Merkle–Damgård with a permutation applied to the
chaining value before the last block). MoFang-768/1024 use **MDPH** = Hirose
double-block-length + MDP. The two XOFs extend the MDP tail by iterating the
permuted compression function ("MDMP").

Specification: `hash-19-spec.pdf` (36 pages, English), Sect. 2.2–2.8,
Algorithms 1–6, Tables 1–8.

## Parameters

| parameter | -256 | -512 | -768 | -1024 | -256-XOF | -768-XOF | meaning |
|---|---|---|---|---|---|---|---|
| mode | DM+MDP | DM+MDP | MDPH | MDPH | DM+MDP | MDPH | Table 1 |
| block cipher n | 576 | 576 | 576 | 576 | 576 | 576 | state of MoFang-BC |
| chaining state | 576 (n) | 576 (n) | 1152 (2n) | 1152 (2n) | 576 (n) | 1152 (2n) | Table 1 |
| message block | 1152 | 1152 | 1152 | 1152 | 1152 | 1152 | = key size, Alg. 5/6 |
| rounds r | 16 | 16 | 16 | 16 | 16 | 16 | Table 1 |
| digest h | 256 | 512 | 768 | 1024 | arbitrary ℓ | arbitrary ℓ | Table 1 |
| domain value (low 4 bits of IV) | 0101 | 0110 | 1010 | 1011 | 0111 | 1111 | Table 7 |
| MDP permutation π | ⊕1 | ⊕1 | ⊕2 (on L) | ⊕2 (on L) | ⊕1 | ⊕2 (on L) | Alg. 5 l.11 / Alg. 6 l.15 |
| claimed collision | 128 | 256 | 384 | 512 | min(128, ℓ/2) | min(384, ℓ/2) | Table 8 |
| claimed preimage | 256 | 512 | 768 | 1024 | min(256, ℓ) | min(768, ℓ) | Table 8 |
| claimed 2nd preimage | 256 | 512 | 768 | 1024 | min(256, ℓ) | min(768, ℓ) | Table 8 |

Digest length, specification vs the built reference libraries
(`OBSERVED/hash-19.txt`):

| instance | ALGORITHM_INSTANCE | h/ℓ spec | digest_bits impl | digest_bytes impl | match |
|---|---|---|---|---|---|
| MoFang-256 | MoFang_256 | 256 | 256 | 32 | yes |
| MoFang-512 | MoFang_512 | 512 | 512 | 64 | yes |
| MoFang-768 | MoFang_768 | 768 | 768 | 96 | yes |
| MoFang-1024 | MoFang_1024 | 1024 | 1024 | 128 | yes |
| MoFang-256-XOF | MoFang_256_XOF | arbitrary | 256 | 32 | yes (one block only) |
| MoFang-768-XOF | MoFang_768_XOF | arbitrary | 768 | 96 | yes (one block only) |

Unlike hash-18 / hash-20, the hash-19 Makefile builds **one** library per XOF at
its header's `DIGEST_BIT_LENGTH`, i.e. exactly one output block. The multi-block
squeeze path is therefore never exercised by any KAT — see Discrepancy 3.

## Pseudocode

### MoFang-BC (spec Algorithm 1), state `I[y][x]` of 64-bit words, 16 rounds
```
K = message block (2 or 3 sub-blocks of 576 bits)
A = I XOR (XOR of all K sub-blocks)                      # whitening
for i = 0..15:
    K  = KeyExp(K, i)
    A  = SBox(A)         # 3-bit S-box down each column, i.e. per row-triple:
                         #   y0 = (x1 & x2) ^ x0
                         #   y1 = (y0 & x2) ^ x1          <- sequential, see Disc. 1
                         #   y2 = (y0 | y1) ^ x2
    A  = MDS(A)          # M = [[3,2,2],[2,3,2],[2,2,3]] over GF(2^3):
                         #   t = S0^S1^S2 ; t = MUL(t) ; S_j ^= t  for j = 0,1,2
                         #   MUL(x0,x1,x2) = (x2, x0^x2, x1)
    A  = ShiftRow(A, i)  # rotate each word by rho[i mod 2][y][x], Table 3
    A  = A XOR (XOR of all K sub-blocks)
return A

KeyExp(K, i):   # per sub-block: S-box on the FIRST ROW only, then a word
                # transposition (Table 5), then a per-word rotation (Table 6);
                # finally  K_last[0][0] ^= RC[i]     (RC = hex digits of pi)
```

### MoFang-256 / -512 / -256-XOF  (spec Algorithm 5)
```
I = 0^572 ‖ domain4              # domain4 = 0101 / 0110 / 0111
M_1..M_s = M ‖ 1 ‖ 0*            # blocks of 1152 bits
for i = 1..s-1:  I = CF(I, M_i)
I = CF(I XOR 1, M_s)             # MDP permutation before the LAST block
CF(h, M)  =  MoFang-BC_M(h) XOR h                        # Davies-Meyer
          # (b) Sect. 2.3.1 (p. 5) writes BC-DM as BC_h(m) XOR m (MMO); Algorithm 5,
          # Sect. 2.4.4 and Sect. 6.1 key the cipher with the message, as here
T = MSB_min(ℓ,256 or 512)(I)
# XOF extension (ℓ > 256):
for i = 1..floor(ℓ/256):
    I = CF(I XOR 1, 0)           # SPEC: the message block is ALL ZERO
    T = T ‖ MSB_t(I)
return T
```

### MoFang-768 / -1024 / -768-XOF  (spec Algorithm 6, Hirose DBL + MDP)
```
L = 0^572 ‖ domain4              # 1010 / 1011 / 1111 ;  R = 0^576
M_1..M_s = M ‖ 1 ‖ 0*            # 1152-bit blocks
CF(L, R, M):
    T  = R ‖ M                                  # 1728-bit key
    L' = MoFang-BC_T(L)     XOR L
    R' = MoFang-BC_T(L ^ 1) XOR L XOR 1         # Hirose constant c = 1
    return (L', R')
for i = 1..s-1:  (L,R) = CF(L, R, M_i)
(L,R) = CF(L XOR 2, R, M_s)      # MDP permutation, applied to L
T = MSB_{ℓ/2}(L) ‖ MSB_{ℓ/2}(R)
# XOF extension (ℓ > 768):
for i = 1..floor(ℓ/768):
    (L,R) = CF(L XOR 2, R, 0)
    T = T ‖ MSB_t(L) ‖ MSB_t(R)
return T
```
Variable digest lengths come from truncation plus additional `CF(· XOR π, 0)`
iterations; **instance separation is the 4-bit domain value in the IV only**.

## Implementation vs specification

Checked: all six `src/MoFang-*/CryptHash_AlgorithmInstance.c` (each is
self-contained; the cipher source is duplicated in every one).

Verified agreements:

- **Rounds:** `#define ROUND 16` in all six files = Table 1. No reduction.
- **Cipher structure:** `encrypt1` is spec Algorithm 1 step for step (whitening
  `I ⊕ K`, then `KeyExp; SBox; MDS; ShiftRow; AddRoundKey`), with the round key
  being the XOR of all key sub-blocks as Sect. 2.4.4 requires.
- **MixColumn:** `mul_row`/`MixColumns` reproduce spec Algorithm 2 exactly
  (`MUL` LFSR and the `t = S0^S1^S2` trick for `M = [[3,2,2],[2,3,2],[2,2,3]]`).
- **ShiftRow:** `rhoy0` / `rhoy1` match spec Table 3 for both round parities,
  all 18 constants, under the array convention `code[y][x]` (which is the
  spec's own "words are fed into the state by row, (0,0),(1,0),(2,0),(0,1)…"
  order — the same convention the message-loading loop uses).
- **Key expansion:** the block transpositions are exchanged relative to
  Algorithms 3 and 4 (Discrepancy 6); `rhok[0]` / `rhok[1]` match spec Table 6 (all 18 entries); the S-box is
  applied to the first row only; `key1[0][0] ^= ct[round]` matches "the round
  constant xor the last byte of the first word for the last block", and
  `ct[0..15] = 0x24,0x3F,…,0x44` matches Table 4.
- **Padding:** `0x80` then zeros to a multiple of **1152** bits, always adding
  at least one bit = `M ‖ 1 ‖ 0*` of Algorithms 5 and 6.
- **MDP:** `if (t == blocks-1) state[2][2] ^= 1` (DM) and `in_g[2][2] ^= 2`
  (DBL) apply the permutation to the chaining value before the last block only.
- **Hirose DBL:** `MoFang_BC_2_DBL` computes `out_g = E(in_g) ^ in_g` and
  `out_h = E(in_g ^ 1) ^ in_g ^ 1` with key `in_h ‖ m0 ‖ m1` — spec Algorithm 6
  lines 2-4 with `c = 1`.
- **Domain values:** `state[2][2] = 5 / 6 / 7` and `in_g[2][2] = 10 / 11 / 15`
  match spec Table 7 (`0101 / 0110 / 0111 / 1010 / 1011 / 1111`) for all six
  instances.
- **Output:** MSB truncation of the chaining value in the spec's word order
  (`state[0][0], state[0][1], state[0][2], state[1][0], …`), and
  `MSB_{ℓ/2}(L) ‖ MSB_{ℓ/2}(R)` for the DBL instances.

### Discrepancy 1 — **(b) spec defect: the S-box formula in Sect. 2.4.1 is wrong (and not even a permutation)**

Sect. 2.4.1 defines
`y0 = (x1 & x2) ^ x0 ;  y1 = (x2 & x0) ^ x1 ;  y2 = (x0 | x1) ^ x2`
using the *original* inputs. Evaluating that map gives `S(6) = 3` and
`S(7) = 0`, contradicting the spec's own lookup **Table 2**
(`S = [0,5,6,7,4,3,1,2]`, so `S(6) = 1`, `S(7) = 2`); worse, the formula maps
both 0 and 7 to 0, so it is **not a bijection** and cannot be a block-cipher
S-box at all.

The implementation (`Sbox()`) uses the *sequential* form
`y0 = (x1&x2)^x0 ; y1 = (y0&x2)^x1 ; y2 = (y0|y1)^x2`, which I verified
reproduces Table 2 on all 8 inputs. So the code is right and the algebraic
definition in Sect. 2.4.1 needs correcting (this is the SEA S-box, which is
indeed defined sequentially).

Sect. 6.2.2 (p. 25) also calls x0 the most significant bit. Table 2 and the
code take x0 as the least significant; under the MSB reading neither the
formulas nor the code give Table 2. The code's S-box meets every property
Sect. 6.2.2 states (bijective, differential uniformity 2, linearity 4,
degree 2).

### Discrepancy 2 — **(a) real weakness: the domain values are not closed under the MDP permutation, contrary to the spec's own requirement**

Sect. 2.6 states: *"The domain encoding should be different from the last
permutation in MDP construction to avoid losing its functionality."* For the
DM instances π = ⊕0001, and the domain set is {0101, 0110, 0111}:

```
0110 (MoFang-512)     XOR 1 = 0111 (MoFang-256-XOF's IV domain)
0111 (MoFang-256-XOF) XOR 1 = 0110 (MoFang-512's IV domain)
```
so the *final-block* domain of MoFang-512 is bit-for-bit identical to the
*initial* domain of MoFang-256-XOF and vice versa (the IVs are all-zero apart
from those four bits, so the two 576-bit values coincide exactly). Concretely,
for any single-block message `M`:

```
MoFang-512(M)          = MSB_512( CF(IV_0111, M) )
MoFang-256-XOF's h_1 on any message starting with M = CF(IV_0111, M)
```
i.e. a MoFang-512 digest hands an attacker 512 of the 576 bits of
MoFang-256-XOF's internal chaining state (the only missing word is
`state[2][2]`, exactly the word holding the domain bits), and symmetrically
MoFang-256-XOF's output exposes MoFang-512's first chaining value. That is
precisely the "losing its functionality" the spec warns about, and it defeats
MDP's length-extension protection in any setting where both instances are used
on related inputs. The DBL triple {1010, 1011, 1111} with π = ⊕0010 maps to
{1000, 1001, 1101}, which is disjoint from the domain set — that half is fine.
The fix is to pick DM domain values whose ⊕1 images are outside the domain set.

### Discrepancy 3 — **(a) MoFang-256-XOF's squeeze uses the last message block instead of a zero block**

Spec Algorithm 5 line 16 is `I ← CF(I ⊕ 1^n, 0)` — a **zero** message block —
and the MDMP security proof in Sect. 2.8/Sect. 3 explicitly rests on "the
iteration is performed with a zero(no) input". The MoFang-256-XOF
implementation instead re-uses `K`, which still holds the **last message
block** from the absorb loop:

```
for (i = 0; i < output_blocks; i++)
    if (i != 0) { state[2][2] ^= 1; MoFang_BC_1_DM(state, K); }   /* K = M_s, not 0 */
```
MoFang-768-XOF does `memset(in_m, 0, …)` before its squeeze loop and is
correct on this point. Because both XOF libraries are built at exactly one
output block (`DIGEST_BIT_LENGTH` = 256 / 768 ⇒ `output_blocks == 1`), the
faulty branch is unreachable in the shipped libraries and untested by the KATs;
it becomes live for any ℓ > 256.

### Discrepancy 4 — **(a) MoFang-768-XOF applies the MDP permutation to the wrong branch in the squeeze**

Spec Algorithm 6 line 20 is `(L,R) ← CF(L ⊕ 2^n, R, 0)` — the permutation acts
on **L**. The code applies it to `in_h`, i.e. **R**:

```
if (i != 0) { in_h[2][2] ^= 2; MoFang_BC_2_DBL(in_g, in_h, in_m, out_g, out_h); }
```
(the absorb phase correctly uses `in_g[2][2] ^= 2`). Same reachability caveat
as Discrepancy 3: unreachable at ℓ = 768, live for ℓ > 768.

### Discrepancy 5 — **(b) spec internal inconsistency: padding block size**

Sect. 2.5 says "The message M is padded to be a multiple of **576** bits",
but Algorithm 5 line 8 and Algorithm 6 line 12 both say the blocks `M_i` are
**1152** bits (which is correct — the message block *is* the cipher key,
`k = 2n`). The implementation uses 1152. Sect. 2.5 should read 1152.

### Discrepancy 6 — **(a) the key-expansion transpositions are exchanged (recorded as hash-19-4)**

In Figure 5's coordinates (word (x, y) = `key[y][x]`, the convention Tables 3
and 6 need), `MessageExpansion1` line 138 computes C[x,y] = B[x+2y, x], which
is Algorithm 3 line 18's second-block formula, and line 150 computes
C[x,y] = B[y, 2x+y], line 7's first-block formula. MoFang-768 and -1024 are
exchanged on all three blocks (Algorithm 4 lines 4, 12 and 23). Table 5 fits
either assignment depending on the direction in which it is read. The printed
formulas reproduce 0 of 4,097 KAT_2_12 vectors at MoFang-256 and -768. Both
maps swap words 5 and 7, so Discrepancy 2's analysis is unaffected.

### Other observations

- **Undefined behaviour: zero rotation.** `rotl(x, 0)` evaluates `x >> 64`,
  which is UB in C. Four table entries are 0 (`rhoy0[0][0]`, `rhoy1[0][0]`,
  `rhok[0][1][1]`, `rhok[1][2][0]`), so this is hit on every single round of
  every call. It happens to give the right answer on x86-64 (the shift count is
  taken mod 64 by the hardware) — which is why the KATs pass — but the compiler
  is entitled to produce anything, and on a target where the shift count is not
  masked the hash would be silently wrong.
- Spec Algorithm 1 line 9 writes the rotation as `ρ^i_y`, i.e. depending only on
  the round and `y`, but Table 3 gives different values for different `x` at the
  same `y`. Notation error only; the table (and the code) are consistent.
- Spec Eq. (1) describes ShiftRow as `RotR(·, ρ)` while the code uses `rotl`.
  Eq. (1)'s index form `D[x,y,z] = C[x,y,z-ρ]` is a left shift of the data when
  bit 0 is the leftmost, so the two readings are the same under the spec's own
  bit numbering; with no test vectors in the specification this could not be
  settled independently. Flagged as a **spec ambiguity**.
- Memory handling is correct here (unlike hash-18): `padded_input` is
  `free()`d and the `malloc` result is NULL-checked.
- Not verified: the differential bounds of Sect. 6.2.3, the MitM analysis of
  Sect. 6.2.4, and the MDMP proof sketch of Sect. 2.8.
