# Quantum-shortcut audit of the 119 NGCC candidates

Date: 2026-09-29  
Method: specification and literature review; no candidate or attack execution  
Status: research audit, not a new set of confirmed vulnerability findings

## Summary

This review did not identify a complete polynomial-time quantum break of a
candidate whose corresponding classical problem remains hard. This is a
screening conclusion, not a proof that no such attack exists. Comparisons with
classical algorithms concern known methods, not proven classical lower bounds.

The strongest structural case is **NIIKE (`kex-08`)**: its oriented class-group
action admits a known subexponential quantum approach. The specification already
accounts for that approach. Other notable cases are exponential quantum
improvements for the isogeny candidates and TRINE, and theoretical quantum
extensions of existing CHAMP, uHash and QSH searches.

| Candidate class | IDs reviewed | Count |
|---|---|---:|
| Key encapsulation | `kem-01` through `kem-41` | 41 |
| Signatures | `sign-01` through `sign-34` | 34 |
| Hash functions | `hash-01` through `hash-35` | 35 |
| Key exchange | `kex-01` through `kex-09` | 9 |
| Total | | **119** |

## Scope and interpretation

The review covered all candidate construction summaries, relevant quantum and
security passages in the specifications, existing reports, and primary
literature. Three parallel reviewers covered KEMs, signatures and hashes; the
coordinating review covered key exchange and cross-checked the principal
conclusions. Unusual algebraic constructions received closer examination.

This was not a line-by-line revalidation of every proof or implementation, nor
an independent reconstruction of every concrete quantum circuit estimate.
No builds, witnesses, attacks or quantum simulations were run for this audit.

Known classical polynomial-time defects were set aside where a local repair
appeared plausible, while retaining the underlying design for quantum review.
For constructions needing substantial redesign, such as the aligned-basis
Polar-KEM construction, no particular secure repair was assumed.

The following distinctions apply throughout:

- A quantum query bound is not a gate count, circuit depth or elapsed-time bound.
- Quantum-accessible memory and QRAM assumptions must be stated. Cheap coherent
  lookup is not interchangeable with a linear scan of a classical table.
- A public computation can be evaluated coherently in principle. A public key
  does not automatically provide coherent access to a secret-key oracle.
- A polynomial reduction between two problems does not solve either problem.
- A non-tight security proof is not an attack attaining its lower bound.
- A speedup of an auxiliary algebraic problem is not a complete candidate attack
  unless the remaining steps and required inputs are available.

The existing [finding inventory](vulnerabilities.csv) supplies stable finding
IDs. Candidate evidence is in `../<id>/pseudocode.md` and
`../<id>/<id>-spec.pdf`; the [specification manifest](../data/specifications.csv)
identifies the archived PDFs. PDF page numbers below count physical PDF pages
unless explicitly described as printed page numbers.

## Principal structural cases

### NIIKE: subexponential quantum class-group methods

NIIKE exposes an abelian ideal-class-group action on oriented supersingular
curves. This is a setting for hidden-shift methods. Kuperberg's underlying
algorithm has time and query complexity of roughly
`exp(O(sqrt(log N)))` for a group of size `N`; a cryptanalytic application must
also account for class-group computations and action evaluation.
[Kuperberg][kuperberg] and [Wesolowski][orientations] establish the relevant
algorithmic context.

The [NIIKE specification](../kex-08/kex-08-spec.pdf), section 9.2, printed p.54
(PDF p.57), explicitly considers Kuperberg and selects discriminant magnitudes
around `2^4096`, `2^8192` and `2^16384`. Generic isogeny algorithms impose a
separate constraint through the field characteristic. Large discriminants alone
do not eliminate that route.

The supporting construction paper describes instances where Kuperberg loses to
generic supersingular path finding. Accordingly, the presence of a
subexponential quantum method is not a newly demonstrated parameter failure.
[Houben][large-discriminant]

