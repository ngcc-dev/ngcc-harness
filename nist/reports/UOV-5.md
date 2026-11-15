# UOV-5 — Low: the formal EUF-CMA theorem does not cover the recommended signer

Severity: Low
Status: Proof gap
Layer: Design
Affected: All four recommended parameter sets and all three key formats
Discovery: Moderate
Exploitation: Confirmed proof-scope and bound gap; no key recovery or forgery
Remediation: Moderate
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-10-18

## Scope mismatch

The specification states in §4, physical p.22, that no formal reduction from a
well-known hard problem has been presented for UOV. It then points to Appendix
A for the EUF-CMA argument for **salt-UOV**. That construction fixes a random
vinegar vector and resamples the salt until the linear system is solvable.

The recommended signer in Figure 2 and the submitted `ov.c` does the opposite:
it samples one salt, derives vinegar from the message, salt, secret seed and an
attempt counter, and retries with new vinegar until its secret linear system is
invertible. The specification argues that known attacks apply to both variants,
but that is not a reduction for the recommended signing distribution.

Theorem 1 is quoted from Sakumoto, Shirai and Hiwatari (PQCrypto 2011).
Kosuge and Xagawa (["Probabilistic Hash-and-Sign with Retry in the Quantum
Random Oracle Model"](https://eprint.iacr.asia/2022/1359.pdf), §4.2 and
Appendix B.2) state that this proof's random-oracle simulation is
flawed, crediting Chatterjee, Das and Pandit, and give a corrected bound; the
specification cites neither. Their Proposition 3 treats original UOV with
fresh, independently sampled vinegar on each retry, and the authors note
applicability to the then-current NIST UOV submission. The pinned Round 3
signer instead derives each vinegar deterministically from message, salt,
`sk_seed` and attempt counter. Applying the proposition to these exact
source-level coins needs an additional argument. The proposition also relies
on a computational preimage-simulatability assumption for which the authors
say no statistical bound is known and which they leave unproven.

## Quantitative limit

Appendix-A Theorem 1 gives the salt-UOV reduction numerator

`1 - (q_h + q_s) * q_s * 2^-128`.

At the NIST evaluation budget `q_s=2^64`, this is already zero when `q_h=0`
and negative for any positive hash-query budget. The theorem also loses a
factor `q_h+q_s+1`. Because `salt_len = 128` at every category (Table 4,
p.21), the numerator is also negative for a single signing query once
`q_h >= 2^128`, a hash budget inside the Category 3 and 5 targets of uov-III
and uov-V. At `q_h = 2^96` it stays above 1/2 only for `q_s` below about
`2^31`. With a 128-bit salt, the additive term of the Kosuge–Xagawa
proposition, `(3/2) q_s sqrt((q_s + q_ro + 1) / 2^128)`, is `2^32.58` at
`q_s = 2^64` for all four sets and reaches one at `q_s = 2^42.28`. These observations concern only what the theorem certifies;
they do not yield an attack on either signer.

Run the retained source and arithmetic certificate with:

```sh
python3 UOV/reproduce_uov_5.py
```

It pins the specification and submitted signer, confirms the fixed-salt,
retry-vinegar source order, evaluates the theorem numerator at the NIST
signing-query budget and at the hash budgets above, and evaluates the
Kosuge–Xagawa additive term. Because a proof for a related signer cannot establish the
required EUF-CMA claim for the submitted signer, this is a Low Proof gap. A
remedy is a reduction covering Figure 2's exact distribution and realistic
query budgets, or adoption of a signing procedure covered by an applicable
proof.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py UOV-5
```
