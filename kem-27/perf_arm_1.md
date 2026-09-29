# kem-27 NTRE Key Encapsulation Mechanism — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-27` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560863326031872.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: NTRE Key Encapsulation Mechanism
- Implementation versions measured: reference
- Parameter sets: `NTRE-128`, `NTRE-256`, `NTRE-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-27/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `NTRE-128` | guide | PASS |
| `NTRE-256` | guide | PASS |
| `NTRE-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `NTRE-128` | keygen | 109.4 k | 40.6 µs | 2.46e+04 | 40.6 µs | 62665 (5 × 12533) |
| `NTRE-128` | enc | 82.1 k | 30.5 µs | 3.28e+04 | 30.4 µs | 96385 (5 × 19277) |
| `NTRE-128` | dec | 73.4 k | 27.2 µs | 3.67e+04 | 27.2 µs | 100000 (5 × 20000) |
| `NTRE-256` | keygen | 188.4 k | 69.9 µs | 1.43e+04 | 69.6 µs | 38930 (5 × 7786) |
| `NTRE-256` | enc | 150.4 k | 55.9 µs | 1.79e+04 | 55.9 µs | 54860 (5 × 10972) |
| `NTRE-256` | dec | 136.9 k | 50.8 µs | 1.97e+04 | 50.8 µs | 59260 (5 × 11852) |
| `NTRE-512` | keygen | 374.5 k | 139 µs | 7.2e+03 | 138 µs | 21685 (5 × 4337) |
| `NTRE-512` | enc | 339.0 k | 126 µs | 7.95e+03 | 126 µs | 24810 (5 × 4962) |
| `NTRE-512` | dec | 326.6 k | 121 µs | 8.25e+03 | 121 µs | 25670 (5 × 5134) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `NTRE-128` | keygen | 23548 | 1416 KiB | 1480 KiB |
| `NTRE-128` | enc | 23548 | 1420 KiB | 1484 KiB |
| `NTRE-128` | dec | 23548 | 1424 KiB | 1488 KiB |
| `NTRE-256` | keygen | 24020 | 1420 KiB | 1492 KiB |
| `NTRE-256` | enc | 24020 | 1432 KiB | 1496 KiB |
| `NTRE-256` | dec | 24020 | 1440 KiB | 1504 KiB |
| `NTRE-512` | keygen | 24964 | 1428 KiB | 1508 KiB |
| `NTRE-512` | enc | 24964 | 1444 KiB | 1508 KiB |
| `NTRE-512` | dec | 24964 | 1460 KiB | 1524 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `NTRE-128` | 972 | 1976 | 972 | 64 |
| `NTRE-256` | 1944 | 3920 | 1944 | 64 |
| `NTRE-512` | 3456 | 6944 | 3456 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `NTRE-128` | keygen | 34% | 7.8% | drng 2, pseudoXOF 2, sm3hash 1 |
| `NTRE-128` | enc | 65% | 6.8% | drng 1, pseudoXOF 2, sm3hash 1 |
| `NTRE-128` | dec | 44% | 0.0% | pseudoXOF 2 |
| `NTRE-256` | keygen | 37% | 4.5% | drng 2, pseudoXOF 2, sm3hash 1 |
| `NTRE-256` | enc | 67% | 4.7% | drng 1, pseudoXOF 2, sm3hash 1 |
| `NTRE-256` | dec | 45% | 0.0% | pseudoXOF 2 |
| `NTRE-512` | keygen | 32% | 2.3% | drng 2, pseudoXOF 2, sm3hash 1 |
| `NTRE-512` | enc | 66% | 2.9% | drng 1, pseudoXOF 2, sm3hash 1 |
| `NTRE-512` | dec | 47% | 0.0% | pseudoXOF 2 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `NTRE-128` | KAT log (sha256 `5c01aec457cb0a2b…`) | `kat/kem-27/NTRE-128.log` |
| `NTRE-128` | timing dec | `records/kem-27/NTRE-128__dec.json` |
| `NTRE-128` | timing enc | `records/kem-27/NTRE-128__enc.json` |
| `NTRE-128` | timing keygen | `records/kem-27/NTRE-128__keygen.json` |
| `NTRE-128` | hash profile dec | `profile/kem-27/NTRE-128__dec.json` |
| `NTRE-128` | hash profile enc | `profile/kem-27/NTRE-128__enc.json` |
| `NTRE-128` | hash profile keygen | `profile/kem-27/NTRE-128__keygen.json` |
| `NTRE-256` | KAT log (sha256 `50a6d0699b3eaf08…`) | `kat/kem-27/NTRE-256.log` |
| `NTRE-256` | timing dec | `records/kem-27/NTRE-256__dec.json` |
| `NTRE-256` | timing enc | `records/kem-27/NTRE-256__enc.json` |
| `NTRE-256` | timing keygen | `records/kem-27/NTRE-256__keygen.json` |
| `NTRE-256` | hash profile dec | `profile/kem-27/NTRE-256__dec.json` |
| `NTRE-256` | hash profile enc | `profile/kem-27/NTRE-256__enc.json` |
| `NTRE-256` | hash profile keygen | `profile/kem-27/NTRE-256__keygen.json` |
| `NTRE-512` | KAT log (sha256 `9550a28fa1c2998d…`) | `kat/kem-27/NTRE-512.log` |
| `NTRE-512` | timing dec | `records/kem-27/NTRE-512__dec.json` |
| `NTRE-512` | timing enc | `records/kem-27/NTRE-512__enc.json` |
| `NTRE-512` | timing keygen | `records/kem-27/NTRE-512__keygen.json` |
| `NTRE-512` | hash profile dec | `profile/kem-27/NTRE-512__dec.json` |
| `NTRE-512` | hash profile enc | `profile/kem-27/NTRE-512__enc.json` |
| `NTRE-512` | hash profile keygen | `profile/kem-27/NTRE-512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

