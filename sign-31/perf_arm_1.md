# sign-31 TSUOV — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-31` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561105597419520.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: TSUOV
- Implementation versions measured: reference
- Parameter sets: `TSUOV_128`, `TSUOV_256`, `TSUOV_512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-31/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `TSUOV_128` | guide | PASS |
| `TSUOV_256` | guide | PASS |
| `TSUOV_512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `TSUOV_128` | keygen | 9.63 M | 3.57 ms | 280 | 3.57 ms | 850 (5 × 170) |
| `TSUOV_128` | sign | 11.95 M | 4.43 ms | 226 | 4.43 ms | 695 (5 × 139) |
| `TSUOV_128` | verify | 10.50 M | 3.9 ms | 257 | 3.88 ms | 810 (5 × 162) |
| `TSUOV_256` | keygen | 62.40 M | 23.2 ms | 43.2 | 23 ms | 140 (5 × 28) |
| `TSUOV_256` | sign | 78.30 M | 29 ms | 34.4 | 29.1 ms | 110 (5 × 22) |
| `TSUOV_256` | verify | 70.79 M | 26.3 ms | 38.1 | 26.3 ms | 120 (5 × 24) |
| `TSUOV_512` | keygen | 801.46 M | 297 ms | 3.36 | 297 ms | 100 (5 × 20) |
| `TSUOV_512` | sign | 798.30 M | 296 ms | 3.38 | 296 ms | 100 (5 × 20) |
| `TSUOV_512` | verify | 785.82 M | 292 ms | 3.43 | 291 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `TSUOV_128` | keygen | 35116 | 1428 KiB | 1504 KiB |
| `TSUOV_128` | sign | 35116 | 3468 KiB | 3584 KiB |
| `TSUOV_128` | verify | 35116 | 1488 KiB | 1560 KiB |
| `TSUOV_256` | keygen | 47276 | 1432 KiB | 1568 KiB |
| `TSUOV_256` | sign | 47276 | 1504 KiB | 1732 KiB |
| `TSUOV_256` | verify | 47276 | 1644 KiB | 1712 KiB |
| `TSUOV_512` | keygen | 158216 | 1448 KiB | 1812 KiB |
| `TSUOV_512` | sign | 158216 | 1744 KiB | 2072 KiB |
| `TSUOV_512` | verify | 158216 | 2008 KiB | 2132 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | signature |
|---|---|---|---|
| `TSUOV_128` | 779 | 32 | 896 |
| `TSUOV_256` | 2170 | 64 | 2412 |
| `TSUOV_512` | 21319 | 128 | 2939 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `TSUOV_128` | keygen | 78% | 0.1% | drng 2, pseudoXOF 123 |
| `TSUOV_128` | sign | 64% | 0.1% | drng 3, pseudoXOF 126, pseudohash 1, sm3hash 1.52 |
| `TSUOV_128` | verify | 71% | 0.0% | pseudoXOF 123, pseudohash 1 |
| `TSUOV_256` | keygen | 77% | 0.0% | drng 2, pseudoXOF 229 |
| `TSUOV_256` | sign | 61% | 0.0% | drng 3, pseudoXOF 233, pseudohash 1, sm3hash 3.23 |
| `TSUOV_256` | verify | 68% | 0.0% | pseudoXOF 229, pseudohash 1 |
| `TSUOV_512` | keygen | 81% | 0.0% | drng 2, pseudoXOF 437 |
| `TSUOV_512` | sign | 81% | 0.0% | drng 3, pseudoXOF 439, pseudohash 1, sm3hash 2 |
| `TSUOV_512` | verify | 83% | 0.0% | pseudoXOF 437, pseudohash 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `TSUOV_128` | KAT log (sha256 `c12bf323184e6b0c…`) | `kat/sign-31/TSUOV_128.log` |
| `TSUOV_128` | timing keygen | `records/sign-31/TSUOV_128__keygen.json` |
| `TSUOV_128` | timing sign | `records/sign-31/TSUOV_128__sign.json` |
| `TSUOV_128` | timing verify | `records/sign-31/TSUOV_128__verify.json` |
| `TSUOV_128` | hash profile keygen | `profile/sign-31/TSUOV_128__keygen.json` |
| `TSUOV_128` | hash profile sign | `profile/sign-31/TSUOV_128__sign.json` |
| `TSUOV_128` | hash profile verify | `profile/sign-31/TSUOV_128__verify.json` |
| `TSUOV_256` | KAT log (sha256 `24a9505d93ae66e7…`) | `kat/sign-31/TSUOV_256.log` |
| `TSUOV_256` | timing keygen | `records/sign-31/TSUOV_256__keygen.json` |
| `TSUOV_256` | timing sign | `records/sign-31/TSUOV_256__sign.json` |
| `TSUOV_256` | timing verify | `records/sign-31/TSUOV_256__verify.json` |
| `TSUOV_256` | hash profile keygen | `profile/sign-31/TSUOV_256__keygen.json` |
| `TSUOV_256` | hash profile sign | `profile/sign-31/TSUOV_256__sign.json` |
| `TSUOV_256` | hash profile verify | `profile/sign-31/TSUOV_256__verify.json` |
| `TSUOV_512` | KAT log (sha256 `8a5e6176e1a0593d…`) | `kat/sign-31/TSUOV_512.log` |
| `TSUOV_512` | timing keygen | `records/sign-31/TSUOV_512__keygen.json` |
| `TSUOV_512` | timing sign | `records/sign-31/TSUOV_512__sign.json` |
| `TSUOV_512` | timing verify | `records/sign-31/TSUOV_512__verify.json` |
| `TSUOV_512` | hash profile keygen | `profile/sign-31/TSUOV_512__keygen.json` |
| `TSUOV_512` | hash profile sign | `profile/sign-31/TSUOV_512__sign.json` |
| `TSUOV_512` | hash profile verify | `profile/sign-31/TSUOV_512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

