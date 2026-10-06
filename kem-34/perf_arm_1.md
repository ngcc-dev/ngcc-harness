# kem-34 Rudraksh2 — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-34` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560881139240960.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: Rudraksh2
- Implementation versions measured: reference
- Parameter sets: `lwekem128`, `lwekem256`, `lwekem512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-34/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `lwekem128` | guide | PASS |
| `lwekem256` | guide | PASS |
| `lwekem512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `lwekem128` | keygen | 2.89 M | 1.07 ms | 932 | 1.07 ms | 2860 (5 × 572) |
| `lwekem128` | enc | 2.87 M | 1.07 ms | 938 | 1.07 ms | 2960 (5 × 592) |
| `lwekem128` | dec | 2.92 M | 1.08 ms | 922 | 1.08 ms | 2920 (5 × 584) |
| `lwekem256` | keygen | 5.84 M | 2.16 ms | 462 | 2.17 ms | 1440 (5 × 288) |
| `lwekem256` | enc | 5.82 M | 2.16 ms | 463 | 2.16 ms | 1455 (5 × 291) |
| `lwekem256` | dec | 5.94 M | 2.21 ms | 453 | 2.18 ms | 1420 (5 × 284) |
| `lwekem512` | keygen | 18.83 M | 6.99 ms | 143 | 6.98 ms | 450 (5 × 90) |
| `lwekem512` | enc | 18.80 M | 6.97 ms | 143 | 6.97 ms | 455 (5 × 91) |
| `lwekem512` | dec | 18.94 M | 7.03 ms | 142 | 7.03 ms | 450 (5 × 90) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `lwekem128` | keygen | 25396 | 1416 KiB | 1496 KiB |
| `lwekem128` | enc | 25396 | 1432 KiB | 1500 KiB |
| `lwekem128` | dec | 25396 | 1432 KiB | 1496 KiB |
| `lwekem256` | keygen | 26068 | 1420 KiB | 1516 KiB |
| `lwekem256` | enc | 26068 | 1452 KiB | 1520 KiB |
| `lwekem256` | dec | 26068 | 1452 KiB | 1520 KiB |
| `lwekem512` | keygen | 28076 | 3448 KiB | 3560 KiB |
| `lwekem512` | enc | 28076 | 1484 KiB | 1552 KiB |
| `lwekem512` | dec | 28076 | 1492 KiB | 1560 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `lwekem128` | 880 | 1776 | 912 | 16 |
| `lwekem256` | 1760 | 3552 | 1728 | 32 |
| `lwekem512` | 3392 | 6848 | 3552 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `lwekem128` | keygen | 94% | 0.3% | drng 2, pseudoXOF 101 |
| `lwekem128` | enc | 94% | 0.1% | drng 1, pseudoXOF 102 |
| `lwekem128` | dec | 93% | 0.0% | pseudoXOF 102 |
| `lwekem256` | keygen | 94% | 0.1% | drng 2, pseudoXOF 101 |
| `lwekem256` | enc | 94% | 0.1% | drng 1, pseudoXOF 102 |
| `lwekem256` | dec | 93% | 0.0% | pseudoXOF 102 |
| `lwekem512` | keygen | 97% | 0.1% | drng 2, pseudoXOF 82 |
| `lwekem512` | enc | 98% | 0.0% | drng 1, pseudoXOF 83 |
| `lwekem512` | dec | 97% | 0.0% | pseudoXOF 83 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `lwekem128` | KAT log (sha256 `0b425f13e606a2ad…`) | `kat/kem-34/lwekem128.log` |
| `lwekem128` | timing dec | `records/kem-34/lwekem128__dec.json` |
| `lwekem128` | timing enc | `records/kem-34/lwekem128__enc.json` |
| `lwekem128` | timing keygen | `records/kem-34/lwekem128__keygen.json` |
| `lwekem128` | hash profile dec | `profile/kem-34/lwekem128__dec.json` |
| `lwekem128` | hash profile enc | `profile/kem-34/lwekem128__enc.json` |
| `lwekem128` | hash profile keygen | `profile/kem-34/lwekem128__keygen.json` |
| `lwekem256` | KAT log (sha256 `f08edc3b80b3fc15…`) | `kat/kem-34/lwekem256.log` |
| `lwekem256` | timing dec | `records/kem-34/lwekem256__dec.json` |
| `lwekem256` | timing enc | `records/kem-34/lwekem256__enc.json` |
| `lwekem256` | timing keygen | `records/kem-34/lwekem256__keygen.json` |
| `lwekem256` | hash profile dec | `profile/kem-34/lwekem256__dec.json` |
| `lwekem256` | hash profile enc | `profile/kem-34/lwekem256__enc.json` |
| `lwekem256` | hash profile keygen | `profile/kem-34/lwekem256__keygen.json` |
| `lwekem512` | KAT log (sha256 `40c58106c46a7c47…`) | `kat/kem-34/lwekem512.log` |
| `lwekem512` | timing dec | `records/kem-34/lwekem512__dec.json` |
| `lwekem512` | timing enc | `records/kem-34/lwekem512__enc.json` |
| `lwekem512` | timing keygen | `records/kem-34/lwekem512__keygen.json` |
| `lwekem512` | hash profile dec | `profile/kem-34/lwekem512__dec.json` |
| `lwekem512` | hash profile enc | `profile/kem-34/lwekem512__enc.json` |
| `lwekem512` | hash profile keygen | `profile/kem-34/lwekem512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

