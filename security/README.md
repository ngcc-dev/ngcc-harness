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

The LoomKEX correctness reproducer uses only the submitted scalar reference
implementation. Run `make -C kex-05 replay` followed by
`tools/reproduce.sh kex-05`; see `LOOM_FAILURE_SEARCH.md` for the deterministic
witness and full-search methodology.

`security/loom_state_rollback_key_recovery.c` is the conditional `kex-05-2`
exploit. Given a restorable serialized pass-1 state, it recovers the complete
ephemeral Loom-KEM secret and predicts the final AKE secret. The report's
rollback/cloning/concurrency limitation is part of the finding, not optional
deployment guidance.
