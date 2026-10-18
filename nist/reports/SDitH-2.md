# SDitH-2 — Low: normative verifier omits the witness correction

Severity: Low
Status: Confirmed
Layer: Design
Affected: Round-3 specification Algorithm 12 for every submitted parameter set; submitted implementation and KATs are internally consistent
Discovery: Trivial
Exploitation: Literal-spec interoperability failure; honest signatures are rejected; forgery impact was not analyzed
Remediation: Local
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-10-18

The normative signing and verification pseudocode compute different
`hlines` values. Algorithm 10, line 24 (displayed p.28 / PDF p.30) hashes
`Delta_wit` along with `haux`, `alpha_plain`, and `alpha_base`. Algorithm 12,
line 26 (displayed p.30 / PDF p.32) hashes
only the first three values even though it parses `Delta_wit` at line 2. A
verifier implemented literally from Algorithm 12 therefore rejects honest
signatures.

The submitted implementation behaves consistently and includes the serialized
witness correction, named `circuit_in_pub`, in both computations. The signer
does so at
`sdith_signature.c:416` (`SDitH/submission/submission_package/Reference_Implementation/cat1_fast/src/sdith_signature.c#L416`)
and the verifier at
`sdith_signature.c:704` (`SDitH/submission/submission_package/Reference_Implementation/cat1_fast/src/sdith_signature.c#L704`).
All 24 reference and optimized copies are byte-identical, and all repository
KATs exercise this consistent code behavior.

Run the specification/source certificate and literal-pseudocode runtime control
with:

```sh
python3 SDitH/reproduce_sdith_2.py --output SDitH/results/SDitH-2.json
```

The script extracts Algorithms 10 and 12 from the pinned specification, checks
the two submitted source calls, copies `cat1_fast` into a temporary directory,
removes only the verifier's `circuit_in_pub` hash input to model Algorithm 12,
and builds the submitted KAT generator. The unmodified repository KAT is the
control; the literal Algorithm 12 build rejects its first honest KAT signature.
No submitted source file is changed.

Add `Delta_wit` as the fourth `Hashlines` input in Algorithm 12, line 26.
This is an editorial repair to the normative specification that aligns it with
Algorithm 10, the submitted implementation, and the KATs. The literal-spec
interoperability failure is rated Low because it is confirmed but locally
repairable. The reproducer tests honest-signature rejection only; it does not
analyze whether the transcript mismatch creates any forgery path.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py SDitH-2
```
