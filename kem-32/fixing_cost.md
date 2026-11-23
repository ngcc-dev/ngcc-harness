No: a stated limit of 2^64 encapsulations per public key repairs kem-32-2 and drawing QCTM512's randomness from the platform generator repairs kem-32-3, with no change to a transmitted object or an operation count; the Lead kem-32-5 is not counted, and if it held the repair would be a change of construction with no bounded cost.

Impact: none
Scope: Function (small), specification and code
With leads: Yes; not bounded; Construction (large), specification and code
Assessed: 2026-11-23

## Attacks considered

- kem-32-2: Same-key multi-instance decoding misses QCTM-128 and -256 targets (Design, Confirmed)
- kem-32-3: The bundled RNG limits QCTM-512 key-generation and encapsulation randomness to 384 bits (Implementation, Confirmed)
- kem-32-5: Reported structural key recovery for QCTM256 and QCTM512 (Design, Lead)

## Remediation

- **kem-32-2.** Root cause: every encapsulation under one public key adds 19 rotated decoding targets to one same-code search. The costed repair is a stated limit of 2^64 encapsulations per public key. At that limit the finding's estimator gives 2^133.452 for QCTM128, 2^260.726 for QCTM256 and 2^531.965 for QCTM512, all above target. Reuse of one key across calls (§7.4, physical p. 21) and the security notion are kept. Scope: Single line, specification. Impact: none. Proposed fix (QCTM team, 2026-10-08, as recorded in the report and not evaluated): code lengths 10,545 for QCTM128 and 19,551 for QCTM256. With t unchanged, the specification's size formula gives public keys of 184,077 bytes (+9.6%) and 629,204 bytes (+5.6%) (count); the ciphertexts do not change. The limit is the cheaper repair, so the answer is taken from it; the proposal's public keys also stay under 10%. Alternative without a limit: the smallest lengths that reach the targets at 2^80 ciphertexts with t unchanged are n = 10,108 and n = 19,133, which is public key +0.8% and +1.4% (estimate). These values are costing aids, not proposed parameters. Not checked: speed at the longer lengths, where key generation and decoding work on n columns; the structural estimates of the report at other lengths; the team's post and revised package, which were not read.
- **kem-32-3.** Root cause: QCTM512 takes its 512-bit key-generation seed and its sampler bits from a bundled AES-256 CTR generator whose whole state is 384 bits and which the API adapter seeds with 48 bytes. The costed repair makes `randombytes` return bytes of the platform generator `get_random_number`, which the adapter already calls, and drops the 48-byte bridge. Scope: Function, code. The specification already asks for a uniform λ = 512-bit seed and uniform sampler bits (physical pp. 5, 12, 13) and stays as it is. Impact: none. The same number of bytes is drawn in the same calls: 64 bytes per key-generation attempt and 9,054 bytes per sampler attempt. Generator state is in no transmitted object. No object has to widen: the seed is already 64 bytes, the encapsulated error of weight 503 in 38,000 positions has about 3,853 bits (log2 C(38000, 503)), and the shared key is 64 bytes. The cost per byte of the platform generator was not measured; calls and bytes are counted as for an ideal generator. Keeping a self-contained generator means replacing `rng.c` by one with at least 512 bits of state, a Sub-algorithm change in code with the same byte counts. The report records no proposed fix. Not checked: the optimized tree line by line.
- **kem-32-5.** Lead; it does not decide the answer. Root cause as reported: folding the public code over its order-19 orbits leaves an ordinary Goppa code of degree t/19 (27 and 53), for which the reporter estimates key recovery at 2^141.3 and 2^142.0 bit operations. The two estimates differ by 0.7 bits while the degree doubles, so no parameter model is available that would bring this attack to 256 or 512 bits within the submitted shape. The orbits that give the quotient are also what compresses the public key (physical p. 6), so a repair has to change the key mechanism or the twist. Scope: Construction, specification and code. Impact: not bounded. The report records no proposed fix. The Lead's open steps were not assessed.

## Evidence

