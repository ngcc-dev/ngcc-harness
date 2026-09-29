# sign-24 Sigurd — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-24` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561096235732992.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: Sigurd
- Implementation versions measured: reference
- Parameter sets: `Sigurd-128`, `Sigurd-256`, `Sigurd-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-24/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Sigurd-128` | guide | PASS |
| `Sigurd-256` | guide | PASS |
| `Sigurd-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Sigurd-128` | keygen | 15.95 M | 5.92 ms | 169 | 5.92 ms | 295 (5 × 59) |
| `Sigurd-128` | sign | 30.42 M | 11.3 ms | 88.6 | 11.2 ms | 245 (5 × 49) |
| `Sigurd-128` | verify | 13.81 M | 5.12 ms | 195 | 5.12 ms | 510 (5 × 102) |
| `Sigurd-256` | keygen | 86.23 M | 32 ms | 31.2 | 31.9 ms | 100 (5 × 20) |
| `Sigurd-256` | sign | 282.20 M | 105 ms | 9.55 | 104 ms | 100 (5 × 20) |
| `Sigurd-256` | verify | 100.85 M | 37.4 ms | 26.7 | 37.4 ms | 100 (5 × 20) |
| `Sigurd-512` | keygen | 461.63 M | 171 ms | 5.84 | 171 ms | 100 (5 × 20) |
| `Sigurd-512` | sign | 2.78 G | 1.03 s | 0.97 | 1.03 s | 100 (5 × 20) |
| `Sigurd-512` | verify | 922.08 M | 345 ms | 2.9 | 344 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Sigurd-128` | keygen | 771640 | 1504 KiB | 4436 KiB |
| `Sigurd-128` | sign | 771640 | 4348 KiB | 4476 KiB |
| `Sigurd-128` | verify | 771640 | 4504 KiB | 10808 KiB |
| `Sigurd-256` | keygen | 838352 | 1576 KiB | 5660 KiB |
| `Sigurd-256` | sign | 838352 | 4020 KiB | 8340 KiB |
| `Sigurd-256` | verify | 838352 | 6004 KiB | 32436 KiB |
| `Sigurd-512` | keygen | 1763548 | 3552 KiB | 17340 KiB |
| `Sigurd-512` | sign | 1763548 | 4524 KiB | 31388 KiB |
| `Sigurd-512` | verify | 1763548 | 17476 KiB | 131240 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | signature |
|---|---|---|---|
| `Sigurd-128` | 112 | 80 | 62868 |
| `Sigurd-256` | 212 | 128 | 137412 |
| `Sigurd-512` | 435 | 256 | 494532 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Sigurd-128` | keygen | 26% | 0.1% | drng 2, pseudoXOF 2 |
| `Sigurd-128` | sign | 36% | 1.6% | drng 2, pseudoXOF 7, sm3hash 3.15e+03 |
| `Sigurd-128` | verify | 45% | 0.0% | pseudoXOF 6, sm3hash 743 |
| `Sigurd-256` | keygen | 39% | 0.0% | drng 2, pseudoXOF 2 |
| `Sigurd-256` | sign | 62% | 0.4% | drng 2, pseudoXOF 7, pseudohash 1.04e+04 |
| `Sigurd-256` | verify | 51% | 0.0% | pseudoXOF 6, pseudohash 1.05e+03 |
| `Sigurd-512` | keygen | 46% | 0.0% | drng 2, pseudoXOF 2 |
| `Sigurd-512` | sign | 63% | 0.2% | drng 2, pseudoXOF 7, pseudohash 3.97e+04 |
| `Sigurd-512` | verify | 34% | 0.0% | pseudoXOF 6, pseudohash 1.96e+03 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Sigurd-128` | KAT log (sha256 `9e997f2b63c1c7e5…`) | `kat/sign-24/Sigurd-128.log` |
| `Sigurd-128` | timing keygen | `records/sign-24/Sigurd-128__keygen.json` |
| `Sigurd-128` | timing sign | `records/sign-24/Sigurd-128__sign.json` |
| `Sigurd-128` | timing verify | `records/sign-24/Sigurd-128__verify.json` |
| `Sigurd-128` | hash profile keygen | `profile/sign-24/Sigurd-128__keygen.json` |
| `Sigurd-128` | hash profile sign | `profile/sign-24/Sigurd-128__sign.json` |
| `Sigurd-128` | hash profile verify | `profile/sign-24/Sigurd-128__verify.json` |
| `Sigurd-256` | KAT log (sha256 `30230e598c678dfe…`) | `kat/sign-24/Sigurd-256.log` |
| `Sigurd-256` | timing keygen | `records/sign-24/Sigurd-256__keygen.json` |
| `Sigurd-256` | timing sign | `records/sign-24/Sigurd-256__sign.json` |
| `Sigurd-256` | timing verify | `records/sign-24/Sigurd-256__verify.json` |
| `Sigurd-256` | hash profile keygen | `profile/sign-24/Sigurd-256__keygen.json` |
| `Sigurd-256` | hash profile sign | `profile/sign-24/Sigurd-256__sign.json` |
| `Sigurd-256` | hash profile verify | `profile/sign-24/Sigurd-256__verify.json` |
| `Sigurd-512` | KAT log (sha256 `9a8fab8cbd7d2362…`) | `kat/sign-24/Sigurd-512.log` |
| `Sigurd-512` | timing keygen | `records/sign-24/Sigurd-512__keygen.json` |
| `Sigurd-512` | timing sign | `records/sign-24/Sigurd-512__sign.json` |
| `Sigurd-512` | timing verify | `records/sign-24/Sigurd-512__verify.json` |
| `Sigurd-512` | hash profile keygen | `profile/sign-24/Sigurd-512__keygen.json` |
| `Sigurd-512` | hash profile sign | `profile/sign-24/Sigurd-512__sign.json` |
| `Sigurd-512` | hash profile verify | `profile/sign-24/Sigurd-512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

