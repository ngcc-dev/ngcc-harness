# kem-39 Weaver — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-39` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560890312183808.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: Weaver
- Implementation versions measured: reference
- Parameter sets: `WeaverKEM-128`, `WeaverKEM-256`, `WeaverKEM-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-39/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `WeaverKEM-128` | guide | PASS |
| `WeaverKEM-256` | guide | PASS |
| `WeaverKEM-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `WeaverKEM-128` | keygen | 496.5 k | 184 µs | 5.43e+03 | 184 µs | 16230 (5 × 3246) |
| `WeaverKEM-128` | enc | 533.0 k | 198 µs | 5.06e+03 | 198 µs | 15205 (5 × 3041) |
| `WeaverKEM-128` | dec | 548.2 k | 203 µs | 4.92e+03 | 204 µs | 15295 (5 × 3059) |
| `WeaverKEM-256` | keygen | 756.7 k | 281 µs | 3.56e+03 | 280 µs | 10405 (5 × 2081) |
| `WeaverKEM-256` | enc | 837.3 k | 311 µs | 3.22e+03 | 308 µs | 10085 (5 × 2017) |
| `WeaverKEM-256` | dec | 838.4 k | 311 µs | 3.21e+03 | 310 µs | 9810 (5 × 1962) |
| `WeaverKEM-512` | keygen | 2.53 M | 938 µs | 1.07e+03 | 932 µs | 3310 (5 × 662) |
| `WeaverKEM-512` | enc | 2.90 M | 1.07 ms | 930 | 1.07 ms | 2930 (5 × 586) |
| `WeaverKEM-512` | dec | 2.95 M | 1.1 ms | 912 | 1.1 ms | 2775 (5 × 555) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `WeaverKEM-128` | keygen | 33776 | 1424 KiB | 1504 KiB |
| `WeaverKEM-128` | enc | 33776 | 1440 KiB | 1504 KiB |
| `WeaverKEM-128` | dec | 33776 | 1444 KiB | 1508 KiB |
| `WeaverKEM-256` | keygen | 38448 | 1432 KiB | 1516 KiB |
| `WeaverKEM-256` | enc | 38448 | 1452 KiB | 1520 KiB |
| `WeaverKEM-256` | dec | 38448 | 1456 KiB | 1524 KiB |
| `WeaverKEM-512` | keygen | 43000 | 1444 KiB | 1552 KiB |
| `WeaverKEM-512` | enc | 43000 | 1484 KiB | 1552 KiB |
| `WeaverKEM-512` | dec | 43000 | 1488 KiB | 3600 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `WeaverKEM-128` | 752 | 1776 | 816 | 16 |
| `WeaverKEM-256` | 1312 | 3072 | 1536 | 32 |
| `WeaverKEM-512` | 2880 | 6400 | 3392 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — grep hits for AES/Keccak/OpenSSL are not reachable in the built library

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `WeaverKEM-128` | keygen | 85% | 1.1% | drng 1, pseudoXOF 31, sm3hash 1 |
| `WeaverKEM-128` | enc | 84% | 0.8% | drng 1, pseudoXOF 36, sm3hash 1 |
| `WeaverKEM-128` | dec | 82% | 0.0% | pseudoXOF 37 |
| `WeaverKEM-256` | keygen | 80% | 0.7% | drng 1, pseudoXOF 21, pseudohash 1 |
| `WeaverKEM-256` | enc | 77% | 0.5% | drng 1, pseudoXOF 25, pseudohash 1 |
| `WeaverKEM-256` | dec | 74% | 0.0% | pseudoXOF 26 |
| `WeaverKEM-512` | keygen | 86% | 0.3% | drng 1, pseudoXOF 21, pseudohash 1 |
| `WeaverKEM-512` | enc | 82% | 0.2% | drng 1, pseudoXOF 29, pseudohash 1 |
| `WeaverKEM-512` | dec | 79% | 0.0% | pseudoXOF 30 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `WeaverKEM-128` | KAT log (sha256 `455a06fbce9cf697…`) | `kat/kem-39/WeaverKEM-128.log` |
| `WeaverKEM-128` | timing dec | `records/kem-39/WeaverKEM-128__dec.json` |
| `WeaverKEM-128` | timing enc | `records/kem-39/WeaverKEM-128__enc.json` |
| `WeaverKEM-128` | timing keygen | `records/kem-39/WeaverKEM-128__keygen.json` |
| `WeaverKEM-128` | hash profile dec | `profile/kem-39/WeaverKEM-128__dec.json` |
| `WeaverKEM-128` | hash profile enc | `profile/kem-39/WeaverKEM-128__enc.json` |
| `WeaverKEM-128` | hash profile keygen | `profile/kem-39/WeaverKEM-128__keygen.json` |
| `WeaverKEM-256` | KAT log (sha256 `047ff53b100fcc37…`) | `kat/kem-39/WeaverKEM-256.log` |
| `WeaverKEM-256` | timing dec | `records/kem-39/WeaverKEM-256__dec.json` |
| `WeaverKEM-256` | timing enc | `records/kem-39/WeaverKEM-256__enc.json` |
| `WeaverKEM-256` | timing keygen | `records/kem-39/WeaverKEM-256__keygen.json` |
| `WeaverKEM-256` | hash profile dec | `profile/kem-39/WeaverKEM-256__dec.json` |
| `WeaverKEM-256` | hash profile enc | `profile/kem-39/WeaverKEM-256__enc.json` |
| `WeaverKEM-256` | hash profile keygen | `profile/kem-39/WeaverKEM-256__keygen.json` |
| `WeaverKEM-512` | KAT log (sha256 `401bb7284209f9a7…`) | `kat/kem-39/WeaverKEM-512.log` |
| `WeaverKEM-512` | timing dec | `records/kem-39/WeaverKEM-512__dec.json` |
| `WeaverKEM-512` | timing enc | `records/kem-39/WeaverKEM-512__enc.json` |
| `WeaverKEM-512` | timing keygen | `records/kem-39/WeaverKEM-512__keygen.json` |
| `WeaverKEM-512` | hash profile dec | `profile/kem-39/WeaverKEM-512__dec.json` |
| `WeaverKEM-512` | hash profile enc | `profile/kem-39/WeaverKEM-512__enc.json` |
| `WeaverKEM-512` | hash profile keygen | `profile/kem-39/WeaverKEM-512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

