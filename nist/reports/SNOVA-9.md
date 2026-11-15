# SNOVA-9 — Informational: Internal signing API binds only the first 64 digest bytes into the vinegar

Severity: Informational
Status: Confirmed
Layer: Implementation
Affected: All trees and parameter sets through the exported internal signing function with a digest longer than 64 bytes (reference I_S, I_K and V_B run); crypto_sign is not affected
Discovery: Moderate
Exploitation: Needs a repeated caller-supplied salt and a digest longer than the specified 64 bytes through an internal function; two such signatures differ by a vector in the secret oil space; not reachable through crypto_sign
Remediation: Local
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-11-15

## Defect

The internal signing function absorbs a fixed `BYTES_DIGEST` (64) bytes of
the digest into the vinegar hash
(`snova_ref.c:882` (`SNOVA/submission/SNOVA_Round3/Reference_Implementation/SNOVA_I_K/snova_ref.c#L882`))
but `len_digest` bytes into the target hash
(`snova_ref.c:501` (`SNOVA/submission/SNOVA_Round3/Reference_Implementation/SNOVA_I_K/snova_ref.c#L501`)).
The optimized and additional trees have the same two lines (source check);
the additional tree exports the function as `snova_sign_digest_skx`.
`crypto_sign` always passes a 64-byte digest.

## Evidence

The reproducer signs four 80-byte digests with one key and one
caller-supplied salt through the internal API, for I_K, I_S and V_B.

- Each signature verifies under its own digest and fails under the others.
- Digests that agree in their first 64 bytes give identical vinegar and
  different targets, so the difference of the two signatures lies in the
  secret oil space. The images of that difference under the public S-power
  maps span 12 of 12 oil dimensions for I_K, 25 of 25 for V_B and 16 of 20 for
  I_S: one such pair gives the whole oil space for I_K and V_B.
- Controls: digests that differ inside the first 64 bytes give unrelated
  vinegar; two messages through `crypto_sign` with a forced repeated salt show
  no relation; a temporary copy that absorbs `len_digest` at line 882 removes
  the relation.

## Classification and limits

Informational / Confirmed / Implementation. The trigger is a repeated salt,
which the randomness-reuse rule in the classification policy rates
Informational, together with a digest longer than the 64 bytes that
Algorithm 14 (p.23) defines, which is outside the specified interface. The
specification states that a repeated salt carries no immediate risk
(§1.5.3, p.12); that holds for 64-byte digests. The NIST API is not
affected. The oil-space test uses the known private key; no key recovery
from public data alone was run. With `len_digest` below 64 the same line reads
past the caller's buffer (source reading). Same root cause as the design
survey's DS-SNOVA-4.

Absorb `len_digest` bytes in the vinegar hash, or reject other digest
lengths.

## Reproduction

```sh
python3 SNOVA/reproduce_snova_9.py
```

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py SNOVA-9
```
