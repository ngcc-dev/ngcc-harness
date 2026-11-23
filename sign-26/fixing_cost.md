No: implementing the specified challenge grinding (sign-26-2) adds 20 hash calls at Level-1 and 429 at Level-3 to each signing and verification, which is at most about 1.6% of the verifier's work, and it changes no size.

Impact: verification +1.6% at Level-3 (estimate)
Scope: Function (small), code
Assessed: 2026-11-23

## Attacks considered

- sign-26-2: Missing challenge grinding drops Levels 1 and 3 below their targets (Implementation, Confirmed)

## Remediation

- **sign-26-2.** Root cause: `hash_to_challenge` makes one XOF call (`sign.c:738–769`), where section 5.1(4) (physical p. 33) builds HASH by iterating a hash ⌈2^λ/3^e2⌉ times. The repair is the one the report states: signer and verifier run the specified k-fold construction, with k = 21 at Level-1 and k = 430 at Level-3; k = 1 at Levels 2 and 4. It is the cheapest credible repair because it changes one routine and keeps the challenge space, the prime and every encoding. Scope: Function, code (the loop in `hash_to_challenge`, called at `sign.c:819` and `sign.c:1132`, and one constant per level). The code skipped specified work, so the corrected code does more than the baseline: 20 and 429 further hash calls per signing attempt and per verification, each on a 32- or 64-byte input. Impact: verification at most +0.6% at Level-1 and +1.6% at Level-3; signing at most +0.1% and +0.3% (estimate). No transmitted object changes: the challenge stays in Z_{3^e2} and the signature keeps its length (Table 1). The report records no other proposed fix.

## Evidence

- Baseline: Table 1 (physical p. 56) gives public key and signature sizes of 64 and 150 bytes at Level-1 and 128 and 293 at Level-3; they do not change. Table 3 (physical p. 57) gives Verify 12.54 ms and 96.01 ms and Sign 64.04 ms and 466.88 ms at Levels 1 and 3, as totals with no breakdown, so the operation counts are the evidence.
- Iteration counts: `python3 -B sign-26/reproduce_grinding_shortfall.py` prints `specified_iterations` 21, 1, 430 and 1 for Levels 1 to 4. Source paths are relative to `Implementations/sqisign2d_lvlN/src`; `sign.c` is `sqisigndim2/ref/sqisigndim2x/sign.c`, which is byte-identical at the four levels.
- Calculation: `python3 -B sign-26/fixing_cost_hash_work.py` compares the added hash work with a lower bound on the verifier's field work, in 64-bit word operations. One iteration is one Keccak-f[1600] permutation of 3,720 word operations (24 rounds of 155, counted in the submitted `common/generic/fips202.c:240`), because the iterated value of 8*NWORDS_FIELD = 32 or 64 bytes fits the 136-byte rate (`fips202.c:20`). One F_p multiplication is 100 word operations at Level-1 and 392 at Level-3 (calls in `fiat_p13178_mul` and `fiat_p263156_mul`). The script prints 20 x 3,720 = 74,400 against 119,181 x 100 = 11,918,100 at Level-1 (0.62%), and 429 x 3,720 = 1,595,880 against 261,301 x 392 = 102,429,992 at Level-3 (1.56%). The signing figures divide by the Sign/Verify ratio of Table 3, 5.11 and 4.86.
- The verifier bound counts only fixed-length loops of `protocols_verif` (`sign.c:1009–1147`): the cofactor ladders and triplings of the three torsion bases, the triplings of the 3^e2-isogeny chain, and the (2,2)-chain of length `SQISIGN_response_length` (128 and 260). It leaves out square roots, inversions, basis retries, the double-scalar ladders, the matrix application, gluing and 3-isogeny evaluations.
- Not checked: nothing was built, run or measured. The percentages are upper bounds in two ways: the verifier count is a lower bound, and a 64-bit multiplication is counted like an XOR. The ICCS build calls `pseudoXOF` (`iccs/xof_iccs.c`); it is treated as an ideal XOF with the dimensions of the SHAKE256 that the generic build uses (`common/generic/xof_shake.c`). Section 5.1(4) gives the iteration count but not the byte encoding of the iterated input; an input longer than one block would scale the count by the number of blocks.
