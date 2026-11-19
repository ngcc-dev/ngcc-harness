# hash-06 Cuishen — algorithm summary

Double-Block-Length (DBL) Merkle–Damgård hash in the style of Naito's scheme,
built on a dedicated ARX block cipher **Octarx** (Octarx-256 / Octarx-512,
both 64 rounds). MD strengthening is replaced by pure zero padding plus a
128-bit *position counter* injected into every compression call; the
finalization function encrypts the two constants `[2]_n`, `[3]_n` under a key
made of the final state, the residual message block and the counter, which
also kills length extension. Not a sponge.

Specification: `hash-06-spec.pdf` (28 pages), §3 (Algorithm 1, §3.1–§3.5) and
§4 (Algorithms 2–5, Table 1), claims in §5.1 / Table 2. English-language spec.

## Parameters

| parameter | Cuishen-512 | Cuishen-768 | Cuishen-1024 | meaning |
|---|---|---|---|---|
| t (digest) | 512 | 768 | 1024 | spec §3, Alg. 1 |
| n (cipher block) | 256 | 512 | 512 | Octarx version |
| internal state 2n | 512 | 1024 | 1024 | chaining `H = t‖b`, no capacity split |
| B (rate) | 1024 | 1024 | 1024 | message bits per iteration (uniform) |
| residual block m_final | 768 | 512 | 512 | absorbed by the finalization function |
| k (cipher key) | 1408 | 1664 | 1664 | = 1024 msg + n chain + 128 counter |
| rounds | 64 | 64 | 64 | Octarx, spec Table 1 |
| w | 64 | 64 | 64 | word size |
| Cnt | 128 bits | 128 | 128 | processed-message-bit counter |
| truncation | none | 1024→768 (leftmost) | none | spec Alg. 1 line 21–23 |
| claimed collision | 2^256 | 2^384 | 2^512 | spec Table 2 |
| claimed preimage | 2^512 | 2^768 | 2^1024 | spec Table 2 |
| claimed 2nd-preimage | 2^512 | 2^768 | 2^1024 | spec Table 2 |
| claimed quantum coll. | 2^170 | 2^256 | 2^341 | §5.1.2 (BHT, t/3) |

Digest sizes, specification vs the built reference library:

| instance | digest spec (bits) | digest impl (bits/bytes) | match |
|---|---|---|---|
| Cuishen-512 | 512 | 512 / 64 | yes |
| Cuishen-768 | 768 | 768 / 96 | yes |
| Cuishen-1024 | 1024 | 1024 / 128 | yes |

Digest length is fixed per instance. An XOF mode is *mentioned* in §1 and
§8.3 but is not specified and not implemented.

## Pseudocode

### Padding (spec §3.1)

```
ZeroPadding(M, n):                     # pure zero padding, no 10*, no length field
    l = |M|
    x = min x >= 0 s.t.  l + x == (n == 256 ? 768 : 512)  (mod 1024)
    M' = M || 0^x
    # M' = m_1 || ... || m_N || m_final ,  |m_i| = 1024,
    #      |m_final| = 768 (n=256) or 512 (n=512)
```

Domain separation is provided by (i) the counter `Cnt` in every key, and
(ii) the distinct constants `[2]_n`, `[3]_n` encrypted in finalization, which
are plaintexts of an iteration call only if `t_{i-1}` is `[2]_n` or `[3]_n` (probability about 2^-n per block), so the separation is probabilistic, not structural; they are otherwise not plaintexts of an iteration call (those are `t_{i-1}` and
`t_{i-1} XOR 1`).

### Hash (spec Algorithm 1)

```
Cuishen-t(M):
    H_0 = t_0 || b_0  <- IV         # 64-bit fractional parts of sqrt of Chen primes > 100
    Cnt_0 = 0
    m_1..m_N, m_final <- ZeroPadding(M)

    for i = 1 .. N:                                 # iteration phase
        Cnt_i = cumulative number of ORIGINAL message bits absorbed up to m_i
        H_i   = CF_DBL(t_{i-1}, b_{i-1}, m_i, Cnt_i)

    Cnt_final = Cnt_N + (original message bits contained in m_final)
    Out1 || Out2 = g_DBL(H_N, m_final, Cnt_final)
    h = Out1 || Out2
    if t == 768:  h = Truncate(h, 768)              # leftmost 768 of 1024
    return h
```

### Compression function CF_DBL (spec §3.4)

