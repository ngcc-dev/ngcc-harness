# QR-UOV-5 — Low: Optimized key generation leaves a matrix that determines S on the stack

Severity: Low
Status: Confirmed
Layer: Implementation
Affected: Optimized_Implementation/opt key generation, all 30 instances by shared source (1q127L3 SHAKE run); the ref and avx2 trees wipe the corresponding arrays
Discovery: Moderate
Exploitation: Residue witness: after key generation the dead stack holds S^T*P1 of the last equation, from which all of S was recomputed; reading it needs an in-process memory-disclosure primitive, and the residue was located using the known key
Remediation: Local
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-11-15

## Defect

Optimized key generation declares four secret-derived arrays
(`qruov.c:149-152` (`QR-UOV/submission/QR_UOV - Round3/Optimized_Implementation/opt/qruov.c#L149`)) but
wipes only `sdt` and `sdt_eval`
(`qruov.c:184-185` (`QR-UOV/submission/QR_UOV - Round3/Optimized_Implementation/opt/qruov.c#L184`)). The
reference tree wipes `sdt`, `sdt_pi1` and `sdt_pi2`
(`qruov.c:74-76` (`QR-UOV/submission/QR_UOV - Round3/Reference_Implementation/ref/qruov.c#L74`)) and the
AVX2 tree its corresponding arrays (`qruov.c:212-218`).

## Evidence

The reproducer runs `crypto_sign_keypair` for 1q127L3 (SHAKE) on a stack the
driver owns, then scans that stack after the call returns, at gcc `-O2` and
`-O3`.

- `sdt_pi1_eval` of the last equation (S^T·P1, 4,680 bytes) is fully present,
  as are the three other unwiped arrays; the two wiped arrays are absent.
- From those bytes and the public seed, interpolation and solving
  `P1 S = X^T` over F_(127^3) give all 936 entries of the secret matrix S.
- Controls: overwriting the stack first removes every match; the reference
  build shows no match for its corresponding arrays while its scan still
  finds a public array; a temporary copy of `opt/qruov.c` with the four
  missing wipes added shows no match.

## Classification and limits

Low / Confirmed / Implementation: a zeroization gap whose residue determines
the key. Reading it needs a separate in-process memory-disclosure primitive,
which is not shown. The scan locates the residue by recomputing it from the
known secret key; finding it in a memory dump from public data alone was not
shown. Only 1q127L3 SHAKE was built; the AVX2 tree was checked in source only.

Wipe all four arrays before returning, as the other two trees do.

## Reproduction

```sh
python3 QR-UOV/reproduce_qruov_5.py
```

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py QR-UOV-5
```
