# MAYO-5 — Low: Lemma 2 treats an average-case restart bound as if it held for every key

Severity: Low
Status: Proof gap
Layer: Design
Affected: Theorem 1 (via Lemma 2) for MAYO1, MAYO2, MAYO3 and MAYO5; shown on a reduced toy instance
Discovery: Non-trivial
Exploitation: Invalid proof step shown by exact enumeration on a toy instance; the final inequality was not shown to fail and no attack follows
Remediation: Moderate
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-11-15

## Proof step

Lemma 2, step 6 (physical p.30) says that Game 5, "conditioned on Good v
happening, is identical to Game 4", and concludes
`Pr[Game5] >= Pr[Game4] Pr[Good v] >= PrGame4 (`MAYO/1 − Qs B`)`. Here `B` comes
from Lemma 1 (physical p.28), which bounds the probability that
`P*(v + ·) + L(Lambda, ·)` lacks full rank **on average** over the oil space,
the public map and the vinegar.

The identity needs the fraction of good vinegar to be the same for every key
and every `Lambda`. Conditioning on Good v otherwise reweights keys and
`Lambda` values by their good-vinegar fraction, and Game 5 conditioned on
Good v is then not Game 4. With only an average-case bound, the step gives an
additive loss `Qs B`, not the multiplicative factor stated.

## Evidence

The reproducer checks the quoted step and Lemma 1's average-case wording on
their pages of the pinned specification, then enumerates every vinegar tuple
for 56,000 (key, `Lambda`) pairs of a toy instance over GF(2) (m = 4, o = 3,
k = 2, n − o = 5) that meets the parameter rules of §2.1.1 except q.

- The bad-vinegar fraction ranges from 0.148 to 0.336 (mean 0.2165,
  standard deviation 0.0128). It varies with `Lambda` for 13,984 of 14,000
  keys, and its exact average over all `Lambda` still varies between keys.
- So conditioning on Good v reweights the pairs, and Game 5 given Good v is
  not Game 4 on the toy.
- Control: the exact average-case bad probability, 0.21655, is below
  `B = 3/8`, as Lemma 1 states, and the sample mean matches it.

## Classification and limits

Low / Proof gap: a candidate-specific proof step that is invalid as written.
No sampled pair exceeds `B`, so the final inequality was not shown to fail;
for typical keys the good-vinegar fraction probably concentrates, and the
step is likely repairable with an extra term for atypical keys. Nothing was
computed at the submitted parameters (q = 16). The step is inherited from
Beullens's 2022 MAYO proof. [MAYO-2](/nist/reports/MAYO-2.html) is the separate limit of the
same theorem to few signing queries.

## Reproduction

```sh
python3 MAYO/reproduce_mayo_5.py
```

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py MAYO-5
```