Repairing NIIKE's existing KDF and key-generation defects leaves this structural
quantum consideration intact. A concrete assessment must compare both attack
families with their arithmetic, memory and circuit costs; inserting a
discriminant bit length into the asymptotic formula is insufficient.

### SQIsign variants and QIMEN-PIKE: exponential quantum improvements

The SQIsign specifications price general supersingular-isogeny recovery at
roughly `p^(1/4)` quantum time. Relevant sections are:

- [SQIsign2D2](../sign-25/sign-25-spec.pdf), section 9.2, PDF pp.110-112.
- [SQIsign2D-push1/2](../sign-26/sign-26-spec.pdf), sections 5.1 and 6.4.1,
  PDF pp.33 and 42.
- [SQIsignTriangle](../sign-27/sign-27-spec.pdf), section 5.3, PDF pp.43-44.

The classical comparison must now acknowledge the heuristic
`p^(1/3+o(1))` time-and-memory algorithm, with substantial unresolved concrete
overhead, rather than automatically repeating `p^(1/2)`.
[Wesolowski, 2026][isogeny-classical]

The reviewed public keys contain a curve, sometimes with deterministic
torsion-basis hints, rather than the secret endomorphism or isogeny images.
Thus no immediate application of the stronger oriented-isogeny algorithms was
established. This does not prove that useful auxiliary information cannot be
extracted from signatures. The distinction between oriented and merely
orientable curves is essential. [Orientations][orientations]

QIMEN's pairings do not supply an immediate Shor break. Discrete logarithms in a
chosen public basis do not identify the missing secret isogeny correspondence.
The [QIMEN specification](../kem-31/kem-31-spec.pdf), Table 8.2, PDF p.69,
already budgets quantum mask search with exponents **81/130/257** and quantum
claw finding for nonce-isogeny recovery. These remain exponential.
[PIKE][pike]

### TRINE: invariant collision search

The [TRINE specification](../sign-30/sign-30-spec.pdf), sections 6.1-6.2,
PDF pp.21-22, gives the following leading scales:

| Method | Scale |
|---|---|
| Classical invariant collision search | `q^((n-2)/2)` |
| Quantum collision search | `O-tilde(q^((n-2)/3))` |

Table 7 reports quantum estimates of **105, 180 and 348 bits**. The audit
checked the stated formulas but did not independently reconstruct all concrete
costs in that table. This remains exponential. TRINE's nonabelian
general-linear group action does not automatically inherit efficient algorithms
for abelian hidden shifts.

### CHAMP: quantum preimage improvement, no complete Shor break

CHAMP is the main algebraic hash candidate in this review. Matrix orders and
finite-field discrete logarithms can expose algebraic relations, but a hash
collision requires suitably short positive words in the two submitted
generators. Relations containing inverses or binary-encoded exponents may
expand to exponentially long literal messages. The determinant also largely
reveals input length, which may already be known.

The [specification](../hash-04/hash-04-spec.pdf), section 7, PDF p.5, discusses
the long-word obstacle. No complete polynomial-time quantum collision or
preimage attack was established.

There is a theoretical improvement to the known-length preimage search in
`hash-04-3`. Applying quantum claw finding to its existing balanced
meet-in-the-middle formulation gives:

| Known input length | Existing classical search | Quantum query estimate |
|---|---:|---:|
| 512 bits | `2^256` | approximately `2^170.7` |
| 1024 bits | `2^512` | approximately `2^341.3` |

This is the audit's theoretical application of [Tani's result][claw], not an
executed or independently published quantum attack. It requires coherent
evaluation and substantial quantum-accessible data structures. The original
preimage guarantees a match, but does not guarantee a distinct second preimage.
The existing projective positive-word lead `hash-04-2` remains a separate
heuristic classical lead.

### Polar-KEM: a classical break precedes the quantum question

