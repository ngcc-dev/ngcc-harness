# hash-11 Garnet — algorithm summary

Permutation-based hash family on a **2048-bit state arranged as a 4 x 4 matrix
of 128-bit words**. One round of the permutation is
`P = zeta o theta o rho o sigma`: `sigma` applies one AES round (AESL /
`AESENC`) to each 128-bit word with a phase-dependent round key `B`, `rho` and
`zeta` are word-level MDS transforms over two *different* `GF(2^128)` fields
applied to the columns and the diagonals, and `theta` XORs Fibonacci constants
into the last column. Garnet-512 and Garnet-768 run the **Sponge** mode;
Garnet-1024a runs the **Sponge-DM** mode (full-state Davies–Meyer
feed-forward). Both modes are extended with a message-length injection and a
separate pre-squeezing permutation, and each phase uses a different AES round
key `B` for domain separation.

Specification: `hash-11-spec.pdf` (39 pages), §2 (§2.2 padding, §2.3 state/IV,
§2.4 permutation, §2.5 modes + Tables 3/4, §2.6 absorb/squeeze patterns),
claims in §4 / Table 5. English-language spec.

## Parameters

State width b = 2048 bits (16 x 128-bit words) for every instance.

| parameter | Garnet-512 | Garnet-768 | Garnet-1024a |
|---|---|---|---|
| mode | Sponge | Sponge | **Sponge-DM** |
| rate r (normative) | 512 … 1024 (5 variants) | 512 | 896 |
| capacity c = 2048 − r | 1536 … 1024 | 1536 | 1152 |
| init rounds P^a | 14 | 15 | 16 |
| absorb rounds P^b | 10 | 11 | 12 |
| middle rounds | 14 | 15 | 16 |
| squeeze rounds | 10 | 11 | 12 |
| `B` (init / absorb / middle / squeeze) | 0 / 0 / 1 / 2 | 0 / 0 / 3 / 4 | 0 / 0 / 7 / 8 |
| digest words | S0, S7, S9, S14 | S0, S3, S6, S9, S13, S15 | S0, S2, S5, S7, S9, S11, S12, S14 |
| digest, spec | 512 | 768 | 1024 |
| claimed collision / preimage / 2nd-preimage | 256 / 512 / 512 | 384 / 768 / 768 | 512 / 1024 / 1024 |

Built labels, rates and OBSERVED digest sizes (all five `Garnet_512_Cap*`
labels are rate variants of Garnet-512 selected by `counter0..counter4`):

| label | source file | rate r | capacity c | digest spec | digest impl | KAT |
|---|---|---|---|---|---|---|
| `Garnet_512_Cap512` | `Garnet_512_512.c` | 512 | 1536 | 512 | 512 / 64 B | PASS |
| `Garnet_512_Cap640` | `Garnet_512_640.c` | 640 | 1408 | 512 | 512 / 64 B | PASS |
| `Garnet_512_Cap768` | `Garnet_512_768.c` | 768 | 1280 | 512 | 512 / 64 B | PASS |
| `Garnet_512_Cap896` | `Garnet_512_896.c` | 896 | 1152 | 512 | 512 / 64 B | PASS |
| `Garnet_512_Cap1024` | `Garnet_512.c` | 1024 | 1024 | 512 | 512 / 64 B | PASS |
| `Garnet_768` | `Garnet_768.c` | 512 | 1536 | 768 | 768 / 96 B | PASS |
| `Garnet_1024` | `Garnet_1024.c` | 896 | 1152 | 1024 | 1024 / 128 B | PASS |
| `Garnet_1024_DM4x4` | *(no C source)* | ? | ? | 1024 | 1024 / 128 B | **MISMATCH** |

All OBSERVED digest sizes match the spec. The `Cap` in the KAT/label names is
a **misnomer**: the numbers 512/640/768/896/1024 are the *rate* `r` encoded by
`counter0..counter4` (spec §2.1), not the capacity.

## Pseudocode

### Padding (spec §2.2)

```
Pad(M): append a single 1 bit and the minimum number of 0 bits so that
        |M| + 1 + z is a multiple of 128.
        M = M_0 || ... || M_{m-1},  |M_i| = 128
        then regroup into w-bit rate blocks M'_0 .. M'_{m'-1}, zero-filling
        the tail of the last w-bit block.
Serialization: input bits are MSB-first within each byte; each 128-bit word is
a little-endian 16-byte integer; the first serialized word goes to the
lowest-numbered selected rate cell, later words to increasing indices.
```

### Mode (spec §2.5)

