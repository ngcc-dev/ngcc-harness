# sign-09 DOVE — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-09` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561077311033344.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: DOVE
- Implementation versions measured: reference
- Parameter sets: `dove_classic_128`, `dove_classic_256`, `dove_classic_512`, `dove_pkc_skc_128`, `dove_pkc_skc_256`, `dove_pkc_skc_512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-09/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `dove_classic_128` | guide | PASS |
| `dove_classic_256` | guide | PASS |
| `dove_classic_512` | guide | PASS |
| `dove_pkc_skc_128` | guide | PASS |
| `dove_pkc_skc_256` | guide | PASS |
| `dove_pkc_skc_512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `dove_classic_128` | keygen | 96.51 M | 35.8 ms | 27.9 | 35.8 ms | 100 (5 × 20) |
| `dove_classic_128` | sign | 2.22 M | 822 µs | 1.22e+03 | 822 µs | 3795 (5 × 759) |
| `dove_classic_128` | verify | 1.18 M | 438 µs | 2.28e+03 | 438 µs | 7145 (5 × 1429) |
| `dove_classic_256` | keygen | 1.98 G | 736 ms | 1.36 | 736 ms | 100 (5 × 20) |
| `dove_classic_256` | sign | 21.95 M | 8.14 ms | 123 | 8.14 ms | 390 (5 × 78) |
| `dove_classic_256` | verify | 4.93 M | 1.84 ms | 544 | 1.83 ms | 1715 (5 × 343) |
| `dove_classic_512` | keygen | 45.42 G | 16.9 s | 0.0593 | 16.9 s | 20 (5 × 4) |
| `dove_classic_512` | sign | 242.40 M | 90 ms | 11.1 | 90 ms | 100 (5 × 20) |
| `dove_classic_512` | verify | 59.19 M | 22 ms | 45.5 | 22 ms | 145 (5 × 29) |
| `dove_pkc_skc_128` | keygen | 85.06 M | 31.6 ms | 31.7 | 31.6 ms | 100 (5 × 20) |
| `dove_pkc_skc_128` | sign | 24.15 M | 8.96 ms | 112 | 8.96 ms | 355 (5 × 71) |
| `dove_pkc_skc_128` | verify | 7.57 M | 2.81 ms | 356 | 2.81 ms | 1125 (5 × 225) |
| `dove_pkc_skc_256` | keygen | 1.86 G | 689 ms | 1.45 | 689 ms | 100 (5 × 20) |
| `dove_pkc_skc_256` | sign | 355.93 M | 132 ms | 7.57 | 132 ms | 100 (5 × 20) |
| `dove_pkc_skc_256` | verify | 74.47 M | 27.6 ms | 36.2 | 27.4 ms | 120 (5 × 24) |
| `dove_pkc_skc_512` | keygen | 43.98 G | 16.3 s | 0.0613 | 16.3 s | 20 (5 × 4) |
| `dove_pkc_skc_512` | sign | 5.50 G | 2.04 s | 0.49 | 2.04 s | 100 (5 × 20) |
| `dove_pkc_skc_512` | verify | 822.09 M | 305 ms | 3.28 | 305 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `dove_classic_128` | keygen | 34236 | 1800 KiB | 1960 KiB |
| `dove_classic_128` | sign | 34236 | 3520 KiB | 3592 KiB |
| `dove_classic_128` | verify | 34236 | 3564 KiB | 3628 KiB |
| `dove_classic_256` | keygen | 34192 | 3428 KiB | 7956 KiB |
| `dove_classic_256` | sign | 34192 | 7420 KiB | 7496 KiB |
| `dove_classic_256` | verify | 34192 | 7380 KiB | 7564 KiB |
| `dove_classic_512` | keygen | 34684 | 1436 KiB | 50004 KiB |
| `dove_classic_512` | sign | 34684 | 45656 KiB | 49864 KiB |
| `dove_classic_512` | verify | 34684 | 45488 KiB | 49312 KiB |
| `dove_pkc_skc_128` | keygen | 34084 | 1468 KiB | 1628 KiB |
| `dove_pkc_skc_128` | sign | 34084 | 1556 KiB | 1636 KiB |
| `dove_pkc_skc_128` | verify | 34084 | 1564 KiB | 1628 KiB |
| `dove_pkc_skc_256` | keygen | 34008 | 3552 KiB | 4428 KiB |
| `dove_pkc_skc_256` | sign | 34008 | 3940 KiB | 4576 KiB |
| `dove_pkc_skc_256` | verify | 34008 | 4044 KiB | 4264 KiB |
| `dove_pkc_skc_512` | keygen | 34400 | 1432 KiB | 13080 KiB |
| `dove_pkc_skc_512` | sign | 34400 | 8368 KiB | 12576 KiB |
| `dove_pkc_skc_512` | verify | 34400 | 8652 KiB | 12868 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | signature |
|---|---|---|---|
| `dove_classic_128` | 191646 | 192990 | 136 |
| `dove_classic_256` | 2050352 | 1925976 | 296 |
| `dove_classic_512` | 22833052 | 20181508 | 648 |
| `dove_pkc_skc_128` | 43576 | 24 | 136 |
| `dove_pkc_skc_256` | 446992 | 40 | 296 |
| `dove_pkc_skc_512` | 5062192 | 72 | 648 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `dove_classic_128` | keygen | 19% | 0.0% | drng 1, pseudoXOF 140 |
| `dove_classic_128` | sign | 0.7% | 0.0% | pseudoXOF 3 |
| `dove_classic_128` | verify | 0.5% | 0.0% | pseudoXOF 1 |
| `dove_classic_256` | keygen | 10% | 0.0% | drng 1, pseudoXOF 299 |
| `dove_classic_256` | sign | 0.2% | 0.0% | pseudoXOF 3 |
| `dove_classic_256` | verify | 0.2% | 0.0% | pseudoXOF 1 |
| `dove_classic_512` | keygen | 4.9% | 0.0% | drng 1, pseudoXOF 664 |
| `dove_classic_512` | sign | 0.0% | 0.0% | pseudoXOF 3 |
| `dove_classic_512` | verify | 0.0% | 0.0% | pseudoXOF 1 |
| `dove_pkc_skc_128` | keygen | 7.8% | 0.0% | drng 1, pseudoXOF 52 |
| `dove_pkc_skc_128` | sign | 27% | 0.0% | pseudoXOF 55 |
| `dove_pkc_skc_128` | verify | 84% | 0.0% | pseudoXOF 52 |
| `dove_pkc_skc_256` | keygen | 3.7% | 0.0% | drng 1, pseudoXOF 107 |
| `dove_pkc_skc_256` | sign | 20% | 0.0% | pseudoXOF 110 |
| `dove_pkc_skc_256` | verify | 93% | 0.0% | pseudoXOF 107 |
| `dove_pkc_skc_512` | keygen | 1.7% | 0.0% | drng 1, pseudoXOF 232 |
| `dove_pkc_skc_512` | sign | 14% | 0.0% | pseudoXOF 235 |
| `dove_pkc_skc_512` | verify | 93% | 0.0% | pseudoXOF 232 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `dove_classic_128` | KAT log (sha256 `23e16a529c5a6371…`) | `kat/sign-09/dove_classic_128.log` |
| `dove_classic_128` | timing keygen | `records/sign-09/dove_classic_128__keygen.json` |
| `dove_classic_128` | timing sign | `records/sign-09/dove_classic_128__sign.json` |
| `dove_classic_128` | timing verify | `records/sign-09/dove_classic_128__verify.json` |
| `dove_classic_128` | hash profile keygen | `profile/sign-09/dove_classic_128__keygen.json` |
| `dove_classic_128` | hash profile sign | `profile/sign-09/dove_classic_128__sign.json` |
| `dove_classic_128` | hash profile verify | `profile/sign-09/dove_classic_128__verify.json` |
| `dove_classic_256` | KAT log (sha256 `792c5647145ad4a7…`) | `kat/sign-09/dove_classic_256.log` |
| `dove_classic_256` | timing keygen | `records/sign-09/dove_classic_256__keygen.json` |
| `dove_classic_256` | timing sign | `records/sign-09/dove_classic_256__sign.json` |
| `dove_classic_256` | timing verify | `records/sign-09/dove_classic_256__verify.json` |
| `dove_classic_256` | hash profile keygen | `profile/sign-09/dove_classic_256__keygen.json` |
| `dove_classic_256` | hash profile sign | `profile/sign-09/dove_classic_256__sign.json` |
| `dove_classic_256` | hash profile verify | `profile/sign-09/dove_classic_256__verify.json` |
| `dove_classic_512` | KAT log (sha256 `69edb4f157ff011e…`) | `kat/sign-09/dove_classic_512.log` |
| `dove_classic_512` | timing keygen | `records/sign-09/dove_classic_512__keygen.json` |
| `dove_classic_512` | timing sign | `records/sign-09/dove_classic_512__sign.json` |
| `dove_classic_512` | timing verify | `records/sign-09/dove_classic_512__verify.json` |
| `dove_classic_512` | hash profile keygen | `profile/sign-09/dove_classic_512__keygen.json` |
| `dove_classic_512` | hash profile sign | `profile/sign-09/dove_classic_512__sign.json` |
| `dove_classic_512` | hash profile verify | `profile/sign-09/dove_classic_512__verify.json` |
| `dove_pkc_skc_128` | KAT log (sha256 `f3c805d44d668704…`) | `kat/sign-09/dove_pkc_skc_128.log` |
| `dove_pkc_skc_128` | timing keygen | `records/sign-09/dove_pkc_skc_128__keygen.json` |
| `dove_pkc_skc_128` | timing sign | `records/sign-09/dove_pkc_skc_128__sign.json` |
| `dove_pkc_skc_128` | timing verify | `records/sign-09/dove_pkc_skc_128__verify.json` |
| `dove_pkc_skc_128` | hash profile keygen | `profile/sign-09/dove_pkc_skc_128__keygen.json` |
| `dove_pkc_skc_128` | hash profile sign | `profile/sign-09/dove_pkc_skc_128__sign.json` |
| `dove_pkc_skc_128` | hash profile verify | `profile/sign-09/dove_pkc_skc_128__verify.json` |
| `dove_pkc_skc_256` | KAT log (sha256 `b40153a220226c63…`) | `kat/sign-09/dove_pkc_skc_256.log` |
| `dove_pkc_skc_256` | timing keygen | `records/sign-09/dove_pkc_skc_256__keygen.json` |
| `dove_pkc_skc_256` | timing sign | `records/sign-09/dove_pkc_skc_256__sign.json` |
| `dove_pkc_skc_256` | timing verify | `records/sign-09/dove_pkc_skc_256__verify.json` |
| `dove_pkc_skc_256` | hash profile keygen | `profile/sign-09/dove_pkc_skc_256__keygen.json` |
| `dove_pkc_skc_256` | hash profile sign | `profile/sign-09/dove_pkc_skc_256__sign.json` |
| `dove_pkc_skc_256` | hash profile verify | `profile/sign-09/dove_pkc_skc_256__verify.json` |
| `dove_pkc_skc_512` | KAT log (sha256 `cc7cb2851e54484f…`) | `kat/sign-09/dove_pkc_skc_512.log` |
| `dove_pkc_skc_512` | timing keygen | `records/sign-09/dove_pkc_skc_512__keygen.json` |
| `dove_pkc_skc_512` | timing sign | `records/sign-09/dove_pkc_skc_512__sign.json` |
| `dove_pkc_skc_512` | timing verify | `records/sign-09/dove_pkc_skc_512__verify.json` |
| `dove_pkc_skc_512` | hash profile keygen | `profile/sign-09/dove_pkc_skc_512__keygen.json` |
| `dove_pkc_skc_512` | hash profile sign | `profile/sign-09/dove_pkc_skc_512__sign.json` |
| `dove_pkc_skc_512` | hash profile verify | `profile/sign-09/dove_pkc_skc_512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