```
CF_DBL(t_{i-1}, b_{i-1}, m_i, Cnt_i):
    K  = m_i || b_{i-1} || Cnt_i                    # k bits
    P1 = t_{i-1}
    P2 = t_{i-1} XOR [1]_n                          # single LSB flip
    t_i = E(K, P1) XOR P1                           # Davies-Meyer, top branch
    b_i = E(K, P2) XOR P2                           # bottom branch
    return t_i || b_i
```

### Finalization g_DBL (spec §3.5)

```
g_DBL(H_N, m_final, Cnt_final):
    K = H_N || m_final || Cnt_final                 # = t_N || b_N || m_final || Cnt
    Out1 = E(K, [2]_n)                              # 0^(n-2)||10 ; NO feed-forward
    Out2 = E(K, [3]_n)                              # 0^(n-2)||11
    return Out1 || Out2
```

### Octarx round function (spec Algorithms 2 and 3)

```
Octarx-256, state (A,B,C,D), rk (w^a..w^d), 64 rounds:
    T = C XOR D
    A' = C ;  B' = D
    C' = ((A XOR w^a XOR c_i) <<< 7)  +  ((T XOR w^d) <<< 20)     # + is mod 2^64
    D' = ((B XOR w^b)         <<< 31) +  ((T XOR w^c) <<< 56)

Octarx-512, state (A..H), rk (w^a..w^h), 64 rounds:
    T = E XOR F XOR G XOR H
    A'..D' = E,F,G,H
    E' = ((A XOR w^a XOR c_i) <<< 18) + ((T XOR w^h) <<< 21)
    F' = ((B XOR w^b)         <<< 33) + ((T XOR w^g) <<< 16)
    G' = ((C XOR w^c)         <<< 47) + ((T XOR w^f) <<< 20)
    H' = ((D XOR w^d)         <<< 60) + ((T XOR w^e) <<< 49)

Key schedule (Algs 4/5), K = M(16 w) || B(n/64 w) || Cnt(2 w), per round:
    w^a = m0 XOR b0 XOR c0 ;  w^b = m1 XOR b1 XOR c1
    w^c = m8 XOR b2        ;  w^d = m9 XOR b3        [ w^e..w^h = b4..b7 for -512 ]
    M-register: m0'=m6, m1'=m7,
                m2' = (m8<<<29) + ((m10^m11)<<<6),  m3' = (m9<<<63) + ((m10^m11)<<<12),
                m4..m9' = m10,m11,m12,m13,m14,m15,
                m10'= (m0<<<14) + ((m2^m3)<<<19),   m11'= (m1<<<43) + ((m2^m3)<<<28),
                m12..m15' = m2,m3,m4,m5
    B-register: rotate left by one word, last' = (b0 <<< 27|47) XOR b1
    Cnt:        c0' = c1, c1' = c0 <<< 2
    c_i: 64 round constants from frac(pi) [Octarx-256] / frac(e) [Octarx-512],
         injected only into the A-branch.
```

## Implementation vs specification

Source reviewed: `hash-06/src/Cuishen-{512,768,1024}/CryptHash_Cuishen-<inst>.c`
(400/410/410 lines; three separate, self-contained C99 files — the -768 and
-1024 files differ only in the IV constants and the final store width).

Verified agreements (line numbers from `CryptHash_Cuishen-512.c` unless noted):

- `ROUNDS = 64` (l.16) for both Octarx versions — matches Table 1.
- Round function `round_function()` (ll. 215–228) is a literal transcription
  of Algorithm 2, with rotations A/B/C/D = 7/31/56/20 (ll. 25–28). The
  Octarx-512 version (`Cuishen-768` ll. 25–32, 224–248) uses 18/33/47/60 and
  49/20/16/21 exactly as Algorithm 3.
- Key schedule `key_schedule()` (ll. 160–212) reproduces Algorithm 4
  word-for-word, including all message rotations
  (MA..MH = 14,43,28,19,29,63,12,6), `BA = 27` (Octarx-512: 47) and the
  counter update `c0'=c1, c1'=c0<<<2`. (b) Figure 9 (p. 13) rotates the other
  word (`c0'=c1<<<2, c1'=c0`); that rule reproduces only the empty-message KAT.
- Round-key derivation `m[0]^b[0]^cnt[0]`, `m[1]^b[1]^cnt[1]`, `m[8]^b[2]`,
  `m[9]^b[3]` (ll. 178–181) matches Alg. 4 lines 19–20 including the fact
  that the counter is *not* mixed into `w^c`, `w^d`.
- Round constants `PI[64]` (ll. 51–73) are exactly the spec's π table; the
  -768/-1024 files use the e-derived table.
