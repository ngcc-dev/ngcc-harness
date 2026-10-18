# NIST evaluation baseline

This repository studies the additional digital signature process, using
NIST's requirements and security definitions. It does not import NGCC's
rules, API, DRNG, category names, or minimum requirements.

The primary sources are the [call for proposals](https://csrc.nist.gov/csrc/media/Projects/pqc-dig-sig/documents/call-for-proposals-dig-sig-sept-2022.pdf)
(September 2022, October 2022 IP-delivery update; local
`NIST-call-for-proposals.pdf`) and [API notes](https://csrc.nist.gov/csrc/media/Projects/pqc-dig-sig/documents/example%20file/api-notes-pqc-dig-sig-page.pdf)
(local `NIST-API-notes.pdf`). Package versions are the current links on the
[Round 3 page](https://csrc.nist.gov/Projects/pqc-dig-sig/round-3-additional-signatures),
except HAWK, whose withdrawal and Round 2 fallback are documented separately.

## Security properties and categories

The required forgery target is **EUF-CMA**, under adaptive chosen-message
attack (call §4.B.2, p.14). A change to a signature on an already-signed
message is not by itself an EUF-CMA break. Strong unforgeability and other
properties may be claimed by a scheme, but should be labeled separately.
NIST allows security estimates to assume no more than 2^64 chosen-message
signing queries, while noting that attacks with more queries may also be
considered, and primarily considers classical access to the signing oracle.

Categories are comparisons against reference problems, not fixed quantum
security bit counts (call §4.B.3, pp.15–18):

| NIST category | Reference attack |
|---|---|
| 1 | AES-128 key search |
| 2 | SHA-256 / SHA3-256 collision search |
| 3 | AES-192 key search |
| 4 | SHA-384 / SHA3-384 collision search |
| 5 | AES-256 key search |

The call discusses several resource metrics and quantum circuit depth
limits; report the attack model and costs explicitly. A classical work
factor, a quantum gate count, and a query bound are different quantities.
The extracted CSV contains **submitter-claimed** categories, not independent
certifications. HAWK-256 is a challenge set with no NIST category.

Minimum technical acceptability includes KeyGen, Sign, Verify, correctness,
message sizes up to 2^63 bits, and concrete parameter values (call §3,
pp.12–13). Side-channel resistance, other security properties, performance,
implementation characteristics and portfolio diversity are evaluated under
§4. A KAT run cannot establish these properties or the maximum message size.

## API and deterministic tests

NIST uses `crypto_sign_keypair(pk,sk)`,
`crypto_sign(sm,&smlen,m,mlen,sk)`, and
`crypto_sign_open(m,&mlen,sm,smlen,pk)` (API notes pp.1–2).
The headers specify public/secret key byte lengths and the maximum signed
message overhead `CRYPTO_BYTES`. Attached signed-message layouts differ
between schemes, so the harness treats `sm` as opaque.

For KATs the specified `randombytes` is AES CTR DRBG. Its seed function takes
48 entropy bytes, an optional personalization string, and strength 256
(API notes pp.2–3). Each library uses the submission's own KAT RNG or a
clearly documented compatible adapter. Library shims expose this to the
common runner, without imposing NGCC's API or DRNG on the schemes.

The compact manifests preserve submitted inputs and SHA-256 hashes of
submitted output bytes. The runner checks deterministic KeyGen/Sign,
recovered message equality, guard canaries, and rejection of modified
signed messages. It reports unavailable, mismatched, and failed tests
explicitly. Passing establishes agreement with the supplied vectors only.

## Relationship to NGCC work

`../ngcc1` and `../ngcc-harness` provided organizational and harness examples.
This repository copies and adapts every rule and script needed for its own
tests; no validation or reproduction command depends on those sibling trees.
NGCC findings can motivate tests here, but a specific issue must be cited and
the corresponding behavior demonstrated in this submission/version before it
becomes a finding here. Distinguish a specification issue, implementation
defect, format mismatch, and build/tooling adaptation.

This repository contains no public website. IP statements and original
archives are not part of the curated tree. Source copyright notices and
licenses stay with the included implementation files.
