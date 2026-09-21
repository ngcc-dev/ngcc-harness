# Design and parameter audit

`design_parameter_audit.py` reproduces the static and information-theoretic
findings whose attacks are too large to execute. Run it with:

```sh
make design-audit
python3 security/design_parameter_audit.py --json
```

Candidate directories without Makefiles are reference-data entries, not
incomplete build targets. Every one contains the submitted specification and
extracted pseudocode/parameter tables; some additionally contain the exact
source files cited by this audit. See `DESIGN_PARAMETER_AUDIT.md` for the
classification rules and limits.

`vulnerabilities.csv` is the complete public inventory of stable `xxx-yy-z`
issue IDs and issue-local substantiation statuses. Its verification field is
one of `runtime`, `static`,
`runtime+static`, or `review`; `review` means the source/specification finding
is identified here without claiming a cheap automated witness. Validate the
inventory with:

```sh
make check-vulnerabilities
python3 security/check_vulnerability_ids.py --reports ../ngcc1
```
