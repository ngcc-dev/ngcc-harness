Yes: no repair is known that keeps CHAMP's two-generator matrix product and invertible output encoding against the square-root collision and preimage searches and the explicit CHAMP-512 collisions, so the impact on CHAMP-512 and CHAMP-1024 cannot be bounded.

Impact: not bounded
Scope: None known (large), specification and code
With leads: Yes; not bounded; None known (large), specification and code
Assessed: 2026-11-23

## Attacks considered

- hash-04-1: Palindrome messages reduce CHAMP collision bounds to 128 and 256 bits (Design, Confirmed)
- hash-04-2: Exact reduced-size subgroup collisions support a square-root full-size route (Design, Lead)
- hash-04-3: Known-length preimages admit an exact square-root search (Design, Confirmed)
- hash-04-6: Explicit full-parameter collisions break CHAMP-512 (Design, Confirmed)
- hash-04-7: Short commuting palindromes have a distribution-free square-root collision search (Design, Confirmed)

## Remediation

- **hash-04-1, hash-04-7.** The root cause is the structure of products of two `2 x 2` generators of equal determinant. Same-length palindromes take at most `p^2 - p` values (hash-04-1) and fall into `p + 2` commuting classes (hash-04-7, for both specified generator pairs). Both bounds depend only on `p`. The digest is the invertible encoding of the four matrix entries, so `h = 4 log2 p`, and the hash-04-7 search costs about `2^(h/8)` against a target of `2^(h/2)` for every prime that this encoding allows. A larger prime is therefore not a Parameter set repair. The bound of hash-04-7 reaches the target only at `log2 p >= h`, where the matrix has `4h` bits and the output step would have to compress it, which Section 3.2 excludes. Replacing the generators is not relied on, because hash-04-7 holds for both specified pairs and a symmetrizing form exists for any pair (see Evidence). A repair therefore has to change the matrix product or the output step. The report records no proposed fix, and none is costed. Scope: Construction, specification and code. Impact: not bounded.
- **hash-04-2.** Lead. It projects the same `sqrt(p)` collision cost as hash-04-7 by a different search, so the same reasoning applies and it adds nothing to the scope or the impact. Scope: Construction, specification and code. Impact: not bounded.
- **hash-04-3.** The root cause is that `H_M(XY) = H_M(X) H_M(Y)` and that the output encoding is invertible. A target digest decodes to its matrix, and a message of known length splits into two halves. The cost `2^(m/2)` does not depend on `p`, so no parameter change repairs it. Section 3.2 requires the invertible encoding in order to keep the features of Section 4. A repair has to give up that requirement and use an output step that cannot be inverted. No such change is proposed. Scope: Construction, specification and code. Impact: not bounded.
- **hash-04-6.** Complete collisions exist at the submitted CHAMP-512 prime. The report does not establish how the search scales with `p` or whether it extends to CHAMP-1024, so no parameter estimate can be made. No proposed fix is recorded. Scope: None known, specification and code. Impact: not bounded.

## Evidence

- Baseline: `p = 2^128 - 15449` for CHAMP-512 and `p = 2^256 - 36113` for CHAMP-1024 (Section 5.1, physical p. 4). The digest is the four matrix entries, each inverted and written as 128 or 256 bits (Sections 3.1, 3.3 and 3.4, physical p. 2; Section 5 steps 2 and 3, physical p. 3). Control: 4 x 128 = 512 and 4 x 256 = 1024. A hash has no bandwidth.
- Work in the reference code: one `2 x 2` product modulo `p` per message byte, which is 8 `mulmod` and 4 `addmod` (`CHAMP/Implementations and Test_Vectors/API_CryptHash/Implementations/Reference_Implementation/CHAMP-512/CryptHash_AlgorithmInstance.c:201-208`, loop `:277-281`; CHAMP-1024 `:225-232`, `:306-310`), and four inversions at the end (`:299-302`; `:328-331`). Table 1 (physical p. 7) gives throughput of the optimized code only and was not used.
- hash-04-7: the report bounds the expected draws by `sqrt(pi (p+2))`. `(log2(pi) + 128)/2 = 64.83` for CHAMP-512 and `(log2(pi) + 256)/2 = 128.83` for CHAMP-1024, against targets of 256 and 512. `sqrt(p) >= 2^(h/2)` needs `log2 p >= h`, which is 512 and 1024 bits; the encoding gives `log2 p = h/4`, which is 128 and 256 bits.
- hash-04-1: `python3 -B hash-04/reproduce_structure.py` prints `palindrome image <= p^2-p` with `2^128` samples for CHAMP-512 and `2^256` for CHAMP-1024. A birthday bound of `2^(h/2)` on that image needs `log2 p >= h/2`, against `h/4`.
- hash-04-3: `2^256` for a 512-bit input against `2^512`, and `2^512` for a 1024-bit input against `2^1024` (report). Section 3.2 (physical p. 2) says the conversion "has to be deterministic, efficient, and efficiently invertible", and Section 4 (physical p. 2-3) derives the claimed features from it.
- Generators: for a symmetric `S` with entries `(a, b, c)`, "`SA` and `SB` symmetric" is two homogeneous linear equations in three unknowns, so a nonzero `S` exists for every pair. For the main pair they are `a - b - 4c = 0` and `2a + 7b + 6c = 0`, solved by the report's `S = (22, -14, 9)`. Whether `S` can be singular for some other pair was not examined.
- Illustration only, not a costed repair: at the 512- and 1024-bit primes that the hash-04-7 bound alone would need, the reference schoolbook `mulmod` would use 8 x 8 = 64 limb products in place of 2 x 2 = 4 (CHAMP-512 `:177-198`) and 16 x 16 = 256 in place of 4 x 4 = 16 (CHAMP-1024 `:201-222`). That is 16 times the limb products per field multiplication, before any change to the output step.
- Not checked: the forum post and the papers behind hash-04-6 and hash-04-7 were read only through the report. No full-size search was run. Other matrix groups or generator sets were not examined, since they would be a different construction.
