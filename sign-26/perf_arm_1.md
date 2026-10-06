# sign-26 SQIsign2D-push1/2 — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-26` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561096483196928.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: SQIsign2D-push1/2
- Implementation versions measured: reference
- Parameter sets: `SQIsign2D-lvl1`, `SQIsign2D-lvl2`, `SQIsign2D-lvl3`, `SQIsign2D-lvl4`

## 2. Assessment environment

| item | value |
|---|---|
| processor | Qualcomm Oryon (CPU 2, one core) |
| machine | ASUS Vivobook S 15 |
| clock | fixed 2.71 GHz (governor performance, minimum = maximum), boost off, SMT none |
| memory | 30562 MiB |
| OS / kernel | Ubuntu 26.04.1 LTS / 7.0.0-34-generic |
| compiler / build tool | gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0 / cmake version 4.2.3 |
| campaign start / end (UTC) | 2026-09-28T15:05:46 / 2026-09-29T08:43:52 |

## 3. Functional testing (KAT)

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-26/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `SQIsign2D-lvl1` | harness-default | PASS |
| `SQIsign2D-lvl2` | harness-default | PASS |
| `SQIsign2D-lvl3` | harness-default | MISMATCH [1] |
| `SQIsign2D-lvl4` | harness-default | PASS |

