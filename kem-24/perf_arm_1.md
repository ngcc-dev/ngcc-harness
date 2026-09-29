# kem-24 MORNING-Scabbard — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-24` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560862906601472.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: MORNING-Scabbard
- Implementation versions measured: reference
- Parameter sets: `scabbard128`, `scabbard256`, `scabbard512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-24/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `scabbard128` | guide | PASS |
| `scabbard256` | guide | PASS |
| `scabbard512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `scabbard128` | keygen | 702.7 k | 261 µs | 3.84e+03 | 261 µs | 11595 (5 × 2319) |
| `scabbard128` | enc | 736.4 k | 273 µs | 3.66e+03 | 273 µs | 11525 (5 × 2305) |
| `scabbard128` | dec | 741.5 k | 275 µs | 3.63e+03 | 275 µs | 11455 (5 × 2291) |
| `scabbard256` | keygen | 1.69 M | 626 µs | 1.6e+03 | 626 µs | 4930 (5 × 986) |
| `scabbard256` | enc | 1.81 M | 671 µs | 1.49e+03 | 671 µs | 4630 (5 × 926) |
| `scabbard256` | dec | 1.87 M | 692 µs | 1.44e+03 | 692 µs | 4525 (5 × 905) |
| `scabbard512` | keygen | 5.05 M | 1.87 ms | 534 | 1.87 ms | 1625 (5 × 325) |
| `scabbard512` | enc | 5.50 M | 2.04 ms | 490 | 2.04 ms | 1515 (5 × 303) |
| `scabbard512` | dec | 5.70 M | 2.12 ms | 473 | 2.12 ms | 1490 (5 × 298) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `scabbard128` | keygen | 24264 | 1416 KiB | 1492 KiB |
| `scabbard128` | enc | 24264 | 1432 KiB | 1496 KiB |
| `scabbard128` | dec | 24264 | 1432 KiB | 1496 KiB |
| `scabbard256` | keygen | 24656 | 1420 KiB | 1512 KiB |
| `scabbard256` | enc | 24656 | 1452 KiB | 1516 KiB |
| `scabbard256` | dec | 24656 | 1452 KiB | 1516 KiB |
| `scabbard512` | keygen | 24448 | 1424 KiB | 1544 KiB |
| `scabbard512` | enc | 24448 | 1476 KiB | 1544 KiB |
| `scabbard512` | dec | 24448 | 1484 KiB | 1552 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `scabbard128` | 736 | 1056 | 760 | 16 |
| `scabbard256` | 1616 | 2256 | 1648 | 32 |
| `scabbard512` | 2880 | 4032 | 3072 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `scabbard128` | keygen | 68% | 1.8% | drng 3, pseudoXOF 92 |
| `scabbard128` | enc | 67% | 0.6% | drng 1, pseudoXOF 95 |
| `scabbard128` | dec | 65% | 0.0% | pseudoXOF 93 |
| `scabbard256` | keygen | 50% | 0.8% | drng 3, pseudoXOF 92 |
| `scabbard256` | enc | 49% | 0.2% | drng 1, pseudoXOF 95 |
| `scabbard256` | dec | 46% | 0.0% | pseudoXOF 93 |
| `scabbard512` | keygen | 49% | 0.3% | drng 3, pseudoXOF 74 |
| `scabbard512` | enc | 47% | 0.1% | drng 1, pseudoXOF 77 |
| `scabbard512` | dec | 44% | 0.0% | pseudoXOF 75 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `scabbard128` | KAT log (sha256 `7b1369e7ff52f39b…`) | `kat/kem-24/scabbard128.log` |
| `scabbard128` | timing dec | `records/kem-24/scabbard128__dec.json` |
| `scabbard128` | timing enc | `records/kem-24/scabbard128__enc.json` |
| `scabbard128` | timing keygen | `records/kem-24/scabbard128__keygen.json` |
| `scabbard128` | hash profile dec | `profile/kem-24/scabbard128__dec.json` |
| `scabbard128` | hash profile enc | `profile/kem-24/scabbard128__enc.json` |
| `scabbard128` | hash profile keygen | `profile/kem-24/scabbard128__keygen.json` |
| `scabbard256` | KAT log (sha256 `6699b5e754a6458d…`) | `kat/kem-24/scabbard256.log` |
| `scabbard256` | timing dec | `records/kem-24/scabbard256__dec.json` |
| `scabbard256` | timing enc | `records/kem-24/scabbard256__enc.json` |
| `scabbard256` | timing keygen | `records/kem-24/scabbard256__keygen.json` |
| `scabbard256` | hash profile dec | `profile/kem-24/scabbard256__dec.json` |
| `scabbard256` | hash profile enc | `profile/kem-24/scabbard256__enc.json` |
| `scabbard256` | hash profile keygen | `profile/kem-24/scabbard256__keygen.json` |
| `scabbard512` | KAT log (sha256 `df9b2c4739d687f2…`) | `kat/kem-24/scabbard512.log` |
| `scabbard512` | timing dec | `records/kem-24/scabbard512__dec.json` |
| `scabbard512` | timing enc | `records/kem-24/scabbard512__enc.json` |
| `scabbard512` | timing keygen | `records/kem-24/scabbard512__keygen.json` |
| `scabbard512` | hash profile dec | `profile/kem-24/scabbard512__dec.json` |
| `scabbard512` | hash profile enc | `profile/kem-24/scabbard512__enc.json` |
| `scabbard512` | hash profile keygen | `profile/kem-24/scabbard512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

