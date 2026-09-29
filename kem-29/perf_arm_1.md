# kem-29 Polar-KEM — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-29` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560872041795584.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: Polar-KEM
- Implementation versions measured: reference
- Parameter sets: `PolarKEM-128`, `PolarKEM-256`, `PolarKEM-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-29/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `PolarKEM-128` | guide | PASS |
| `PolarKEM-256` | guide | PASS |
| `PolarKEM-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `PolarKEM-128` | keygen | 158.4 k | 58.8 µs | 1.7e+04 | 58.5 µs | 50315 (5 × 10063) |
| `PolarKEM-128` | enc | 495.7 k | 184 µs | 5.44e+03 | 183 µs | 16845 (5 × 3369) |
| `PolarKEM-128` | dec | 985.7 k | 366 µs | 2.73e+03 | 364 µs | 8605 (5 × 1721) |
| `PolarKEM-256` | keygen | 309.4 k | 115 µs | 8.71e+03 | 115 µs | 26610 (5 × 5322) |
| `PolarKEM-256` | enc | 619.8 k | 230 µs | 4.35e+03 | 230 µs | 13020 (5 × 2604) |
| `PolarKEM-256` | dec | 1.25 M | 464 µs | 2.16e+03 | 462 µs | 6690 (5 × 1338) |
| `PolarKEM-512` | keygen | 612.2 k | 227 µs | 4.4e+03 | 227 µs | 13635 (5 × 2727) |
| `PolarKEM-512` | enc | 1.23 M | 455 µs | 2.2e+03 | 452 µs | 6890 (5 × 1378) |
| `PolarKEM-512` | dec | 2.46 M | 915 µs | 1.09e+03 | 912 µs | 3430 (5 × 686) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `PolarKEM-128` | keygen | 23948 | 1416 KiB | 1484 KiB |
| `PolarKEM-128` | enc | 23948 | 1424 KiB | 1492 KiB |
| `PolarKEM-128` | dec | 23948 | 1424 KiB | 1492 KiB |
| `PolarKEM-256` | keygen | 24396 | 1420 KiB | 1488 KiB |
| `PolarKEM-256` | enc | 24396 | 1432 KiB | 1500 KiB |
| `PolarKEM-256` | dec | 24396 | 1432 KiB | 1500 KiB |
| `PolarKEM-512` | keygen | 24844 | 1428 KiB | 1496 KiB |
| `PolarKEM-512` | enc | 24844 | 1444 KiB | 1512 KiB |
| `PolarKEM-512` | dec | 24844 | 1448 KiB | 1516 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `PolarKEM-128` | 1024 | 2048 | 768 | 16 |
| `PolarKEM-256` | 2048 | 4096 | 1280 | 32 |
| `PolarKEM-512` | 4096 | 8192 | 2304 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `PolarKEM-128` | keygen | 94% | 5.4% | drng 2, pseudoXOF 2, sm3hash 1 |
| `PolarKEM-128` | enc | 94% | 0.8% | drng 1, pseudoXOF 6, sm3hash 3 |
| `PolarKEM-128` | dec | 96% | 0.0% | pseudoXOF 10, sm3hash 5 |
| `PolarKEM-256` | keygen | 97% | 2.8% | drng 2, pseudoXOF 2, sm3hash 1 |
| `PolarKEM-256` | enc | 92% | 0.7% | drng 1, pseudoXOF 6, sm3hash 3 |
| `PolarKEM-256` | dec | 94% | 0.0% | pseudoXOF 10, sm3hash 5 |
| `PolarKEM-512` | keygen | 98% | 1.4% | drng 2, pseudoXOF 2, sm3hash 1 |
| `PolarKEM-512` | enc | 92% | 0.5% | drng 1, pseudoXOF 7, sm3hash 3 |
| `PolarKEM-512` | dec | 94% | 0.0% | pseudoXOF 12, sm3hash 5 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `PolarKEM-128` | KAT log (sha256 `c2502fa32beacfb5…`) | `kat/kem-29/PolarKEM-128.log` |
| `PolarKEM-128` | timing dec | `records/kem-29/PolarKEM-128__dec.json` |
| `PolarKEM-128` | timing enc | `records/kem-29/PolarKEM-128__enc.json` |
| `PolarKEM-128` | timing keygen | `records/kem-29/PolarKEM-128__keygen.json` |
| `PolarKEM-128` | hash profile dec | `profile/kem-29/PolarKEM-128__dec.json` |
| `PolarKEM-128` | hash profile enc | `profile/kem-29/PolarKEM-128__enc.json` |
| `PolarKEM-128` | hash profile keygen | `profile/kem-29/PolarKEM-128__keygen.json` |
| `PolarKEM-256` | KAT log (sha256 `1548f7e7f0f51ab9…`) | `kat/kem-29/PolarKEM-256.log` |
| `PolarKEM-256` | timing dec | `records/kem-29/PolarKEM-256__dec.json` |
| `PolarKEM-256` | timing enc | `records/kem-29/PolarKEM-256__enc.json` |
| `PolarKEM-256` | timing keygen | `records/kem-29/PolarKEM-256__keygen.json` |
| `PolarKEM-256` | hash profile dec | `profile/kem-29/PolarKEM-256__dec.json` |
| `PolarKEM-256` | hash profile enc | `profile/kem-29/PolarKEM-256__enc.json` |
| `PolarKEM-256` | hash profile keygen | `profile/kem-29/PolarKEM-256__keygen.json` |
| `PolarKEM-512` | KAT log (sha256 `6ae84953cf4f7c37…`) | `kat/kem-29/PolarKEM-512.log` |
| `PolarKEM-512` | timing dec | `records/kem-29/PolarKEM-512__dec.json` |
| `PolarKEM-512` | timing enc | `records/kem-29/PolarKEM-512__enc.json` |
| `PolarKEM-512` | timing keygen | `records/kem-29/PolarKEM-512__keygen.json` |
| `PolarKEM-512` | hash profile dec | `profile/kem-29/PolarKEM-512__dec.json` |
| `PolarKEM-512` | hash profile enc | `profile/kem-29/PolarKEM-512__enc.json` |
| `PolarKEM-512` | hash profile keygen | `profile/kem-29/PolarKEM-512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