The specified aligned public basis already exposes the isometry by classical
linear algebra (`kem-29-2`). A satisfactory repair must conceal the alignment
while preserving the complete scheme; no arbitrary basis change was assumed
to accomplish that.

For context, general lattice-isomorphism algorithms have bounds
`n^(n+o(n))` classically and `n^((2/3)n+o(n))` quantumly in the QRAM model.
Both remain superpolynomial. These are not validated security estimates for a
repaired Polar-KEM. [Aggarwal, Jiang, Li and Liu][lip]

## Quantum consequences of existing findings

### Long-message hash searches

The existing uHash and QSH findings describe multi-target searches whose
success density is approximately `T / 2^s`. Amplitude amplification suggests
search query cost around `2^((s-log2(T))/2)`, provided membership in the target
set can be tested coherently and efficiently. [Grover][grover]

| Variant | Existing classical search exponent | Quantum search exponent inferred here | Generic digest second-preimage exponent, quantum |
|---|---:|---:|---:|
| uHash-512 | 457.6 | 228.8 | 256 |
| uHash-768 | 713 | 356.5 | 384 |
| uHash-1024 | 968 | 484 | 512 |
| QSH-512 | 462 | 231 | 256 |
| QSH-1024 | 975 | 487.5 | 512 |

These are theoretical extensions of `hash-05-2` and `hash-24-2`, not complete
gate-cost estimates. They require target preprocessing of at least `O(T)`,
target/message I/O, and storage of eligible intermediate values. A linear table
scan per query would invalidate the displayed timing interpretation. QSH-768's
1024-bit internal chaining value leaves this route slower than generic search
against its 768-bit digest.

Specification anchors: [uHash](../hash-05/hash-05-spec.pdf), Algorithm 1,
Table 9 and section 3.2.2; [QSH](../hash-24/hash-24-spec.pdf), Algorithms 4-5,
Table 5 and the message-length limit on PDF p.33.

For AXIS-1024, the classical `2^768` state-splice argument in `hash-02-3`
instead suggests a quantum claw estimate near `2^512`. This matches generic
Grover search against its 1024-bit digest and does not establish an additional
quantum shortfall.

For the archived CHIME constants, the reduced images in `hash-26-1` suggest
quantum collision estimates near `2^42.7` and `2^149.3`, under appropriate
distribution and memory assumptions. These extend an existing classical
weakness. The report records an erratum changing the constants and removing
that invariant; the estimates are not evidence against the revised design.

### Signature challenge and prehash weaknesses

Existing challenge-search findings in Aigis-Sig+ (`sign-01-5`), CS
(`sign-07-2`) and SQIsignTriangle (`sign-27-3`) have generic quantum-search
counterparts. These remain exponential and are not new quantum discoveries.

The approximately `2^68.59` estimate for Aigis-Sig+ set I counts quantum
queries. It does not, by itself, establish a violation of an 80-bit gate-cost
target. The claim's metric and the reversible computation per query must be
accounted for before making that comparison.

For ideal `n`-bit unsalted prehashes admitting the already-reported signature
transfer, quantum collision search has a query scale near `2^(n/3)`, compared
with classical birthday search at `2^(n/2)`. Thus the fixed-prehash findings
`sign-02-1`, `sign-06-1`, `sign-08-1`, `sign-18-1`, `sign-22-1`, `sign-31-1`
and `sign-33-2` merit quantum accounting. The 512-bit and 256-bit cases give
approximately 170.7 and 85.3 query bits, respectively, with significant memory
and coherent-access qualifications. [Brassard, Hoyer and Tapp][bht]

### Entropy ceilings and cost metrics

Existing 256-bit seed or support ceilings in `kem-11-1`, `kem-12-1`,
`kem-17-2`, `kem-21-1`, `kem-27-1` and `kem-38-1` have generic quantum
counterparts around `2^128` predicate evaluations. Similar reasoning applies
to the already-documented KEX seed-entropy defects. These are consequences of
existing findings, not polynomial-time quantum breaks.

