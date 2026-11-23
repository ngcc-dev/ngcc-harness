No: taking the PORS+FP instance index from the counter-bearing hash and adding the two missing checks to the specified OTS verifier change no signature byte and add well under 1% of hash work in all eight FlexTree parameter sets.

Impact: none
Scope: Function (small), specification and code
Assessed: 2026-11-23

## Attacks considered

- sign-11-1: PORS counter grinding reduces every claimed security level (Design, Confirmed)
- sign-11-3: The specified OTS verifier omits its encoding checks (Design, Confirmed)

## Remediation

- **sign-11-1.** The finding states two repairs. The costed one binds the counter into the hash that selects the PORS+FP instance: `idxTree` and `idxLeaf` are taken from the output of `H_PORS(R, ctr, md)`, which already carries the counter, and no longer from `H_MSG`, which does not. Every counter trial then selects a new instance, which is the selection `(R, msg, ctr) → (idxTree, idxLeaf, I)` that Section 3.1 already describes and the model of the specification's bound (3.7). Scope: Function, specification and code: the index derivation moves from Algorithms 23 and 24 to Algorithms 19 and 20, and the `H_PORS` output grows by the index bytes (Tables 1.4 and 1.5). Impact: none (count). The signature already carries `R` and the 4-byte counter and gains nothing. The number of `H_PORS` calls is unchanged, because acceptance depends only on the subset. Each call outputs 9 or 10 more bytes, which is one more 32-byte output block in three sets and none in the other five. The trials are 0.7% of the signing hash calls at 160f, which needs no extra block, and under 0.01% in the other seven sets; verification makes one such call. The other reading of the same repair, adding the counter to `H_MSG`, hashes the message once per trial, about 1,251 times at 160f, so its cost grows with the message length; it is not the cheapest. The second stated repair, a unique minimal counter that the verifier checks, adds 1/τ − 1 `H_PORS` evaluations to verification: +24% of the verification hash calls at 160f and at most +3% elsewhere (estimate). The report has no `Proposed fixes` subsection. Not checked: whether the repaired selection meets the subset-resilience property that Section 3.1 assumes; the reporter's adjusted 20- to 32-bit figures; non-hash work per trial, which does not change.
- **sign-11-3.** The repair adds the zero-bit test and the constant-sum test to Algorithm 10 and to `Ver` in Construction 1, as Algorithm 9 already requires of the signer. The submitted verifier performs both tests, so this is a specification change that adds no work and no byte. Scope: Function, specification. Impact: none. Not checked: the code tests a different bit window from the one Algorithm 9 names; that is a separate finding below Critical and is not costed.

## Evidence

- Script: `python3 -B sign-11/fixing_cost_grinding.py` (standard library, about four minutes) prints every figure below.
- Baseline sizes: Table 1.3 (physical p. 46). Equation (4.1) (physical p. 59) reproduces all eight stated signature sizes, for example 20 + (4 + 27·20 + 155·20) + 17·(4 + 40·20) + 67·20 = 18,672 bytes for FlexTree-160f. No term changes.
- Baseline work: Table 4.1 (physical p. 61). Equation (4.7) reproduces the eight verification counts exactly. The signing count of physical p. 60, with τ computed exactly by the script, reproduces the eight signing counts to within 2 calls when its tree term uses C_xmssNode(0) where the text has C_xmssNode(0) + 1; with the text's term it is 0.02% to 0.14% higher.
- Trials: 1/τ(t, k, mMAX) = 1.8, 1251.4, 1.0, 15.0, 93.4, 35.9, 351.8 and 189.4 for 160s, 160f, 256s, 256f, 384s, 384f, 512s and 512f. Against the signing counts of Table 4.1 the trials are 0.695% of the calls at 160f and at most 0.007% elsewhere.
- `H_PORS` output: n + ⌈log2 C(t,k)/8⌉ bytes (Tables 1.4 and 1.5, physical pp. 47–48) is 44, 50, 73, 77, 110, 126, 151 and 157 bytes. The index bytes ⌈(h − h0)/8⌉ + ⌈h0/8⌉ (Algorithm 23, lines 5–6, physical p. 43) are 10 for 384s and 512s and 9 otherwise. At 32 output bytes per block the count rises by one block for 384f, 512s and 512f.
- Minimal-counter alternative: 1/τ − 1 more calls per verification is 23.6% of the verification calls at 160f, 2.6% at 512s, 1.5% at 384s, 1.0% at 512f and at most 0.2% elsewhere.
- Bound (3.7) (physical p. 57) at 2^64 signatures evaluates to 160.000, 160.007, 256.000, 256.035, 384.000, 384.000, 512.000 and 512.000 bits, each at least 8n. This is the specification's own model with one fresh instance per trial; it is a costing aid and not an evaluation of the repair.
- Report witness: `python3 security/flextree_kudinov_validation.py` ran and passed; it gives counter-grinding figures from 128.892 bits (160f) to 480.017 bits (512s) for the submitted design.
- Specification steps: Algorithm 23, lines 4–9 (index and `md` from `H_MSG`, no counter); Algorithm 19, lines 3–7 (counter search on `H_PORS`); Algorithm 24, lines 9–18; Algorithm 20, line 2 (physical pp. 40, 43–44). Algorithm 9, line 6, against Algorithm 10, lines 6–10 (physical pp. 30–31); Construction 1 (physical p. 50).
- Source (`Implementations/Reference_Implementation/Flextree-160f`): `sign.c:133,141` and `sign.c:213,220` (digest and index, then PORS+FP); `hash_sm3.c:113–118` (index from the message digest); `pors_fp.c:219–236` (`pseudoXOF` over R ‖ ctr ‖ md); `pors_fp.c:681–699` (counter loop). `wots.c:99` in the eight reference trees and `wots.c:134` in the eight optimized trees test the constant sum and the zero bits.
