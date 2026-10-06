# kem-18 LoongKEM — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-18` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560853666549760.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: LoongKEM
- Implementation versions measured: reference
- Parameter sets: `Loong128`, `Loong256`, `Loong384`, `Loong512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-18/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Loong128` | guide | PASS |
| `Loong256` | guide | PASS |
| `Loong384` | guide | PASS |
| `Loong512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Loong128` | keygen | 5.81 M | 2.16 ms | 464 | 2.05 ms | 1480 (5 × 296) |
| `Loong128` | enc | 5.56 M | 2.06 ms | 485 | 2.06 ms | 1525 (5 × 305) |
| `Loong128` | dec | 5.64 M | 2.09 ms | 478 | 2.08 ms | 1480 (5 × 296) |
| `Loong256` | keygen | 13.73 M | 5.09 ms | 196 | 5.09 ms | 490 (5 × 98) |
| `Loong256` | enc | 13.82 M | 5.13 ms | 195 | 5.13 ms | 615 (5 × 123) |
| `Loong256` | dec | 14.08 M | 5.23 ms | 191 | 5.19 ms | 610 (5 × 122) |
| `Loong384` | keygen | 43.04 M | 16 ms | 62.6 | 16 ms | 195 (5 × 39) |
| `Loong384` | enc | 43.53 M | 16.1 ms | 61.9 | 16 ms | 200 (5 × 40) |
| `Loong384` | dec | 43.73 M | 16.2 ms | 61.6 | 16.2 ms | 190 (5 × 38) |
| `Loong512` | keygen | 71.33 M | 26.5 ms | 37.8 | 26.4 ms | 120 (5 × 24) |
| `Loong512` | enc | 71.52 M | 26.5 ms | 37.7 | 26.5 ms | 115 (5 × 23) |
| `Loong512` | dec | 72.30 M | 26.8 ms | 37.3 | 26.8 ms | 120 (5 × 24) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Loong128` | keygen | 29272 | 1424 KiB | 1768 KiB |
| `Loong128` | enc | 29272 | 1708 KiB | 1776 KiB |
| `Loong128` | dec | 29272 | 1716 KiB | 1784 KiB |
| `Loong256` | keygen | 24580 | 1424 KiB | 2200 KiB |
| `Loong256` | enc | 24580 | 2140 KiB | 2212 KiB |
| `Loong256` | dec | 24580 | 2164 KiB | 2236 KiB |
| `Loong384` | keygen | 25844 | 3452 KiB | 4616 KiB |
| `Loong384` | enc | 25844 | 4280 KiB | 4344 KiB |
| `Loong384` | dec | 25844 | 4308 KiB | 4764 KiB |
| `Loong512` | keygen | 25568 | 1444 KiB | 5368 KiB |
| `Loong512` | enc | 25568 | 3528 KiB | 5508 KiB |
| `Loong512` | dec | 25568 | 3588 KiB | 5516 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `Loong128` | 1472 | 1848 | 1512 | 16 |
| `Loong256` | 3248 | 3888 | 3520 | 32 |
| `Loong384` | 6444 | 7340 | 6680 | 48 |
| `Loong512` | 10640 | 11832 | 10848 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Loong128` | keygen | 83% | 0.2% | drng 2, pseudoXOF 3, sm3hash 1 |
| `Loong128` | enc | 83% | 0.1% | drng 1, pseudoXOF 5, sm3hash 1 |
| `Loong128` | dec | 82% | 0.0% | pseudoXOF 6 |
| `Loong256` | keygen | 73% | 0.1% | drng 2, pseudoXOF 3, sm3hash 1 |
| `Loong256` | enc | 84% | 0.0% | drng 1, pseudoXOF 5, sm3hash 1 |
| `Loong256` | dec | 83% | 0.0% | pseudoXOF 6 |
| `Loong384` | keygen | 89% | 0.0% | drng 2, pseudoXOF 3, sm3hash 1 |
| `Loong384` | enc | 88% | 0.0% | drng 1, pseudoXOF 5, sm3hash 1 |
| `Loong384` | dec | 88% | 0.0% | pseudoXOF 6 |
| `Loong512` | keygen | 88% | 0.0% | drng 2, pseudoXOF 3, sm3hash 1 |
| `Loong512` | enc | 87% | 0.0% | drng 1, pseudoXOF 5, sm3hash 1 |
| `Loong512` | dec | 86% | 0.0% | pseudoXOF 6 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Loong128` | KAT log (sha256 `beb359f015e9c806…`) | `kat/kem-18/Loong128.log` |
| `Loong128` | timing dec | `records/kem-18/Loong128__dec.json` |
| `Loong128` | timing enc | `records/kem-18/Loong128__enc.json` |
| `Loong128` | timing keygen | `records/kem-18/Loong128__keygen.json` |
| `Loong128` | hash profile dec | `profile/kem-18/Loong128__dec.json` |
| `Loong128` | hash profile enc | `profile/kem-18/Loong128__enc.json` |
| `Loong128` | hash profile keygen | `profile/kem-18/Loong128__keygen.json` |
| `Loong256` | KAT log (sha256 `df131a2c805665fa…`) | `kat/kem-18/Loong256.log` |
| `Loong256` | timing dec | `records/kem-18/Loong256__dec.json` |
| `Loong256` | timing enc | `records/kem-18/Loong256__enc.json` |
| `Loong256` | timing keygen | `records/kem-18/Loong256__keygen.json` |
| `Loong256` | hash profile dec | `profile/kem-18/Loong256__dec.json` |
| `Loong256` | hash profile enc | `profile/kem-18/Loong256__enc.json` |
| `Loong256` | hash profile keygen | `profile/kem-18/Loong256__keygen.json` |
| `Loong384` | KAT log (sha256 `3934679dc0b28256…`) | `kat/kem-18/Loong384.log` |
| `Loong384` | timing dec | `records/kem-18/Loong384__dec.json` |
| `Loong384` | timing enc | `records/kem-18/Loong384__enc.json` |
| `Loong384` | timing keygen | `records/kem-18/Loong384__keygen.json` |
| `Loong384` | hash profile dec | `profile/kem-18/Loong384__dec.json` |
| `Loong384` | hash profile enc | `profile/kem-18/Loong384__enc.json` |
| `Loong384` | hash profile keygen | `profile/kem-18/Loong384__keygen.json` |
| `Loong512` | KAT log (sha256 `3b88498cfd76a96f…`) | `kat/kem-18/Loong512.log` |
| `Loong512` | timing dec | `records/kem-18/Loong512__dec.json` |
| `Loong512` | timing enc | `records/kem-18/Loong512__enc.json` |
| `Loong512` | timing keygen | `records/kem-18/Loong512__keygen.json` |
| `Loong512` | hash profile dec | `profile/kem-18/Loong512__dec.json` |
| `Loong512` | hash profile enc | `profile/kem-18/Loong512__enc.json` |
| `Loong512` | hash profile keygen | `profile/kem-18/Loong512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