Some FO KEMs have a 128-bit message domain, suggesting about `2^64` search
evaluations. That alone does not refute an 80-bit gate-cost claim.
[Weaver](../kem-39/kem-39-spec.pdf), section 2.4, PDF p.16, explicitly uses
`2^64 * C_oracle` and identifies the required `C_oracle >= 2^16` estimate as
heuristic. [Loom](../kex-05/kex-05-spec.pdf), PDF pp.14-15, also distinguishes
oracle evaluations from the AES-based gate benchmark.

## Apparent polynomial shortcuts that did not establish attacks

1. **NTRU/RLWE versus principal ideals.** Quantum polynomial short-generator
   algorithms require an appropriate principal ideal and short-generator
   promise. An NTRU public ratio does not directly provide that input. Quantum
   approximate ideal-SVP results also do not automatically supply the accuracy
   needed for candidate decryption. No completing reduction was identified.
   [Short-generator analysis][principal-ideal]
2. **Facto-DSA.** Its factorization concerns finite-field polynomials, already
   classically tractable. The hidden cubic-map problem is not integer factoring.
   Its existing classical public-trapdoor finding is a separate issue.
3. **Chinith, Galas, GreatWall and Lynxer.** One public OWF input/output pair
   does not supply the secret-key evaluation oracle used by familiar Simon
   attacks. Offline Simon also needs suitable keyed data; ordinary signature
   queries do not automatically provide it. [Offline Simon][offline-simon]
4. **Code symmetry.** Quasi-cyclic or quasi-dyadic structure does not itself
   supply an efficiently usable hidden-period oracle. Nonabelian HSP reductions
   are not polynomial solutions. [Code equivalence][code-equivalence]
5. **Quantum samples.** Results assuming secret-dependent superpositions of
   noisy samples do not automatically apply to classical public keys. Preparing
   the required state is a missing premise, not a free operation.
6. **Weak proof bounds.** Conservative bounds in AFS-TrEDM, AXIS, Iphe,
   Litchi, Neulaser and Wish are not attacks attaining those bounds. AFS-TrEDM
   explicitly distinguishes its proven bounds from generic attack targets.

## Complete candidate coverage

The tables record the outcome of this screen, not security certifications.
"No additional route established" means no complete structural quantum
shortcut was established beyond the relevant generic algorithms.

For lattice candidates, quantum sieving/search improvements remain exponential;
their concrete costs depend on memory and model assumptions.
[Chailloux and Loyer][lattice-walk]
For code candidates, quantum decoding/search improvements also remain
exponential; one must not blindly halve every algebraic solver's exponent.
[Kachigar and Tillich][qisd]

### KEMs: 41 candidates

