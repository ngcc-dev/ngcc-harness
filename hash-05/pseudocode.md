# hash-05 uHash — algorithm summary

Block-cipher-based wide-pipe hash: a *Counter-bDM* (b-fold Davies–Meyer with
branch counters) compression function built on **uBlock-1024** (256-bit block,
1024-bit key, 52 rounds), iterated Merkle–Damgård-style with a tweaked final
call for domain separation. The chaining value is the full digest width
(bn = h bits), so the design is "wide pipe with no truncation": the digest is
the whole final state. Security is proved in the ideal-cipher model.

Specification: `hash-05-spec.pdf` (39 pages), §1.2 "Algorithm Specification"
(Algorithm 1, Tables 2–7), §2.2 padding rationale, §3.1 security claims.
English-language spec.

## Parameters

| parameter | uHash-512 | uHash-768 | uHash-1024 | meaning |
|---|---|---|---|---|
| h = bn | 512 | 768 | 1024 | digest length = chaining-value width (spec Table 2) |
| n | 256 | 256 | 256 | uBlock-1024 block size |
| b | 2 | 3 | 4 | parallel branches (E-calls) per chunk |
| a = 5 − b | 3 | 2 | 1 | n-bit blocks per message chunk |
| rate (an) | 768 bits | 512 bits | 256 bits | message bits absorbed per iteration |
| "capacity" | n/a (wide pipe, bn-bit chaining, no truncation) | | | |
| key length of E | 1024 | 1024 | 1024 | (b−1)·n chaining bits ‖ a·n message bits |
| rounds of E | 52 | 52 | 52 | uBlock-1024 (spec §1.2.7) |
| throughput a/b | 3/2 | 2/3 | 1/4 | spec Table 8 |
| claimed collision | 256 | 384 | 512 | h/2 bits (spec Table 9) |
| claimed (2nd-)preimage | 512 | 768 | 1024 | h bits (spec Table 9) |
| claimed indistinguishability | 256 | 384 | 512 | spec Table 9 |

Digest sizes, specification vs the built reference library:

| instance | digest spec (bits) | digest impl (bits/bytes) | match |
|---|---|---|---|
| uHash-512 | 512 | 512 / 64 | yes |
| uHash-768 | 768 | 768 / 96 | yes |
| uHash-1024 | 1024 | 1024 / 128 | yes |

Digest length is **fixed per instance**; there is no XOF/variable-length mode.
`CryptHash()` dispatches on `digest_len_bits ∈ {512,768,1024}` and every
instance contains all three code paths.

## Pseudocode

### Padding and splitting (spec §1.2.1–§1.2.2)

```
padding(M, an):                       # bit-level, no length encoding
    M' = M || 1 || 0^z,   z minimal >= 0 s.t. |M'| = l*an, l >= 1
    # note: mandatory — if |M| is already a multiple of an, a whole extra
    # chunk 1||0^(an-1) is appended.
    return M1 || M2 || ... || Ml      # each Mi is an bits
```

Domain separation comes **only** from the tweaked final iteration (below);
the padding carries no length field and no digest-length/variant byte (the
IV encodes bn instead).

### Hash (spec Algorithm 1)

```
uHash-h(M):
    M1..Ml   <- padding(M)
    u1       <- IV = IV_1 || IV_2 || ... || IV_b          # §1.2.3
       IV_1 = 0^128 || 0^64  || <bn>_64
       IV_2 = 0^128 || <bn>_64 || 0^64
       IV_3 = 0^64  || <bn>_64 || 0^128
       IV_4 = <bn>_64 || 0^64  || 0^128

    for i = 1 .. l-1:                                     # CF_bn
        u_i^1 || u_i^2 || ... || u_i^b  <- u_i
        K_i <- u_i^2 || ... || u_i^b || M_i               # 1024 bits
        for j = 1 .. b:                                   # b parallel E-calls
            u_{i+1}^j <- E_{K_i}( u_i^1 XOR (j-1) ) XOR u_i^1

    # final iteration CF'_bn (§1.2.5): key tweaked per branch
    K_l <- u_l^2 || ... || u_l^b || M_l
    for j = 1 .. b:
        u_{l+1}^j <- E_{K_l XOR (j+15)}( u_l^1 XOR (j-1) ) XOR u_l^1

    return H = u_{l+1}^1 || ... || u_{l+1}^b              # h bits, no truncation
```

