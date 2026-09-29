# sign-15 MORNING-ATLAS — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-15` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561078120534016.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: MORNING-ATLAS
- Implementation versions measured: reference
- Parameter sets: `lwrdsa128`, `lwrdsa192`, `lwrdsa256`, `lwrdsa512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-15/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `lwrdsa128` | harness-default | OVERFLOW [1] |
| `lwrdsa192` | harness-default | OVERFLOW [1] |
| `lwrdsa256` | harness-default | OVERFLOW [1] |
| `lwrdsa512` | harness-default | OVERFLOW [1] |

[1] OVERFLOW: sig_sign returns (and writes) signatures up to 64 bytes longer than the declared maximum sn length (confirmed finding sign-15-1, out-of-bounds heap disclosure); timed with a 4 KiB guard buffer and the excess recorded. In addition, sig_verify accepted the honest signature once and then rejected the same unchanged inputs on later calls, so the verification time is that of calls with inconsistent verdicts (counted as verify_rejections in each record). These instances are timed anyway; their output is not validated.

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `lwrdsa128` | keygen | 2.49 M | 924 µs | 1.08e+03 | 924 µs | 3320 (5 × 664) |
| `lwrdsa128` | sign | 3.39 M | 1.26 ms | 794 | 1.26 ms | 2390 (5 × 478) |
| `lwrdsa128` | verify | 2.57 M | 955 µs | 1.05e+03 | 955 µs | 3280 (5 × 656) |
| `lwrdsa192` | keygen | 5.93 M | 2.2 ms | 455 | 2.19 ms | 1420 (5 × 284) |
| `lwrdsa192` | sign | 7.75 M | 2.88 ms | 348 | 2.86 ms | 1095 (5 × 219) |
| `lwrdsa192` | verify | 6.10 M | 2.26 ms | 442 | 2.26 ms | 1395 (5 × 279) |
| `lwrdsa256` | keygen | 3.48 M | 1.29 ms | 775 | 1.29 ms | 1815 (5 × 363) |
| `lwrdsa256` | sign | 9.43 M | 3.5 ms | 286 | 3.5 ms | 700 (5 × 140) |
| `lwrdsa256` | verify | 3.62 M | 1.34 ms | 744 | 1.34 ms | 1855 (5 × 371) |
| `lwrdsa512` | keygen | 10.35 M | 3.84 ms | 260 | 3.84 ms | 635 (5 × 127) |
| `lwrdsa512` | sign | 35.69 M | 13.2 ms | 75.5 | 13.2 ms | 190 (5 × 38) |
| `lwrdsa512` | verify | 10.83 M | 4.02 ms | 249 | 4.02 ms | 625 (5 × 125) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `lwrdsa128` | keygen | 35468 | 1436 KiB | 1548 KiB |
| `lwrdsa128` | sign | 35468 | 3508 KiB | 3620 KiB |
| `lwrdsa128` | verify | 35468 | 1532 KiB | 1600 KiB |
| `lwrdsa192` | keygen | 31332 | 1428 KiB | 1596 KiB |
| `lwrdsa192` | sign | 31332 | 3560 KiB | 3692 KiB |
| `lwrdsa192` | verify | 31332 | 1604 KiB | 1672 KiB |
| `lwrdsa256` | keygen | 31604 | 1436 KiB | 1608 KiB |
| `lwrdsa256` | sign | 31604 | 1544 KiB | 1708 KiB |
| `lwrdsa256` | verify | 31604 | 1632 KiB | 1696 KiB |
| `lwrdsa512` | keygen | 33404 | 1448 KiB | 1760 KiB |
| `lwrdsa512` | sign | 33404 | 1692 KiB | 1940 KiB |
| `lwrdsa512` | verify | 33404 | 1868 KiB | 1940 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | signature |
|---|---|---|---|
| `lwrdsa128` | 1328 | 2128 | 2081 |
| `lwrdsa192` | 2112 | 3152 | 3365 |
| `lwrdsa256` | 2848 | 4016 | 4656 |
| `lwrdsa512` | 6688 | 7920 | 10081 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `lwrdsa128` | keygen | 86% | 0.2% | drng 1, pseudoXOF 62 |
| `lwrdsa128` | sign | 74% | 0.0% | pseudoXOF 62 |
| `lwrdsa128` | verify | 84% | 0.0% | pseudoXOF 57 |
| `lwrdsa192` | keygen | 86% | 0.1% | drng 1, pseudoXOF 142 |
| `lwrdsa192` | sign | 75% | 0.0% | pseudoXOF 143 |
| `lwrdsa192` | verify | 85% | 0.0% | pseudoXOF 134 |
| `lwrdsa256` | keygen | 67% | 0.1% | drng 1, pseudoXOF 72 |
| `lwrdsa256` | sign | 42% | 0.0% | pseudoXOF 81 |
| `lwrdsa256` | verify | 65% | 0.0% | pseudoXOF 59 |
| `lwrdsa512` | keygen | 65% | 0.0% | drng 1, pseudoXOF 86 |
| `lwrdsa512` | sign | 52% | 0.0% | pseudoXOF 227 |
| `lwrdsa512` | verify | 62% | 0.0% | pseudoXOF 59 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `lwrdsa128` | KAT log (sha256 `bc08acca98ef36d5…`) | `kat/sign-15/lwrdsa128.log` |
| `lwrdsa128` | timing keygen | `records/sign-15/lwrdsa128__keygen.json` |
| `lwrdsa128` | timing sign | `records/sign-15/lwrdsa128__sign.json` |
| `lwrdsa128` | timing verify | `records/sign-15/lwrdsa128__verify.json` |
| `lwrdsa128` | hash profile keygen | `profile/sign-15/lwrdsa128__keygen.json` |
| `lwrdsa128` | hash profile sign | `profile/sign-15/lwrdsa128__sign.json` |
| `lwrdsa128` | hash profile verify | `profile/sign-15/lwrdsa128__verify.json` |
| `lwrdsa192` | KAT log (sha256 `b250523772e37f23…`) | `kat/sign-15/lwrdsa192.log` |
| `lwrdsa192` | timing keygen | `records/sign-15/lwrdsa192__keygen.json` |
| `lwrdsa192` | timing sign | `records/sign-15/lwrdsa192__sign.json` |
| `lwrdsa192` | timing verify | `records/sign-15/lwrdsa192__verify.json` |
| `lwrdsa192` | hash profile keygen | `profile/sign-15/lwrdsa192__keygen.json` |
| `lwrdsa192` | hash profile sign | `profile/sign-15/lwrdsa192__sign.json` |
| `lwrdsa192` | hash profile verify | `profile/sign-15/lwrdsa192__verify.json` |
| `lwrdsa256` | KAT log (sha256 `d0adf95410eb90ac…`) | `kat/sign-15/lwrdsa256.log` |
| `lwrdsa256` | timing keygen | `records/sign-15/lwrdsa256__keygen.json` |
| `lwrdsa256` | timing sign | `records/sign-15/lwrdsa256__sign.json` |
| `lwrdsa256` | timing verify | `records/sign-15/lwrdsa256__verify.json` |
| `lwrdsa256` | hash profile keygen | `profile/sign-15/lwrdsa256__keygen.json` |
| `lwrdsa256` | hash profile sign | `profile/sign-15/lwrdsa256__sign.json` |
| `lwrdsa256` | hash profile verify | `profile/sign-15/lwrdsa256__verify.json` |
| `lwrdsa512` | KAT log (sha256 `86ce37323564f7d4…`) | `kat/sign-15/lwrdsa512.log` |
| `lwrdsa512` | timing keygen | `records/sign-15/lwrdsa512__keygen.json` |
| `lwrdsa512` | timing sign | `records/sign-15/lwrdsa512__sign.json` |
| `lwrdsa512` | timing verify | `records/sign-15/lwrdsa512__verify.json` |
| `lwrdsa512` | hash profile keygen | `profile/sign-15/lwrdsa512__keygen.json` |
| `lwrdsa512` | hash profile sign | `profile/sign-15/lwrdsa512__sign.json` |
| `lwrdsa512` | hash profile verify | `profile/sign-15/lwrdsa512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