| ID | Candidate | Assessment |
|---|---|---|
| kem-01 | Aigis-Enc+ | Lattice speedups; no additional route established after separating rejection defects. |
| kem-02 | Amoeba | Cyclotomic RLWE; lattice speedups, with classical decoder/FO issues separate. |
| kem-03 | BAG-Loong | Rank-decoding/support search; restore secret random supports before assessing the repaired instance. |
| kem-04 | BAG-Piglet | Ideal rank decoding; auxiliary Gabidulin decoding supplies no complete polynomial quantum route. |
| kem-05 | BIKE-MLThre | Quantum QC-MDPC decoding; learned thresholds give no established extra shortcut. |
| kem-06 | BRA | Blockwise rank decoding; square-root estimates are model-dependent. |
| kem-07 | BRQC | Blockwise rank decoding; no established polynomial quantum reduction. |
| kem-08 | BW-KEM | MLWE lattice speedups; public Barnes-Wall message decoding does not solve the hard instance. |
| kem-09 | CheetahKEM | Lattice speedups; existing quotient lead changes classical and quantum dimensions alike. |
| kem-10 | C-Multi-UR-AG | Rank-support learning; multiple samples also require classical algebraic analysis. |
| kem-11 | COMPASS-KEM | MLWR lattice speedups; existing root-seed ceiling has a quantum-search counterpart. |
| kem-12 | CTL | NTRU/RLWR; no principal-ideal reduction established; existing seed ceiling is separate. |
| kem-13 | DKEM | MLWE; contributory/malicious-key failures do not establish Shor or Simon applicability. |
| kem-14 | DTRU | NTRU/RLWE; public E8 message encoding supplies no additional quantum route. |
| kem-15 | FLIT | NTRU/RLWE; repetition encoding supplies no identified hidden-subgroup oracle. |
| kem-16 | HARE | Quantum QC syndrome decoding/search. |
| kem-17 | HEP-QC | Quantum decoding; hidden code equivalence is not generically Shor-solvable. |
| kem-18 | LoongKEM | Lattice speedups; reducible-ring quotient lead affects both classical and quantum estimates. |
| kem-19 | Lore | MLWR lattice speedups; quotient lead is not polynomial quantum recovery. |
| kem-20 | MAMBA-Frost | Unstructured LWQ; public dithering does not provide secret-dependent quantum samples. |
| kem-21 | MAMBA-Viper | Module LWQ; existing seed ceiling is separate from quantization hardness. |
| kem-22 | Mithril | Radical-ring LWR; no principal-ideal or hidden-shift reduction established. |
| kem-23 | Mito | Quantum decoding; dyadic symmetry alone does not supply Simon's oracle. |
| kem-24 | MORNING-Scabbard | Saber-style MLWR; lattice speedups. |
| kem-25 | NEV | NTRU/RLWE; no additional route established. |
| kem-26 | NSS-HQC | Quantum QC syndrome decoding/search. |
| kem-27 | NTRE | Cyclotomic NTRU/RLWE; existing seed ceiling remains the clear generic search limit. |
| kem-28 | OAEP-NTRU | NTRU/RLWE; repair canonical parsing separately; no new quantum route established. |
| kem-29 | Polar-KEM | Already classically polynomial as specified; hypothetical repaired LIP discussed above. |
| kem-30 | PolarLAC | MLWE; public polar decoding does not recover the secret. |
| kem-31 | QIMEN-PIKE | Exponential quantum mask search/claw finding; no direct Shor break from pairings. |
| kem-32 | QCTM | Quantum decoding; nonabelian code-equivalence HSP is not an efficient solver. |
| kem-33 | QUBE | Quantum multi-block QC syndrome decoding/search. |
| kem-34 | Rudraksh2 | MLWE lattice speedups; no additional route established. |
| kem-35 | Scloud+ | Unstructured LWE; cited ideal-SVP quantum results concern a different problem. |
| kem-36 | TRIKE | QC syndrome decoding; ring divisions are not a discrete-log assumption. |
| kem-37 | TriQ-KEM | Quantum QC syndrome decoding; specification includes quantum-Prange analysis. |
| kem-38 | UVW | Quantum decoding/search; existing support-entropy ceiling has a quantum counterpart. |
| kem-39 | Weaver | MLWR; explicitly distinguishes message-search queries from gate cost. |
| kem-40 | YuanYang.KEM | NTRU/RLWE after repairing omitted error; shared-key composition needs separate review. |
| kem-41 | ZEN | NTRU/RLWE; zero-divisor encoding supplies no identified hidden-subgroup oracle. |

### Signatures: 34 candidates

