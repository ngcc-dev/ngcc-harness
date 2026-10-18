# MQOM-2 — Informational: repeated signing entropy reveals the secret vector

Severity: Informational
Status: Confirmed
Layer: Design
Affected: All 18 On-Ramp Round 3 CT and OT parameter sets when two different messages reuse the same mseed and the salt prefix retained by TweakSalt; retained runtime witnesses use MQOM3-L1-gf16-fast-ct and MQOM3-L1-gf16-fast-ot
Discovery: Non-trivial
Exploitation: End-to-end key recovery under a repeated signer-RNG output outside EUF-CMA
Remediation: Local
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-10-18

The MQOM signing algorithm draws `mseed` and `salt` from external randomness.
They are not derived from the signing key or message. Reusing `mseed` and the
salt prefix retained by `TweakSalt` fixes the seed trees and masked secret
corrections. The full salts need not match: `TweakSalt` replaces the last two
bytes for CT sets and the last three bytes for OT sets with domain-separation
data. Different messages and full salts normally select different hidden
leaves. In a repetition where the two hidden leaves differ, the union of both
openings reveals every leaf; summing their expanded secret shares and the
public correction recovers the full secret vector `x`.

Run the retained witness against the submitted Python reference implementation:

```sh
python3 MQOM/reproduce_mqom_2.py
python3 MQOM/reproduce_mqom_2.py MQOM3-L1-gf16-fast-ot
```

The submitted Python reference requires NumPy, Numba, PyCryptodome and joblib,
as listed in its `requirements.txt`. Install those optional dependencies with:

```sh
python3 -m pip install -r \
  MQOM/submission/submission_package_v3/Reference_Implementation_Python/requirements.txt
```

The script exits with `SKIP` when a dependency is unavailable. With the
dependencies installed, each retained command creates two valid signatures
with the same `mseed` and salt prefix but different full salts, recovers `x`,
builds a replacement secret key, and verifies a signature on a fresh message.
It also signs with the same `mseed` and a different salt prefix and confirms
that the attack candidate is not the secret vector.

This does not give an attacker control of randomness in the standard EUF-CMA
experiment and the submission does not claim hedged signing. NIST's call
§4.B.4 identifies resistance to RNG malfunction and nonce reuse as a desirable
rather than mandatory property. The public harness methodology therefore rates
this Informational while recording the demonstrated end-to-end consequence.
A stuck RNG, process fork, or VM restore supplies the required precondition.
Deriving `mseed` from the secret key, message, fresh randomness, and a domain
tag would prevent identical internal seeds under that failure.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py MQOM-2
```
