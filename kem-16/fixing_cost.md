No: a stated limit of 2^64 encapsulations per public key repairs kem-16-2 for HARE-5 and HARE-9, and HARE-7 also needs a slightly heavier and longer parameter set, estimated at about 1% more encapsulation key and 1.5% more work per operation.

Impact: encapsulation key +1% at HARE-7 (estimate); ciphertext +0.5% at HARE-7 (estimate); key generation, encapsulation and decapsulation +1.5% at HARE-7 (estimate)
Scope: Parameter set (large), specification and code
Assessed: 2026-11-23

## Attacks considered

- kem-16-2: Same-key multi-instance decoding misses three HARE targets (Design, Confirmed)

## Remediation

- **kem-16-2.** Root cause: every ciphertext exposes u = r1 + h·r2 under the fixed h, so Q ciphertexts give nQ targets for one same-code search. A stated limit of 2^64 encapsulations per public key is a credible repair for HARE-5 (HARE-256) and HARE-9 (HARE-512): at the limit the finding's estimator gives 2^259.097 and 2^516.244, and the IND-CCA2 claim (§7.3.2, physical p. 19) and use with static keys are kept. It does not cover HARE-7 (HARE-384), whose estimate at 2^64 is 2^383.175. The costed repair is therefore the limit plus a new HARE-7 parameter set. The smallest change that keeps the shape w = wr of Table 2 is w = wr = 194 (from 193), which gives 2^385.226 at the limit; the Model 1 failure bound that Table 2 states for HARE-7 (< 2^-384) then needs n = 105883 (from 104869), the first primitive prime at which the bound holds again; we, n1, n2 and α stay. Scope: Parameter set, specification and code: one statement of the limit, the HARE-7 rows of Tables 2 and 3, and the HARE-384 `parameters.h`. Impact at HARE-7: encapsulation key 13,157 to 13,284 bytes (+1%), ciphertext 23,693 to 23,820 bytes (+0.5%), and about +1.5% work in each operation, which is dominated by one, two and three multiplications of length n (estimate, n^log2(3) model; counting the schoolbook leaf products of the submitted Karatsuba recursion gives +0.8%). HARE-2, HARE-5 and HARE-9 do not change. The estimate has no margin and is a lower bound; it is under 5%. The report records no proposed fix for this finding. HARE-7 is the optional 384-bit level: withdrawing it would leave the limit as the whole repair, with impact none, but the note costs the repair. Raising only wr to 194 needs n = 105331 (+0.4%). Alternative without a limit: at 2^80 ciphertexts the smallest weights are w = wr = 133, 197 and 261 for HARE-5, -7 and -9, with n = 53891, 108947 and 176507. That is encapsulation key +2.9%, +3.9% and +1.5%, ciphertext +1.6%, +2.2% and +0.8%, and multiplication work +4.7%, +6.3% and +2.3% under the same model, or +7.2%, +9.7% and +1.9% by the leaf count (estimate); for HARE-7 the alternative is between 5% and 10%. For HARE-5 the alternative holds the Model 1 value at its submitted 2^-239.21, because the Model 2 bound of Table 2 cannot be recomputed from the submission. All values are costing aids, not proposed parameters. Not checked: the ISD estimates of Table 4 at the new values, the Model 2 bound, other ways to restore the failure bound (we, α, n1), and the multiplication of the optimized implementation.

## Evidence

