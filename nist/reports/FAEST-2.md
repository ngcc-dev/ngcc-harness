# FAEST-2 — Low: ROM and QROM reductions miss claimed-category guarantees at the NIST signing-query baseline

Severity: Low
Status: Proof gap
Layer: Design
Affected: ROM and QROM security claims for all twelve On-Ramp Round 3 FAEST and FAEST-EM parameter sets
Discovery: Moderate
Exploitation: Cost/model gap in the concrete reduction; no forgery or key-recovery attack
Remediation: Moderate
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-10-18

## Bound and concrete substitution

Corollary 10.54 (physical p. 139) permits up to `Q_sig = 2^64`
signing-oracle queries and includes the nonnegative additive term

```text
6 Q_sig sqrt(ceil(log2 L) AdvPRP_PRG2lambda[1]
             + AdvPRP_LeafCommit[1]
             + 3 sqrt(Q1) 2^-lambda
             + Q2,1 2^-2lambda).
```

The first two primitive advantages are one-query advantages.  Under the
standard ideal-primitive key-search substitution, each is at least about
`2^-lambda`: an adversary guesses the keyed primitive's `lambda`-bit key and
tests it with one query.  Dropping the other nonnegative terms therefore gives

```text
6 Q_sig sqrt((ceil(log2 L) + 1) 2^-lambda).
```

At `Q_sig = 2^64`, this term alone is between `2^4.44` and `2^4.58` for
the four `lambda=128` parameter sets.  The right-hand side of the reduction is
therefore already greater than one under the one-key-guess substitution and
provides no concrete upper bound on forgery probability for those four sets at
NIST's signing-query baseline.

The `lambda=128` conclusion does not depend on the primitive substitution.
Keeping only the statistical summand `3 sqrt(Q1) 2^-lambda` inside the same
square root gives `6 sqrt(3) Q1^(1/4) >= 2^3.38` for any adversary making at
least one `H1` query. Independently, the corollary's EUF-KO bound contains
`10 Q^2 (tau + dzk) 2^-lambda` (FAEST) or `10 Q^2 tau 2^-lambda` (FAEST-EM),
multiplied by `(1 - dom/cod)^-1 = 4` at `lambda=128`, with
`Q >= (tau+4) Qsig`. At `Qsig = 2^64` and no adversarial hash queries, twice
that term is between `2^17.6` and `2^19.7` for the four `lambda=128` sets, and
it stays below one only for `Qsig` below about `2^54.2` to `2^55.2`.

For the higher categories, use the ordinary Grover key-search model
`AdvPRP(t) = t^2 / 2^lambda` for both primitive terms.  The same additive term
reaches one at only the following quantum-work exponents:

| `lambda` | short profiles | fast profiles | ideal key-search exponent |
|---:|---:|---:|---:|
| 192 | 27.37 | 27.51 | 96 |
| 256 | 59.37 | 59.51 | 128 |

The small difference between short and fast profiles comes from their values
of `L`.  Thus Corollary 10.54 also fails to certify the claimed-category
quantum work at the full signing-query baseline for the 192- and 256-bit sets.
The submission describes its QROM bound as giving meaningful concrete security
on physical p. 4 and describes the proofs as almost tight apart from signing
query loss on physical p. 140. “Vacuous” here applies directly to the
`lambda=128` bounds at `Q_sig = 2^64`; for `lambda=192` and `lambda=256`, the
calculation instead shows that the reduction reaches one at quantum work far
below the claimed-category target.

## Classical ROM bound

The classical-ROM Theorem 10.42 (physical p. 115) has the same per-signature
hybrid structure without the square root. Substituting the generic one-key
advantage `t/2^lambda` for each of its `3 + (tau+1) ceil(log2 L)` single-key
PRF/PRP terms, the bound reaches one at `t = 2^55.2` to `2^55.5`,
`2^118.6` to `2^118.9` and `2^182.2` to `2^182.5` for the 128-, 192- and
256-bit sets at `Qsig = 2^64`. This is NIST's primary model (classical
signing queries), and the shortfall is about 73 bits at every level. The
design survey recorded this as DS-FAEST-2 (Informational); it has the same
root cause and is folded into this report. The submission's per-signature IVs
and per-node tweaks argue against multi-target key search (§10.1.2, physical
p. 72), so the loss is presumably a proof artifact that an ideal-cipher
treatment of the PRG would remove.

## Classification and limits

This is a Low / Proof gap because it is a candidate-specific shortfall in
explicit concrete ROM and QROM claims.  It is **not an attack**.  The calculation does
not construct a FAEST forgery, recover a key, or show that the parameter sets
have the displayed work factors in practice.  It only shows that the stated
reductions cannot establish their claimed ROM and QROM category guarantees at `2^64` signing
queries under the standard primitive substitutions.

The substitution reads `AdvPRP[1]` as the usual best one-query advantage.  For
`lambda=192` and `lambda=256`, if those symbols are instead restricted to only
the particular reduction adversaries constructed in the proof, Corollary 10.54
supplies no concrete upper bounds for those advantages; a claimed-category
concrete guarantee still does not follow from the displayed corollary. For
`lambda=128` the bound exceeds one from its statistical terms alone. The
submission's QROM summary on physical p. 127 (§10.11.4) describes the loss as
a multiplicative factor of about 8 and omits the `Qsig` factor.  Closing the gap requires
tighter ROM and QROM reductions, additional concrete bounds for the reduction
adversaries, or an explicit usage limit included in the security claim.

## Reproduction

```sh
python3 FAEST/reproduce_faest_2.py
```

The deterministic script checks the pinned specification hash and repository
NIST baseline, reads all twelve extracted `L` values, and recomputes the
one-key-guess and Grover substitutions, the `lambda=128` statistical and
EUF-KO terms, and the classical-ROM thresholds above.  This is a cost/model check of
the stated reduction, not a cryptanalytic attack.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py FAEST-2
```
