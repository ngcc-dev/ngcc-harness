# UVW signature (NICCS round-1 "sign-32"): design-level assessment from the decoding side

Date: 2026-10-09. Scope: design-level security of UVW, a Wave-type ternary
hash-and-sign scheme, assessed with our own finite-size cost formulas. The
four implementation findings already in the audit (sign-32-1 accept-all
verifier, sign-32-2 all-zero crash, sign-32-3 pair-correlated signatures,
sign-32-4 DRBG reset) are cited, not re-derived. Under the rules for this
pass nothing from the submission archive was built or executed, Feussner's
package was not run, and the toy experiment uses our own generator.
Every number is marked (stated) = taken from the spec or audit,
(cited) = from a paper, or (computed) = our own scripts in
`decoding/{cost.py,keyatk.py,toy.py}` (outputs in `decoding/out_*.txt`).
`cost.py` and `keyatk.py` use the Python standard library; `toy.py` needs
NumPy. The larger estimator runs are recorded in the output files.

## 0. Parameters

(q, n, k, k1, k2, w) = (3, 9700, 4850, 3250, 1600, 8633), (3, 19200, 9600,
6433, 3167, 17088), (3, 39000, 19500, 13066, 6434, 34710) for UVW128/256/512
(stated, spec Table 1). R = 1/2 and W = w/n = 0.8900 for all three
(computed). The spec claims classical Msg/Key security 133.7/304.5,
257.7/603.1, 517.4/1226.1 bits (stated, Table 2) while section 1.6 of the
same document says "classical security strengths of 160, 256, and 512 bits"
(stated): the headline for Level 1 is 26 bits above its own Table 2 forgery
figure. The expected number of weight-w solutions is 2^8626, 2^17080, 2^34699
(computed): UVW forgery sits in the extreme many-solution regime.

## 1. Forgery (message attack)

### 1.1 What the spec evaluates

Section 3.3.2 quotes Loyer's Theorem 7 (cited, ePrint 2023/1263):

    T_msg = max{ (3^l / 2^{(k+l)/2^{a-1}})^{1/(a-2)} ,  3^{n-k-l} / (2^{w-p} C(n-k-l, w-p)) }

The first term is the list size 2^lambda of the smoothed Wagner algorithm of
Bricout-Chailloux-Debris-Alazard-Lequesne (BCDL19 Proposition 4, cited:
lambda = l log2(3)/(a-2) - (k+l)/((a-2) 2^{a-1}), valid for the largest
integer a with 3^{l/(a-1)} < 2^{(k+l)/2^{a-1}}, a >= 3); the second is
1/P_{p,l} from BCDL19 Proposition 2 with p = k+l (all information-set
coordinates nonzero). Loyer's own version has 2^a - 1 search leaves because
one leaf holds the DOOM syndromes; the spec writes the DOOM-free exponent
2^{a-1}. Both are "up to polynomial factors".

### 1.2 Our finite-size evaluation (computed, cost.py)

All costs are log2, optimised over integer a and l; p = k+l throughout.

| model | UVW128 | UVW256 | UVW512 | Wave I | Wave III | Wave V |
|---|---|---|---|---|---|---|
| spec formula as written, integer a (BCDL19 constraint) | 132.7 (a=6,l=439) | 258.9 (6,859) | 521.6 (6,1734) | 133.7 | 198.6 | 263.4 |
| same with Loyer's 2^a-1 DOOM leaves | 132.5 | 258.5 | 520.6 | 133.5 | 198.3 | 263.0 |
| Sendrier 2023 eq. (12), real-valued a, GBA-DOOM | 131.5 (a=5.37) | 256.5 | 516.8 | 132.7 | 197.2 | 261.7 |
| same without DOOM | 131.9 | 257.2 | 518.2 | 133.2 | 197.9 | 262.7 |
| BCDL19 Thm 1 plain Wagner, no smoothing | 135.1 (a=5) | 263.9 | 532.0 | 134.7 | 200.0 | 265.0 |
| Narisada et al. 2026 Thm 1 tree model, best over split/rep patterns | 133.5 (all-split, a=6) | 260.8 (a=6) | 526.7 (a=6) | 135.7 | 199.5 | 264.6 |
| Narisada et al. 2026 Prop. 5 bit cost (All-Split, log-memory factor) | 157.4 (a=6, 2^146 bits memory) | 287.8 | 554.3 | 158.3 | 225.7 | 292.3 |
| spec Table 2 (stated) | 133.7 | 257.7 | 517.4 | - | - | - |
| Narisada Table 2 (cited) | - | - | - | 158 | 226 | 290 |
| Loyer Table 1 smoothed Wagner (cited) | - | - | - | 129 | 194 | 258 |