| ID | Candidate | Assessment |
|---|---|---|
| sign-01 | Aigis-Sig+ | Lattice speedups and existing challenge-search weakness; query/gate qualification above. |
| sign-02 | BiT | Lattice speedups and existing prehash collision ceiling; sampler leakage is classical. |
| sign-03 | CEDRUS+C | Generic hash search; index-collapse implementation defect is separate. |
| sign-04 | CEDRUS-alpha | Generic hash search; truncated-root implementation defect is separate. |
| sign-05 | Chinith | Generic OWF/challenge search; no matching Simon keyed oracle established. |
| sign-06 | COMPASS-SIG | Lattice speedups plus existing seed/prehash ceilings. |
| sign-07 | CS | Lattice speedups plus quantum counterpart of existing challenge-search finding. |
| sign-08 | DARTS | Lattice speedups and prehash ceiling; assess repaired rejection sampler separately. |
| sign-09 | DOVE | Quantum MQ/hybrid search; no polynomial solver established for the structured relation. |
| sign-10 | Facto-DSA | Hidden cubic map, not integer factoring; satisfactory small repair of classical break unestablished. |
| sign-11 | FlexTree | Generic hash search/collision effects; no polynomial structural shortcut established. |
| sign-12 | Galas | Single public OWF pair; no identified Simon oracle. |
| sign-13 | GreatWall | Single-target OWF search; no applicable polynomial quantum shortcut established. |
| sign-14 | Lynxer | Generic OWF/algebraic search; repaired proof constraints require renewed validation. |
| sign-15 | MORNING-ATLAS | Lattice speedups; finite-support/repeated-mask classical defects are separate. |
| sign-16 | Octarine | Radical-ring LWR/SIS; no principal-ideal reduction established; proof gap is not an attack. |
| sign-17 | OPS | Module-lattice speedups; no additional route established. |
| sign-18 | Origami | Quantum MQ/hybrid search; adequate minor repair of classical structural failures unestablished. |
| sign-19 | Phoenix | Generic hash/multitarget search; concrete hash instantiation incompletely pinned. |
| sign-20 | Qing Luan | Quantum restricted decoding; tiny multiplicative subgroup is not a large-DLP assumption. |
| sign-21 | ReSolveD-alpha | Quantum regular decoding/challenge search; no polynomial route established. |
| sign-22 | Rhyme | Lattice speedups with partial Gram information; no vulnerable rank-one ideal reduction established. |
| sign-23 | Shuttle | Lattice speedups after repairing classical sampler defect. |
| sign-24 | Sigurd | Quantum decoding/challenge search; chunked-code implementation leakage is classical. |
| sign-25 | SQIsign2D2 | Exponential quantum isogeny algorithms; public basis hints do not directly reveal secret orientation. |
| sign-26 | SQIsign2D-push1/2 | Exponential quantum isogeny algorithms; no polynomial route established. |
| sign-27 | SQIsignTriangle | Isogeny algorithms plus existing challenge-search weakness; classical defects dominate submitted version. |
| sign-28 | SYDO | Quantum regular decoding/challenge search; no polynomial route established. |
| sign-29 | Tins | NSBC polynomial system is not itself a discrete logarithm; existing classical leakage is separate. |
| sign-30 | TRINE | Explicit invariant collision quantum improvement; remains exponential. |
| sign-31 | TSUOV | Quantum MQ/hybrid search plus existing prehash collision ceiling. |
| sign-32 | UVW signature | Quantum decoding/search after repairing verifier; no polynomial route established. |
| sign-33 | VDOO | Quantum MQ/MinRank search; no complete polynomial route for dense specified oil layers. |
| sign-34 | YuanYang.DSA | NTRU lattice speedups; short-principal-generator results do not directly apply. |

### Hashes: 35 candidates

The generic ideal-function benchmarks for an `n`-bit digest are classical
collision/preimage costs `2^(n/2)`/`2^n` and quantum query costs near
`2^(n/3)`/`2^(n/2)`. These are not proofs that a particular construction attains
those bounds. XOF capacity and the selected security level also matter.

