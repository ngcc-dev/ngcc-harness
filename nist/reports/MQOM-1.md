# MQOM-1 — Low: portable table Rijndael leaks secret-dependent cache addresses

**Severity:** Low
**Status:** Confirmed
**Layer:** Side-channel
**Affected:** On-Ramp Round 3 portable `RIJNDAEL_TABLE` backend; all 18 parameter sets on non-constant-time table hardware
**Discovery:** Trivial
**Exploitation:** source-only; no measured channel or key recovery
**Remediation:** Local
**Credit:** Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
**Date:** 2026-10-18

## Affected code and observation model

The finding covers the submitted `RIJNDAEL_TABLE` backend for all 18 MQOM
parameter directories when it runs on a machine whose table lookups are not
constant time. The public harness's reference-library Makefile selects this backend
with `-DRIJNDAEL_TABLE` for practical KAT runtime. An attacker must observe a
fine-grained local cache or memory-access trace during key generation or
signing. The report does not claim a remote timing attack.

In the representative category-1 source,
`seed_commit_impl.h` (`MQOM/submission/submission_package_v3/Reference_Implementation/mqom3_cat1_gf16_fast_ct/blc/seed_commit_impl.h#L101`)
passes the secret tree seed as both plaintexts to `sc_encrypt_x2_x2` at line
105. `rijndael_table.c` (`MQOM/submission/submission_package_v3/Reference_Implementation/mqom3_cat1_gf16_fast_ct/rijndael/rijndael_table.c#L471`)
indexes `Te0` through `Te3` with cipher-state bytes at lines 475–493 and the
S-box with state bytes at lines 498–519. Those addresses therefore depend on
the secret seed even though MQOM's cipher keys are public tweaked salts. The
same files are byte-identical across all 18 submitted reference directories.

The specification explicitly characterizes the table implementation as
constant time only on platforms with constant-time table lookup, giving a
cacheless Cortex-M4 with tables in SRAM as an example (specification §3.4.3,
physical p.55). It also supplies a constant-time bitsliced backend. Accordingly,
this is a documented limitation of one implementation choice, rather than an
MQOM design break or a contradiction of a general constant-time claim.

## Impact and classification

Source review proves secret-dependent memory addresses on ordinary cacheful
systems. It does not show that an observer can recover a GGM seed or signing
key, distinguish transcripts, or forge a signature. The existing bitsliced
`rijndael_ct64.c` and native-instruction backends give a local replacement.
Under the project classification, source-only dependence with a supplied local
repair is Low.

This finding is independent of KAT correctness. The `ct` suffix in MQOM
parameter names identifies the correlated-tree protocol variant; it is not a
constant-time implementation claim.

## Reproduction

From the public harness root:

```sh
python3 MQOM/reproduce_mqom_1.py
```

The deterministic witness checks the pinned table-source hash in all 18
copies, the secret-seed call path, the secret-indexed round expressions, the
harness backend selection, and the presence of the submitted bitsliced
alternative. It is a root-cause source check, not a cache trace. No candidate
source is modified.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py MQOM-1
```
