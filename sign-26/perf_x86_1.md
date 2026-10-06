# sign-26 SQIsign2D-push1/2 — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-26` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561096483196928.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: SQIsign2D-push1/2
- Implementation versions measured: reference
- Parameter sets: `SQIsign2D-lvl1`, `SQIsign2D-lvl2`, `SQIsign2D-lvl3`, `SQIsign2D-lvl4`

## 2. Assessment environment

| item | value |
|---|---|
| processor | 12th Gen Intel(R) Core(TM) i7-12700 (CPU 2, one core) |
| clock | max 2.10 GHz, governor performance, turbo off, SMT off |
| memory | 31788 MiB |
| OS / kernel | Debian GNU/Linux 13 (trixie) / 6.12.107+deb13-amd64 |
| compiler / build tool | gcc (Debian 14.2.0-19) 14.2.0 / cmake version 3.31.6 |
| campaign start / end (UTC) | 2026-09-25T10:02:21 / 2026-09-28T10:51:21 |

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
| `SQIsign2D-lvl1` | keygen | 558.85 M | 268 ms | 3.73 | 267 ms | 100 (5 × 20) |
| `SQIsign2D-lvl1` | sign | 619.25 M | 299 ms | 3.35 | 296 ms | 100 (5 × 20) |
| `SQIsign2D-lvl1` | verify | 135.94 M | 65 ms | 15.4 | 65.1 ms | 100 (5 × 20) |
| `SQIsign2D-lvl2` | keygen | 1.85 G | 887 ms | 1.13 | 889 ms | 100 (5 × 20) |
| `SQIsign2D-lvl2` | sign | 3.29 G | 1.57 s | 0.637 | 1.57 s | 100 (5 × 20) |
| `SQIsign2D-lvl2` | verify | 438.06 M | 210 ms | 4.77 | 210 ms | 100 (5 × 20) |
| `SQIsign2D-lvl3` | keygen | 5.57 G | 2.67 s | 0.375 | 2.67 s | 100 (5 × 20) |
| `SQIsign2D-lvl3` | sign | 5.32 G | 2.55 s | 0.392 | 2.56 s | 100 (5 × 20) |
| `SQIsign2D-lvl3` | verify | 1.33 G | 640 ms | 1.56 | 643 ms | 100 (5 × 20) |
| `SQIsign2D-lvl4` | keygen | 44.58 G | 21.4 s | 0.0467 | 21.4 s | 40 (5 × 8) |
| `SQIsign2D-lvl4` | sign | 39.28 G | 18.9 s | 0.053 | 18.8 s | 45 (5 × 9) |
| `SQIsign2D-lvl4` | verify | 9.78 G | 4.69 s | 0.213 | 4.69 s | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `SQIsign2D-lvl1` | keygen | 64467189 | 1968 KiB | 2864 KiB |
| `SQIsign2D-lvl1` | sign | 64467189 | 2696 KiB | 9208 KiB |
| `SQIsign2D-lvl1` | verify | 64467189 | 3204 KiB | 9212 KiB |
| `SQIsign2D-lvl2` | keygen | 96558053 | 1976 KiB | 3056 KiB |
| `SQIsign2D-lvl2` | sign | 96558053 | 2880 KiB | 16712 KiB |
| `SQIsign2D-lvl2` | verify | 96558053 | 3808 KiB | 17184 KiB |
| `SQIsign2D-lvl3` | keygen | – | 2004 KiB | 3244 KiB |
| `SQIsign2D-lvl3` | sign | – | 3032 KiB | 28184 KiB |
| `SQIsign2D-lvl3` | verify | – | 4584 KiB | 29196 KiB |
| `SQIsign2D-lvl4` | keygen | 273579493 | 1988 KiB | 4180 KiB |
| `SQIsign2D-lvl4` | sign | 273579493 | 4000 KiB | 52152 KiB |
| `SQIsign2D-lvl4` | verify | 273579493 | 11944 KiB | 117208 KiB |

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
| `SQIsign2D-lvl1` | keygen | 0.0% | 0.5% | drng 595 |
| `SQIsign2D-lvl1` | sign | 0.0% | 1.8% | drng 2.59e+03, pseudoXOF 1 |
| `SQIsign2D-lvl1` | verify | 0.0% | 0.0% | pseudoXOF 1 |
| `SQIsign2D-lvl2` | keygen | 0.0% | 0.3% | drng 1.27e+03 |
| `SQIsign2D-lvl2` | sign | 0.0% | 0.5% | drng 6.11e+03, pseudoXOF 1.67 |
| `SQIsign2D-lvl2` | verify | 0.0% | 0.0% | pseudoXOF 1 |
| `SQIsign2D-lvl3` | keygen | 0.0% | 0.2% | drng 1.54e+03 |
| `SQIsign2D-lvl3` | sign | 0.0% | 1.1% | drng 1.17e+04, pseudoXOF 1 |
| `SQIsign2D-lvl3` | verify | 0.0% | 0.0% | pseudoXOF 1 |
| `SQIsign2D-lvl4` | keygen | 0.0% | 0.1% | drng 4.22e+03 |
| `SQIsign2D-lvl4` | sign | 0.0% | 0.2% | drng 9.46e+03, pseudoXOF 1 |
| `SQIsign2D-lvl4` | verify | 0.0% | 0.0% | pseudoXOF 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `SQIsign2D-lvl1` | KAT log (sha256 `0ec44c0ac13d36d5…`) | `kat/sign-26/SQIsign2D-lvl1.log` |
| `SQIsign2D-lvl1` | timing keygen | `records/sign-26/SQIsign2D-lvl1__keygen.json` |
| `SQIsign2D-lvl1` | timing sign | `records/sign-26/SQIsign2D-lvl1__sign.json` |
| `SQIsign2D-lvl1` | timing verify | `records/sign-26/SQIsign2D-lvl1__verify.json` |
| `SQIsign2D-lvl1` | hash profile keygen | `profile/sign-26/SQIsign2D-lvl1__keygen.json` |
| `SQIsign2D-lvl1` | hash profile sign | `profile/sign-26/SQIsign2D-lvl1__sign.json` |
| `SQIsign2D-lvl1` | hash profile verify | `profile/sign-26/SQIsign2D-lvl1__verify.json` |
| `SQIsign2D-lvl2` | KAT log (sha256 `2d9d2f78a339a238…`) | `kat/sign-26/SQIsign2D-lvl2.log` |
| `SQIsign2D-lvl2` | timing keygen | `records/sign-26/SQIsign2D-lvl2__keygen.json` |
| `SQIsign2D-lvl2` | timing sign | `records/sign-26/SQIsign2D-lvl2__sign.json` |
| `SQIsign2D-lvl2` | timing verify | `records/sign-26/SQIsign2D-lvl2__verify.json` |
| `SQIsign2D-lvl2` | hash profile keygen | `profile/sign-26/SQIsign2D-lvl2__keygen.json` |
| `SQIsign2D-lvl2` | hash profile sign | `profile/sign-26/SQIsign2D-lvl2__sign.json` |
| `SQIsign2D-lvl2` | hash profile verify | `profile/sign-26/SQIsign2D-lvl2__verify.json` |
| `SQIsign2D-lvl3` | KAT log (sha256 `2199418541c0f31c…`) | `kat/sign-26/SQIsign2D-lvl3.log` |
| `SQIsign2D-lvl3` | timing keygen | `records/sign-26/SQIsign2D-lvl3__keygen.json` |
| `SQIsign2D-lvl3` | timing sign | `records/sign-26/SQIsign2D-lvl3__sign.json` |
| `SQIsign2D-lvl3` | timing verify | `records/sign-26/SQIsign2D-lvl3__verify.json` |
| `SQIsign2D-lvl3` | hash profile keygen | `profile/sign-26/SQIsign2D-lvl3__keygen.json` |
| `SQIsign2D-lvl3` | hash profile sign | `profile/sign-26/SQIsign2D-lvl3__sign.json` |
| `SQIsign2D-lvl3` | hash profile verify | `profile/sign-26/SQIsign2D-lvl3__verify.json` |
| `SQIsign2D-lvl4` | KAT log (sha256 `1c91fcd66cb8d45b…`) | `kat/sign-26/SQIsign2D-lvl4.log` |
| `SQIsign2D-lvl4` | timing keygen | `records/sign-26/SQIsign2D-lvl4__keygen.json` |
| `SQIsign2D-lvl4` | timing sign | `records/sign-26/SQIsign2D-lvl4__sign.json` |
| `SQIsign2D-lvl4` | timing verify | `records/sign-26/SQIsign2D-lvl4__verify.json` |
| `SQIsign2D-lvl4` | hash profile keygen | `profile/sign-26/SQIsign2D-lvl4__keygen.json` |
| `SQIsign2D-lvl4` | hash profile sign | `profile/sign-26/SQIsign2D-lvl4__sign.json` |
| `SQIsign2D-lvl4` | hash profile verify | `profile/sign-26/SQIsign2D-lvl4__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