| ID | Candidate | Assessment beyond generic benchmarks |
|---|---|---|
| hash-01 | AFS-TrEDM | No additional route established; weak proven bounds explicitly distinguished from attacks. |
| hash-02 | AXIS | Quantum splice estimate matches generic AXIS-1024 Grover cost. |
| hash-03 | C Hash | No Simon promise identified in counter-separated and short-input paths. |
| hash-04 | CHAMP | Quantum claw improvement above; no complete polynomial positive-word attack. |
| hash-05 | uHash | Long-target quantum search extrapolation, with preprocessing and memory qualifications. |
| hash-06 | Cuishen | Counter binding and finalization prevent direct reuse of uHash's position-free argument. |
| hash-07 | Dragon | No full-round quantum shortcut established from the reviewed differential analysis. |
| hash-08 | Duet | No hidden XOR period established for the dual-permutation/feed-forward construction. |
| hash-09 | Eijen | No additional route established after separating fixable classical implementation collisions. |
| hash-10 | FEILIAN | No additional route established after separating the fixable RTL collision. |
| hash-11 | Garnet | Plain-sponge and full-state-DM variants reviewed separately; no extra route established. |
| hash-12 | Iphe | Conservative quarter-digest collision proof bound is not an attack. |
| hash-13 | JuziHash | Long-message quantum extension remains unvalidated; cannot simply halve the full classical exponent. |
| hash-14 | Laurus | No applicable Simon periodic function identified; specification discusses the issue. |
| hash-15 | Litchi | Parallel counter squeezing gives no identified hidden shift; proof bound is not an attack. |
| hash-16 | LLH | Length-bound IV/feed-forward/final permutation reviewed; no extra route established. |
| hash-17 | MasterCube | Quantum proof-review lead; no demonstrated quantum break of the complete hash. |
| hash-18 | Megascon | No extra route established for intended scalar design; AVX-512 defects are classical. |
| hash-19 | MoFang | Existing deterministic classical collisions; hypothetical repaired design requires re-review. |
| hash-20 | Mozi | No extra route established; cross-instance relations are already classical. |
| hash-21 | Neulaser | Existing state mergers are classical; no quantum-polynomial shortcut established. |
| hash-22 | Pavelor | Reduced-round quantum rebound results do not automatically reach the submitted full construction. |
| hash-23 | QILIN | Specification's three-round quantum result does not cover full 12/14/16-round variants. |
| hash-24 | QSH | Long-target quantum search extrapolation; free-start findings do not prove a fixed-IV polynomial break. |
| hash-25 | TaiChi | Conditional quantum estimates; inverse-rate route matches generic cost where applicable. |
| hash-26 | CHIME | Reduced-image quantum estimates concern archived constants; erratum removes that invariant. |
| hash-27 | Vedak | No additional route established for the full 40-round construction. |
| hash-28 | XRH-1 | EDM/feed-forward quantum assumptions need separate treatment; no extra route established. |
| hash-29 | XRH-2 | No Simon promise identified in full-state DM absorption. |
| hash-30 | ZC-DM | Quantum estimates assume idealized absorption and absence of structural shortcuts. |
| hash-31 | ZC-DMC | No demonstrated quantum bypass of extra output constraints. |
| hash-32 | ZC-EDMC | Specified midpoint feed-forward distinguished from existing implementation divergence. |
| hash-33 | Thunder | No full 12-round collision below the generic quantum scale established by reviewed analysis. |
| hash-34 | WChain | State feedback/checksum does not itself yield a hidden period; numerical preimage claim unclear. |
| hash-35 | Wish | Conservative quarter-digest collision proof bound is explicitly non-tight. |

MasterCube's collapsing and ideal-permutation arguments deserve a separate
formal proof review. The audit did not establish an attack from those gaps.
More generally, results for the ordinary sponge must not automatically be
extended to every feed-forward or dual-permutation variant.

### Key exchange: 9 candidates

