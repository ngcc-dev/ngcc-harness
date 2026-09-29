# kem-37 TriQ-KEM — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-37` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560881558671360.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: TriQ-KEM
- Implementation versions measured: reference
- Parameter sets: `TriQ-KEM-128`, `TriQ-KEM-256`, `TriQ-KEM-384`, `TriQ-KEM-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-37/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `TriQ-KEM-128` | guide | PASS |
| `TriQ-KEM-256` | guide | PASS |
| `TriQ-KEM-384` | guide | PASS |
| `TriQ-KEM-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `TriQ-KEM-128` | keygen | 2.51 M | 932 µs | 1.07e+03 | 931 µs | 3285 (5 × 657) |
| `TriQ-KEM-128` | enc | 4.59 M | 1.7 ms | 587 | 1.7 ms | 1850 (5 × 370) |
| `TriQ-KEM-128` | dec | 6.85 M | 2.54 ms | 394 | 2.54 ms | 1240 (5 × 248) |
| `TriQ-KEM-256` | keygen | 13.68 M | 5.07 ms | 197 | 5.07 ms | 620 (5 × 124) |
| `TriQ-KEM-256` | enc | 26.90 M | 9.98 ms | 100 | 9.98 ms | 320 (5 × 64) |
| `TriQ-KEM-256` | dec | 39.89 M | 14.8 ms | 67.6 | 14.8 ms | 215 (5 × 43) |
| `TriQ-KEM-384` | keygen | 36.83 M | 13.7 ms | 73.2 | 13.7 ms | 230 (5 × 46) |
| `TriQ-KEM-384` | enc | 72.73 M | 27 ms | 37.1 | 27 ms | 120 (5 × 24) |
| `TriQ-KEM-384` | dec | 108.65 M | 40.3 ms | 24.8 | 40.3 ms | 100 (5 × 20) |
| `TriQ-KEM-512` | keygen | 80.05 M | 29.7 ms | 33.7 | 29.7 ms | 110 (5 × 22) |
| `TriQ-KEM-512` | enc | 157.68 M | 58.5 ms | 17.1 | 58.5 ms | 100 (5 × 20) |
| `TriQ-KEM-512` | dec | 233.75 M | 86.7 ms | 11.5 | 86.7 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `TriQ-KEM-128` | keygen | 40680 | 3448 KiB | 3560 KiB |
| `TriQ-KEM-128` | enc | 40680 | 1492 KiB | 1556 KiB |
| `TriQ-KEM-128` | dec | 40680 | 1496 KiB | 1560 KiB |
| `TriQ-KEM-256` | keygen | 43644 | 1452 KiB | 1664 KiB |
| `TriQ-KEM-256` | enc | 43644 | 1616 KiB | 1688 KiB |
| `TriQ-KEM-256` | dec | 43644 | 3644 KiB | 3708 KiB |
| `TriQ-KEM-384` | keygen | 53736 | 3480 KiB | 3784 KiB |
| `TriQ-KEM-384` | enc | 53736 | 1808 KiB | 1876 KiB |
| `TriQ-KEM-384` | dec | 53736 | 1852 KiB | 1924 KiB |
| `TriQ-KEM-512` | keygen | 60448 | 1528 KiB | 1980 KiB |
| `TriQ-KEM-512` | enc | 60448 | 2028 KiB | 2096 KiB |
| `TriQ-KEM-512` | dec | 60448 | 2112 KiB | 2184 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `TriQ-KEM-128` | 2054 | 2102 | 4086 | 16 |
| `TriQ-KEM-256` | 6328 | 6424 | 12472 | 32 |
| `TriQ-KEM-384` | 12255 | 12399 | 24335 | 48 |
| `TriQ-KEM-512` | 19768 | 19960 | 39320 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `TriQ-KEM-128` | keygen | 17% | 0.5% | drng 2, pseudoXOF 8, sm3hash 1 |
| `TriQ-KEM-128` | enc | 9.3% | 0.2% | drng 2, pseudoXOF 6.01, sm3hash 2 |
| `TriQ-KEM-128` | dec | 7.0% | 0.0% | pseudoXOF 6, sm3hash 3 |
| `TriQ-KEM-256` | keygen | 6.1% | 0.1% | drng 2, pseudoXOF 9.09 |
| `TriQ-KEM-256` | enc | 4.3% | 0.0% | drng 2, pseudoXOF 8, pseudohash 1, sm3hash 1 |
| `TriQ-KEM-256` | dec | 3.5% | 0.0% | pseudoXOF 8, pseudohash 1, sm3hash 2 |
| `TriQ-KEM-384` | keygen | 4.2% | 0.0% | drng 2, pseudoXOF 10.8 |
| `TriQ-KEM-384` | enc | 2.9% | 0.0% | drng 2, pseudoXOF 9, pseudohash 1 |
| `TriQ-KEM-384` | dec | 2.8% | 0.0% | pseudoXOF 10, pseudohash 1 |
| `TriQ-KEM-512` | keygen | 5.4% | 0.0% | drng 2, pseudoXOF 10.9 |
| `TriQ-KEM-512` | enc | 3.8% | 0.0% | drng 2, pseudoXOF 11.1, pseudohash 1 |
| `TriQ-KEM-512` | dec | 3.3% | 0.0% | pseudoXOF 12, pseudohash 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `TriQ-KEM-128` | KAT log (sha256 `71a372f47b434569…`) | `kat/kem-37/TriQ-KEM-128.log` |
| `TriQ-KEM-128` | timing dec | `records/kem-37/TriQ-KEM-128__dec.json` |
| `TriQ-KEM-128` | timing enc | `records/kem-37/TriQ-KEM-128__enc.json` |
| `TriQ-KEM-128` | timing keygen | `records/kem-37/TriQ-KEM-128__keygen.json` |
| `TriQ-KEM-128` | hash profile dec | `profile/kem-37/TriQ-KEM-128__dec.json` |
| `TriQ-KEM-128` | hash profile enc | `profile/kem-37/TriQ-KEM-128__enc.json` |
| `TriQ-KEM-128` | hash profile keygen | `profile/kem-37/TriQ-KEM-128__keygen.json` |
| `TriQ-KEM-256` | KAT log (sha256 `e3883e15b4f965e1…`) | `kat/kem-37/TriQ-KEM-256.log` |
| `TriQ-KEM-256` | timing dec | `records/kem-37/TriQ-KEM-256__dec.json` |
| `TriQ-KEM-256` | timing enc | `records/kem-37/TriQ-KEM-256__enc.json` |
| `TriQ-KEM-256` | timing keygen | `records/kem-37/TriQ-KEM-256__keygen.json` |
| `TriQ-KEM-256` | hash profile dec | `profile/kem-37/TriQ-KEM-256__dec.json` |
| `TriQ-KEM-256` | hash profile enc | `profile/kem-37/TriQ-KEM-256__enc.json` |
| `TriQ-KEM-256` | hash profile keygen | `profile/kem-37/TriQ-KEM-256__keygen.json` |
| `TriQ-KEM-384` | KAT log (sha256 `923fa5f947db944c…`) | `kat/kem-37/TriQ-KEM-384.log` |
| `TriQ-KEM-384` | timing dec | `records/kem-37/TriQ-KEM-384__dec.json` |
| `TriQ-KEM-384` | timing enc | `records/kem-37/TriQ-KEM-384__enc.json` |
| `TriQ-KEM-384` | timing keygen | `records/kem-37/TriQ-KEM-384__keygen.json` |
| `TriQ-KEM-384` | hash profile dec | `profile/kem-37/TriQ-KEM-384__dec.json` |
| `TriQ-KEM-384` | hash profile enc | `profile/kem-37/TriQ-KEM-384__enc.json` |
| `TriQ-KEM-384` | hash profile keygen | `profile/kem-37/TriQ-KEM-384__keygen.json` |
| `TriQ-KEM-512` | KAT log (sha256 `a234f5860365b710…`) | `kat/kem-37/TriQ-KEM-512.log` |
| `TriQ-KEM-512` | timing dec | `records/kem-37/TriQ-KEM-512__dec.json` |
| `TriQ-KEM-512` | timing enc | `records/kem-37/TriQ-KEM-512__enc.json` |
| `TriQ-KEM-512` | timing keygen | `records/kem-37/TriQ-KEM-512__keygen.json` |
| `TriQ-KEM-512` | hash profile dec | `profile/kem-37/TriQ-KEM-512__dec.json` |
| `TriQ-KEM-512` | hash profile enc | `profile/kem-37/TriQ-KEM-512__enc.json` |
| `TriQ-KEM-512` | hash profile keygen | `profile/kem-37/TriQ-KEM-512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

