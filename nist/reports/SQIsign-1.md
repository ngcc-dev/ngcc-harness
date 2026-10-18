# SQIsign-1 — Informational: distinct response encodings verify for the same message

Severity: Informational
Status: Confirmed
Layer: Design
Affected: p324_3, p500_27, and p664_17
Discovery: Moderate
Exploitation: Trivial derivation of a second signature on an already signed message; no fresh-message forgery
Remediation: Architectural
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-10-18

All three submitted parameter sets accept at least 64 distinct encodings of the
response matrix as valid signatures for the same public key and message. The
64-element family combines 16 choices for the unused high matrix bits, a
single-column sign choice modulo the global `M`/`-M` normalization, and a
signature-preserving scale of the first column by
`1 + 2^RESPONSE_BITS`. Every member was accepted at all three levels; lower
response-bit and changed-message controls were rejected.

This matrix freedom composes with further verified aliases. For four
independently generated signatures, every alternative auxiliary-basis hint that
decoded to a full-order basis could be compensated by a matrix change and was
accepted: 91 and 38 alternatives at p324_3, 97 at p500_27, and 36 at p664_17.
Transporting the basis and matrix to the isomorphic Montgomery model with
coefficient `-A` also produced accepted signatures at all three levels. The
counts depend on the curve and signature; they are observations rather than a
fixed number of valid hints.

This is specified design behavior. [SQIsign §8.4.2](https://csrc.nist.gov/csrc/media/Projects/pqc-dig-sig/documents/sqisign_specification.pdf) (physical
PDF page 111) states that SQIsign does not target strong unforgeability and that
a valid signature can be transformed into another valid signature on the same
message by changing the auxiliary isogeny or its non-unique two-dimensional
representation. SQIsign claims EUF-CMA security (§8.1), and the NIST call asks
for existential unforgeability. Re-signing an already signed message does not
win EUF-CMA, and no strong-unforgeability claim is violated.

The encoding and verification path matches the specification:

- Algorithm 3.10 (physical PDF page 20) normalizes only between `M` and `-M`
  modulo `2^e`.
- Algorithms 3.23 and 3.24 (physical PDF page 24) encode the response matrix
  modulo `2^(ersp+2)` and require the range and that sign normalization.
- The implementation decodes four fixed-width `RESPONSE_BYTES` scalars at
  `encode_public.c:156` (`SQIsign/submission/SQIsign/Reference_Implementation/src/verification/ref/lvlx/encode_public.c#L156`)
  and checks exactly those canonicality rules at
  `verify.c:7` (`SQIsign/submission/SQIsign/Reference_Implementation/src/verification/ref/lvlx/verify.c#L7`).
- It applies the matrix to points of order `2^(RESPONSE_BITS+2)` at
  `verify.c:102` (`SQIsign/submission/SQIsign/Reference_Implementation/src/verification/ref/lvlx/verify.c#L102`)
  and verifies the represented response with a `RESPONSE_BITS` theta chain at
  `verify.c:156` (`SQIsign/submission/SQIsign/Reference_Implementation/src/verification/ref/lvlx/verify.c#L156`).

The accepted witnesses pass the range and `M`/`-M` normalization test; this is
not an ignored-byte or decoder-length bug. The verifier also requires only that
a supplied hint decode to a valid torsion basis and does not enforce the
specification's normalized Montgomery representative for `Eaux`. These are
format-level freedoms in the response representation.

Run:

```sh
python3 SQIsign/reproduce_sqisign_1.py --output SQIsign/results/SQIsign-1.json
```

The script seeds key generation and signing deterministically, enumerates the
64-element matrix family for every parameter set, verifies recovered-message
equality, changes the attached message as a control for every alias, and checks
a lower matrix bit as a negative control. The optional Sage-assisted
[`analysis/hint-alias`](https://github.com/ngcc-dev/ngcc-harness/blob/main/nist/SQIsign/analysis/hint-alias/README.md) tooling derives and
checks the basis-hint and `-A` witnesses retained from the independent review.

Applications requiring unique or strongly unforgeable signatures cannot obtain
that property from raw SQIsign signatures. They need an additional transform or
a revised unique response representation with a separate proof and compatibility
review. No change is required to satisfy SQIsign's stated EUF-CMA claim on the
basis of this observation alone.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py SQIsign-1
```