`(j-1)` and `(j+15)` are XORed into the least-significant end (last byte) of
the 256-bit plaintext and the 1024-bit key respectively. If l = 1, only the
final iteration runs.

### Underlying permutation: uBlock-1024 (spec §1.2.7)

```
uBlock-1024(X, RK^0..RK^51):
    X0 || X1 <- X                       # 2 x 128 bits
    for i = 0..51:
        RK0 || RK1 <- RK^i
        X0 <- S32(X0 XOR RK0);  X1 <- S32(X1 XOR RK1)     # 32 parallel 4-bit S-boxes
        X1 ^= X0
        X0 ^= (X1 <<<32 4);  X1 ^= (X0 <<<32 8)
        X0 ^= (X1 <<<32 8);  X1 ^= (X0 <<<32 20)
        X0 ^= X1
        X0 <- PL(X0);  X1 <- PR(X1)     # nibble permutations, Table 5
    return X0 || X1

KeySchedule: K (1024 b) -> K0..K7 (128 b each); RK^0..RK^3 = K0||K1, K2||K3,
    K4||K5, K6||K7; then 12 updates
    K <- f(K2 XOR RC_i)||g(K5)||f(K1)||g(K0)||f(K3)||g(K6)||f(K7)||g(K4)
    # (c) the spec does not say where the 32-bit RC_i enters the 128-bit K2;
    #     the code uses bytes 0-3 (RC || 0^96); bytes 12-15 reproduce no KAT
  each producing the next four round keys  -> 4 + 12*4 = 52 round keys.
```

## Implementation vs specification

Source reviewed: `hash-05/src/uHash-{512,768,1024}/CryptHash_AlgorithmInstance.c`
(identical 1112-line file in all three directories; only
`CryptHash_AlgorithmInstance.h`'s `ALGORITHM_INSTANCE` / `DIGEST_BIT_LENGTH`
differ). Line numbers below refer to that file. The file is ISO-8859/GBK
encoded with CRLF.

Agreements:

- `#define ROUNDS 52`, `BlockSize 32` (256 bits), key buffer `4*BlockSize`
  (1024 bits) — match spec §1.2.7.
- S-box table (line 38) is exactly spec Table 4; three further tables at
  lines 37/40/49 are the same S-box pre-OR'ed with 0x10/0x20/0x30 nibble tags.
- Key schedule: `RK[0..3]` from the master key, then 12 update rounds → 52
  round keys, matching §1.2.7.
- IV constants `IV512_1/2`, `IV768_1/2/3`, `IV1024_1/2/3/4` (lines 75–85)
  encode bn = 0x0200 / 0x0300 / 0x0400 in exactly the 64-bit positions the
  spec's IV_1..IV_4 require.
- Branch plaintext counters `u^1 XOR (j-1)`: implemented as
  `P_j[BlockSize-1] ^= (j-1)` (lines 528, 723–725, 925–928) — matches.
- Chunk sizes: a = 3 / 2 / 1 blocks of 32 bytes for the 512/768/1024
  instances (lines 490, 650, and the 1024 analogue) — matches Table 2.
- Feedback `XOR u_i^1` is implemented via the saved `FB` buffer — matches.

### Discrepancy 1 (real deviation, all instances): final-iteration key tweak is `j+14`, not `j+15`

Spec §1.2.5 / Algorithm 1 line 14: `u_{l+1}^j = E_{K_l XOR (j+15)}(...)`, i.e.
the branch keys are `K_l XOR 16, 17, ..., 15+b`.