Calibration: our Proposition 5 implementation reproduces Narisada's
158/226/290 for Wave I/III/V as 158.3/225.7/292.3, and the as-written spec
formula reproduces the spec's three Table 2 values within -1.0, +1.2 and
+4.2 bits. The spec's numbers are therefore the smoothed-Wagner
(BCDL19/Loyer) finite-size formula at a = 6, l about 0.045 n, p = k+l,
without polynomial factors, without DOOM leaves; the residual differences
are grid/rounding.

Representations: the Narisada framework at q = 3 reduces to shifted binary
vectors with no nonzero-overlap representations (alpha-bar = 0), so "rep"
levels are MMT-style one-sided splits. Our optimiser over all 2^(a-1) mode
patterns up to a = 7 chose all-split for every parameter set; BCDL19 section
4.5 says the same for R = 0.5 ("the gain obtained by using representations
is relatively small") and Narisada's optimiser selects All-Split a = 6 for
the Wave point (cited). The depth-2/3 representation trees that win at the
hardest instances (BCDL19 section 6, exponent 0.247; Narisada All-Rep a = 3,
0.243) lose here. DOOM is worth under one bit at a = 5-6 (table rows 3-4;
Sendrier 2023 and Narisada footnote agree, cited).

### 1.3 Why 133.7 is below 0.0151 n

0.0151 n = 146.5 / 289.9 / 588.9 bits (computed) is the exponent for Wave's
point W = 0.894 (Narisada Table 1, tau = 0.0151 at (3, 0.500, 0.894), cited;
Sendrier Table: c = 0.0150016 at W = 0.89412, cited). The forgery exponent at
R = 1/2 increases with W up to W = 1 (BCDL19 Fig. 8, cited; Sendrier's
table: c = 0.01399 at W = 0.89193, 0.01622 at W = 0.89665, cited). UVW uses
W = 0.8900, below all of Wave's candidate points, and the same GBA-DOOM model
gives exponent 0.01356 / 0.01336 / 0.01325 there (computed, finite-n
evaluation; at Wave I our finite-n value is 0.01548 against Sendrier's
asymptotic 0.01500, the difference being binomial polynomial factors). So the
claimed numbers are consistent with the asymptotic model at UVW's own W; the
0.0151 figure simply does not apply at W = 0.89. UVW128 (n = 9700) and Wave I
(n = 8576) end up with the same forgery cost to within one bit: UVW spends
13% more length to pay for the lower weight.

### 1.4 Verdict on forgery

In the spec's own cost model generic forgery is AT the claimed Table 2 level
(within 1-4 bits, all three sets). In a gate/bit-count model with
logarithmic memory cost (Narisada Prop. 5) it is about 24-33 bits above
Table 2: 157 / 288 / 554. Against the section 1.6 headline "160 classical
bits" UVW128 is 2.6 bits short even in the generous bit model and 27 bits
short in the model the spec itself uses; UVW256/512 exceed 256/512 in the
bit model only. Nothing we know undercuts the Table 2 values: plain Wagner,
smoothed Wagner, representation trees and DOOM all land within 2 bits of each
other. The 2^146-bit memory of the a = 6 tree is the usual caveat on these
figures.

## 2. Key recovery and distinguishing

### 2.1 The trapdoor (spec section 2.2.1, Algorithms 4-7)

Secret: H_X, H_Y in F3^{(m-k1) x m}, H_Z in F3^{(m-k2) x m} with m = n/2, and a
monomial D (permutation times diagonal in {1,2}). H_sk = [[H_X, H_Y],
[-H_Z, H_Z]] D; the public key is the systematic form (I | R) of H_sk
(stated). Writing a codeword before D as (y1, y2): H_Z (y2 - y1) = 0 and
(H_X + H_Y) y1 = -H_Y (y2 - y1), so the code is

    C = { (u + v_z, u + v_z + z) D : u in U = ker(H_X + H_Y) (dim k1), z in Z = ker H_Z (dim k2) }

with v_z a fixed linear lift (computed derivation). This is Wave's
generalized (U | U+V) code {(a*u + b*v, c*u + d*v)} (DST19 Prop. 1, cited)
with the U-part scaled only by the monomial (a_i, c_i) = (d_pi(i),
d_pi(i+m)) and the V-part replaced by two different vectors (v_z, v_z + z)
from a k2-dimensional space; the "W" of the name is V + Z. Spec section 2.2.2
shows Wave is the special case D = diag(c) + I, H_Z = H_V, H_Y = -b*H_U, H_X
= (c+b)*H_U (stated). Consequences: (i) the code contains the paired subcode
{(u, u) D}, dim k1, with N_(u,u)(t) = C(m, t/2) 2^{t/2} / 3^{m-k1} words of
weight t (spec Prop. 6, stated; identical to Wave's type-U count, cited);
(ii) the dual contains the paired subcode {(-h, h) D : h in rowspace H_Z},
dim m - k2 = k1 since k = m; (iii) type-V words (v_z, v_z + z) have no
pairing, so UVW has fewer low-weight anomalies than Wave's type-V words.

Decoding: Dec_Z solves e2 H_Z^T = s2 by one Prange step with t ~ D2 nonzero
information-set coordinates (D2 uniform on [0, k2] in the code; the spec
does not fix it, stated in pseudocode.md). Dec_XY takes an information set I
of H_X + H_Y of size k1, forces e1(i) != 0 and e1(i) + e2(i) != 0 for every
i in I (over F3 this means e1(i) = e2(i) whenever e2(i) != 0), solves the rest
linearly and repeats until |(e1, e1+e2)| = w exactly (stated, Alg. 5). The
expected output weight is 2k1 + (2/3)(n - 2k1) (stated), so k1 = (3/2) w - n
+ g with gap g; UVW128 has g = 0.5, i.e. Wave's "no gap" setting (computed;
Sendrier 2023 section 4.3: g = 0 "would require an additional heuristic
assumption", cited). Hence k1/k = 0.67 is not a free security margin as spec
Fig. 9 presents it: it is forced by W = 0.89 and g = 0.

### 2.2 Key-recovery cost

Spec: Loyer Theorem 5 / Sendrier's type-U search (Dumer ISD for one of the
N_(u,u)(t) paired codewords, optimised over t), 304.5 / 603.1 / 1226.1 bits
(stated). Our own Dumer model, iteration cost max(L, L^2/3^l) with L =
C((k+l)/2, p/2) 2^{p/2}, success per iteration N_U(t) C(k+l,p) C(n-k-l,t-p)
/ C(n,t), optimised over t, p, l (computed, keyatk.py): 308.2 (t = 0.210 n,
p = 72, l = 192), 608.7, 1288 (p-grid bound reached, upper bound) and 146.1
for Wave I, against Loyer/Sendrier's 138 with MMT (cited). Optimal t/n = 0.21
matches Sendrier's remark (0.209, cited), far above the weight t0 = 0.089 n
where the first (u,u) word appears (computed) and the public code's
Gilbert-Varshamov distance 0.160 n (computed). The code and its dual give the
same cost because k = n/2. Loyer's Remark 2: Wagner does not beat Dumer here
(cited). CryptographicEstimators' SDFq cannot model the structured
multiplicity and rejects w > n-k, so it was used only as a unique-solution
sanity point: Stern 913 bits at (9700, 4850, 862) (computed), consistent with
the exponent of our iteration model. Key recovery is therefore about 2.3x the
forgery exponent: the parameters are forgery-bound, and the k1/k = 0.696
crossing in spec Fig. 9 is Sendrier's balance point for Wave (kU/k = 0.692
at R = 1/2, cited), which UVW deliberately does not use.