- Baseline: §8.2 (physical p. 22) gives public keys of 167,987 / 595,662 / 2,374,727 bytes and ciphertexts of 640 / 1,153 / 2,264 bytes. Parameters (physical p. 6): m = 18, ℓ = 19; (n, t, w) = (10070, 285, 142), (19000, 513, 256), (38000, 1007, 503).
- Control, formula of §3.3 (physical p. 6): u = m(t − 1) + 1, r0 = u mod ℓ, pkbits = ((u − r0)/ℓ + r0)(n − u + r0), ct = ⌈u/8⌉. QCTM128: u = 5113, r0 = 2, 271 × 4959 = 1,343,889 bits = 167,987 bytes. QCTM256: u = 9217, r0 = 2, 487 × 9785 = 4,765,295 bits = 595,662 bytes. QCTM512: u = 18109, r0 = 2, 955 × 19893 = 18,997,815 bits = 2,374,727 bytes. Ciphertexts ⌈5113/8⌉ = 640, ⌈9217/8⌉ = 1,153, ⌈18109/8⌉ = 2,264. All match §8.2 and the public-key assertions of the reference code (`QCTM128/sizes.h:70`, `QCTM256/sizes.h:82`, `QCTM512/sizes.h:82`). §3.2 (physical p. 6) states other public-key sizes, 167,919 / 595,541 / 2,374,489 bytes, which are the same row counts times n − u columns; §8.2 is used.
- kem-32-2, estimator: the pinned `dsDOOM` loaded by `kem-32/reproduce_multi_instance.py`, with the witness's inputs (length n, dimension n − u + 1, weight w, 19·2^s targets). It needs numpy and scipy; the default `python3` here lacks scipy, so an interpreter that has both was used. Command, for the first figure: `python3 -B -c "import sys; sys.path.insert(0,'kem-32'); import reproduce_multi_instance as w; print(w.load_estimator()(10070, 4958, 142, 19 << 64)[:2])"`.

  | inputs (n, dimension, w) | ciphertexts | time | memory |
  |---|---|---|---|
  | 10070, 4958, 142 | 2^64 | 2^133.452 | 2^80.994 |
  | 19000, 9784, 256 | 2^64 | 2^260.726 | 2^85.007 |
  | 38000, 19892, 503 | 2^64 | 2^531.965 | 2^98.041 |
  | 10070, 4958, 142 | 2^80 | 2^127.382 | 2^98.161 |
  | 19000, 9784, 256 | 2^80 | 2^253.524 | 2^97.979 |
  | 10089, 4977, 142 | 2^80 | 2^127.771 | 2^98.191 |
  | 10108, 4996, 142 | 2^80 | 2^128.160 | 2^98.221 |
  | 19114, 9898, 256 | 2^80 | 2^255.749 | 2^98.020 |
  | 19133, 9917, 256 | 2^80 | 2^256.119 | 2^98.027 |
  | 10545, 5433, 142 | 2^80 | 2^136.294 | 2^96.593 |
  | 19551, 10335, 256 | 2^80 | 2^264.170 | 2^98.196 |

- Proposal: the report records only the new lengths. The last two rows use the submitted t and return the 2^136.294 and 2^264.170 that the report records for the proposal, so t is taken as unchanged. Sizes: 271 × (10545 − 5113 + 2) = 1,472,614 bits = 184,077 bytes, +16,090 bytes = +9.58%; 487 × (19551 − 9217 + 2) = 5,033,632 bits = 629,204 bytes, +33,542 bytes = +5.63%. The ciphertext ⌈u/8⌉ does not depend on n.
- Alternative: n moves in steps of ℓ = 19 with n ≤ 2^18 (physical p. 5). 10108 = 19 × 532 and 19133 = 19 × 1007 are the first lengths at or above target in the table. Sizes: 271 × 4997 = 1,354,187 bits = 169,274 bytes, +0.77%; 487 × 9918 = 4,830,066 bits = 603,759 bytes, +1.36%.
- kem-32-3, source under `kem-32/Implementations/Reference_Implementation/QCTM512/`: `rng.h:19–23` (32-byte Key and 16-byte V); `rng.c:113–130` (48 bytes of seed material); `rng.c:132–163` (`randombytes`); `KEM_AlgorithmInstance.c:18–29` (48 bytes from `get_random_number`); `seeded_keygen.h:8` and `kem.c:580` (64-byte seed); `m2e.c:61` with `sizes.h:38–45` (18 × 8 × 503 = 72,432 bits = 9,054 bytes per attempt). Specification: λ = 512 for QCTM512 (physical p. 5); Algorithm 8 step 1 (physical p. 13); Algorithm 5 step 3 (physical p. 12); 64-byte shared key (§8.2, physical p. 22).