The implementation applies, cumulatively on the last key byte:

- uHash-512 (lines 578, 586): `^0x0f` then `^0x1f` → keys `K XOR 15`, `K XOR 16`.
- uHash-768 (lines 749, 761, 763): `^0xf`, `^0x1f`, `^0x1` → `K XOR 15, 16, 17`.
- uHash-1024 (lines 962–969): `K^0x0f`, `K0^0x10`, `K1^0x11`, `K2^0x12` →
  `K XOR 15, 16, 17, 18`.

So every instance uses `K_l XOR (j+14)`. A spec-faithful implementation would
produce different digests, and the shipped KATs encode the off-by-one. Impact
is cosmetic for security (the tweaks are still non-zero and distinct, so
CF' ≠ CF separation survives), but it is a plain spec/implementation
mismatch and the specification, not the code, is what a second implementer
will follow.

### Discrepancy 2 (real deviation): padding does not mask the partial final byte, and uses `+` instead of `|`

`Padding()` lines 442–474:

```c
msg_len_bytes_col = msg_len_bits % 8;
if (msg_len_bytes_col == 0)  M[msg_len_bytes] = 0x80;
else  M[msg_len_bytes] = msg[msg_len_bytes] + (1 << (7 - msg_len_bytes_col));
```

Two faults in the `else` branch:

1. The whole caller byte `msg[msg_len_bytes]` is copied, so the
   `8 - (msg_len_bits mod 8)` bits **after** the declared end of the message
   are absorbed into the hash. The spec is unambiguous here — §1.2.1 and
   §2.2 define padding on the *bitstring* M as `M || 1 || 0*`, and the
   injectivity argument in §2.2 assumes exactly that — so this is an
   **implementation** defect, not a spec ambiguity. This is the source of
   the already-recorded `hash-unused-bits` finding for all three instances
   (see `hash-05/security_findings.md`).
2. `+` rather than `|`: if the stale bit at position `7 - col` is already 1,
   the addition carries **into the declared message bits** (e.g. declared
   length 1 bit, buffer byte `0xFF` → `0xFF + 0x40 = 0x3F` mod 256, flipping
   the one declared bit). The digest is therefore not even a function of the
   declared bits plus the tail; the tail can corrupt the message.

The source carries the authors' own comment at line 445–446:
`//***NOTE*** The correctness details of padding should be verified.`

A conforming fix is
`M[k] = (msg[k] & (0xFF << (8-col))) | (1 << (7-col));`.

### Discrepancy 3 (real deviation): maximum message length far below the spec requirement

The spec's introduction states the algorithm "shall support a maximum message
length of at least 2^64 − 1 bits" and the API takes
`unsigned long long msg_len_bits`. But lines 450–453 / 492–494 truncate the
chunk count through `int`:

```c
row = (int)(msg_len_bits / AN_bits) + 1;
actual_len_bytes = (int)(row / 8);
```

For uHash-512 (`AN_bits = 6144`) this overflows around 2^31·6144 ≈ 2^43.6
bits; `actual_len_bytes` is then a truncated/negative value passed to
`malloc`. Messages beyond roughly 2^40 bits are mis-hashed or crash. Not
reachable in the KAT battery, but it is a hard limit the spec does not have.

### Minor observations

- `CryptHash()` returns 0 ("success") when `digest_len_bits` is none of
  512/768/1024 and leaves `digest` untouched — a silent-failure API path.
- The three instance directories ship byte-identical `.c` files; the only
  per-instance configuration is in the header. No `params.h`/`config.h`
  exists, so the parameter spot-check above was done against the `#define`s
  and initialised arrays in the single source file.

### Not verified

- Nibble permutation tables `PL`, `PR`, `f`, `g` and the 12 round constants
  `RC_i` were not compared entry-by-entry against spec Tables 5–7.
- The security proof of §3.2 was not reviewed; the claims in the parameter
  table are the spec's own.