### 2.3 Cheap structural invariants (toy experiment, computed, toy.py)

Keys generated with our own numpy code following Alg. 6 (random H_X, H_Y,
H_Z, random monomial D), compared with random F3 codes, three keys each:

| n, k, k1, k2 | hull dim UVW / random | Schur square of C and of C^perp | shortened-code squares |
|---|---|---|---|
| 48, 24, 16, 8 | 0,0,0 / 1,0,1 | 48 (full) both | equal to min(n-s, k'(k'+1)/2) for both |
| 80, 40, 27, 13 | 0,0,2 / 0,0,0 | 80 (full) both | equal |
| 120, 60, 40, 20 | 0,0,1 / 0,1,2 | 120 (full) both | equal |

No invariant separates the families, as expected from DST19's argument that
non-trivial a, b, c, d (here the monomial) kill hull attacks (cited). The
exhaustive weight distribution at n = 20, k = 10, k1 = 7, k2 = 3 does show the
Prop. 6 tails: weight-4 counts 8, 4, 10 (UVW) vs 4, 2, 0 (random) against a
predicted (u,u) excess of 6.7, and weight-20 counts 50, 50, 44 vs 24, 22, 18
against predicted 17.8 + 37.9 = 55.7 (computed). The paired subcode is real
but, at the UVW sizes, only reachable by the 300+-bit ISD above.

