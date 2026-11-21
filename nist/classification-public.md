# NIST additional-signature report classification

This policy is specific to the NIST additional digital signature process. It
uses the required EUF-CMA model, claimed security categories, and evaluation
criteria on the official [NIST call page](https://csrc.nist.gov/Projects/pqc-dig-sig/standardization/call-for-proposals)
and in the [call PDF](https://csrc.nist.gov/csrc/media/Projects/pqc-dig-sig/documents/call-for-proposals-dig-sig-sept-2022.pdf).
The call permits security estimates to assume at most `2^64` chosen-message
signing queries. Ratings here are independent project assessments, not ratings issued by NIST.
NGCC findings may supply prior evidence, but their ratings do not transfer
automatically.

Each numbered report carries a `Credit` attribution and independent
`Severity`, `Status`, `Layer`, `Discovery`, `Exploitation`, and `Remediation`
fields. These use the same
classification axes as the NGCC study, but severity is assessed against the
NIST additional-signature call and the submission's own claims. An earlier
NGCC rating is evidence, not a rating that automatically transfers here.

Ratings describe cryptanalytic impact on the **affected scheme and parameters**:

| Rating | Interpretation |
|---|---|
| Critical | Full key recovery or general forgery invalidates the intended security target of a recommended parameter set; state query/work requirements and whether demonstrated or estimated. |
| High | A serious failure short of a required-property break: for example, a well-supported recovery/forgery path with one material gap, an end-to-end side-channel recovery without an explicit resistance claim, failure of a claimed SUF-CMA property when EUF-CMA remains intact, or a demonstrated break confined to non-recommended alternative parameter sets. |
| Medium | Confirmed repeated or measured leakage without recovery, a material constant-time rewrite, a demonstrated failure of an unclaimed stronger security property such as SUF-CMA, or a substantive binding/robustness defect whose attack consequence is unresolved. |
| Low | Source-level secret dependence without a measured channel when a local repair is known, or another narrow security-relevant robustness/API weakness. |
| Informational | Historical context, a withdrawn claim, a loss generic to a proof technique, or an observation without an established attack and with no candidate-specific security consequence on the evaluated submission version. |

A Critical rating does not mean every parameter can be attacked cheaply.
The result, concrete costs, scope and confidence must accompany the rating.
These are project assessments under the NIST evaluation baseline, not CVSS
scores or NIST-issued ratings. Strong-unforgeability, side-channel or fault
models are distinguished from the required EUF-CMA model.

Critical is reserved for a break of a parameter set that the submission
recommends. A demonstrated key recovery or forgery that is confined to
non-recommended alternative parameter sets listed in the specification is
High: it is a serious failure of the submitted design, but the required
property is not shown to fail for the parameter sets the submission proposes.
The report title and `Affected` field must state that scope. Estimates, leads
and proof gaps that concern only alternative sets follow the ordinary rules.

A candidate-specific proof gap is normally Low when a reproduced calculation
shows that the submitted reduction becomes ineffective within NIST's stated
evaluation budget, but no attack follows. Generic looseness without a concrete
boundary is Informational. A limitation that the submission itself identifies
as a proof artifact remains Informational only when it is generic looseness
without a reproduced concrete boundary. A candidate-specific calculation that
makes the submitted reduction ineffective within NIST's stated evaluation
budget is Low even when the submission acknowledges the loss and no attack
follows. High requires a supported recovery or forgery path with a material
remaining gap; failure of a proof alone is not High.

A live, candidate-specific security-relevant defect is normally Low or above,
following the NGCC classification policy, when its consequence is
demonstrated in a submitted or supported build or through the submitted API.
In particular, an ignored randomness, allocation or primitive failure, a
crash or hang reachable through the submitted API, a canonicality or
key-validation gap, and a specification/code mismatch that breaks
interoperability are Low when that consequence is shown and no stronger one
is. A behaviour shown to have no effect, a condition of negligible
probability for honest use, or a property that needs inputs an honest party
never produces stays Informational until such a consequence is demonstrated.
A candidate-specific false premise or invalid proof step is Low / Proof gap
even without a numeric boundary; the absence of a formal proof for a claimed
property is not by itself Low. Informational remains for losses generic to a
proof technique, historical or withdrawn records, and the misuse cases rated
Informational by the randomness-reuse rule below.

The `2^64` chosen-message figure is an estimation assumption, not a hard
boundary on relevant attacks. NIST's call §4.B.2 (physical p.14) says that
attacks using more messages may also be considered and that NIST is primarily
concerned with classical, rather than quantum, queries to the signing oracle.
Reports therefore state both the query count and query model; crossing `2^64`
does not by itself make an attack irrelevant, and a quantum-query reduction is
not presented as the call's primary signing-oracle model.

A concrete security-margin calculation may be a Low / Lead without a
demonstrated attack when it is candidate-specific, reproducible from a primary
model, and leaves less than two bits of margin under that model while a
documented unresolved modeling choice could erase the margin. The report must
identify the omitted cost, memory assumption, or heuristic and must not present
the calculation as an attack. A model-dependent observation without such a
concrete boundary is Informational.

`Status` describes evidence strength, independently of severity:

- **Confirmed:** source/specification reasoning, a mathematical certificate,
  or a controlled reproducible experiment directly establishes the exact
  reported behavior. This does not imply end-to-end exploitation.
- **Probable:** the mechanism and consequence are strongly supported, but an
  important source, platform, or experimental step remains unchecked.
- **Lead:** a plausible candidate-specific attack still depends on a material
  unvalidated assumption or experiment.
- **Proof gap:** a claimed theorem, reduction, or bound is insufficient, but
  an attack of the claimed strength has not been established.
- **Withdrawn:** the stable report is false, duplicate, out of scope, or
  superseded. Withdrawn reports have Informational severity. A withdrawn
  *candidate* or claim is recorded separately from report status.

`Layer` identifies the root cause:

- **Design:** the normative construction, parameter choice, proof, or
  advertised portable performance profile contains the weakness.
- **Implementation:** submitted code introduces an API, parser,
  memory-safety, arithmetic, or randomness defect not required by the design.
- **Side-channel:** the finding depends on a timing, branch, cache, division,
  power, fault, or other execution trace and does not meet the Design rule.
- **Evaluation:** the issue is confined to KAT, archive, build, provenance, or
  other evaluation material.

`Discovery` records the reasoning needed to identify and substantiate the
mechanism, rather than severity or attack runtime: **Trivial** for direct local
inspection; **Moderate** for focused review or a standard attack pattern;
**Non-trivial** for a multi-step candidate-specific argument; and **Hard** for
substantial new cryptanalytic insight or engineering.

`Remediation` may be **None** for an accurate historical or withdrawn-claim record that requires no change to the evaluated submission.

`Exploitation` is a short factual boundary such as `source-only`,
`measured-channel`, `candidate-specific path`, or `end-to-end`. It states the
strongest completed step and the observation model. `Remediation` is
**None**, **Local**, **Moderate**, or **Architectural**, describing respectively
no change for an accurate historical record; a small constant-time/API repair;
a material implementation replacement or proof/parameter reassessment that
preserves the construction and API; or a change to the construction, API, or
advertised performance profile.

## Constant-time classification

Constant-time surveys distinguish public control flow from operations on a
secret or secret-derived operand. A report answers, in order: what is secret;
who can observe the trace; whether a channel was measured and repeated;
whether observations have a candidate-specific map to a key or forgery; and
whether that map was executed end to end. Source-level review alone is not a
timing measurement or a constant-time certification.

| Evidence and consequence | Usual severity |
|---|---|
| Secret branch/address in source, no measured channel, local repair | Low |
| Repeated secret leakage, measured channel, or material rewrite; no recovery | Medium |
| Fine-grained leakage intrinsic to an advertised fast path with no supplied or known competitive portable constant-time realization | High may be justified |
| Stable oracle and candidate-specific recovery path with one channel or scale gap | High |
| End-to-end recovery/forgery without an explicit resistance claim | High |
| End-to-end recovery/forgery contradicting an explicit relevant resistance claim | Critical |

Remediation does not mechanically set severity. Local normally supports Low;
a Moderate rewrite supports Medium only when the report independently
establishes repeated finer-grained leakage and explains why the affected trace
must be hidden. Architectural remediation may support High for an intrinsic
advertised fast path. A costly repair alone does not raise source-only secret
dependence above Low, particularly when the specification treats the observed
predicate as public or simulatable. Public-indexed tables, branches on public
lengths, and verifier rejection paths are not secret leaks. The absence of a
hardened implementation by itself does not prove a design defect.

## Entropy and malleability classification

The survey traces key-generation and signing entropy from the NIST
`randombytes` boundary to all scheme-specific expanders and salts. Seed replay
in KAT testing is a control, not an attacker capability. Deterministic
signing or a signer that deliberately ignores fresh external coins is an
observation until the specification, proof, nonce-reuse behavior, or a
demonstrated attack makes it security relevant. Entropy estimates count
reachable states under the scheme's actual derivation and domain separation
rather than just buffer widths.

NIST's call §4.B.4 (physical p.18) identifies resistance to isolated coding
errors, random-number-generator malfunction, and nonce reuse as a desirable
property, rather than a minimum acceptability requirement. This project rates
a break that requires one exact repeat of the external signing-RNG output
Informational when the submission makes no explicit hedged-signing or
randomness-reuse-resistance claim. The report still records a demonstrated key
recovery or fresh-message forgery and its precise operational consequence.
Low or a stronger rating requires an additional candidate-specific trigger in
the claimed operating model, a contradiction of an explicit robustness claim,
or a break that does not require RNG failure. FAEST cross-profile reuse remains
Low because its trigger includes the specified deterministic-signing mode and
missing profile separation, rather than only an external RNG malfunction.

Malleability tests distinguish (1) a modified signed message that verifies to
the same message, which is relevant to SUF-CMA or canonical encoding, from (2)
a modification that verifies to a different message, which is a direct
EUF-CMA concern. Sampled mutation, truncation, extension, zero-input, and
wrong-key tests provide negative evidence only for the tested cases. A
demonstrated efficient transformation of a valid signature into a distinct
valid signature on the same message is normally Medium even when the
submission does not claim SUF-CMA: it is an end-to-end break of a commonly
used additional signature property, while leaving NIST's required EUF-CMA
target intact. Failure of an explicit SUF-CMA claim is normally High. An EUF-CMA
forgery is Critical when it defeats the claimed category of a recommended
parameter set, and High when it is confined to alternative sets.
