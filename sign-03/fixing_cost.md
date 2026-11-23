No: decoding the hypertree index from the message digest, as Algorithm 17 specifies, is a code change that leaves every size and every hash count of the eight CEDRUS+C parameter sets as submitted.

Impact: none
Scope: Function (small), code
Assessed: 2026-11-23

## Attacks considered

- sign-03-1: Hypertree index collapse causes repeated few-time keys (Implementation, Confirmed)

## Remediation

- **sign-03-1.** The costed repair is the one the finding states: decode `idxTree` from the digest where the code assigns `*tree = 0`, and correct the height macros that size and mask the two indices. The specification is sound on the point (Algorithm 17, lines 11–14, and Algorithm 18, lines 13–16), so this is the usual repair for code that departs from its specification, and nothing cheaper removes the root cause. The change is confined to the digest-parsing step of `hash_message` and the two height macros it relies on, repeated in the sixteen submitted trees. Scope: Function, code. Impact: none (count). The signature does not carry the index and its length does not depend on it, so no transmitted byte changes. The code skipped no specified work: with the tree address forced to zero, signing still builds one full XMSS subtree and one WOTS+C signature at each of the `d` layers, and the corrected code builds the same subtrees at other addresses. The counter search on `H_msg` and all of verification are unchanged. The report records no proposed fix. Not checked: nothing was built or run; the optimized trees were checked only for the same assignment and macros, not for their tree-building code.

## Evidence

- Baseline sizes: Table 1.1 (physical p. 38). Equation (4.1) (physical p. 47), `n + (4 + k(a+1)n) + d(4 + len·n) + h·n`, reproduces all eight stated signature sizes from the stated parameters, for example 20 + 4,804 + 17·804 + 1,320 = 19,812 bytes for CEDRUS+C-160f. No term depends on `idxTree` or `idxLeaf`.
- Specification: Algorithm 17 (physical p. 35) takes `idxTree` and `idxLeaf` from the digest and reduces them mod 2^(h−h0) and 2^h0; `signHT` (Algorithm 10, physical p. 29) signs at all `d` layers for any index. The specification gives timings and hash-call counts (Tables 4.1 and 4.2, physical p. 49); the count of Section 4.1 has no index-dependent term.
- Source, `Implementations/Reference_Implementation/CEDRUSC-160f`: `hash_sm3.c:172` is `*tree = 0;` (the same line in all eight reference and all eight optimized trees). `params/params-cedrusc-sm3-160f.h:37` defines `SPX_BOTTOM_TREE_HEIGHT` as `SPX_TREE_HEIGHT + 1` without parentheses, used by the leaf mask at `hash_sm3.c:78,176`. `hash_sm3.c:76` defines `SPX_TREE_BITS` as `SPX_FULL_HEIGHT - SPX_TREE_HEIGHT + 1`, two bits more than h − h0 in every set, since h0 = ⌈h/d⌉ (physical p. 44) and `d` divides `h` in none of them (Table 1.1); a decoded index needs the corrected width.
- Work: `sign.c:154–171` calls `merkle_sign` for every layer whatever the index, and `merkle.c:85` builds the whole subtree. The `H_msg` counter loop is `hash_sm3.c:143–159`; signer and verifier parse the digest in the same routine (`sign.c:132,217`).