[1] The submitted Test_Vectors/KAT_SIG_SQIsign2D-lvl3.txt is a splice of level-2 and level-3 records (10 of 12 records have the level-2 secret-key length of 676 bytes instead of 900), so no level-3 build can reproduce it; the public keys agree (sign-26/pseudocode.md, discrepancy 2). These instances are timed anyway; their output is not validated.

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `SQIsign2D-lvl1` | keygen | 524.97 M | 195 ms | 5.13 | 195 ms | 100 (5 × 20) |
| `SQIsign2D-lvl1` | sign | 559.85 M | 208 ms | 4.81 | 208 ms | 100 (5 × 20) |
| `SQIsign2D-lvl1` | verify | 131.84 M | 49 ms | 20.4 | 49 ms | 100 (5 × 20) |
| `SQIsign2D-lvl2` | keygen | 1.69 G | 627 ms | 1.59 | 627 ms | 100 (5 × 20) |
| `SQIsign2D-lvl2` | sign | 2.89 G | 1.07 s | 0.933 | 1.07 s | 100 (5 × 20) |
| `SQIsign2D-lvl2` | verify | 400.18 M | 149 ms | 6.72 | 149 ms | 100 (5 × 20) |
| `SQIsign2D-lvl3` | keygen | 3.96 G | 1.47 s | 0.681 | 1.47 s | 100 (5 × 20) |
| `SQIsign2D-lvl3` | sign | 3.87 G | 1.43 s | 0.697 | 1.43 s | 100 (5 × 20) |
| `SQIsign2D-lvl3` | verify | 941.11 M | 350 ms | 2.86 | 350 ms | 100 (5 × 20) |
| `SQIsign2D-lvl4` | keygen | 45.65 G | 16.9 s | 0.0591 | 17 s | 20 (5 × 4) |
| `SQIsign2D-lvl4` | sign | 40.22 G | 14.9 s | 0.067 | 14.9 s | 20 (5 × 4) |
| `SQIsign2D-lvl4` | verify | 10.17 G | 3.78 s | 0.265 | 3.8 s | 95 (5 × 19) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `SQIsign2D-lvl1` | keygen | 64445160 | 1640 KiB | 2560 KiB |
| `SQIsign2D-lvl1` | sign | 64445160 | 2428 KiB | 9948 KiB |
| `SQIsign2D-lvl1` | verify | 64445160 | 4628 KiB | 9740 KiB |
| `SQIsign2D-lvl2` | keygen | 96521832 | 1664 KiB | 2764 KiB |
| `SQIsign2D-lvl2` | sign | 96521832 | 2604 KiB | 18008 KiB |
| `SQIsign2D-lvl2` | verify | 96521832 | 3600 KiB | 16956 KiB |
| `SQIsign2D-lvl3` | keygen | 128660584 | 1664 KiB | 2996 KiB |
| `SQIsign2D-lvl3` | sign | 128660584 | 2756 KiB | 28808 KiB |
| `SQIsign2D-lvl3` | verify | 128660584 | 5192 KiB | 29804 KiB |
| `SQIsign2D-lvl4` | keygen | 273510704 | 1668 KiB | 3888 KiB |
| `SQIsign2D-lvl4` | sign | 273510704 | 5756 KiB | 26504 KiB |
| `SQIsign2D-lvl4` | verify | 273510704 | 11380 KiB | 111620 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `SQIsign2D-lvl1` | 64 | 456 | 150 |
| `SQIsign2D-lvl2` | 96 | 676 | 218 |
| `SQIsign2D-lvl3` | 128 | 900 | 293 |
| `SQIsign2D-lvl4` | 262 | 1838 | 593 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — iccs/xof_iccs.c glue; NIST SHAKE/AES leftovers unused

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `SQIsign2D-lvl1` | keygen | 0.0% | 0.6% | drng 727 |
| `SQIsign2D-lvl1` | sign | 0.0% | 1.6% | drng 2.24e+03, pseudoXOF 1 |
| `SQIsign2D-lvl1` | verify | 0.0% | 0.0% | pseudoXOF 1 |
| `SQIsign2D-lvl2` | keygen | 0.0% | 0.3% | drng 1.27e+03 |
| `SQIsign2D-lvl2` | sign | 0.0% | 0.5% | drng 6.11e+03, pseudoXOF 1.67 |
| `SQIsign2D-lvl2` | verify | 0.0% | 0.0% | pseudoXOF 1 |
| `SQIsign2D-lvl3` | keygen | 0.0% | 0.2% | drng 1.54e+03 |
| `SQIsign2D-lvl3` | sign | 0.0% | 1.4% | drng 1.17e+04, pseudoXOF 1 |
| `SQIsign2D-lvl3` | verify | 0.0% | 0.0% | pseudoXOF 1 |
| `SQIsign2D-lvl4` | keygen | 0.0% | 0.1% | drng 4.22e+03 |
| `SQIsign2D-lvl4` | sign | 0.0% | 0.1% | drng 9.46e+03, pseudoXOF 1 |
| `SQIsign2D-lvl4` | verify | 0.0% | 0.0% | pseudoXOF 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `SQIsign2D-lvl1` | KAT log (sha256 `1789a9c338e6ff90…`) | `kat/sign-26/SQIsign2D-lvl1.log` |
| `SQIsign2D-lvl1` | timing keygen | `records/sign-26/SQIsign2D-lvl1__keygen.json` |
| `SQIsign2D-lvl1` | timing sign | `records/sign-26/SQIsign2D-lvl1__sign.json` |
| `SQIsign2D-lvl1` | timing verify | `records/sign-26/SQIsign2D-lvl1__verify.json` |
| `SQIsign2D-lvl1` | hash profile keygen | `profile/sign-26/SQIsign2D-lvl1__keygen.json` |
| `SQIsign2D-lvl1` | hash profile sign | `profile/sign-26/SQIsign2D-lvl1__sign.json` |
| `SQIsign2D-lvl1` | hash profile verify | `profile/sign-26/SQIsign2D-lvl1__verify.json` |
| `SQIsign2D-lvl2` | KAT log (sha256 `42c9cd5053278b0d…`) | `kat/sign-26/SQIsign2D-lvl2.log` |
| `SQIsign2D-lvl2` | timing keygen | `records/sign-26/SQIsign2D-lvl2__keygen.json` |
| `SQIsign2D-lvl2` | timing sign | `records/sign-26/SQIsign2D-lvl2__sign.json` |
| `SQIsign2D-lvl2` | timing verify | `records/sign-26/SQIsign2D-lvl2__verify.json` |
| `SQIsign2D-lvl2` | hash profile keygen | `profile/sign-26/SQIsign2D-lvl2__keygen.json` |
| `SQIsign2D-lvl2` | hash profile sign | `profile/sign-26/SQIsign2D-lvl2__sign.json` |
| `SQIsign2D-lvl2` | hash profile verify | `profile/sign-26/SQIsign2D-lvl2__verify.json` |
| `SQIsign2D-lvl3` | KAT log (sha256 `69ed27e663bac327…`) | `kat/sign-26/SQIsign2D-lvl3.log` |
| `SQIsign2D-lvl3` | timing keygen | `records/sign-26/SQIsign2D-lvl3__keygen.json` |
| `SQIsign2D-lvl3` | timing sign | `records/sign-26/SQIsign2D-lvl3__sign.json` |
| `SQIsign2D-lvl3` | timing verify | `records/sign-26/SQIsign2D-lvl3__verify.json` |
| `SQIsign2D-lvl3` | hash profile keygen | `profile/sign-26/SQIsign2D-lvl3__keygen.json` |
| `SQIsign2D-lvl3` | hash profile sign | `profile/sign-26/SQIsign2D-lvl3__sign.json` |
| `SQIsign2D-lvl3` | hash profile verify | `profile/sign-26/SQIsign2D-lvl3__verify.json` |
| `SQIsign2D-lvl4` | KAT log (sha256 `25c5893c06d3664f…`) | `kat/sign-26/SQIsign2D-lvl4.log` |
| `SQIsign2D-lvl4` | timing keygen | `records/sign-26/SQIsign2D-lvl4__keygen.json` |
| `SQIsign2D-lvl4` | timing sign | `records/sign-26/SQIsign2D-lvl4__sign.json` |
| `SQIsign2D-lvl4` | timing verify | `records/sign-26/SQIsign2D-lvl4__verify.json` |
| `SQIsign2D-lvl4` | hash profile keygen | `profile/sign-26/SQIsign2D-lvl4__keygen.json` |
| `SQIsign2D-lvl4` | hash profile sign | `profile/sign-26/SQIsign2D-lvl4__sign.json` |
| `SQIsign2D-lvl4` | hash profile verify | `profile/sign-26/SQIsign2D-lvl4__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