### 2.4 Does the generalization weaken anything?

Not structurally: it is Wave's family with a monomial instead of four
coefficient vectors, the same k1-dimensional paired subcodes in code and
dual, and a less structured V-part. The weaknesses are parametric and
distributional: g = 0 (section 2.1), and the sampler (section 3).

## 3. Signature-distribution leakage (sign-32-3)

Mechanism (audit report.md and Feussner's note, cited): a signature gives
the last k coordinates of e and the full e is recomputed as e' = H(m,r) - e
R^T. Before D, e = (e1, e1 + e2); for every i in the fresh information set I
(|I| = k1 = 3250) both members of the hidden pair are nonzero, and their
ratio is +1 when e2(i) = 0 and -1 otherwise. The fixed D keeps the same 4850
pairs and relative scales for the key's lifetime. Our heuristic pair
statistics (computed): P(e2 coordinate = 0) = 0.388; for a true pair P(both
nonzero) = 0.823 vs 0.792 for a random pair of a weight-0.89 vector, and
P(opposite | both nonzero) = 0.58 vs 0.50, up to the fixed scale d_i d_j.
With 300 signatures a Barreto-Persichetti-type statistic |#opposite -
#equal| gives about 2.5 sigma per true pair over 4.7e7 candidate pairs;
Feussner's likelihood score plus mutual-nearest-neighbour filtering yielded
4,074 candidates of which 3,992 were true, only 1,600 are needed to span the
pair-sum subspace S(F3^1600 x 0), and quotienting the 9,700 public columns by
it recovers all 4,850 pairs and scales exactly (cited); 200 signatures
failed the public quotient certificate (cited). The equivalent key
[[H~_X, H~_Y], [-H~_Z, H~_Z]] then runs the spec's own decoders (cited).

Design or implementation? Design. Spec Alg. 5 step 3 mandates the forced
pair condition |x_I| = |(x + e2)_I| = k1, the only rejection is on the total
weight, and the p. 10 claim that this makes the output "indistinguishable
from those of a purely random large-weight vector [10]" is wrong: total
weight does not control pair statistics. Wave had exactly this problem in
its first draft, Barreto-Persichetti (ePrint 2018/1111) recovered the key
from a few hundred signatures with the statistic above, and Wave's final
design added two-dimensional rejection sampling on (|e_V|, m1(e)) where
m1(e) counts pairs (i, i+n/2) with exactly one nonzero, with a proven
statistical-distance bound (DST19 sections 5.1-5.3, Props. 4-6, Thm 1, and
section 8.4: 25,000 proper signatures resisted the attack; cited). UVW's
spec has no counterpart: section 3.2.3 analyses syndromes of vectors already
uniform on the sphere, not the decoder's output (Feussner section 8, cited;
our reading agrees), D2 is left unspecified, and the reference code follows
the spec (uniform D2, weight-only rejection; audit pseudocode.md, stated).
A fix needs Wave-style tuned distributions for t = |e2| and rejection on the
pair statistics, which costs rejection rate and, following Sendrier, a gap
g > 0, i.e. a parameter change, not a code patch. UVW256/512 use the same
sampler (stated); only UVW128 was demonstrated (cited).

## 4. Relevance to our decoding work

