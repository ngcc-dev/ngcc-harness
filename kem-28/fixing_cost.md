No: rejecting ciphertext coefficient fields of value q or more in decapsulation changes no size and adds only one comparison per coefficient to an operation of three transforms, two products and two hash calls, in all three parameter sets.

Impact: none
Scope: Function (small), specification and code
Assessed: 2026-11-23

## Attacks considered

- kem-28-1: Noncanonical ciphertext encoding defeats IND-CCA security (Design, Confirmed)

## Remediation

- **kem-28-1.** The root cause is that DecodePoly (Algorithms 12 and 14) accepts every 13- or 15-bit field and Decap (Algorithm 3) neither range-checks the decoded ciphertext polynomial nor compares the received bytes, so the fields `c` and `c + q` give one residue and one shared key. The costed repair is the standard canonical-encoding check: Decap returns ⊥ when a decoded ciphertext coefficient is `q` or more. It is the cheapest credible repair because it is one step of one algorithm, it leaves each ciphertext polynomial with a single accepted encoding in all three sets, and it keeps the transform and the parameters. Scope: Function, specification and code. Algorithm 3 gains the check after line 1, and each `kem_dec` gains it after `poly_frombytes(&p_c, ct)`. The specification changes because it does not state the check. The ciphertext format is unchanged: fewer byte strings are accepted, and EncodePoly already writes residues below `q`, so honest ciphertexts pass. Impact: none. No transmitted object changes. Decapsulation gains `n` comparisons (648, 1296 or 2592), one pass over the coefficients. In the reference OAEP-NTRU-648 code it already makes about 14,800 coefficient multiplications in three transforms and two base products, and 30 SM3 compressions in its two hash calls, so the added pass is under 1% of the operation by count. The share is no larger at the other two sets: their transforms have 7 and 8 layers, and their hash inputs and outputs grow with `n`. Key generation and encapsulation are unchanged. The report records no proposed fix. The report states that the optimized and additional trees have the same omission; their decoders were not read for this note, and nothing was built or measured.

## Evidence

- Baseline: Table 4 (physical p. 29) gives |pk| = 1053, 2430, 4860 and |ct| = 1085, 2494, 4988 bytes. Control: ct = EncodePoly(c) ‖ σ (Algorithm 2 line 13, physical p. 7) with σ of λ/4 bytes (Algorithm 10, physical p. 9), so 13·648/8 + 32 = 1085, 15·1296/8 + 64 = 2494 and 15·2592/8 + 128 = 4988. The check changes none of these terms.
- Unused field values: q = 7129 in 13-bit fields, q = 17497 and 28513 in 15-bit fields (Table 1, physical p. 15; Algorithms 11–14, physical pp. 10–13).
- Specification steps: Algorithm 3 lines 1 and 14–16 (physical p. 7). Table 3 (physical p. 28) counts Decap as 2 hashes, 1 NTT, 2 inverse NTTs and 2 multiplications.
- Source, reference trees: `poly_frombytes` at `poly.c:104` has no range check. `kem_dec` calls it on the ciphertext at `KEM_AlgorithmInstance.c:170` (OAEP-NTRU-648), `:215` (-1296) and `:185` (-2592), and already accumulates its other checks in `fail`.
- Count for OAEP-NTRU-648, transforms: the forward transform makes 324 + 3·648 + 2·324 = 2,916 modular multiplications in 6 layers (`ntt.c:57–143`), and the inverse makes 2·324 + 3·648 + 3·324 = 3,564 (`ntt.c:155–224`). Decapsulation runs one forward and two inverse transforms (`KEM_AlgorithmInstance.c:174–182`; the second inverse is the normalized variant, counted as the first), 2,916 + 2·3,564 = 10,044.
- Count for OAEP-NTRU-648, products: each base product is 216 blocks of 11 multiplications and 5 reductions (`ntt.c:399–407`, `poly.c:496–503`), 2,376 multiplications; two products give 4,752. With the transforms, 10,044 + 4,752 = 14,796.
- Count for OAEP-NTRU-648, hashing: `hash_g` returns 162 bytes from a 163-byte input, 6 output blocks of 3 SM3 compressions; `hash_h_prime` returns 64 bytes from a 357-byte input, 2 output blocks of 6 compressions (`symmetric.c:26,39`; the contest `pseudoXOF` hashes input and counter once per 32 output bytes, `auxfunc.c:500–519`). Total 18 + 12 = 30.
- Share: taking a transform multiplication with its Montgomery reduction and butterfly additions as 7 word operations, a base-product reduction as 4, an SM3 compression as about 2,000 and a comparison with its OR as 2, decapsulation costs 10,044·7 + 4,752 + 2·1,080·4 + 30·2,000 = 143,700 and the check adds 2·648 = 1,296, which is 0.9%. The weights are the assessor's; nothing was measured.
- Not counted: decoding of three polynomials, the mod-3 lift, the SOTP check and the short-polynomial encodings, which add further passes over the coefficients and lower the share.
- Layers at the other sets: (d, L3, L2) = (2, 4, 2) and (3, 3, 4) in Table 1 (physical p. 15), one initial layer plus L3 + L2, so 7 and 8 against 6.
