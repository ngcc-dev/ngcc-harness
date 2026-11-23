No: setting the NTRE-512 sampling seed to the 64 bytes that Algorithm 5 specifies is a one-constant code change that alters no size and adds no counted work.

Impact: none
Scope: Single line (small), code
Assessed: 2026-11-23

## Attacks considered

- kem-27-1: NTRE-512's 256-bit secret seed caps offline key-recovery work (Implementation, Confirmed)

## Remediation

- **kem-27-1.** Root cause: `NTRE_SYMBYTES` is 32 (`src/params.h:10` in the reference trees, `params.h:10` in the optimized trees), so key generation draws a 32-byte seed for each of f' and g' (`KEM_AlgorithmInstance.c:91–105`), while Algorithm 5 (physical p. 6) draws s from B^64. The costed repair is the one the NTRE team proposes: set `NTRE_SYMBYTES` to 64, so that the code follows the specification. Every use of the seed length goes through that macro (`KEM_AlgorithmInstance.c:26–104`, `src/symmetric.c:36–41`), so nothing else changes. Scope: Single line, code; one constant in each of the six `params.h` copies. The specification stays as it is. Impact: none. The seed is used only inside key generation. It is not stored in the public key, the ciphertext or the secret key, so no object changes size. Key generation makes the same two XOF calls with the same 576 output bytes each, and each call absorbs 65 bytes instead of 33. Encapsulation and decapsulation do not use the seed. The proposal states no sizes or timings. Not counted: the XOF is the contest's `pseudoXOF`, which the ruleset costs as an ideal replacement by calls and bytes. In the helper as built, every 32-byte output block hashes the input and a 4-byte counter with SM3 (`auxfunc.c:482`). The longer input needs two SM3 compressions per block where the submitted one needs one, so key generation takes 127 compressions instead of 91. The share of key generation that this is was not measured.

## Evidence

- Baseline sizes: Table 4 (physical p. 38) and Algorithms 8 and 11 (physical pp. 7–8). NTRE-3457-2304: pk = ct = 12 · 2304 / 8 = 3,456 bytes; sk = 2 · 3,456 + 32 = 6,944 bytes. None of the three contains the sampling seed.
- Sampling: Algorithm 5 outputs XOF(s, n/4), which is 576 bytes at n = 2304 (`NTRE_SAMPLEBYTES`, `src/params.h:19`). Key generation calls it once per attempt for f' and once for g' (Algorithm 8, physical p. 7).
- Helper count, as built: output blocks per sampling call = 576 / 32 = 18. SM3 compressions per block = ceil((L + 9) / 64), where L = 1 + seed + 4 is the prefix, seed and counter: 1 at L = 37 and 2 at L = 69. Two calls: 36 submitted, 72 repaired. G(pk) = ceil((3,456 + 9) / 64) = 55 in both. Totals: 36 + 55 = 91 and 72 + 55 = 127.
- Stated timings (Table 4, physical p. 38), context only: NTRE-3457-2304 key generation 376,672 cycles in C and 137,912 with AVX2. The table does not separate hashing.
- Proposed fix, quoted as reported and not evaluated: the [NTRE team's response](https://list.niccs.org.cn/archives/list/pkcforum@list.niccs.org.cn/message/OAGYDCAYEJTJ6WE5PHBNNCTAGDZ57LXD/) sets `NTRE_SYMBYTES` to 64 and regenerates the KATs.