Our ternary target is decodingchallenge.org's LW3SD at R = log3(2) = 0.36907,
w = floor(0.99 n), n = 250 (stated on the challenge page), the
unique-solution corner where BCDL19 (section 6, 0.247) and Narisada (All-Rep
a = 3, 0.243) put the hardest ternary instances; the ladder stands at n = 240
(Narisada et al., "a variant of Wagner's algorithm", 2025-10-22, after
Esser-May-Zweydinger's MMT variant to n = 200; cited from the hall of fame).
UVW forgery is the opposite corner: 2^8626 solutions, all-split Wagner with
a = 5-6 and lists of 2^133, representations and DOOM worth under two bits.
Neither the record ladder's algorithms nor its constants say anything about
UVW's cost; the ladder validates representation trees at R = 0.37, W = 0.99,
not depth-6 merge trees at R = 0.5, W = 0.89. What a concrete implementation
would calibrate is the 24-bit gap between the spec's polynomial-factor-free
133 and the bit-cost 157: per-element merge and memory cost of a depth-5/6
ternary Wagner tree, measurable at n = 1500-2500 where the predicted cost is
2^{0.0135 n + c} = 2^30-2^45 (computed, extrapolation). That calibration is
only useful if UVW survives the sampler redesign; as submitted, sign-32-1
and sign-32-3 make the forgery cost moot.

## 5. Summary

1. Generic forgery: 132.7/258.9/521.6 (our evaluation of the spec's formula)
   vs claimed 133.7/257.7/517.4: at the claim. Bit-cost model: 157/288/554.
   UVW128 misses its own 160-bit headline in the models evaluated here, but
   exceeds the NGCC 128-bit target; this discrepancy is not a finding.
2. The 0.0151 n exponent belongs to W = 0.894; at UVW's W = 0.890 the model
   gives 0.0133-0.0136 n, which is what the spec's numbers reflect.
3. Key recovery: 308/609/1288 bits (Dumer, t = 0.21 n) vs claimed
   304.5/603.1/1226.1; no cheap hull/square distinguisher at toy sizes.
4. The (U+V, U+W) structure is Wave's with a monomial; k1/k = 0.67 is the
   zero-gap setting, not a margin.
5. sign-32-3 is a design defect: the Barreto-Persichetti leakage that Wave
   fixed with rejection sampling in 2018, reintroduced by Alg. 5 step 3.

## References

- UVW Signature Scheme, NICCS round-1 submission archive (spec PDF sign-32-spec.pdf): https://www.niccs.org.cn/niccs/Proposal/Public-Key%20Cryptographic%20Algorithms/Round%201%20candidates/UVW%20signature.zip
- Audit record: [public UVW-Sign report](https://ngcc.dev/reports/sign-32.html) and [harness pseudocode](https://github.com/ngcc-dev/ngcc-harness/blob/main/sign-32/pseudocode.md).
- M. Feussner, "Pairwise Signature Leakage in UVW-128", PKC Forum post 2026-09-27: https://list.niccs.org.cn/archives/list/pkcforum@list.niccs.org.cn/message/SDUWEI2UT7BAEIDNVKTSMJF3YYJXUDFQ/ ; note: https://github.com/martinfeussner/NGCC-Signature-Audit/blob/91f2ddf0590a24ad39afc2ce2ee4f2627ec54726/UVW/UVW_Attack_Description.pdf
- R. Bricout, A. Chailloux, T. Debris-Alazard, M. Lequesne, "Ternary Syndrome Decoding with Large Weight", SAC 2019: https://eprint.iacr.org/2019/304
- S. Narisada, H. Okada, Y. Aikawa, K. Fukushima, "A Generalized ISD Framework for Large-Weight Syndrome Decoding", 2026: https://eprint.iacr.org/2026/1947
- J. Loyer, "Quantum security analysis of Wave", IACR CiC: https://eprint.iacr.org/2023/1263
- N. Sendrier, "Wave Parameter Selection", PQCrypto 2023: https://eprint.iacr.org/2023/588
- T. Debris-Alazard, N. Sendrier, J.-P. Tillich, "Wave: A New Family of Trapdoor One-Way Preimage Sampleable Functions Based on Codes", ASIACRYPT 2019: https://eprint.iacr.org/2018/996 (text used: https://arxiv.org/abs/1810.07554)
- P. S. L. M. Barreto, E. Persichetti, "Cryptanalysis of the Wave Signature Scheme", 2018: https://eprint.iacr.org/2018/1111
- N. Sendrier, "Decoding One Out of Many", PQCrypto 2011: https://eprint.iacr.org/2011/367
- Wave NIST submission and documentation: https://wave-sign.org/
- Decoding challenge, Large Weight Ternary Syndrome Decoding: https://decodingchallenge.org/large-weight
- CryptographicEstimators (SDFq, low weight only): https://github.com/Crypto-TII/CryptographicEstimators
