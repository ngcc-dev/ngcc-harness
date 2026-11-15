# MQOM-4 — Low: Opt-in GF(256) table multipliers index tables with secret field elements

Severity: Low
Status: Confirmed
Layer: Side-channel
Affected: All 18 parameter sets when built with -DUSE_GF256_TABLE_MULT or -DUSE_GF256_TABLE_LOG_EXP (FIELDS_REF only); neither option is a default or used by the KAT builds
Discovery: Trivial
Exploitation: Taint-tracking witness of secret-indexed loads in key generation and signing; no timing or cache measurement and no key recovery
Remediation: Local
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-11-15

## Defect

Two optional GF(256) multipliers in the reference field backend use the
operands as table indices: a 65 kB product table indexed by `(y << 8) | x`
(`fields_ref.h:140-142` (`MQOM/submission/submission_package_v3/Reference_Implementation/mqom3_cat1_gf16_short_ct/fields/fields_ref.h#L140`)) and log/exp tables
(`fields_ref.h:229-233` (`MQOM/submission/submission_package_v3/Reference_Implementation/mqom3_cat1_gf16_short_ct/fields/fields_ref.h#L229`)). The source labels
the first "NON constant time" and the second constant time only "on embedded
platforms without cache to SRAM"
(`fields.h:105-112` (`MQOM/submission/submission_package_v3/Reference_Implementation/mqom3_cat1_gf16_short_ct/fields.h#L105`)). Neither option is set by the
sources or by any of the 36 submitted Makefiles. [MQOM-1](/nist/reports/MQOM-1.html) is the
separate table-Rijndael finding.

## Evidence

The reproducer builds mqom3_cat1_gf16_short_ct three times (default
multiplier, `-DUSE_GF256_TABLE_MULT`, `-DUSE_GF256_TABLE_LOG_EXP`), with the
bitsliced Rijndael so that MQOM-1 adds no reports. Under Valgrind memcheck the
secret key and signing randomness are marked undefined and released values
are declassified.

| Build | Key generation | Signing |
|---|---:|---:|
| 65 kB table | 70,656 | 852,576 |
| log/exp tables | 141,312 | 1,705,140 |
| default multiplier (control) | 0 | 0 |

Every report is a secret-dependent load address inside `fields_ref.h`; none
is a branch. Most signing reports come from `compute_t1` and `ComputePz`
(`piop_default.c:28,59`).

## Classification and limits

Low / Confirmed / Side-channel: source-level secret-indexed tables with no
measured channel and a supplied constant-time alternative (the default
multiplier). One parameter set and one execution per run; the other sets share
the source.

## Reproduction

```sh
python3 MQOM/reproduce_mqom_4.py
```

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py MQOM-4
```