```
I_r(X) = the 2048-bit state with the r-bit block X in the selected rate cells
         and zeros everywhere else.

S^0  = [ c0  0   0   0 ;  0  c1  0   0 ;  0  0  c2  t0 ;  t1  0  0  c3 ]
          # c0..c3 = first 512 bits of pi ; t0 = digest length ; t1 = counter_i
          # (b) Garnet-768's precomputed S (Appendix A, p. 36; Garnet_768.c:762-780)
          #     is P^15(S^0) with t0 = 512, not 768
S    = P^a(S^0)                                   # PRE-COMPUTED per instance

# Sponge (Garnet-512, Garnet-768)
for j = 0 .. m'-1:   S = P^b( S XOR I_r(M'_j) )

# Sponge-DM (Garnet-1024a)
for j = 0 .. m'-1:   X = S XOR I_r(M'_j) ;  S = P^b(X) XOR X

# length injection + middle + pre-squeeze, common to both
S = S XOR I_r(mlength)
S = P^{middle}(S)          # B = 1 / 3 / 7
S = P^{squeeze}(S)         # B = 2 / 4 / 8
digest = the selected squeeze cells, lowest index least significant
```

### Permutation round (spec §2.4)

```
P = zeta o theta o rho o sigma      # applied in the order sigma, rho, theta, zeta

sigma  (SubWord)      : S_i <- AESL(S_i, B) for all 16 words
                        AESL = MixColumns o SubBytes o ShiftRows  (one AESENC)
rho    (MixColumn)    : each column lambda_i <- T1 . lambda_i over
                        GF(2)[x]/(x^128 + x^7 + x^2 + x + 1)
                        columns: (S0,S4,S8,S12) (S1,S5,S9,S13)
                                 (S2,S6,S10,S14) (S3,S7,S11,S15)
theta  (AddConstant)  : (S7, S11, S15) ^= (c0^(r), c1^(r), c2^(r))
                        Fibonacci constants, Table 1, rounds 1..16
zeta   (MixDiagonal)  : each diagonal eta_i <- T2 . eta_i over
                        GF(2)[x]/(x^128 + x^29 + x^15 + x^2 + 1)
                        diagonals: (S0,S5,S10,S15) (S1,S6,S11,S12)
                                   (S2,S7,S8,S13)  (S3,S4,S9,S14)

T1 = T2 =  [ a   a+1  1   1  ]     a = the primitive element of the
           [ 1    a  a+1  1  ]         respective field
           [ 1    1   a  a+1 ]
           [ a+1  1   1   a  ]
```

## Implementation vs specification

Source reviewed: the single shared directory
`hash-11/src/<label>/` (all eight labels symlink to
`Garnet/Implementations/API_CryptHash/Implementations/Reference_Implementation/Garnet`):
`CryptHash_Garnet.c` (39 lines, dispatch on `digest_len_bits`),
`Garnet_512.c` / `Garnet_512_{512,640,768,896}.c`, `Garnet_768.c`,
`Garnet_1024.c`. Line numbers below are from `Garnet_1024.c` and
`Garnet_512_512.c`.

Verified agreements:

- Round order `subword -> mix_column -> add_constant -> mix_diagonal`
  (`permutation_p_4x4_st`, `Garnet_1024.c` ll. 652–658) matches
  `P = zeta o theta o rho o sigma`.
- `mix_column_4x4_st` and `mix_diagonal_4x4_st` compute
  `t0 = a*A + (a+1)*B + C + D`, `t1 = A + a*B + (a+1)*C + D`,
  `t2 = A + B + a*C + (a+1)*D`, `t3 = (a+1)*A + B + C + a*D` — exactly `T1`/`T2`,
  with two distinct `gf128_mul_alpha` helpers (`_MC4_st` for the column field,
  `_MD4_st` for the diagonal field). The diagonal index sets
  `(0,5,10,15) (1,6,11,12) (2,7,8,13) (3,4,9,14)` match §2.4.
- `add_constant_4x4_st` XORs into `S7, S11, S15` (ll. 638–648) and
  `Fibonacci_4x4[16][3]` (ll. 301–311) is **byte-for-byte spec Table 1**
  (`{1,1,2} … {1836311903, 2971215073, 4807526976}`).
- Round counts match spec Table 3 for every built instance: absorb / middle /
  squeeze = 10 / 14 / 10 for all five Garnet-512 rate variants, 11 / 15 / 11
  for Garnet-768, 12 / 16 / 12 for Garnet-1024a. The initialization `P^a` is
  folded into the hard-coded starting state, as §2.3 allows.
