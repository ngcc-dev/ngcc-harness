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

Additional focused validators and runtime witnesses are:

```sh
# FlexTree design/source arithmetic for sign-11-1 through sign-11-8
python3 security/flextree_kudinov_validation.py
# TRIKE shipped-minimum versus specified-maximum whole-KEM differential
make -C kem-36 lib/libTRIKE-2.so
python3 security/trike_threshold_differential.py --trials 1000
# UVW decoder-versus-validation failure oracle
make -C kem-38 lib/libUVW-KEM-128.so
python3 security/kem_mutation_oracle.py kem-38/lib/libUVW-KEM-128.so --bits 0,846
# Generic isolated-process driver for malformed KEM/signature inputs
make -C security
# Resource-contingent hash error-handling witness
make -C security hash_oom_false_success
# CS scaled universal forgery, FactoDSA reduced algebraic lead, YuanYang witnesses
make -C sign-07 exploit
make -C sign-10/cryptanalysis test
make -C sign-34 exploit
# New candidate-local witnesses are included in the aggregate runner
tools/reproduce.sh hash-14
tools/reproduce.sh kem-06
tools/reproduce.sh sign-04
```

`CODE_PARAMETER_AUDIT.md` consolidates the Mito-E, TRIKE, and UVW source and
runtime evidence. FactoDSA finding `sign-10-1` remains a Lead: its reduced-size
recovery is runnable, but its submitted-size work factors have not been
demonstrated. The independently tracked `sign-10-2` public-key signing trapdoor
has a complete confirmed forgery witness.

`vulnerabilities.csv` is the complete public inventory of stable `xxx-yy-z`
issue IDs and issue-local substantiation statuses. Its verification field is
one of `runtime`, `static`,
`runtime+static`, or `review`; `review` means the source/specification finding
is identified here without claiming a cheap automated witness. Validate the
inventory with:

```sh
make check-vulnerabilities
python3 security/check_vulnerability_ids.py --reports /path/to/report-checkout
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
