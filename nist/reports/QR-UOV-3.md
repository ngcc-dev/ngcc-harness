# QR-UOV-3 — Informational: one repeated signing-RNG output exposes the oil space

Severity: Informational
Status: Confirmed
Layer: Design
Affected: All On-Ramp Round 3 parameter sets by the common construction; runtime forgeries use 1q127L3-shake and 3q127L3-shake
Discovery: Non-trivial
Exploitation: End-to-end fresh-message forgery after one exact signing-RNG repetition
Remediation: Local
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-10-18

QR-UOV selects its vinegar variables, salt, and free variables from external
randomness without hedging them with the secret key or message (specification
Algorithm 2, §4.2, physical pp.18–19; submitted `sign.c:21-46`). If two
signing calls on different messages receive the same RNG stream, their vinegar
vectors are equal. After accounting for canonical sign, the difference of the
two signatures is a nonzero vector in the secret oil space.

One such vector is sufficient. For every public quadratic form represented by
a symmetric lifted matrix `P_i`, the polar linear form `x -> o^T P_i x` of a
known oil vector `o` vanishes on the whole hidden oil space. The common kernel
of these public linear forms recovers that space. In the retained runs its rank
was `V=52` with kernel dimension `M=18` for `1q127L3`, and `V=76` with kernel
dimension `M=26` for `3q127L3`. Expanding those extension-field bases gives
the respective 54- and 78-dimensional oil spaces over the base field.

The retained witness resets the shared harness DRBG before two different-message
signatures, obtains one oil vector, recovers the common polar kernel, solves the
standard known-oil linear system for a fresh message, and checks the forgery
with both an independent public model and the submitted library. A pair made
with distinct RNG seeds is the negative control:

```sh
python3 QR-UOV/reproduce_qruov_3.py
python3 QR-UOV/reproduce_qruov_3.py 3q127L3
```

The precondition is outside EUF-CMA: the attack needs one exact repetition of
the signing RNG stream across two different messages. A correct RNG makes this
infeasible. NIST's call §4.B.4 identifies RNG malfunction and nonce reuse as
desirable rather than mandatory robustness considerations. Because QR-UOV
makes no hedged-signing or reuse-resistance claim, the project rates the result
Informational while recording the demonstrated single-repeat forgery.
Deriving the signing coins from the secret key, message, fresh randomness, and
a parameter-set domain tag would address this failure.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py QR-UOV-3
```
