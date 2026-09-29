# sign-27 SQIsignTriangle — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-27` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561096613220352.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: SQIsignTriangle
- Implementation versions measured: reference
- Parameter sets: `SQIsignTriangle_lvl1`, `SQIsignTriangle_lvl2`, `SQIsignTriangle_lvl5`, `SQIsignTriangle_lvl6`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-27/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `SQIsignTriangle_lvl1` | guide | PASS |
| `SQIsignTriangle_lvl2` | guide | PASS |
| `SQIsignTriangle_lvl5` | guide | PASS |
| `SQIsignTriangle_lvl6` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `SQIsignTriangle_lvl1` | keygen | 96.61 M | 35.8 ms | 27.9 | 35.8 ms | 100 (5 × 20) |
| `SQIsignTriangle_lvl1` | sign | 185.52 M | 68.8 ms | 14.5 | 67.8 ms | 100 (5 × 20) |
| `SQIsignTriangle_lvl1` | verify | 16.49 M | 6.12 ms | 163 | 6.12 ms | 495 (5 × 99) |
| `SQIsignTriangle_lvl2` | keygen | 148.83 M | 55.2 ms | 18.1 | 55.3 ms | 100 (5 × 20) |
| `SQIsignTriangle_lvl2` | sign | 343.83 M | 128 ms | 7.84 | 124 ms | 100 (5 × 20) |
| `SQIsignTriangle_lvl2` | verify | 25.75 M | 9.55 ms | 105 | 9.54 ms | 330 (5 × 66) |
| `SQIsignTriangle_lvl5` | keygen | 477.10 M | 177 ms | 5.65 | 170 ms | 100 (5 × 20) |
| `SQIsignTriangle_lvl5` | sign | 913.34 M | 339 ms | 2.95 | 339 ms | 100 (5 × 20) |
| `SQIsignTriangle_lvl5` | verify | 134.00 M | 49.7 ms | 20.1 | 49.8 ms | 100 (5 × 20) |
| `SQIsignTriangle_lvl6` | keygen | 3.51 G | 1.3 s | 0.768 | 1.3 s | 100 (5 × 20) |
| `SQIsignTriangle_lvl6` | sign | 9.03 G | 3.35 s | 0.298 | 3.25 s | 100 (5 × 20) |
| `SQIsignTriangle_lvl6` | verify | 3.06 G | 1.13 s | 0.881 | 1.13 s | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `SQIsignTriangle_lvl1` | keygen | 262712 | 1668 KiB | 2564 KiB |
| `SQIsignTriangle_lvl1` | sign | 262712 | 2380 KiB | 4560 KiB |
| `SQIsignTriangle_lvl1` | verify | 262712 | 4492 KiB | 4556 KiB |
| `SQIsignTriangle_lvl2` | keygen | 255696 | 1656 KiB | 4544 KiB |
| `SQIsignTriangle_lvl2` | sign | 255696 | 2408 KiB | 4832 KiB |
| `SQIsignTriangle_lvl2` | verify | 255696 | 4584 KiB | 4648 KiB |
| `SQIsignTriangle_lvl5` | keygen | 262996 | 1668 KiB | 4576 KiB |
| `SQIsignTriangle_lvl5` | sign | 262996 | 4476 KiB | 4548 KiB |
| `SQIsignTriangle_lvl5` | verify | 262996 | 4668 KiB | 4732 KiB |
| `SQIsignTriangle_lvl6` | keygen | 273488 | 1680 KiB | 4632 KiB |
| `SQIsignTriangle_lvl6` | sign | 273488 | 2680 KiB | 4964 KiB |
| `SQIsignTriangle_lvl6` | verify | 273488 | 2956 KiB | 3056 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | signature |
|---|---|---|---|
| `SQIsignTriangle_lvl1` | 65 | 353 | 204 |
| `SQIsignTriangle_lvl2` | 81 | 437 | 255 |
| `SQIsignTriangle_lvl5` | 129 | 701 | 408 |
| `SQIsignTriangle_lvl6` | 257 | 1409 | 816 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — `shake*` names are shims over pseudoXOF

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `SQIsignTriangle_lvl1` | keygen | 0.0% | 7.5% | drng 1.62e+03 |
| `SQIsignTriangle_lvl1` | sign | 3.0% | 8.4% | drng 3.56e+03, pseudoXOF 49.3 |
| `SQIsignTriangle_lvl1` | verify | 4.0% | 0.0% | pseudoXOF 21 |
| `SQIsignTriangle_lvl2` | keygen | 0.0% | 4.6% | drng 1.59e+03 |
| `SQIsignTriangle_lvl2` | sign | 6.1% | 5.1% | drng 4.59e+03, pseudoXOF 76.9 |
| `SQIsignTriangle_lvl2` | verify | 0.4% | 0.0% | pseudoXOF 7 |
| `SQIsignTriangle_lvl5` | keygen | 0.0% | 2.9% | drng 3.13e+03 |
| `SQIsignTriangle_lvl5` | sign | 4.4% | 3.4% | drng 8.01e+03, pseudoXOF 67.8 |
| `SQIsignTriangle_lvl5` | verify | 42% | 0.0% | pseudoXOF 121 |
| `SQIsignTriangle_lvl6` | keygen | 0.0% | 0.7% | drng 4.72e+03 |
| `SQIsignTriangle_lvl6` | sign | 2.1% | 0.6% | drng 1.07e+04, pseudoXOF 119 |
| `SQIsignTriangle_lvl6` | verify | 84% | 0.0% | pseudoXOF 506 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `SQIsignTriangle_lvl1` | KAT log (sha256 `50654ed991102bb7…`) | `kat/sign-27/SQIsignTriangle_lvl1.log` |
| `SQIsignTriangle_lvl1` | timing keygen | `records/sign-27/SQIsignTriangle_lvl1__keygen.json` |
| `SQIsignTriangle_lvl1` | timing sign | `records/sign-27/SQIsignTriangle_lvl1__sign.json` |
| `SQIsignTriangle_lvl1` | timing verify | `records/sign-27/SQIsignTriangle_lvl1__verify.json` |
| `SQIsignTriangle_lvl1` | hash profile keygen | `profile/sign-27/SQIsignTriangle_lvl1__keygen.json` |
| `SQIsignTriangle_lvl1` | hash profile sign | `profile/sign-27/SQIsignTriangle_lvl1__sign.json` |
| `SQIsignTriangle_lvl1` | hash profile verify | `profile/sign-27/SQIsignTriangle_lvl1__verify.json` |
| `SQIsignTriangle_lvl2` | KAT log (sha256 `abdd3dc441e83e18…`) | `kat/sign-27/SQIsignTriangle_lvl2.log` |
| `SQIsignTriangle_lvl2` | timing keygen | `records/sign-27/SQIsignTriangle_lvl2__keygen.json` |
| `SQIsignTriangle_lvl2` | timing sign | `records/sign-27/SQIsignTriangle_lvl2__sign.json` |
| `SQIsignTriangle_lvl2` | timing verify | `records/sign-27/SQIsignTriangle_lvl2__verify.json` |
| `SQIsignTriangle_lvl2` | hash profile keygen | `profile/sign-27/SQIsignTriangle_lvl2__keygen.json` |
| `SQIsignTriangle_lvl2` | hash profile sign | `profile/sign-27/SQIsignTriangle_lvl2__sign.json` |
| `SQIsignTriangle_lvl2` | hash profile verify | `profile/sign-27/SQIsignTriangle_lvl2__verify.json` |
| `SQIsignTriangle_lvl5` | KAT log (sha256 `cd59356ad08ce15a…`) | `kat/sign-27/SQIsignTriangle_lvl5.log` |
| `SQIsignTriangle_lvl5` | timing keygen | `records/sign-27/SQIsignTriangle_lvl5__keygen.json` |
| `SQIsignTriangle_lvl5` | timing sign | `records/sign-27/SQIsignTriangle_lvl5__sign.json` |
| `SQIsignTriangle_lvl5` | timing verify | `records/sign-27/SQIsignTriangle_lvl5__verify.json` |
| `SQIsignTriangle_lvl5` | hash profile keygen | `profile/sign-27/SQIsignTriangle_lvl5__keygen.json` |
| `SQIsignTriangle_lvl5` | hash profile sign | `profile/sign-27/SQIsignTriangle_lvl5__sign.json` |
| `SQIsignTriangle_lvl5` | hash profile verify | `profile/sign-27/SQIsignTriangle_lvl5__verify.json` |
| `SQIsignTriangle_lvl6` | KAT log (sha256 `a74525bc7fb68cd1…`) | `kat/sign-27/SQIsignTriangle_lvl6.log` |
| `SQIsignTriangle_lvl6` | timing keygen | `records/sign-27/SQIsignTriangle_lvl6__keygen.json` |
| `SQIsignTriangle_lvl6` | timing sign | `records/sign-27/SQIsignTriangle_lvl6__sign.json` |
| `SQIsignTriangle_lvl6` | timing verify | `records/sign-27/SQIsignTriangle_lvl6__verify.json` |
| `SQIsignTriangle_lvl6` | hash profile keygen | `profile/sign-27/SQIsignTriangle_lvl6__keygen.json` |
| `SQIsignTriangle_lvl6` | hash profile sign | `profile/sign-27/SQIsignTriangle_lvl6__sign.json` |
| `SQIsignTriangle_lvl6` | hash profile verify | `profile/sign-27/SQIsignTriangle_lvl6__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

