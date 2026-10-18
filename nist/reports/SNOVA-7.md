# SNOVA-7 — Low: the submitted l=2 builds omit claimed public-key binding

Severity: Low
Status: Confirmed
Layer: Implementation
Affected: The three On-Ramp Round 3 l=2 alternative parameter sets; recommended l=4 and l=5 sets are unaffected
Discovery: Moderate
Exploitation: End-to-end public-key-substitution witness against a claimed BUFF property; no EUF-CMA forgery under a challenger-supplied key
Remediation: Local
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-10-18

The submitted implementation disables public-key hashing whenever `l=2`.
All three l=2 alternative configurations consequently accept an existing
signature under a distinct public key constructed to have the same evaluation
at that signed input. The kernel depends on the response input rather than the
message, so this also violates BUFF non-resignability and exclusive-ownership
properties. This contradicts the specification's public-key-binding transform
and its stated BUFF property. It
does not forge a new message under the fixed public key in the NIST EUF-CMA
game, and the nine recommended parameter sets use `l=4` or `l=5`.

The specification says that signing hashes the complete public key to a
48-byte digest, signs `Hash(pkd || md || salt)`, and stores that digest in the
secret key. It therefore specifies a 96-byte secret key for every proposed
set. See [§1.5.3, physical PDF page 12](https://csrc.nist.gov/csrc/media/Projects/pqc-dig-sig/documents/SNOVA_specifications.pdf), Algorithm 10 on
physical page 22, and Table 6 on physical page 30. Table 6 identifies the
three affected alternatives as `(50,17,16,2,2,17,68)`,
`(76,25,16,2,2,25,100)`, and `(104,33,16,2,2,33,132)`.

The common submitted header instead defines
`HASH_PK` as `(SNOVA_l > 2)` (`SNOVA/submission/SNOVA_Round3/Additional_Implementations/generic/snova.h#L40`).
That choice reduces `BYTES_SK` from 96 to 48 at
`snova.h:137` (`SNOVA/submission/SNOVA_Round3/Additional_Implementations/generic/snova.h#L137`).
The signer then hashes only the 16-byte public seed at
`snova_ref.c:852` (`SNOVA/submission/SNOVA_Round3/Additional_Implementations/generic/snova_ref.c#L852`),
and the verifier does the same at
`snova_ref.c:1324` (`SNOVA/submission/SNOVA_Round3/Additional_Implementations/generic/snova_ref.c#L1324`).
The generic parameter header explicitly includes the category-1 l=2
configuration, and the source contains fixed-ABQ branches for all three. Those
branches use the seeds `SNOVA_ABQ_2`, `_3`, and `_4`, which the specification
does not define; l=2 interoperability is therefore also underspecified.

Run from the public harness root:

```sh
python3 SNOVA/reproduce_snova_7.py --output SNOVA/results/SNOVA-7.json
```

The witness pins the submitted header and source hashes, compiles all three
l=2 configurations in a temporary directory, and generates a key and
signature for each. A temporary instrumented copy exports the already computed
public-map value for kernel search; it does not change the verifier's
accept/reject path. For each signature it evaluates enough public-key
coefficients to find a nonzero kernel vector over GF(16). Applying that vector
changes three serialized public-key bytes while preserving the public map at
the signed value. The submitted build accepts the original signature and
message under both keys. A temporary `HASH_PK=1` build preserves the same
public-map equality but rejects the substituted key. Changed-message controls
also reject. The observed submitted key size is 48 bytes in all three cases;
the control's 96-byte key agrees with the specification.

Define `HASH_PK=1` for the l=2 configurations, include the 48-byte digest in
their serialized secret keys, and publish matching vectors. Existing 48-byte
l=2 secret keys are format-incompatible with that correction. This is a local
implementation and vector change; it does not alter the multivariate public
map or the recommended parameter sets.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py SNOVA-7
```
