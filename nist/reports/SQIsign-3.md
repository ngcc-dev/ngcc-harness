# SQIsign-3 — Low: verification continues after a gluing failure

Severity: Low
Status: Confirmed
Layer: Implementation
Affected: Reference, broadwell, and arm64 verification code for p324_3, p500_27, and p664_17
Discovery: Moderate
Exploitation: Reachable malformed-signature path reads unset stack state; 167 observed paths rejected and no false acceptance was found
Remediation: Local
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-10-18

The verifier does not propagate failure from the first gluing step of its
theta-isogeny chain. A malformed signature can make `gluing_compute` return
failure before it initializes the output gluing object. `gluing_start_chain`
returns `0`, which its caller treats as a valid chain index and immediately
reads from the unset `first_step.codomain` stack object.

The failure is explicit in the submitted contracts and control flow:

- `gluing_compute` (`SQIsign/submission/SQIsign/Reference_Implementation/src/hd/ref/lvlx/gluing.c#L407`)
  documents a zero return for an incorrect or malformed kernel. Its earliest
  verification check returns zero at line 444 before writing `out`.
- `gluing_start_chain` (`SQIsign/submission/SQIsign/Reference_Implementation/src/hd/ref/lvlx/gluing.c#L608`)
  returns that same zero at lines 673–674.
- `theta_chain_compute_impl` (`SQIsign/submission/SQIsign/Reference_Implementation/src/hd/ref/lvlx/theta_isogenies.c#L102`)
  assigns the return value to `current` at line 140 without checking for
  failure. Lines 149–151 then consume `first_step.codomain`.
- Verification reaches this path through
  `theta_chain_compute_and_eval_verify` (`SQIsign/submission/SQIsign/Reference_Implementation/src/verification/ref/lvlx/verify.c#L157`).

The broadwell and arm64 submissions contain byte-identical `gluing.c` and
`theta_isogenies.c` files, and all three parameter sets compile the shared
level-independent source. This is an implementation defect rather than a
property of the SQIsign construction.

The failure-handling pattern resembles NGCC finding `sign-25-1`, where stale
stack contents could decide a verifier result. The exact NGCC defect does not
transfer: this is a new unchecked gluing return in different code, and every
NIST SQIsign malformed input tested here still rejected.

Run:

```sh
python3 SQIsign/reproduce_sqisign_3.py --output SQIsign/results/SQIsign-3.json
```

The script pins both submitted source files and obtains the public harness's
reviewed p324_3 compile command with `make -n`. In a temporary copy, it marks
the three gluing rejection sites, fills `first_step` with `0xA5`, and records
whether execution reaches the first codomain read. The honest generated
signature verifies without setting any marker. Of 300 deterministic one-bit
signature mutations, 167 reach the earliest gluing rejection; every one
continues past the unchecked return and rejects later. The recorded codomain
byte remains `0xA5`, confirming that the failed callee did not initialize the
object subsequently read. No submitted source file is patched by the witness.

No false acceptance, crash, or disclosure was observed. The result establishes
undefined behavior on attacker-controlled verifier input, so the impact is
limited to a robustness and memory-safety defect at present. Stack layout and
compiler choices can affect behavior after the invalid read.

Give `gluing_start_chain` an unambiguous failure result and return failure from
`theta_chain_compute_impl` before using `first_step`, `thetaQ1`, or `thetaQ2`.
The verifier should keep the existing final rejection behavior while avoiding
all reads of unset outputs.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py SQIsign-3
```
