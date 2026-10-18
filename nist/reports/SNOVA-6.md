# SNOVA-6 — Low: a fresh verifier rejects its first valid signature

Severity: Low
Status: Confirmed
Layer: Implementation
Affected: All 18 submitted Round-3 q=16 reference API variants (nine K/B/S choices times SHAKE/AES) when verification is the process's first SNOVA operation
Discovery: Trivial
Exploitation: End-to-end correctness and availability failure; valid input is rejected and no forgery is produced
Remediation: Local
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-10-18

The submitted reference implementation rejects an honestly generated signature
when `crypto_sign_open` is the first SNOVA operation in a fresh process. The
identical public key and signed message succeed on the second call in that
process. The public harness witness reproduced this sequence for all 18 reference
libraries.

The field tables and `gf_S` are zero-initialized globals at
`snova_ref.c:21` (`SNOVA/submission/SNOVA_Round3/Reference_Implementation/SNOVA_I_K/snova_ref.c#L21`);
`fixed_abq` is separately zero-initialized at
`snova_ref.c:630` (`SNOVA/submission/SNOVA_Round3/Reference_Implementation/SNOVA_I_K/snova_ref.c#L630`).
Key generation and signing call `snova_init()` before using them at
`snova_ref.c:668` (`SNOVA/submission/SNOVA_Round3/Reference_Implementation/SNOVA_I_K/snova_ref.c#L668`)
and
`snova_ref.c:778` (`SNOVA/submission/SNOVA_Round3/Reference_Implementation/SNOVA_I_K/snova_ref.c#L778`).
Public-key expansion has no corresponding initialization at
`snova_ref.c:1144` (`SNOVA/submission/SNOVA_Round3/Reference_Implementation/SNOVA_I_K/snova_ref.c#L1144`)
and consumes `fixed_abq` at
`snova_ref.c:1204` (`SNOVA/submission/SNOVA_Round3/Reference_Implementation/SNOVA_I_K/snova_ref.c#L1204`).
Only afterward does verification initialize the globals at
`snova_ref.c:1232` (`SNOVA/submission/SNOVA_Round3/Reference_Implementation/SNOVA_I_K/snova_ref.c#L1232`).
The external verifier invokes these functions in that order at
`sign.c:66` (`SNOVA/submission/SNOVA_Round3/Reference_Implementation/SNOVA_I_K/sign.c#L66`).

With zero-initialized `fixed_abq`, the first expansion has zero A, B and Q
matrices, so its public-map evaluation is zero and verification fails except
for the negligible zero-target case. The submitted optimized implementation
calls `SNOVA_INIT` at the start of `pk_expand` and is not affected by this
reference-path defect.

Ordinary KAT execution hides the defect because key generation runs first and
initializes the process-global state. Applications that verify before generating
or signing encounter the failure. A failed first call also initializes the
globals, which explains why an unchanged second call succeeds.

Reproduce the isolated two-process test with:

```sh
python3 SNOVA/reproduce_snova_6.py --output SNOVA/results/SNOVA-6.json
```

For every library, the script generates a valid public key and signed message
in one child process, then loads the library in a new child and calls
`nist_sig_open` twice. It requires return values `-1` and `0`, respectively,
and checks that the second call recovers the original message. Temporary keys
and signatures are deleted on completion. If the reference libraries are
absent, the reproducer builds the ignored `SNOVA/lib/*.so` artifacts first.

Initialize the field and fixed-matrix state before public-key expansion, using
the same guarded initialization as the other entry points. The initialization
mechanism should also be made safe for concurrent first use. This local repair
does not require a change to the SNOVA construction or serialized formats.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py SNOVA-6
```