- Baseline: Table 2 (physical p. 11): HARE-5 n = 52379, w = wr = 131, we = 224, n1 = 81, n2 = 640, α = 9; HARE-7 n = 104869, w = wr = 193, we = 361, n1 = 91, n2 = 1152, α = 15, DFR < 2^-384 (Model 1); HARE-9 n = 173981, w = wr = 259, we = 449, n1 = 151, n2 = 1152, α = 13, DFR < 2^-519 (Model 1). Table 3 (physical p. 12): |ek| and |c| are 6580 and 11790 bytes (HARE-5), 13157 and 23693 (HARE-7), 21812 and 39294 (HARE-9).
- Size formulas, §4.2 (physical p. 11): |ek| = |seed| + ⌈n/8⌉ and |c| = ⌈n/8⌉ + ⌈(l1 − l2 + k2⌊l1/n2⌋)/8⌉ + |salt|, with l1 = n1·n2 of Table 2, the covering code's k2 = 41 and block length 51 (physical p. 5; `Reference_Implementation/HARE-384/kr/parameters.h:25–33`), and seed and salt of level/8 bytes. Control, HARE-7: 48 + ⌈104869/8⌉ = 48 + 13109 = 13157; l1 = 104832, ⌊l1/51⌋ = 2055, l1 − l2 = 27, ⌈(27 + 41 × 2055)/8⌉ = 10536, and 13109 + 10536 + 48 = 23693. At n = 105883: ⌈n/8⌉ = 13236, so |ek| = 13284 (+127 bytes, +0.97%) and |c| = 23820 (+0.54%). The script below checks all four rows of Table 3.
- Estimator: the pinned `dsDOOM` loaded by `kem-16/reproduce_multi_instance.py`, with the witness's inputs (length 2n, dimension n, weight 2wr, n·2^s targets). It needs numpy and scipy; the default `python3` here lacks scipy, so an interpreter that has both was used. Command, for the first figure: `python3 -B -c "import sys; sys.path.insert(0,'kem-16'); import reproduce_multi_instance as w; print(w.load_estimator()(2*52379, 52379, 2*131, 52379 << 64)[:2])"`.

  | n | wr | ciphertexts | time | memory |
  |---|---|---|---|---|
  | 52379 | 131 | 2^64 | 2^259.097 | 2^95.356 |
  | 104869 | 193 | 2^64 | 2^383.175 | 2^97.358 |
  | 173981 | 259 | 2^64 | 2^516.244 | 2^98.818 |
  | 104869 | 194 | 2^64 | 2^385.138 | 2^97.358 |
  | 105883 | 194 | 2^64 | 2^385.226 | 2^97.385 |
  | 105331 | 194 | 2^64 | 2^385.178 | 2^97.370 |
  | 52379 | 131, 132, 133 | 2^80 | 2^253.007, 2^254.941, 2^256.877 | 2^111.356 at 133 |
  | 104869 | 193, 196, 197 | 2^80 | 2^376.576, 2^382.443, 2^384.399 | 2^113.358 at 197 |
  | 173981 | 259, 260, 261 | 2^80 | 2^509.701, 2^511.667, 2^513.634 | 2^114.818 at 261 |
  | 53891 | 133 | 2^80 | 2^257.018 | 2^111.438 |
  | 108947 | 197 | 2^80 | 2^384.754 | 2^113.468 |
  | 176507 | 261 | 2^80 | 2^512.907 | 2^114.860 |

- Failure bound and code length: `python3 -B kem-16/fixing_cost_dfr.py` (standard library; it imports the Model 1 functions of `kem-16/reproduce_model1_dfr.py`). It prints, for HARE-7: submitted log2 DFR = -384.9304; w = wr = 194 at n = 104869: -366.5341; smallest primitive prime n = 105883 (+0.967%): -384.7885; |ek| 13157 -> 13284, |ct| 23693 -> 23820; 1639 -> 1655 words, +1.55%; leaf word products 366128 -> 368878, +0.75%. For wr = 194 only: n = 105331, -384.0693. The search bisects on n and assumes that the bound falls as n grows.
- Alternative: `python3 -B kem-16/fixing_cost_dfr.py --ceiling` prints HARE-5 n = 53891 (-239.4568 against the submitted -239.2123), |ek| 6769, |ct| 11979, +4.68% (leaf count +7.20%); HARE-7 n = 108947 (-384.2060), |ek| 13667, |ct| 24203, +6.26% (leaf count +9.65%); HARE-9 n = 176507 (-519.1046 against the bound -519), |ek| 22128, |ct| 39610, +2.28% (leaf count +1.86%).
- Speed: `vect_mul` is a Karatsuba multiplication of ⌈n/64⌉ words (`gf2x.c:74–156` under `kem-16/Implementations and Test_Vectors/Implementations/_shared/hare_core/ref/`); it is called once in key generation (`hqc.c:54`), twice in encryption (`hqc.c:120,127`) and once in decryption (`hqc.c:191`), and decapsulation re-encrypts (Algorithm 8, physical p. 11). Table 5 (physical p. 20) gives 106450.7, 220198.8 and 330734.3 kilocycles for HARE-7, a ratio of 1 : 2.07 : 3.11, so the multiplications dominate. The work ratio used is (words'/words)^log2(3) = (1655/1639)^1.585 = 1.0155. The leaf count is the number of word products in the schoolbook leaves of that recursion, which splits down to at most 16 words (`gf2x.c:22,39–60,75–78`). It moves in steps with n: the leaves are 12 or 13 words at both HARE-7 lengths of the costed repair, and 13 or 14 words at n = 108947. Nothing was measured.