| ID | Candidate | Assessment |
|---|---|---|
| kex-01 | ADKEX | MLWE DKEM composition; lattice speedups, no additional route established. |
| kex-02 | AFS-KEX | MLWE/Barnes-Wall construction; public message decoding does not solve the lattice instance. |
| kex-03 | CreTAKE | Reviewed credential frameworks use PolarLAC, ZEN and BiT; no hidden classical-DH assumption. Existing entropy defect is separately repairable. |
| kex-04 | DKEX | MLWE DKE and ML-DSA authentication; lattice speedups, no extra route established. |
| kex-05 | Loom | Shifted module-LWR plus Shuttle; quantum search accounting explicitly separates queries and gates. |
| kex-06 | MAMBA-NIKE | Ring LWQ; public dithering does not supply secret-dependent quantum samples. Existing seed ceiling is separate. |
| kex-07 | NEV-AKE | NTRU/RLWE composition; no principal-ideal or hidden-period shortcut established. |
| kex-08 | NIIKE | Subexponential oriented class-group route already considered by the specification; see above. |
| kex-09 | TriQ-KEX | Code-based KEM composition; quantum decoding/search, no additional route established. |

## Assessment and remaining research priorities

The strongest structural priorities are NIIKE's concrete quantum accounting,
the auxiliary public information in the isogeny schemes, and CHAMP's
short-positive-word problem. TRINE's quantum cost table and the quantum
consequences of existing challenge/prehash findings deserve explicit cost-model
annotations.

Across the remaining constructions, the supported quantum advantages were
search, collision, decoding, algebraic-search or lattice-reduction improvements
that remain exponential. No new quantum-polynomial vulnerability is established
by this audit. The theoretical extensions above need their stated assumptions
preserved if used in later reports.

## Primary references

- [Kuperberg: A subexponential-time quantum algorithm for the dihedral hidden subgroup problem][kuperberg].
- [Wesolowski: Orientations and the supersingular endomorphism ring problem][orientations].
- [Houben: Efficient post-quantum commutative group actions from orientations of large discriminant][large-discriminant].
- [Wesolowski: The supersingular isogeny problem in time and memory p^(1/3+o(1))][isogeny-classical].
- [Cai, Chen, Lai and Lin: PIKE][pike].
- [Tani: Claw Finding Algorithms Using Quantum Walk][claw].
- [Aggarwal, Jiang, Li and Liu: An n^(n+o(n))-Time Algorithm for the Lattice Isomorphism Problem][lip].
- [Grover: A fast quantum mechanical algorithm for database search][grover].
- [Brassard, Hoyer and Tapp: Quantum Algorithm for the Collision Problem][bht].
- [Cramer, Ducas, Peikert and Regev: Recovering Short Generators of Principal Ideals in Cyclotomic Rings][principal-ideal].
- [Bonnetain, Hosoyamada, Naya-Plasencia, Sasaki and Schrottenloher: Quantum Attacks without Superposition Queries][offline-simon].
- [Dinh, Moore and Russell: Quantum Fourier sampling, Code Equivalence, and the quantum security of the McEliece and Sidelnikov cryptosystems][code-equivalence].
- [Chailloux and Loyer: Lattice sieving via quantum random walks][lattice-walk].
- [Kachigar and Tillich: Quantum Information Set Decoding Algorithms][qisd].

[kuperberg]: https://arxiv.org/abs/quant-ph/0302112
[orientations]: https://eprint.iacr.org/2021/1583
[large-discriminant]: https://eprint.iacr.org/2025/1098
[isogeny-classical]: https://eprint.iacr.org/2026/1486
[pike]: https://eprint.iacr.org/2026/473
[claw]: https://arxiv.org/abs/0708.2584
[lip]: https://eprint.iacr.org/2026/1406
[grover]: https://arxiv.org/abs/quant-ph/9605043
[bht]: https://arxiv.org/abs/quant-ph/9705002
[principal-ideal]: https://eprint.iacr.org/2015/313
[offline-simon]: https://eprint.iacr.org/2019/614
[code-equivalence]: https://arxiv.org/abs/1111.4382
[lattice-walk]: https://arxiv.org/abs/2105.05608
[qisd]: https://arxiv.org/abs/1703.00263
