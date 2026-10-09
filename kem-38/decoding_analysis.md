# UVW-KEM (NICCS round-1 "kem-38"): structural key recovery without a decoder oracle

Assessment of 2026-10-09. Scripts and logs: `decoding/`
(`uvwlin.py`, `exp1_squares.py`, `exp2_pairs.py`, `exp3_counting.py`,
`exp4_fp_scan.py`, `exp5_dfr.py`, `*.log`). All test keys were generated
in Python from `kem-38/pseudocode.md` with own randomness; the reference
implementation was not built or run. Numbers are marked (stated) = from
the spec or an audit finding, (computed) = here, (cited) = literature.
The finite-size experiments need Python with NumPy; `exp3_counting.py` and
`exp4_fp_scan.py` additionally need `cryptographic_estimators`. Run scripts
from `decoding/`; the included logs retain the larger experiments' output.

## 1. Setting

UVW128 (stated): q = 433, n = 860, k = 430, k1 = k2 = 215, w = 116,
`Gsk = [[GU, GU], [GV, GW]] D`, GU and GW random 215 x 430, `GV = GRS + GW`
with GRS the [430, 215] Reed-Solomon code on alpha = 1..430, D a secret
monomial matrix, pk the systematic form. Since k1 + k2 = n/2 and
`[GU; GW]` is invertible, the public code is a graph (computed):

    C      = { (a + a Phi,  a) D     : a in F_q^430 },
    C^perp = { (x, -x - x Phi^T) D'  : x in F_q^430 },  D' = (D^-1)^T,

with Phi = [GU; GW]^-1 [0; GRS] of rank 215, kernel U = rowspace(GU),
image RS. So C contains the (u,u)D subcode T = {(a, a)D : a in U} of
dimension k1 = 215 and C^perp contains S = {(x, -x)D' : x in RS^perp} of
dimension n/2 - k2 = 215, RS^perp a [430, 215] generalized RS code. Both
are (U | U+V)-type codes with hidden pairs; in C the pair-invariant part
is random and the "V" part is RS, in C^perp the pair-anti-invariant part
is GRS and the "V" part U^perp is random. The 430 pairs (i, pi(i)) with
ratios lambda_i are the whole secret: c_i - lambda_i c_pi(i) exposes an
RS codeword plus error on known evaluation points, and public GS list
decoding finishes decryption (what kem-38-3 and kem-38-5 exploit).

## 2. Global distinguishers (question 1)

Method: exact modular linear algebra in numpy (float64 matmul, exact
below 2^53). Schur squares are certified full when products of random
codeword pairs reach rank n; secret subcodes use exhaustive basis
products. Random [n, n/2] codes over F_433 are the controls.

| n (k = n/2) | dim C^2 | dim (C^perp)^2 | dim C * C^perp | hull | T^2 (secret) | S^2 (secret) |
|---|---|---|---|---|---|---|
| 60 | 60 / 60 | 60 / 60 | 59 / 59 | 0 / 0 | 30 (= n/2) | 29 (= n/2 - 1) |
| 100 | 100 / 100 | 100 / 100 | 99 / 99 | 0 / 0 | 50 | 49 |
| 200 | 200 / 200 | 200 / 200 | 199 / 199 | 0 / 0 | 100 | 99 |
| 400 | 400 / 400 | 400 / 400 | 399 / 399 | 0 / 0 | 200 | 199 |
| 860 (UVW128 size) | 860 / 860 | 860 / 860 | 859 / 859 | 0 / 0 | 430 | 429 |

Entries are structured / random (computed). The hidden subcodes behave as
predicted (S^2 has the GRS dimension 2k' - 1 = 429, T^2 is full on the
430 pair classes), but they are masked: the public squares, the
primal-dual product (859 = n - 1 for any dual pair, since coordinates of
c * c' sum to <c, c'> = 0) and the hulls equal the random controls at every
size. The diagonal of D cancels in C * C^perp, leaving only the
permutation; that product is nevertheless generic.

Shortening and puncturing (computed, `exp1_*.log`): at every size and for
s random positions from 2 to k - 6, shortened and punctured C and C^perp
have the same (dimension, square dimension) as the random control, also
where k'(k'+1)/2 < n' (both then give exactly k'(k'+1)/2). Shortening at
s random positions cuts S to about 215 - s dimensions while the random
complement of equal dimension saturates every product. Positive control:
shortening at p *true* pairs (secret) leaves a 430 - 2p dimensional code
with a (x,-x)-type GRS subcode of dimension 215 - p, and its square is
deficient once p is close to k2 (n = 400: p = 90 of 100 gives 174
against 210; n = 200: p = 45 of 50 gives 49 against 55; full size below),
by exactly the S-subcode bound (2(k2-p)-1) + (k2-p)^2 + (k2-p)(k2-p+1)/2.
It needs about 0.93 k2 correct pairs, so it verifies a pairing but
cannot search for one.

Conclusion for question 1: no Schur-square, product, hull, shortening or
puncturing statistic separates the public code or its dual from a random
[860, 430] code over F_433 (computed at n = 60..860). This is expected:
the GRS component is masked by an equal-dimension random component (215
against 215), unlike Berger-Loidreau subcodes, Wieschebrink's inserted
columns, BBCRS's low-rank perturbation or RLCE's few random columns, where
the random part is small and squares stay deficient (cited: Wieschebrink
2010; Couvreur et al. 2014, 2015; Couvreur-Lequesne-Tillich 2019).
Filtration attacks (Couvreur-Otmani-Tillich 2014) need small squares
along a shortening chain; here every shortened public code is generic.

### 2.1 Full-size shortening (n = 860)

(dimension, square dimension) of C^perp, structured against random
control (computed, `exp1_full.log`); C behaves identically.

| shortened at | n' | structured | random | generic bound | structured bound |
|---|---|---|---|---|---|
| 2 random positions | 858 | (428, 858) | (428, 858) | 858 | - |
| 107 random | 753 | (323, 753) | (323, 753) | 753 | - |
| 215 random | 645 | (215, 645) | (215, 645) | 645 | - |
| 387 random | 473 | (43, 473) | (43, 473) | 473 | - |
| 418 random | 442 | (12, 78) | (12, 78) | 78 | - |
| 424 random | 436 | (6, 21) | (6, 21) | 21 | - |
| punctured at 2 / 215 / 428 random | 858 / 645 / 432 | (430, n') | (430, n') | n' | - |
| 172 true pairs (secret) | 516 | (86, 516) | (86, 516) | 516 | 516 |
| 193 true pairs | 474 | (44, 474) | (44, 474) | 474 | 474 |
| 199 true pairs | 462 | (32, 423) | (32, 462) | 462 | 423 |
| 206 true pairs | 448 | (18, 143) | (18, 171) | 171 | 143 |

The structured square becomes deficient only when at least about 199 of
the 215 pair classes are shortened correctly, and then by exactly the
amount the S-subcode predicts.

## 3. Finding the pairing directly (question 2)

Pair tests on coordinates (i, j) with a hypothesized ratio lambda, true
pair with true ratio against false pair with random ratio (computed,
`exp2_200.log`, `exp2_860.log`):

| test | true pair | false pair | n = 860 true / false |
|---|---|---|---|
| dim C shortened at {i, j} | k - 2 | k - 2 | 428 / 428 |
| dim C^perp shortened at {i, j} | k - 2 | k - 2 | 428 / 428 |
| dim C punctured at {i, j} | k | k | 430 / 430 |
| dim {c in C : c_i = lambda c_j} | k - 1 | k - 1 | 429 / 429 |
| square of {c in C : c_i = lambda c_j} | n | n | 859 / 859 |
| square of C, C^perp shortened at {i, j} | n - 2 | n - 2 | 858 / 858 |
| dim C ∩ J_1(C), J_1 the scaled transposition (i j) | k - 1 | k - 1 | 429 / 429 |

Every single-pair statistic is identical for true and false pairs, and
exactly so: a (pair, ratio) hypothesis is one linear condition
c_i = lambda c_j on C, which always cuts one dimension. For a true pair
the functional c_i - lambda c_j is the RS evaluation at that pair's
hidden index and vanishes on T; for a false pair it is generic. Telling
them apart means seeing that the true functionals span only the
215-dimensional annihilator of T. With m known true pairs plus one
candidate (computed, n = 860, k2 = 215; same pattern at n = 200, k2 = 50):

    m = 1, 2, 107, 213, 214:  rank(known + true) = rank(known + false) = m + 1
    m = 215 = k2:             rank(known + true) = 215, rank(known + false) = 216
    m = 216, 220:             215 against 216

Equivalently dim C ∩ J_m(C) = k - min(m, k2) for the correct pairs and
k - m otherwise, where J_m swaps m scaled pairs. The first signal therefore
needs k2 + 1 = 216 simultaneously correct (pair, ratio) triples out of
C(860, 2)(q - 1) = 1.6 * 10^8 candidates (computed), with no gradient
below the threshold. A scan of all single pairs costs about 1.6 * 10^8
rank computations (0.2 s each in numpy at n = 200, 7 s at n = 860 under
load), i.e. roughly 2^30 seconds, and would return nothing.

Codeword-level statistics do not help either: the product test
dim(s * Q ∩ s' * Q) for two words of Q = C^perp is 1 for two S-words and 1
for two random words, because s * s' lies in both spaces trivially
(computed). Low-weight dual words: the (x,-x)D' words of C^perp have
weight 2 * wt(x) with x in a [430, 215] MDS code, so none below 432
(computed, exact MDS weight distribution), while a random [860, 430] code
over F_433 already has about 2^0.5 words at weight 336 and 2^853 at 430
(computed). The structured dual words are therefore not low-weight
outliers; they are the ordinary-weight words the spec's model counts.

What does work once a few structured words are in hand (computed,
`exp2_860.log`, `exp2d_860.log`): the coordinate ratio vectors of r
random (u,u)D words of C (or (x,-x)D' words of C^perp) identify the
pairing. At n = 860, r = 2 words give all 430 true pairs plus 852 false
candidates (ratio collisions in a 433-element field); r = 3 gives 430
true and 0 false, as does r = 4 (dual side: 428-429 true, the rest hidden
by a zero coordinate of the first word). Three (u,u)D words are a
complete key recovery; finding one is the problem the spec prices
(Section 4).

Conclusion for question 2: no polynomial-time pair test exists within
linear-algebraic invariants; the pairing is recoverable only from
structured codewords (3-4 suffice) or from 216 simultaneous correct
guesses, and the decoder-oracle attacks kem-38-3 and kem-38-5 are the
only known practical routes.

## 4. The spec's distinguishing model (question 3)

The spec (stated, Section 3.2.1) counts (u,u)D words E1(t) and other
words E2(t) by eq. (55), takes P(t) = E1/(E1 + E2) and prices the
distinguisher as F(t)/P(t) with F from the Niebuhr et al. idealized ISD
bound, obtaining a minimum of 2^403.71 at t = 410 for UVW128 and 2^502.65
for any word at t = 336.

Reproduction (computed, `exp3.log`, `exp4.log`): E2/E1 at t = 430 is
2^428.8 (spec: about 2^428); both word types first reach expectation 1 at
t = 336-338 (spec: 336); CryptographicEstimators' q-ary Stern (the package
has only Prange, Lee-Brickell and Stern for SDFq, no q-ary representation
algorithm) gives F(336) = 2^516.2 (spec 2^502.6) and
min_t F(t)/P(t) = 2^416.9 at t = 412 (spec 2^403.71 at t = 410). The
model is internally consistent; the 13-bit difference is the estimator's
polynomial overhead against the spec's lower-bound formula.

Assessment of the model:

1. It is the right generic attack for this structure and the argument
   Wave uses (cited, Debris-Alazard-Sendrier-Tillich 2019); no way to
   bias ISD toward paired supports is known without the pairs.
2. It does not model the dual-side GRS subcode, but the omission favors
   the designers: "k1 = n/2 - k2 makes C and C^perp equivalent" treats
   the dual's pair-anti-invariant part as a random [430, 215] code (first
   words at weight 2 * 169 = 338), whereas it is MDS with first words at
   weight 2 * 216 = 432, where P is 2^-437.5 (computed). The dual is
   strictly less exposed in the counting model.
3. It ignores algebraic distinguishers; Sections 2-3 test the standard
   ones and find none, so this fills a gap rather than contradicting it.
4. It prices the key at rest only. The pairing is also what the
   decoder's information-set step depends on, and the CD/GD model says
   nothing about decryption behaviour, which is where the candidate
   fails (kem-38-2, -3, -5, cited).
5. One found (u,u)D word is not yet a break; three are (Section 3), and
   the second and third cost no more than the first, so F/P stands as
   the structural-attack cost.

## 5. Generic message attack: the 2^146.55 against 2^128.36 gap

The audit's SDFqEstimator figures (cited from the established context:
Stern 2^146.55 / 2^275.95 / 2^533.31 bit operations) exceed the spec's
2^128.36 / 2^256.84 / 2^513.22 (stated) by 18-20 bits. The spec's
Proposition 8 is the Niebuhr et al. formula, described there as "an
idealized ISD variant, considering only essential steps and ignoring
overhead/memory access costs" (stated): a lower bound that charges
K_q = 2w per candidate and nothing for the per-iteration Gaussian
elimination (about 2^30 bit operations here, computed from the
estimator's floor at large t) or list handling, while the estimator
charges a full Stern iteration in bit operations. A polynomial factor of
order n^2 (2^19.4 at n = 860) is exactly the difference between a
lower-bound exponent and a bit-operation count; both agree on the
exponential term. The gap does not weaken the message-security claim; it
means 2^128.36 is a floor, not an attack cost, and the estimator's figure
is the one comparable with other submissions. No q-ary
BJMM/representation figure is given: the package has none, and
representation gains shrink with q (cited, Peters 2010; Niebuhr et al.
2017).

## 6. Comparison with Wave and the GRS literature; design implication (question 4)

Wave (cited) uses a permuted (U | U+V) code over F_3 with U and V random;
its structural security is the same (u,u)/(0,v) counting, and its decoder
output (signatures) is made key-independent by rejection sampling with a
proof. UVW keeps the counting argument but makes V - W a public RS code
for an efficient decoder. Sections 2-3 show the algebraic part is well
masked at rest: 215 random dimensions saturate every product, so the
attacks on Niederreiter-GRS (Sidelnikov-Shestakov 1992), Berger-Loidreau
subcodes (Wieschebrink 2010), Wieschebrink's and BBCRS's maskings
(Couvreur et al. 2014, 2015) and short RLCE keys (Couvreur-Lequesne-
Tillich 2019), all of which needed a low-rank or few-column random part,
do not apply.

The price is paid in the decoder. To use the RS code the decryptor forms
c11 - c12 over the hidden pairs, which cancels any error whose two
coordinates sit on a pair with the hidden ratio. Such positions are
invisible in ebar, land in the "error-free" set I, and spoil the
information-set solve for r1 whenever I1 contains one. The exact model
(computed, `exp5_dfr.py`, `exp5.log`): a weight-116 error covers both
coordinates of 7.77 hidden pairs on average, each cancels with
probability 1/(q - 1), P(c >= 1 cancelled) = 2^-5.8, and summing the
chance that a random I1 of size 215 inside I hits one of the c cancelled
positions gives a per-attempt failure of 0.011877, equal to the spec's
1 - 0.988123 (stated, Theorem 2) to six digits. With the implementation's
1,000-attempt cap the same model gives a final failure rate of 2^-43.32
against the exact 2^-43.317 of kem-38-5 (cited); c = 5 contributes
2^-43.5, c = 4 and c = 6 about 2^-47.7 each. So the per-attempt DFR and
the final DFR have one cause: the information-set step is blind to
errors that cancel on hidden pairs. Each attempt succeeds with
probability about 0.34^c, so the spec's unbounded loop runs for a time
exponential in c (the timing oracle of kem-38-2/-3) and the cap turns
c >= 5 into a visible failure (kem-38-5). Neither hides c, and c depends
on the secret pairing and the attacker-chosen error. Wave has no such
secret-dependent failure.

Design implication of kem-38-5: a 2^-43 overall DFR is not negligible for
a CCA KEM with explicit rejection (the FO bound carries q_dec * delta,
cited HHK 2017; lattice and code KEMs target 2^-128 or below), and these
failures are key-dependent, so each leaks about five pairs; 1,000
failures gave the full pairing (cited, kem-38-5). Raising the cap only
trades failures for time. A pair-independent decoder would have to
locate the cancelled positions, i.e. decode the random U component,
which is the hard problem the construction relies on; making U decodable
too would restore the algebraic structure that Sections 2-3 show is
currently masked. The construction is sound against the structural
attacks considered here and unsound as a CCA KEM with this decoder, for
the same reason.

## 7. Status

Structural key recovery without the decoder: closed for Schur-square,
product, hull, shortening, puncturing, single- and multi-pair linear
tests and low-weight statistics at n = 60..860 (computed); the remaining
cost is the ISD-for-(u,u)D figure, 2^416.9 (computed, Stern) against
2^403.71 (stated). The exposed surface is the decoder (kem-38-2/-3/-5).

## References

- NICCS round-1 archive UVW-KEM.zip:
  https://www.niccs.org.cn/niccs/Proposal/Public-Key%20Cryptographic%20Algorithms/Round%201%20candidates/UVW-KEM.zip
- Audit records: [public UVW-KEM report](https://ngcc.dev/reports/kem-38.html)
  and [harness pseudocode](https://github.com/ngcc-dev/ngcc-harness/blob/main/kem-38/pseudocode.md).
- kem-38-3, Xie and Liu (openHiTLS), PKC Forum:
  https://list.niccs.org.cn/archives/list/pkcforum@list.niccs.org.cn/message/A6PUI23BHC7YNE5UQ3SMRO33GDBWCF2P/
  and follow-up
  https://list.niccs.org.cn/archives/list/pkcforum@list.niccs.org.cn/message/XLK4PVBFDFWXQA4QXKNBPNQRKKITR5OW/
- kem-38-5/6, Xiong and Wang, PKC Forum:
  https://list.niccs.org.cn/archives/list/pkcforum@list.niccs.org.cn/message/N2M32DODRHH53N6CQRXVSY3J77PZEWEK/
- T. Debris-Alazard, N. Sendrier, J.-P. Tillich, "Wave: A New Family of
  Trapdoor One-Way Preimage Sampleable Functions Based on Codes",
  ASIACRYPT 2019. https://eprint.iacr.org/2018/996
- V. M. Sidelnikov, S. O. Shestakov, "On insecurity of cryptosystems based
  on generalized Reed-Solomon codes", Discrete Math. Appl. 2(4), 1992.
  https://doi.org/10.1515/dma.1992.2.4.439
- T. P. Berger, P. Loidreau, "How to mask the structure of codes for a
  cryptographic use", Des. Codes Cryptogr. 35, 2005.
  https://doi.org/10.1007/s10623-003-6151-2
- C. Wieschebrink, "Cryptanalysis of the Niederreiter Public Key Scheme
  Based on GRS Subcodes", PQCrypto 2010.
  https://doi.org/10.1007/978-3-642-12929-2_5
- A. Couvreur, P. Gaborit, V. Gauthier-Umaña, A. Otmani, J.-P. Tillich,
  "Distinguisher-based attacks on public-key cryptosystems using
  Reed-Solomon codes", Des. Codes Cryptogr. 73, 2014.
  https://arxiv.org/abs/1307.6458
- A. Couvreur, A. Otmani, J.-P. Tillich, V. Gauthier-Umaña, "A
  Polynomial-Time Attack on the BBCRS Scheme", PKC 2015.
  https://arxiv.org/abs/1501.03736
- M. Baldi, M. Bianchi, F. Chiaraluce, J. Rosenthal, D. Schipani,
  "Enhanced public key security for the McEliece cryptosystem",
  J. Cryptology 29, 2016. https://arxiv.org/abs/1108.2462
- A. Couvreur, A. Otmani, J.-P. Tillich, "Polynomial Time Attack on Wild
  McEliece over Quadratic Extensions", EUROCRYPT 2014 / IEEE Trans. IT
  63(1), 2017. https://arxiv.org/abs/1402.3264
- A. Couvreur, M. Lequesne, J.-P. Tillich, "Recovering short secret keys
  of RLCE in polynomial time", PQCrypto 2019.
  https://arxiv.org/abs/1805.11489
- Y. Wang, "Quantum resistant random linear code based public key
  encryption scheme RLCE", ISIT 2016. https://eprint.iacr.org/2015/298
- C. Peters, "Information-Set Decoding for Linear Codes over F_q",
  PQCrypto 2010. https://eprint.iacr.org/2009/589
- R. Niebuhr, E. Persichetti, P.-L. Cayrel, S. Bulygin, J. Buchmann, "On
  lower bounds for information set decoding over F_q and on the effect of
  partial knowledge", Int. J. Inf. Coding Theory 4(1), 2017.
  https://doi.org/10.1504/IJICOT.2017.081458
- A. Esser, J. Verbel, F. Zweydinger, E. Bellini, "CryptographicEstimators:
  a Software Library for Cryptographic Hardness Estimation", 2023.
  https://eprint.iacr.org/2023/589 and
  https://github.com/Crypto-TII/CryptographicEstimators
- D. Hofheinz, K. Hövelmanns, E. Kiltz, "A Modular Analysis of the
  Fujisaki-Okamoto Transformation", TCC 2017.
  https://eprint.iacr.org/2017/604
