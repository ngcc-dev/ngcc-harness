# Fixing-cost analysis

This document defines how NGCC estimates what it would cost to repair a
candidate's Critical findings. The estimate is a subjective assessment. It is
made from the specification, the submitted source, proposed fixes and
parameter estimates. No fix is implemented and nothing is measured.

Every candidate directory gets one note, `fixing_cost.md`. The note is not a
finding. It has no ID and no severity, it does not change any classification
in `report.md`, and it does not say whether a repaired candidate would be
secure or competitive.

## The question

> Is repairing the candidate's Critical findings likely to make the candidate
> at least 10% larger or slower?

"Likely" means more likely than not in the assessor's judgment. The answer is
`Yes` or `No`. Two primary estimates stand behind it and are always reported:

- **Impact:** the estimated change, in percent, in bandwidth and speed.
- **Scope:** how much of the submission has to change, on the fixed scale
  below, from a single line to the whole construction.

The two are independent, as severity and remediation burden are in
[`REPORT_CLASSIFICATION.md`](https://ngcc.dev/report-classification.html). A one-line fix can
double the signing time, and a replaced sub-algorithm can leave every size and
operation count as it was. Impact alone decides the answer, whether the repair
changes the design, the parameters or only the code. Scope is reported beside
it and shows which of these the repair is.

## Findings considered

- Every live finding with `Severity: Critical` is costed, whatever its `Layer`
  (Design, Implementation or Side-channel) and `Status`. Withdrawn findings
  are excluded.
- Confirmed and Probable findings decide the answer and the `Impact` and
  `Scope` lines. Each is taken to hold as reported. When the answer rests on a
  Probable finding alone, the answer sentence says so.
- A Lead finding depends on a step that is not validated, so its repair is not
  yet likely to be needed. It is costed like any other finding, but it does
  not decide the answer. The `With leads` line gives the answer, impact and
  scope that would apply if every Lead held.
- Only findings filed in the candidate's own `report.md` are considered. A
  composite candidate does not inherit findings filed under the candidate ID
  of one of its components.
- Findings below Critical are not costed and do not affect the answer.
- A candidate without a live Critical finding gets the short form of the note
  (see The note).

## Baseline

The baseline is the Round 1 submission as it was submitted and advertised:

- **Sizes** are those that the specification states for each parameter set.
  Cite the table and the physical PDF page.
- **Speed** is the work that the submitted code does, because the stated
  timings were taken from that code. Stated timings are context and are not
  required. Where a specification gives no timings, no breakdown, or only the
  timings of vector code, the operation counts from the specification and the
  source are the whole evidence, and the note says so.

The repaired candidate is one whose specification and code are both correct
and agree with each other. A difference between the two at submission is
therefore handled the same way in both directions:

- Where the code skips work that the specification requires, the corrected
  code does more work than the baseline. That is impact (see Speed).
- Where the code already contains a repair that the specification lacks, the
  repair is a specification change. It adds no work, and it adds bytes only
  when the stated sizes do not already include them.
- Where the sizes of the built library differ from the stated sizes for any
  other reason, record both values and compare with the stated ones.

What is compared:

- **Bandwidth** is every object that is sent or published: the public key,
  ciphertext and signature, and for a key exchange each protocol message and
  long-term public key. Hash candidates have no bandwidth, because the call
  fixes the digest lengths.
- **Speed** is each main operation taken separately: key generation,
  encapsulation and decapsulation; signing and verification; each party's
  steps in a key exchange; and for a hash, the cost per byte of a long
  message.
- The cost of hashing a short message counts only when the repair adds work to
  every call whatever the message length, such as an extra finalization call.
  It is then compared on a message that fits one block of the submitted
  design. An extra block that is needed only at particular lengths, as when
  padding is corrected, is `none`.
- A finding that exists only in a hardware (RTL) implementation is costed on
  that implementation, by cycles per block.
- Secret-key size, memory, code size and hardware area are recorded when a
  repair changes them noticeably. They do not decide the answer.

Every percentage compares one object or operation of one parameter set with
its own baseline value. Only increases count, and a saving elsewhere does not
offset them.

## Significance

The answer is `Yes` when the combined impact of the deciding repairs reaches
10% in at least one object or operation of at least one submitted parameter
set or named variant. Otherwise it is `No`.

- Combine the repairs before comparing with 10%. Added bytes add, added
  operations add, and expected repetitions multiply. When one change repairs
  several findings, count it once.
- When a repair can trade one metric against another, such as signature size
  against signing time, the answer is `No` only if one consistent choice keeps
  every metric under 10%.
- An impact that cannot be bounded counts as 10% or more.
- When a two-sided estimate has a range that straddles 10%, answer for the
  central estimate and give the range. A zero-margin parameter estimate is a
  one-sided lower bound and has its own rule (see Parameter estimates).
- Withdrawing a parameter set at a required level (128, 256 or 512 bits) is
  not a repair. For the optional 384-bit level or an optional variant,
  withdrawal may be mentioned as an alternative, but the note still costs the
  repair and answers from it.

## The repair that is costed

Cost the cheapest credible repair of each finding. A repair is credible when
all of the following hold:

1. It removes the root cause that the report states, in every affected
   parameter set. Blocking the published witness is not a repair.
2. It keeps what the call requires and what the specification claims: the
   security levels, the security notion, 2^64 signatures per key, the message
   lengths, and a KEM key at least as long as the level (see Security targets
   in `REPORT_CLASSIFICATION.md`).
3. It keeps correctness: the failure probability stays within the
   specification's own bound.
4. It is a standard technique, or a fix that the submitters or the reporter
   proposed. A countermeasure newly invented by the assessor is not a basis
   for a `No` answer.
5. It stays as close to the submitted design as the root cause allows.

The note costs a repair and does not evaluate one. It must not say that a
repair is sufficient, complete or secure. Parameter values computed for an
estimate are costing aids, not recommended parameters.

### Proposed fixes

Proposed fixes are evidence, and they are read first:

- Use the `Proposed fixes` subsections of the report and any later forum post,
  revised specification or revised code. A proposal that the report does not
  record yet is added there first, under the rules in
  `REPORT_CLASSIFICATION.md`. Revised packages are untrusted downloads. They
  are read, and they are not built, run or measured.
- A proposed fix is the first candidate for the costed repair. Its stated
  parameters, sizes and timings may be used for Impact, and its diff for
  Scope. Quote them as reported by their source and not evaluated.
- Do not rely on a proposal that leaves the stated root cause in place, or
  against which the report records an attack. Say why, and cost another
  repair.
- When a credible repair is cheaper than the proposal, answer from the cheaper
  one and state the proposal's cost beside it.

### Usage limits

Some attacks need many uses of one key, such as multi-instance decoding over
many ciphertexts. A stated limit on the use of a key is a credible repair only
when the classification rules would then record no finding (see the
query-budget rules under Security targets in `REPORT_CLASSIFICATION.md`):

- The limit is at least 2^64 uses per key. The call requires 2^64 messages
  per signing key and sets no per-key cap for KEMs, and the classification
  rules accept a stated limit of at least 2^64 uses.
- Within the limit, the attack of the finding stays at or above the target
  under the finding's own model.
- The submission keeps its security notion and its use with static keys.
  Restricting a KEM to ephemeral keys is not a repair.

Such a repair changes one statement of the specification. Its scope is Single
line, specification, and its impact is `none`. The note states the limit and
the attack cost at the limit. It also gives, as an alternative, the cost of
parameters that would withstand the attack at the evaluation ceiling of 2^80.

## Scope of remediation

Scope is the largest unit of the specification or code that must change:

| Scope | What changes | Examples |
|---|---|---|
| Single line | One constant, length, argument, comparison, statement or missing call | Compare every ciphertext byte; seed the generator; bits where bytes were meant; state a key-use limit |
| Function | One routine, or one step of a specified algorithm | Hash the received ciphertext on rejection; reject non-canonical encodings; 64-bit length arithmetic; add an index or tweak to a hash call |
| Sub-algorithm | One component is rewritten or replaced | Sampler; encoder or decoder; hash or XOF; challenge derivation; rejection step; padding rule |
| Parameter set | The algorithms stay and the parameter tables change | Larger dimension, rank, weight or modulus, with the sizes and the failure and security analysis redone |
| Construction | The core mechanism, transform or hardness assumption changes | A different trapdoor; a different compression function; a new proof |
| None known | No repair that keeps the design is known | |

Single line and Function are **small**. The other scopes are **large**.

- State where the change falls: `code` when the specification is sound and
  stays as it is, `specification` when only the specification changes, and
  otherwise `specification and code`. A `Layer: Implementation` finding still
  needs a specification change when the specification is silent or wrong on
  the point.
- Scope counts what must change in the submission. It does not count the
  effort of finding the repair, and it does not count regenerated test
  vectors, which every repair needs.
- Say separately when a repair changes the encoding of a public key,
  ciphertext or signature, for example by adding a salt. That does not raise
  the scope by itself.
- For a side-channel finding, start from the remediation burden defined in
  `REPORT_CLASSIFICATION.md`: Local is usually Function at most, Moderate is
  Sub-algorithm, and Architectural is Construction.
- The candidate's scope is the largest scope among its deciding findings.

## Impact

State the impact of each repair on each object and operation that it changes:

- `none`: no transmitted object changes, and the operation counts change by
  less than 1%.
- A percentage or a range for each affected object or operation, with the
  parameter set and the basis, such as `signature +12% at 512 (count)` or
  `signing +20% to +40% at 128 (estimate)`.
- `not bounded`: the repair is a Construction change with no costed proposal,
  or no repair is known.

The basis of every figure is one of:

- **count:** exact arithmetic from the specification's formulas, such as the
  bytes added to a signature;
- **estimate:** the output of a stated size or cost model;
- **judgment:** reasoning from the structure of the scheme, where no
  calculation is possible.

Give counts exactly. Give estimates to the precision the model has, which is
usually a few percent or a range. The candidate's impact is the combined
change in each object and operation, each given for the parameter set where
it is largest.

### Bandwidth

- Use the specification's own size formulas. First reproduce the submitted
  size table from the submitted parameters, as a control. Then evaluate the
  formulas at the repaired values.
- Trace a widened value through the scheme. Every transmitted object that
  carries or encodes it grows: a salt, commitment or challenge in a signature,
  a seed in a public key, a message encoded in a ciphertext. A value that is
  only hashed or kept secret adds no bytes.
- A KEM key cannot hold more entropy than the encapsulated message and coins.
  When the finding is a short key or a short seed, check the width of the
  plaintext and not only the length of the output.

### Speed

Nothing is measured, so speed is compared by counting work in the
specification and the source:

- Count the operations that dominate: hash or permutation calls and bytes
  hashed; polynomial, matrix and field operations under a stated cost model
  (for example n log n, n^2 or n^3); expected iterations of a rejection loop,
  from the specification's acceptance probability; parallel repetitions; tree
  nodes; rounds.
- Report the ratio of repaired to submitted work for the same operation.
  Timings in the specification show which part dominates. Do not turn them
  into predicted timings.
- A change confined to a part that the counts show to be a small share of the
  operation is `none`.
- Count the portable algorithm, not one platform's vector code, unless the
  finding is specific to an optimized implementation.
- Where the submission uses the contest's `pseudohash` or `pseudoXOF`, count
  calls and bytes, and assume an ideal replacement with the needed dimensions.
- When a wider hash is needed and the submission names none, cost it by its
  dimensions. At a fixed permutation, a sponge's time per byte is inversely
  proportional to its rate. An iterated hash's time per byte is inversely
  proportional to the message bytes absorbed per compression call.
- If the submitted code skips work that the specification requires, such as
  grinding, rounds or challenge weight, the corrected code does more work than
  the baseline. Count that work as impact, even though only the code changes.

### Parameter estimates

When the repair is a new parameter set:

1. Use the cost model of the finding with the finding's own conventions, and
   the report's witness or audit script where one exists. The target is the
   level of the affected parameter set.
2. Find the smallest change that brings the finding's attack to the target,
   with no margin, inside the construction's own constraints: the parameter
   shapes that the specification allows and its failure-probability bound. If
   the only free parameter moves in coarse steps, such as a power-of-two ring
   dimension, the cost is one step.
3. Where the report supplies a model for another attack on the candidate,
   recompute it at the new parameters. A repair must not reopen a recorded
   attack.
4. Compute sizes from the specification's formulas, and work from the stated
   cost model.

The result is a one-sided lower bound, because designers add margin and
another attack may bind first. An estimate of 10% or more therefore supports
`Yes`, and an estimate under 5% supports `No`. Between 5% and 10% the case is
borderline: answer by judgment and say what would tip it.

When no model can be evaluated, use the relative deficit: the target minus the
attack cost, in bits, divided by the target. State the scaling law assumed,
for example that the attack exponent grows linearly in the dimension and the
sizes linearly or quadratically, and where the specification or the attack
shows it. Label the result `judgment`.

## Usual repairs

These are starting points for consistency, not verdicts. Each case is decided
on its own specification and source.

- **The code departs from a sound specification** (wrong length or unit,
  partial comparison, missing seed, nonce or check). Make the code follow the
  specification. Scope: Single line or Function, code. Impact: none, unless
  the code skipped specified work.
- **A transform or protocol omits or misbinds an input** (rejection key,
  Fiat–Shamir or key-derivation transcript, domain separation, tweak, index).
  Hash the missing value. Scope: Function. Impact: none when both sides
  already hold the value, and bytes when something new must be sent.
- **Non-canonical or unchecked encodings** (padding bits, field elements,
  norm-only checks). Reject them. Scope: Single line or Function. Impact:
  none.
- **A secret seed, key-generation root or generator state narrower than the
  level.** Widen the seed and the expander's state. Scope: Function, or
  Sub-algorithm when the hash or XOF must be replaced. Impact: none for secret
  values, bytes for a seed stored in a public object, and hashing work when
  the wider hash has a smaller rate.
- **A hash, prehash, message representative, salt, challenge or commitment
  narrower than the level.** Widen it or add a salt. Scope: Function to
  Sub-algorithm. Impact: bytes for every transmitted copy, and hashing work
  where hashing dominates, as in hash-based and MPC- or VOLE-in-the-head
  signatures. For a hash-and-sign scheme, check whether the public map's own
  codomain is too small; that is a Parameter set repair.
- **A KEM key or exchanged secret shorter than the level.** Output more bytes.
  Scope: Single line when the encapsulated message and coins already hold
  enough entropy. Otherwise the plaintext space must grow, which is a
  Sub-algorithm or Parameter set repair with ciphertext bytes and failure
  probability to estimate.
- **A challenge space below the level.** Use a heavier or wider challenge.
  Scope: Sub-algorithm or Parameter set. Impact: the rejection bound and the
  expected repetitions, and the signature size when the masks must grow.
- **Leakage through signatures or failures** (undersized rejection envelope,
  finite mask support, exploitable decryption failures). Enlarge the masks or
  bounds, or lower the failure probability. Scope: Parameter set. Impact:
  bytes per coefficient, expected repetitions, modulus and ciphertext size.
- **An attack cost below the target under a parameter model** (decoding,
  lattice reduction, algebraic, multi-instance). Choose new parameters. Scope:
  Parameter set. Impact: a parameter estimate. Where a multi-target gain comes
  from shared hash inputs and not from the hard problem, separating those
  inputs is a Function repair. Where the attack needs more than 2^64 uses of
  one key, see Usage limits.
- **A hash with a narrow internal state** (long-message second preimages,
  truncated chaining values, missing block counter). Widen the state, capacity
  or chaining value, or add a counter. Scope: Function for a counter,
  otherwise Sub-algorithm. Impact: the change in rate, permutation width or
  rounds.
- **A structural property of a hash** (invariant subspace, cancelling round
  keys, state collisions). Scope: Function when a standard change of constants
  or schedule removes the root cause, otherwise Construction or None known.
  Impact: the work added per block, or not bounded.
- **A structural break of the hard problem or trapdoor** (the public key
  reveals the secret structure, forgery from algebraic structure). Scope:
  Construction or None known, unless a proposed fix shows otherwise. Impact:
  not bounded, or the proposal's figures.
- **A Critical side channel.** Use the known constant-time technique that the
  finding names. Impact: the cost of that technique by operation count.

## The note

`fixing_cost.md` has this shape:

```markdown
Yes: <one sentence: the deciding repair, the objects or operations affected, and the parameter sets>

Impact: <object or operation> +<N>% at <parameter set> (<count | estimate | judgment>); ... | none | not bounded
Scope: <scope> (<small | large>), <code | specification | specification and code>
With leads: <Yes | No>; <impact>; <scope>
Assessed: <YYYY-MM-DD>

## Attacks considered

- <id>: <title> (<Layer>, <Status>)

## Remediation

- **<id>.** <The repair that is costed, and why it is the cheapest credible one.> Scope: <scope>, <where>. Impact: <figures with their basis>. <Proposed fix, alternatives, and what was not checked.>

## Evidence

- <Baseline: specification table and physical page, with the values used.>
- <Calculation: formula, inputs and result, or the script command and the numbers it prints.>
- <Source lines, specification steps and proposed-fix links relied on.>
```

- The first line starts with `Yes:` or `No:`. Nothing comes before it.
- `Impact` and `Scope` give the combined impact and the largest scope of the
  Confirmed and Probable findings. When there are none, they are `none`.
- `With leads` appears only when the candidate has a live Lead finding. It
  gives the answer, combined impact and largest scope that would apply if
  every Lead held together with the other findings. When its answer differs
  from the first line, the answer sentence names the Lead findings.
- `Assessed` is the UTC date of the latest assessment.
- `Attacks considered` lists exactly the candidate's live Critical findings,
  by ID and title as in `report.md`.
- `Remediation` has one bullet per finding. Findings repaired by one change
  may share a bullet.
- `Evidence` lets a reader redo every figure. It may be omitted when no figure
  is computed.
- A calculation that cannot be redone by hand names a script in the repository
  and its command: an existing audit script, or a new stdlib-only script
  beside the note, named `fixing_cost_<topic>.py`. Such a script computes
  parameters, sizes and counts. It must not implement the repaired scheme.
  List every such helper in [`FIXING_COST_HELPERS.md`](FIXING_COST_HELPERS.md).
- The note is self-contained. It does not compare the candidate with other
  NGCC candidates and does not rank it. A published scheme outside the
  competition may be cited as a numeric anchor, with a reference.
- Write in the plain style of the reports, and say what was not checked.

A candidate without a live Critical finding gets the short form, and nothing
else. `Scope: none` is used only where no finding decides the scope.

```markdown
No: no Critical finding is recorded for <candidate>.

Impact: none
Scope: none
Assessed: <YYYY-MM-DD>
```

## Decision procedure

For each candidate:

1. List the live Critical findings in the candidate's report, and mark the
   Leads.
2. Record the baseline sizes and the work of the affected operations.
3. Read the proposed fixes.
4. For each finding, name the root cause and the cheapest credible repair.
5. Give the repair's scope and say where it falls.
6. Trace what the repair changes, and estimate the impact on each object and
   operation, with its basis.
7. Combine the impacts per parameter set, and take the largest scope, first
   for the Confirmed and Probable findings and then with the Leads.
8. Answer `Yes` or `No` from the combined impact of the Confirmed and Probable
   findings, and write the note.

## Keeping notes current

Reassess a candidate when one of its Critical findings is added, reclassified
or withdrawn, and when a proposed fix or a revised submission appears.

The remediation text in a finding and the note must not contradict each other,
for example on whether a repair is local. The report does not need the note's
costing detail. When the two contradict, correct whichever is wrong.