- Domain-separation keys match spec Table 4: `zero_key = 0` for absorbing,
  `one_key = 1 / 3 / 7` for middle processing and `two_key = 2 / 4 / 8` for
  squeezing in the 512 / 768 / 1024a files.
- The hard-coded starting state of `Garnet_512_512.c`
  (`0xde30d1b89ddaa011782b1463e5b448c2`, `0x634ee0b3bf88f484d92183bde01eaf39`,
  `0x149d3ee603859d70f8fb07c0f0d96f62`, `0xf48dce216aa60cac4c92e211237707ef`,
  …) is exactly the `S^a` list the spec prints in §2.3 for Garnet-512 with
  `counter0` — so `Garnet_512_Cap512` is the spec's r = 512 instance.
- Absorbing patterns: r = 512 uses `S0, S5, S10, S15` in the order
  `msg[0..3]`, matching the spec's example
  `M_i = S15||S10||S5||S0` with `M_{i,0}` (first serialized, least
  significant) in `S0`. The other rates add cells in increasing index order
  (640: +S12; 768: +S3; 896: +S6; 1024: +S9). Garnet-1024a uses
  `{S0,S3,S5,S6,S9,S10,S15}` (896 bits) with capacity
  `{S1,S2,S4,S7,S8,S11,S12,S13,S14}`.
- Digest extraction: 512 emits `S0, S7, S9, S14`; 768 emits
  `S0, S3, S6, S9, S13, S15`; 1024a emits
  `S0, S2, S5, S7, S9, S11, S12, S14` — the spec's
  `H = S14||S9||S7||S0`, `S15||S13||S9||S6||S3||S0`,
  `S14||S12||S11||S9||S7||S5||S2||S0` with `S0` least significant, serialized
  little-endian-first. Match.
- Only `Garnet_1024.c` has `keep_capacity5_4x4_st` /
  `absorb_capacity5_4x4_st`. It re-XORs the message and saved capacity after
  the permutation, but not the old rate state: its update is
  `S' = P^12(X) XOR I_r(M) XOR (0_r || S_c)`, where
  `X = S XOR I_r(M)`. This differs from §2.5's full-state Davies–Meyer
  feed-forward `P^12(X) XOR X` by `I_r(S_r)` (hash-11-2).
  `Garnet_512*.c` and `Garnet_768.c` are plain Sponge.
- **Partial-byte input is handled correctly.** `pad_message()`
  (`Garnet_1024.c` ll. 683–724, and the analogues in the other files) first
  masks the caller's last byte to the declared `bit_len % 8` bits
  (`mask1 = 0xFF << (8 - bit_idx)`), then sets the pad bit at position
  `7 - bit_idx` and clears everything below it. Bits outside `msg_len_bits`
  cannot reach the state.

### Discrepancy 1 (real deviation, all instances; recorded as hash-11-4): a spurious all-zero rate block is absorbed for 1-in-64 message lengths

The absorbing loop consumes whole rate blocks while `(i + W) <= byte_num`
where `byte_num = ceil(mlength/8)` and `W` is the rate in bytes
(`Garnet_512_512.c` l. 662, `Garnet_768.c` l. 787, `Garnet_1024.c` l. 728).
The final-block code that follows is then executed **unconditionally**
(`absorb_message*; for j < b: permutation`).

When the *padded* message exactly fills an integral number of rate blocks and
the pad bit landed inside the last message byte — i.e. when
`mlength mod 8 != 0` and `ceil(mlength/8)` is a multiple of `W` — the loop has
already absorbed the entire padded message, and the code absorbs **one extra
all-zero rate block and runs one extra `P^b`**.

Concretely for Garnet-512 with r = 512 (W = 64 bytes) this happens for every
`mlength` in `{512k-7, …, 512k-1}`, k >= 1 (e.g. 505..511, 1017..1023): the
spec's `Pad(M)` gives exactly `512k` bits = `k` rate blocks and therefore `k`
absorb steps, while the implementation performs `k+1`. The same holds for
Garnet-768 (W = 64) and Garnet-1024a (W = 112, i.e. `mlength` in
`{896k-7,…,896k-1}`).

The extra block is deterministic and the subsequent `mlength` injection keeps
the map injective, so this is not a collision; but the implementation computes
a different function from the specification for 7 out of every `w` message
lengths. The shipped KATs (which do cover lengths 505–511) were generated by
this code, so they encode the deviation.

### Discrepancy 2 (real deviation, all instances; recorded as hash-11-4): the message-length injection is replicated into four rate cells instead of `I_r(mlength)`