- Effective cipher key width: 20 words key + 2 words counter = 1408 bits for
  Octarx-256, 24 + 2 = 1664 for Octarx-512 — matches Table 1. (The source
  comment "1280 bits" at l. 22 counts only the part passed in the `key[]`
  array; the counter is a separate argument.)
- `CF_DBL`: `lsb_separate()` (ll. 252–256) gives `P1 = t`, `P2 = t XOR 1`
  with the XOR landing on the big-endian least significant word — matches
  `P2 = t_{i-1} XOR [1]_n`. Feed-forward `XOR P1 / XOR P2` present
  (ll. 280–287).
- `finalization()` (ll. 296–330) encrypts `make_const256(2)` and
  `make_const256(3)` under key `t‖b‖m_final` + counter, with **no**
  feed-forward — matches §3.5 eqs (4)–(5).
- Cuishen-768 truncation is `store768_be()` over the first 12 of 16 output
  words, i.e. the leftmost 768 bits — matches Alg. 1 line 22.
- Counter semantics match §3.2: `(block+1)*1024` per iteration (l. 378),
  `final_counter_bits = msg_len_bits` (l. 373), and 0 for the empty message.
- **Partial-byte input is handled correctly.** `copy_partial_bits()`
  (ll. 120–134) masks the final byte with `0xFF << (8 - tail_bits)` before
  copying, so bits beyond `msg_len_bits` cannot influence the digest. This is
  the correct behaviour that hash-05 and hash-10 lack, and it matches the
  MSB-first partial-byte convention stated in the shipped ICCS `README`.
- `CryptHash()` rejects `digest_len_bits != DIGEST_BIT_LENGTH` with `-1`
  (l. 359) and rejects a NULL message with a non-zero length.

### Discrepancy 1 (spec error / ambiguity, implementation is the sane reading): `N = floor(l/1024)`

Spec §3.1 and Alg. 1 line 6 say the number of full iteration blocks is
`N = floor(l/1024)`. That is inconsistent with the padding rule whenever
`l mod 1024 > 768` (n = 256) or `> 512` (n = 512): e.g. for `l = 1000` and
n = 256 the rule gives `x = 792`, so `|M'| = 1792 = 1*1024 + 768` and one
full block *must* be absorbed, yet `floor(1000/1024) = 0`. The correct count
is `N = (l + x - |m_final|)/1024`.

The implementation does the right thing: after absorbing
`floor(l/1024)` complete blocks it compresses one extra zero-padded block when
`rem_bits > 768` (l. 386; `> 512` in the -768/-1024 files at l. 399) and then
uses an all-zero `m_final`. So this is a **specification defect**, not an
implementation one: Alg. 1 cannot parse M′ at those lengths, so it defines no
digest there.

### Discrepancy 2 (real deviation, all instances): the 128-bit counter is only 64 bits

`compress_one_block()` l. 271–272 and `finalization()` l. 310–311:

```c
counter[0] = 0;
counter[1] = (uint64_t)msg_bits_processed;
```

The high half of the 128-bit `Cnt` is hard-wired to zero. §1 of the spec
claims support for messages "up to 2^128 − 1 bits" and §2 defines `Cnt` as a
128-bit counter; the implementation therefore caps the distinguishable
message length at 2^64 − 1 bits. In practice the ICCS API only passes an
`unsigned long long msg_len_bits`, so the defect is unreachable through the
tested interface, but the stated 2^128-bit capability is not implemented and
the counter word `c0` starts at a constant instead of a message-dependent
value.

### Minor observations

- No `params.h`/`config.h`; all constants are `enum`s and `static const`
  arrays in the single instance `.c` file. The parameter spot-check above
  covers rounds, all 13 rotation constants of each Octarx version, the 64
  round constants, the key/word layout and the truncation width.
- The IV words were verified only as plausible `frac(sqrt(p))·2^64` values
  for the first Chen prime of each range (101 → `0x0cc4a61194f81760`,
  149 → `0x34e0d42e61a33f99`); the remaining 36 words were not recomputed.
- The indifferentiability, differential and MitM analyses of §5 were not
  reviewed; the security numbers in the table above are the spec's own.

## Review of 2026-11-17

All 40 IV words and all round constants were regenerated from the stated rules and match. The §5.2.2 diffusion figures were confirmed structurally. The §5.2.1 differential figures for Cases 1, 2 and 4 are contradicted by probability-1 trails through 6 rounds of Octarx-256 and 5 rounds of Octarx-512; filed as `hash-06-1`. Algorithm 1's block count `N = ⌊ℓ/1024⌋` is wrong for the longest residues, where the code processes the extra block the padding needs. All implementation trees agree with the reference for every bit length from 0 to 4,400.
