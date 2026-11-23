No: the costed repair of kem-37-3 is a stated limit of 2^64 encapsulations per public key, which changes no transmitted object and no operation in any of the four sets.

Impact: none
Scope: Single line (small), specification
Assessed: 2026-11-23

## Attacks considered

- kem-37-3: Same-key multi-instance decoding puts all TriQ-KEM sets below target (Design, Confirmed)

## Remediation

- **kem-37-3.** Root cause: every ciphertext under one key gives n rotated targets u = r1 + h·r2 for one [2n, n] decoding search, and the estimate falls below target only after 2^73 (TriQ-KEM-128), 2^77 (TriQ-KEM-256, -384) and 2^78 (TriQ-KEM-512) honest ciphertexts. The costed repair is a stated limit of 2^64 encapsulations per public key. At that limit the finding's estimator gives 2^130.500, 2^260.775, 2^389.227 and 2^517.231, all at or above target. The limit is on honest encapsulations: use with a long-term key, the IND-CCA2 notion and the specification's budget of 2^80 decapsulation queries (physical p. 11) are kept. Scope: Single line, specification. Impact: none. The report records no proposed fix for this finding. Alternative without a limit: at 2^80 ciphertexts the estimates are 2^125.247, 2^254.586, 2^382.493 and 2^510.565. At unchanged n the smallest weights wr that reach the targets are 69 (from 67), 133 (from 132), 197 (from 196) and 261 (from 260). A heavier r1, r2 raises the error that the decoder sees. The specification sets wx = wy = wr (physical p. 11); if that shape is kept and n grows in proportion to the error products 2·wr^2 + we with the code unchanged, the encapsulation key grows about 6%, 1.5%, 1.0% and 0.8% and the ciphertext about half as much (judgment). Raising wr alone would halve these figures and leave the stated shape. Table 8 (physical p. 19) states failure bounds 2.3, 21.6, 28.9 and 18.6 bits beyond the targets of physical p. 11, so part of the heavier error may fit at unchanged n. Not checked: the failure bounds of Table 6 and the bounded-density parameters of Table 4 at the new values, the ISD tables, and the work of the alternative. These values are costing aids, not proposed parameters.

## Evidence

- Baseline: Table 2 (physical p. 11) gives (n1·n2, n, wr, we) = (16128, 16301, 67, 106), (49152, 50363, 132, 234), (96768, 97651, 196, 350) and (156672, 157627, 260, 475). Sizes (physical p. 12): |ek| = k/8 + ⌈n/8⌉ and |c| = ⌈n/8⌉ + ⌈n1·n2/8⌉ + 32. Control against Table 5 (physical p. 12): TriQ-KEM-128 16 + 2038 = 2054 and 2038 + 2016 + 32 = 4086; TriQ-KEM-512 64 + 19704 = 19768 and 19704 + 19584 + 32 = 39320. The limit changes none of these values.
- Estimator: the pinned `dsDOOM` loaded by `security/reproduce_code_multi_instance.py`, with the witness's inputs (length 2n, dimension n, weight 2wr, n·2^s targets). It needs numpy and scipy; the default `python3` here lacks scipy, so an interpreter that has both was used. Command, for the first figure: `python3 -B -c "import sys; sys.path.insert(0,'security'); import reproduce_code_multi_instance as w; print(w.load_estimator()(2*16301, 16301, 134, 16301 << 64)[:2])"`.

  | set | 2^64 ciphertexts: time, memory | 2^80 ciphertexts: time | wr below target at 2^80 | smallest wr at target at 2^80 |
  |---|---|---|---|---|
  | TriQ-KEM-128 | 2^130.500, 2^91.993 | 2^125.247 | 68: 2^127.099 | 69: 2^128.954 |
  | TriQ-KEM-256 | 2^260.775, 2^95.243 | 2^254.586 | 132: 2^254.586 | 133: 2^256.511 |
  | TriQ-KEM-384 | 2^389.227, 2^97.152 | 2^382.493 | 196: 2^382.493 | 197: 2^384.450 |
  | TriQ-KEM-512 | 2^517.231, 2^98.533 | 2^510.565 | 260: 2^510.565 | 261: 2^512.532 |

- Alternative, scaling: the decoder's error is e′ = x·r2 + r1·y + e (physical p. 20), with products wx·wr + wr·wy + we. With wx = wy = wr this is 2·67^2 + 106 = 9084 at TriQ-KEM-128 and 2·69^2 + 106 = 9628 at wr = 69 (+6.0%); the other sets give 35612 / 35082 (+1.5%), 77968 / 77182 (+1.0%) and 136717 / 135675 (+0.8%). Theorem V.8 (physical p. 21) states that the error probability falls as n grows at fixed weights. The specification calls n the smallest primitive prime above n1·n2 (physical p. 11), but the submitted n are not the smallest (16139 is one above 16128), so n is taken as free above n1·n2. For TriQ-KEM-128, n is then about 17,277, |ek| = 16 + 2160 = 2176 (+5.9%) and |c| = 2160 + 2016 + 32 = 4208 (+3.0%). At n = 17299, 51133, 98669 and 158843, the next primitive primes from the scaled lengths, the estimator gives 2^128.951, 2^256.673, 2^384.548 and 2^512.616.
