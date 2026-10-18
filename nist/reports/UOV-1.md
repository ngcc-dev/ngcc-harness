# UOV-1 — Low: rejected signing systems expose secret-derived control flow

Severity: Low
Status: Confirmed
Layer: Side-channel
Affected: Reference Is, Ip, III, and V signers in classic, PKC, and PKC+SKC formats
Discovery: Moderate
Exploitation: Pinned-source ordered-statement witness; plausible local timing or branch observation, but no measured channel, key recovery, or forgery
Remediation: Moderate
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-10-18

The reference signer reveals how many secret-derived linear systems were
discarded before one was invertible. Although Gaussian elimination itself uses
masked pivot selection, its success bit controls the outer signing loop. The
number of complete attempts is therefore part of the observable control-flow
and timing trace but is not encoded explicitly in the returned signature.

The signer marks the secret key as secret for its Valgrind analysis, derives
vinegar from the secret-key seed, message, salt, and counter, and combines it
with the secret central map to form the signing system. The salt is sampled at
`ov.c:101` (`UOV/submission/UOV-R3/Reference_Implementation/Ip/ov.c#L101`), the
vinegar is derived at lines 121--130, and the matrix is constructed at line
138.
The portable Gaussian eliminator returns the accumulated pivot-success bit at
`blas_matrix_ref.c:88` (`UOV/submission/UOV-R3/Reference_Implementation/Ip/blas_matrix_ref.c#L88`)
or
`blas_matrix_ref.c:208` (`UOV/submission/UOV-R3/Reference_Implementation/Ip/blas_matrix_ref.c#L208`).
Immediately before branching, the caller explicitly marks `l1_succ` public to
Valgrind, then continues or exits the attempt loop at
`ov.c:146` (`UOV/submission/UOV-R3/Reference_Implementation/Ip/ov.c#L146`) and
lines 156--162 for GF(16). It generates counters 0 through 255, but the check
at line 171 also rejects a success found on the last attempt because
`n_attempt` has already reached 256.

The [UOV specification](https://csrc.nist.gov/csrc/media/Projects/pqc-dig-sig/documents/UOV_Specification_R3.pdf), §5.1.1 (physical PDF page 29), accurately
describes the fixed-trace row operations and says that revealing matrix
noninvertibility through timing is acceptable because the matrix is discarded.
That is a stated leakage rationale, not a proof in this report that the attempt
distribution is independent of the long-term secret map. A fixed public salt
does not make the vinegar or hidden signing matrix public because their
derivation includes the secret seed and secret map.

The 12 harnessed reference directories have byte-identical `ov.c` files,
SHA-256 `863e59ecf3838fa8643c3a65e0bfa8cb86eeefaec7a31f1260767b2a7444affc`,
so the trace applies to every key format and parameter set. Run:

```sh
python3 UOV/reproduce_uov_1.py --output UOV/results/UOV-1.json
```

The deterministic certificate pins the specification and every `ov.c`, checks
the ordered secret-key/vinegar/system/rank/retry statements, and records
illustrative first-attempt and third-attempt branch traces. It does not execute
the signer, measure timing, or establish any correlation, recovery, or
forgery.

The finding is Low because it establishes only source-level dependence, with no
measured channel or attack, and because the specification explicitly accepts
revealing whether a discarded matrix is noninvertible. Strict removal would
require computing a fixed number of complete attempts and mask-selecting a
successful signature, or another fixed-work construction with a bounded
residual failure probability. That remains a Moderate remediation if the trace
is to be removed, but the remediation cost does not establish additional
security impact. No primary NIST EUF-CMA property is shown to fail.

A follow-up should instrument attempt counts over many keys under controlled
messages, salts, and backends, test whether the distribution depends on the
long-term map, and attempt candidate-specific inference. Documentation can
clarify the assumed independence, but only fixed-work signing removes the
execution trace.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py UOV-1
```
