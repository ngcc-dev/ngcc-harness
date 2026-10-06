# sign-17 OPS Digital Signature Algorithm — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-17` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561086848880640.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: OPS Digital Signature Algorithm
- Implementation versions measured: reference
- Parameter sets: `OPSsig-128`, `OPSsig-256`, `OPSsig-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-17/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `OPSsig-128` | guide | PASS |
| `OPSsig-256` | guide | PASS |
| `OPSsig-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `OPSsig-128` | keygen | 2.22 M | 822 µs | 1.22e+03 | 813 µs | 3880 (5 × 776) |
| `OPSsig-128` | sign | 2.52 M | 936 µs | 1.07e+03 | 935 µs | 3350 (5 × 670) |
| `OPSsig-128` | verify | 2.05 M | 759 µs | 1.32e+03 | 759 µs | 3960 (5 × 792) |
| `OPSsig-256` | keygen | 3.73 M | 1.38 ms | 723 | 1.37 ms | 2230 (5 × 446) |
| `OPSsig-256` | sign | 4.91 M | 1.82 ms | 549 | 1.82 ms | 2035 (5 × 407) |
| `OPSsig-256` | verify | 3.47 M | 1.29 ms | 778 | 1.29 ms | 2445 (5 × 489) |
| `OPSsig-512` | keygen | 7.34 M | 2.72 ms | 367 | 2.7 ms | 1155 (5 × 231) |
| `OPSsig-512` | sign | 10.06 M | 3.73 ms | 268 | 3.73 ms | 685 (5 × 137) |
| `OPSsig-512` | verify | 6.89 M | 2.56 ms | 391 | 2.55 ms | 1180 (5 × 236) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `OPSsig-128` | keygen | 34288 | 1436 KiB | 1556 KiB |
| `OPSsig-128` | sign | 34288 | 1484 KiB | 1596 KiB |
| `OPSsig-128` | verify | 34288 | 1532 KiB | 1600 KiB |
| `OPSsig-256` | keygen | 34952 | 1444 KiB | 1588 KiB |
| `OPSsig-256` | sign | 34952 | 3552 KiB | 3688 KiB |
| `OPSsig-256` | verify | 34952 | 1588 KiB | 1660 KiB |
| `OPSsig-512` | keygen | 37440 | 1456 KiB | 1688 KiB |
| `OPSsig-512` | sign | 37440 | 1608 KiB | 1820 KiB |
| `OPSsig-512` | verify | 37440 | 1756 KiB | 1836 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `OPSsig-128` | 2560 | 3840 | 4349 |
| `OPSsig-256` | 3392 | 5056 | 5540 |
| `OPSsig-512` | 6720 | 9920 | 12021 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — optimized AVX2 auxfunc.c adds sm3x4 / pseudoXOF_4x

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `OPSsig-128` | keygen | 92% | 0.6% | drng 2, pseudoXOF 17, pseudohash 1 |
| `OPSsig-128` | sign | 90% | 0.3% | drng 1, pseudoXOF 15.8, pseudohash 1, sm3hash 1.13 |
| `OPSsig-128` | verify | 87% | 0.0% | pseudoXOF 12, pseudohash 2, sm3hash 1 |
| `OPSsig-256` | keygen | 93% | 0.4% | drng 2, pseudoXOF 27.5, pseudohash 1 |
| `OPSsig-256` | sign | 89% | 0.2% | drng 1, pseudoXOF 27.6, pseudohash 2.93 |
| `OPSsig-256` | verify | 90% | 0.0% | pseudoXOF 18, pseudohash 3 |
| `OPSsig-512` | keygen | 92% | 0.2% | drng 2, pseudoXOF 27.1, pseudohash 1 |
| `OPSsig-512` | sign | 87% | 0.1% | drng 1, pseudoXOF 28.6, pseudohash 3.1 |
| `OPSsig-512` | verify | 89% | 0.0% | pseudoXOF 18, pseudohash 3 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `OPSsig-128` | KAT log (sha256 `f692d363f21e7d87…`) | `kat/sign-17/OPSsig-128.log` |
| `OPSsig-128` | timing keygen | `records/sign-17/OPSsig-128__keygen.json` |
| `OPSsig-128` | timing sign | `records/sign-17/OPSsig-128__sign.json` |
| `OPSsig-128` | timing verify | `records/sign-17/OPSsig-128__verify.json` |
| `OPSsig-128` | hash profile keygen | `profile/sign-17/OPSsig-128__keygen.json` |
| `OPSsig-128` | hash profile sign | `profile/sign-17/OPSsig-128__sign.json` |
| `OPSsig-128` | hash profile verify | `profile/sign-17/OPSsig-128__verify.json` |
| `OPSsig-256` | KAT log (sha256 `3d42fb4c2d7e0fd9…`) | `kat/sign-17/OPSsig-256.log` |
| `OPSsig-256` | timing keygen | `records/sign-17/OPSsig-256__keygen.json` |
| `OPSsig-256` | timing sign | `records/sign-17/OPSsig-256__sign.json` |
| `OPSsig-256` | timing verify | `records/sign-17/OPSsig-256__verify.json` |
| `OPSsig-256` | hash profile keygen | `profile/sign-17/OPSsig-256__keygen.json` |
| `OPSsig-256` | hash profile sign | `profile/sign-17/OPSsig-256__sign.json` |
| `OPSsig-256` | hash profile verify | `profile/sign-17/OPSsig-256__verify.json` |
| `OPSsig-512` | KAT log (sha256 `6e2b4228f846b432…`) | `kat/sign-17/OPSsig-512.log` |
| `OPSsig-512` | timing keygen | `records/sign-17/OPSsig-512__keygen.json` |
| `OPSsig-512` | timing sign | `records/sign-17/OPSsig-512__sign.json` |
| `OPSsig-512` | timing verify | `records/sign-17/OPSsig-512__verify.json` |
| `OPSsig-512` | hash profile keygen | `profile/sign-17/OPSsig-512__keygen.json` |
| `OPSsig-512` | hash profile sign | `profile/sign-17/OPSsig-512__sign.json` |
| `OPSsig-512` | hash profile verify | `profile/sign-17/OPSsig-512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

