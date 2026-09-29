# sign-07 CS — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-07` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561077046792192.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: CS
- Implementation versions measured: reference
- Parameter sets: `CS-128`, `CS-256`, `CS-512`

## 2. Assessment environment

| item | value |
|---|---|
| processor | Qualcomm Oryon (CPU 2, one core) |
| machine | ASUS Vivobook S 15 |
| clock | fixed 2.71 GHz (governor performance, minimum = maximum), boost off, SMT none |
| memory | 30562 MiB |
| OS / kernel | Ubuntu 26.04.1 LTS / 7.0.0-34-generic |
| compiler / build tool | gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0 / cmake version 4.2.3 |
| campaign start / end (UTC) | 2026-09-28T15:05:46 / 2026-09-29T02:47:54 |

## 3. Functional testing (KAT)

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-07/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `CS-128` | guide | PASS |
| `CS-256` | guide | PASS |
| `CS-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `CS-128` | keygen | 487.3 k | 181 µs | 5.53e+03 | 181 µs | 15580 (5 × 3116) |
| `CS-128` | sign | 3.72 M | 1.38 ms | 724 | 1.37 ms | 3135 (5 × 627) |
| `CS-128` | verify | 522.9 k | 194 µs | 5.15e+03 | 194 µs | 15660 (5 × 3132) |
| `CS-256` | keygen | 797.0 k | 296 µs | 3.38e+03 | 296 µs | 9985 (5 × 1997) |
| `CS-256` | sign | 10.86 M | 4.03 ms | 248 | 4.03 ms | 570 (5 × 114) |
| `CS-256` | verify | 997.6 k | 370 µs | 2.7e+03 | 368 µs | 8320 (5 × 1664) |
| `CS-512` | keygen | 2.60 M | 964 µs | 1.04e+03 | 964 µs | 3135 (5 × 627) |
| `CS-512` | sign | 41.58 M | 15.4 ms | 64.8 | 15.4 ms | 210 (5 × 42) |
| `CS-512` | verify | 3.29 M | 1.22 ms | 819 | 1.22 ms | 2595 (5 × 519) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `CS-128` | keygen | 93472 | 1460 KiB | 1552 KiB |
| `CS-128` | sign | 93472 | 1488 KiB | 1608 KiB |
| `CS-128` | verify | 93472 | 1540 KiB | 1604 KiB |
| `CS-256` | keygen | 125496 | 1464 KiB | 1580 KiB |
| `CS-256` | sign | 125496 | 1516 KiB | 1704 KiB |
| `CS-256` | verify | 125496 | 1632 KiB | 1696 KiB |
| `CS-512` | keygen | 255668 | 1472 KiB | 1660 KiB |
| `CS-512` | sign | 255668 | 1592 KiB | 1952 KiB |
| `CS-512` | verify | 255668 | 1888 KiB | 1952 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | signature |
|---|---|---|---|
| `CS-128` | 976 | 1888 | 1548 |
| `CS-256` | 1760 | 3968 | 3164 |
| `CS-512` | 4288 | 7808 | 5975 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `CS-128` | keygen | 4.7% | 61% | drng 16, pseudoXOF 2 |
| `CS-128` | sign | 62% | 14% | drng 75.6, pseudoXOF 16.5 |
| `CS-128` | verify | 6.9% | 62% | drng 33, pseudoXOF 3 |
| `CS-256` | keygen | 10% | 63% | drng 16, pseudoXOF 2 |
| `CS-256` | sign | 57% | 10% | drng 155, pseudoXOF 18 |
| `CS-256` | verify | 12% | 63% | drng 54, pseudoXOF 3 |
| `CS-512` | keygen | 15% | 60% | drng 42.1, pseudoXOF 2 |
| `CS-512` | sign | 69% | 7.0% | drng 410, pseudoXOF 31.2 |
| `CS-512` | verify | 16% | 62% | drng 171, pseudoXOF 3 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `CS-128` | KAT log (sha256 `9934c932cc4177eb…`) | `kat/sign-07/CS-128.log` |
| `CS-128` | timing keygen | `records/sign-07/CS-128__keygen.json` |
| `CS-128` | timing sign | `records/sign-07/CS-128__sign.json` |
| `CS-128` | timing verify | `records/sign-07/CS-128__verify.json` |
| `CS-128` | hash profile keygen | `profile/sign-07/CS-128__keygen.json` |
| `CS-128` | hash profile sign | `profile/sign-07/CS-128__sign.json` |
| `CS-128` | hash profile verify | `profile/sign-07/CS-128__verify.json` |
| `CS-256` | KAT log (sha256 `b3e4e6d6b9227c58…`) | `kat/sign-07/CS-256.log` |
| `CS-256` | timing keygen | `records/sign-07/CS-256__keygen.json` |
| `CS-256` | timing sign | `records/sign-07/CS-256__sign.json` |
| `CS-256` | timing verify | `records/sign-07/CS-256__verify.json` |
| `CS-256` | hash profile keygen | `profile/sign-07/CS-256__keygen.json` |
| `CS-256` | hash profile sign | `profile/sign-07/CS-256__sign.json` |
| `CS-256` | hash profile verify | `profile/sign-07/CS-256__verify.json` |
| `CS-512` | KAT log (sha256 `1f4ecd9cebbd2cc4…`) | `kat/sign-07/CS-512.log` |
| `CS-512` | timing keygen | `records/sign-07/CS-512__keygen.json` |
| `CS-512` | timing sign | `records/sign-07/CS-512__sign.json` |
| `CS-512` | timing verify | `records/sign-07/CS-512__verify.json` |
| `CS-512` | hash profile keygen | `profile/sign-07/CS-512__keygen.json` |
| `CS-512` | hash profile sign | `profile/sign-07/CS-512__sign.json` |
| `CS-512` | hash profile verify | `profile/sign-07/CS-512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