§2.5 says "After all message blocks have been absorbed, `mlength` is injected
into the rate part **in the same serialized convention**", and §2.5 defines
`I_r(X)` as "placing the r-bit rate block X into the selected rate cells and
filling all remaining cells by zero". With §2.2's convention (first serialized
128-bit word into the lowest-numbered rate cell), a single 128-bit `mlength`
word belongs in `S0` alone.

Every implementation file instead calls `absorb_message_4x4(state, msglen)`,
which XORs the **same** 128-bit value into four cells:

```c
state[0] ^= msg;  state[5] ^= msg;  state[10] ^= msg;  state[15] ^= msg;
```

(`Garnet_1024.c` ll. 601–608, called at l. 767; `Garnet_512_512.c` l. 534,
called at l. 711; `Garnet_768.c` likewise.) For the r = 512 instances those
four cells happen to *be* the whole rate, so the injection is "rate-wide" but
still not the specified serialization; for r = 640/768/896/1024 and for
Garnet-1024a it covers only four of the five to eight rate cells. Either way
the specification and the code define different functions. (The spec sentence
is terse, so a reader could argue ambiguity, but `I_r` is defined explicitly
two paragraphs earlier and does not admit replication.)

### Discrepancy 3 (missing reference implementation): `Garnet_1024_DM4x4`

The submission ships **two** 1024-bit KAT sets, `KAT_*_Garnet_1024.txt` and
`KAT_*_Garnet_1024_DM4x4.txt`, but only one 1024-bit C source. I compared the
two KAT files directly: of 4097 common message lengths they agree for
`Msg_Len = 0 … 511` and differ for **all 3585** lengths from 512 bits upward.
Since `Garnet_1024.c` has rate 896 (so lengths 0..895 all take the same code
path), the divergence at exactly 512 bits shows the `DM4x4` variant uses a
512-bit rate — a parameter set that appears in neither Table 3 nor the
`README.txt` build recipes. The only code for it in the tree is
`Optimized_Implementation/Garnet_1024_DM/Garnet_1024.S` (x86-64 assembly).
Consequently the `Garnet_1024_DM4x4` label is built from the same C sources as
`Garnet_1024` and MISMATCHes its KAT from length 512 on (already recorded in
`RESULTS.md`). **A submitted parameter set has no reference implementation and
is unverifiable from C.**

The spec's `README.txt` further says `DIGEST_BIT_LENGTH 1024` produces
"Garnet-1024a (4x4 state SPONGE-DM)", which is what `Garnet_1024.c` is. So the
naming is inconsistent: the C code named `Garnet_1024` *is* the spec's
Garnet-1024a, and the KAT set named `Garnet_1024_DM4x4` is an undocumented
fourth variant.

### Minor observations

- The shipped API header hard-wires `ALGORITHM_INSTANCE "Garnet"` and
  `DIGEST_BIT_LENGTH 512` for **all** builds; the per-instance name and digest
  width have to be supplied externally (the NGCC harness does this with
  `-DNGCC_INSTANCE`/`-DNGCC_DIGEST_BITS`). As shipped, the KAT driver can only
  be retargeted by editing the header, and the metadata an ICCS consumer sees
  is wrong for seven of the eight instances.
- `pad_message` and `pad_message2` are **non-static** in `Garnet_512.c` and
  `Garnet_512_512.c` (static in `Garnet_768.c`/`Garnet_1024.c`), so linking two
  rate variants together fails with duplicate symbols. This is why the
  `README.txt` recipes link exactly one `Garnet_512*.c` at a time.
- The residual-block copy uses `memcpy(lastblock, message + i, (byte_num & 0xf) + 1)`
  — one byte more than the message holds when `mlength % 8 != 0`. The
  implementations always hash a `calloc`'d `msg_len_bytes + 256` scratch copy,
  so the extra byte is a guaranteed zero and no out-of-bounds read occurs;
  calling the internal `Garnet_*()` functions directly on a caller buffer
  would over-read by one byte.
- `mlength` is carried as a single 64-bit word (`msglen.v[0] = mlength`,
  `v[1] = 0`), consistent with the spec's 2^64 − 1 bit maximum.
- `CryptHash()` prints a misleading error ("Only 768 bits is supported") for
  any unsupported digest length.

### Not verified

- Starting states, recomputed with the code's round function: counters 0–4
  (t0 = 512) and Garnet-1024a (t0 = 1024) match Sect. 2.3; Garnet-768 matches
  only with t0 = 512 (b). With t0 = 768 none of its 16 state words match.
- The `gf128_mul_alpha_aa_*` helpers' field-reduction polynomials were read but
  not exercised against test vectors.
- The MILP active-S-box bounds of §4; the numbers in the parameter table are
  the spec's own claims.
