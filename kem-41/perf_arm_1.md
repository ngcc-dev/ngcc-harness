# kem-41 ZEN — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-41` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560890563842048.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: ZEN
- Implementation versions measured: reference
- Parameter sets: `ZEN_128`, `ZEN_256`, `ZEN_512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-41/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `ZEN_128` | guide | PASS |
| `ZEN_256` | guide | PASS |
| `ZEN_512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `ZEN_128` | keygen | 153.2 k | 56.8 µs | 1.76e+04 | 56.8 µs | 43720 (5 × 8744) |
| `ZEN_128` | enc | 122.1 k | 45.3 µs | 2.21e+04 | 45.2 µs | 66950 (5 × 13390) |
| `ZEN_128` | dec | 145.3 k | 53.9 µs | 1.85e+04 | 53.9 µs | 56540 (5 × 11308) |
| `ZEN_256` | keygen | 263.3 k | 97.7 µs | 1.02e+04 | 97.5 µs | 27925 (5 × 5585) |
| `ZEN_256` | enc | 152.4 k | 56.6 µs | 1.77e+04 | 56.5 µs | 53040 (5 × 10608) |
| `ZEN_256` | dec | 211.2 k | 78.4 µs | 1.28e+04 | 78.4 µs | 36145 (5 × 7229) |
| `ZEN_512` | keygen | 735.6 k | 273 µs | 3.66e+03 | 273 µs | 12330 (5 × 2466) |
| `ZEN_512` | enc | 425.3 k | 158 µs | 6.34e+03 | 158 µs | 19600 (5 × 3920) |
| `ZEN_512` | dec | 569.0 k | 211 µs | 4.74e+03 | 211 µs | 14825 (5 × 2965) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `ZEN_128` | keygen | 34372 | 1428 KiB | 1496 KiB |
| `ZEN_128` | enc | 34372 | 1436 KiB | 1500 KiB |
| `ZEN_128` | dec | 34372 | 1440 KiB | 1504 KiB |
| `ZEN_256` | keygen | 39768 | 1436 KiB | 1512 KiB |
| `ZEN_256` | enc | 39768 | 1448 KiB | 1512 KiB |
| `ZEN_256` | dec | 39768 | 1448 KiB | 1512 KiB |
| `ZEN_512` | keygen | 46376 | 1444 KiB | 1544 KiB |
| `ZEN_512` | enc | 46376 | 1480 KiB | 1548 KiB |
| `ZEN_512` | dec | 46376 | 1480 KiB | 1548 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `ZEN_128` | 615 | 1303 | 512 | 16 |
| `ZEN_256` | 1229 | 2605 | 1024 | 32 |
| `ZEN_512` | 2458 | 5210 | 2048 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `ZEN_128` | keygen | 41% | 6.5% | drng 2, pseudoXOF 3.05, sm3hash 1 |
| `ZEN_128` | enc | 74% | 3.5% | drng 1, pseudoXOF 2, pseudohash 1, sm3hash 1 |
| `ZEN_128` | dec | 62% | 0.0% | pseudoXOF 2, pseudohash 1, sm3hash 1 |
| `ZEN_256` | keygen | 50% | 3.7% | drng 2, pseudoXOF 3.02, sm3hash 1 |
| `ZEN_256` | enc | 58% | 2.8% | drng 1, pseudoXOF 2, pseudohash 1, sm3hash 1 |
| `ZEN_256` | dec | 40% | 0.0% | pseudoXOF 2, pseudohash 1, sm3hash 1 |
| `ZEN_512` | keygen | 58% | 1.5% | drng 2, pseudoXOF 2.93, pseudohash 1 |
| `ZEN_512` | enc | 64% | 1.3% | drng 1, pseudoXOF 2, pseudohash 2 |
| `ZEN_512` | dec | 45% | 0.0% | pseudoXOF 3, pseudohash 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `ZEN_128` | KAT log (sha256 `d512ed6722630117…`) | `kat/kem-41/ZEN_128.log` |
| `ZEN_128` | timing dec | `records/kem-41/ZEN_128__dec.json` |
| `ZEN_128` | timing enc | `records/kem-41/ZEN_128__enc.json` |
| `ZEN_128` | timing keygen | `records/kem-41/ZEN_128__keygen.json` |
| `ZEN_128` | hash profile dec | `profile/kem-41/ZEN_128__dec.json` |
| `ZEN_128` | hash profile enc | `profile/kem-41/ZEN_128__enc.json` |
| `ZEN_128` | hash profile keygen | `profile/kem-41/ZEN_128__keygen.json` |
| `ZEN_256` | KAT log (sha256 `8581e6db3ce27bc6…`) | `kat/kem-41/ZEN_256.log` |
| `ZEN_256` | timing dec | `records/kem-41/ZEN_256__dec.json` |
| `ZEN_256` | timing enc | `records/kem-41/ZEN_256__enc.json` |
| `ZEN_256` | timing keygen | `records/kem-41/ZEN_256__keygen.json` |
| `ZEN_256` | hash profile dec | `profile/kem-41/ZEN_256__dec.json` |
| `ZEN_256` | hash profile enc | `profile/kem-41/ZEN_256__enc.json` |
| `ZEN_256` | hash profile keygen | `profile/kem-41/ZEN_256__keygen.json` |
| `ZEN_512` | KAT log (sha256 `ffab40d5a99ad40d…`) | `kat/kem-41/ZEN_512.log` |
| `ZEN_512` | timing dec | `records/kem-41/ZEN_512__dec.json` |
| `ZEN_512` | timing enc | `records/kem-41/ZEN_512__enc.json` |
| `ZEN_512` | timing keygen | `records/kem-41/ZEN_512__keygen.json` |
| `ZEN_512` | hash profile dec | `profile/kem-41/ZEN_512__dec.json` |
| `ZEN_512` | hash profile enc | `profile/kem-41/ZEN_512__enc.json` |
| `ZEN_512` | hash profile keygen | `profile/kem-41/ZEN_512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

