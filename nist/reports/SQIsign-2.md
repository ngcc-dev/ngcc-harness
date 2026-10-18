# SQIsign-2 — Low: a secret response norm reaches variable-time multiprecision arithmetic

Severity: Low
Status: Confirmed
Layer: Side-channel
Affected: Reference p324_3, p500_27, and p664_17 signing
Discovery: Moderate
Exploitation: Source-confirmed repeated secret-dependent control flow; plausible local/shared-host observation, but no measured channel, secret recovery, or forgery
Remediation: Moderate
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-10-18

The reference signer passes a secret response norm directly to a multiprecision
modular inverse whose loop counts, branches, division shapes, and copy lengths
depend on those values. This is a concrete path from per-signature secret state
to the program's control-flow and memory trace in every submitted parameter set.

During response generation, `quat_response_element` derives `ideal_skchall`
from the secret ideal, challenge ideal, and randomized response work at
`sign.c:266` (`SQIsign/submission/SQIsign/Reference_Implementation/src/signature/ref/lvlx/sign.c#L266`).
The signer passes `ideal_skchall.norm` to `ibz_invmod` at
`sign.c:331` (`SQIsign/submission/SQIsign/Reference_Implementation/src/signature/ref/lvlx/sign.c#L331`).
It also branches on GCD/sampling results at lines 276–313. A second inverse at
line 359 uses `deg_odd`; that value is the response degree `q_rsp` disclosed by
the signature representation (specification physical pp.8 and 105), so it does
not support this secret-dependent trace. The finding rests on the first inverse.

`ibz_invmod` labels itself variable time and uses the secret operand to choose
initial ordering, trim limb lengths, control the outer Lehmer loop, select
Lehmer versus division steps, size temporary arrays/copies, and terminate at
`mp.c:2446` (`SQIsign/submission/SQIsign/Reference_Implementation/src/mp/ref/lvlx/mp.c#L2446`).
Representative secret-dependent sites include the comparison at line 2467,
length scans at lines 2479–2485 and 2549–2555, loop termination at lines
2488–2491, and the division/Lehmer choice at lines 2495–2508.

This is not an accidental contradiction in the documentation. The
[specification](https://csrc.nist.gov/csrc/media/Projects/pqc-dig-sig/documents/sqisign_specification.pdf), §8.5 (physical PDF page 111), states that the
supplied implementation is not constant time overall. The submitted
`README.md:100` (`SQIsign/submission/SQIsign/Reference_Implementation/README.md#L100`)
and
`ct-quaternion.supp:1` (`SQIsign/submission/SQIsign/Reference_Implementation/test/ct-quaternion.supp#L1`)
deliberately exclude the multiprecision, quaternion, and `id2iso` layers from
the general ctgrind result. Those disclosures correctly limit the implementation
claim; they do not remove the secret-dependent execution behavior from the
submitted signer.

Run the deterministic source certificate with:

```sh
python3 SQIsign/reproduce_sqisign_2.py --output SQIsign/results/SQIsign-2.json
```

It verifies the all-parameter harness inventory, the secret-to-inverse call
sites, the value-dependent inverse implementation, and the submission's stated
ctgrind exclusions. A small integer control illustrates differing inverse
traces. It is not a timing test and does not claim to recover any part of a
secret key.

The rating is Low because this is a source-level secret-dependent trace with no
measured channel, candidate-specific extraction, or key recovery. The specific
trace can be closed by replacing the inverse of `ideal_skchall.norm` with a
fixed-bound constant-time inverse. The modulus `remain` is a power of two, and
the same submitted `mp.c` already contains the fixed-iteration Newton–Hensel
helper `ibz_invmod_2e`, used by `ibz_invmat`. Exposing or adapting that helper
for this call is a concrete starting point, although its complete operand-level
constant-time behavior and bounds still require validation. Integrating and
validating the repair across all parameter sets is a material implementation
change, so remediation is Moderate rather than Architectural. That repair would
address this finding; certifying the full signer would separately require
reviewing every secret path through the exempted multiprecision, quaternion,
and ideal-to-isogeny components, followed by unsuppressed taint and controlled
timing tests on all three parameter sets.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py SQIsign-2
```
