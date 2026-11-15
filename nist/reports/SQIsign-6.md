# SQIsign-6 — Low: Hint-distinguishing premise of Theorem 8.1.7 is false as printed

Severity: Low
Status: Proof gap
Layer: Design
Affected: Theorem 8.1.7 for p324_3, p500_27 and p664_17
Discovery: Non-trivial
Exploitation: False proof premise under a degree model, with a boundary of 2^13 to 2^16 hints; no forgery
Remediation: Local
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-11-15

## Premise

Theorem 8.1.7 (physical p.106) bounds forgery by terms that include the
advantage against Problem 8.1.5, distinguishing hints from `H^sim` and
`H^unif` (Fig. 4, physical p.105). Each hint reveals the degree `d` of the
response isogeny.

- `H^sim` line 4 samples `phi_2` uniformly among all isogenies of odd degree
  below `D_rsp`, with no cyclicity condition, matching the signer, which
  samples lattice elements with no primitivity test (Algorithm 4.83,
  physical p.52). The degree law is then proportional to `sigma(n)`, the
  number of subgroups of order `n`.
- `H^unif` line 2 draws the degree with weight
  `prod (p_i + 1) p_i^(e_i − 1)`, which is `psi(n)`, the number of cyclic
  subgroups, and line 3 then samples a possibly non-cyclic isogeny of that
  degree.

`sigma(n) = psi(n)` only for squarefree `n`; for example `sigma(9) = 13` and
`psi(9) = 12`.

## Evidence

The reproducer confirms these lines on their pages of the pinned
specification and computes the two degree laws over odd `n`:

- Total variation 0.01158; `Pr[9 | d]` is 0.1440 under `H^sim` and 0.1333
  under `H^unif` (limits 35/243 and 2/15).
- Counting hints with `9 | d` distinguishes the laws at 3 sigma with about
  2^13.1 hints and at 8 sigma with about 2^16.0; a Hellinger bound gives
  distinguishing advantage at least 1/2 from 5,305 hints. The theorem's bound
  therefore exceeds one far below NIST's 2^64 signing queries.
- On toy supersingular graphs (p = 191, 383) the exact `H^sim` degree law is
  within 2·10^-5 of the `sigma` law and 0.0116 from the printed `psi` law.
- Controls: the laws agree on squarefree degrees, and a cyclic-only `H^sim`
  would match `psi`.

## Classification and limits

Low / Proof gap: a candidate-specific false premise with a concrete boundary
inside NIST's budget. No forgery follows, since real signatures follow
`H^sim`. The exact distance at the submitted primes and `D_rsp` was not
computed; only the degree model, its limit and toy graphs. Only the degree of
`phi_2` is compared. If `H^sim` was meant to draw cyclic isogenies only, the
gap moves to Lemma 8.1.3, where the simulator would no longer match the
signer. Weighting the `H^unif` degree by `sigma(n)` would repair the premise.
[SQIsign-5](/nist/reports/SQIsign-5.html) is the separate statistical loss of the same
theorem.

## Reproduction

```sh
python3 SQIsign/reproduce_sqisign_6.py
```

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py SQIsign-6
```
