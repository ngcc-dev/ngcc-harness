# kem-38 UVW Key Encapsulation Mechanism — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-38` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560881697083392.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: UVW Key Encapsulation Mechanism
- Implementation versions measured: reference
- Parameter sets: `UVW-KEM-128`, `UVW-KEM-256`, `UVW-KEM-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-38/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `UVW-KEM-128` | guide | PASS |
| `UVW-KEM-256` | guide | PASS |
| `UVW-KEM-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `UVW-KEM-128` | keygen | 677.81 M | 252 ms | 3.98 | 252 ms | 100 (5 × 20) |
| `UVW-KEM-128` | enc | 534.0 k | 198 µs | 5.05e+03 | 198 µs | 15535 (5 × 3107) |
| `UVW-KEM-128` | dec | 1.47 G | 546 ms | 1.83 | 546 ms | 100 (5 × 20) |
| `UVW-KEM-256` | keygen | 5.22 G | 1.94 s | 0.516 | 1.94 s | 100 (5 × 20) |
| `UVW-KEM-256` | enc | 1.27 M | 471 µs | 2.12e+03 | 470 µs | 6640 (5 × 1328) |
| `UVW-KEM-256` | dec | 9.49 G | 3.52 s | 0.284 | 3.52 s | 100 (5 × 20) |
| `UVW-KEM-512` | keygen | 41.20 G | 15.3 s | 0.0654 | 15.3 s | 20 (5 × 4) |
| `UVW-KEM-512` | enc | 19.79 M | 7.35 ms | 136 | 7.35 ms | 430 (5 × 86) |
| `UVW-KEM-512` | dec | 66.64 G | 24.7 s | 0.0404 | 24.7 s | 15 (5 × 3) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `UVW-KEM-128` | keygen | 221720 | 1656 KiB | 6480 KiB |
| `UVW-KEM-128` | enc | 221720 | 4312 KiB | 4984 KiB |
| `UVW-KEM-128` | dec | 221720 | 7976 KiB | 8040 KiB |
| `UVW-KEM-256` | keygen | 765244 | 3596 KiB | 15464 KiB |
| `UVW-KEM-256` | enc | 765244 | 6320 KiB | 14076 KiB |
| `UVW-KEM-256` | dec | 765244 | 14068 KiB | 44852 KiB |
| `UVW-KEM-512` | keygen | 2948436 | 1464 KiB | 52248 KiB |
| `UVW-KEM-512` | enc | 2948436 | 15856 KiB | 45512 KiB |
| `UVW-KEM-512` | dec | 2948436 | 33880 KiB | 110160 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `UVW-KEM-128` | 208013 | 35 | 1032 | 64 |
| `UVW-KEM-256` | 911645 | 35 | 2199 | 64 |
| `UVW-KEM-512` | 4001850 | 67 | 4820 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `UVW-KEM-128` | keygen | 0.0% | 2.9% | drng 218 |
| `UVW-KEM-128` | enc | 25% | 35% | drng 3, pseudoXOF 4 |
| `UVW-KEM-128` | dec | 0.0% | 2.7% | drng 437, pseudoXOF 5 |
| `UVW-KEM-256` | keygen | 0.0% | 1.5% | drng 858 |
| `UVW-KEM-256` | enc | 19% | 18% | drng 3.41, pseudoXOF 4 |
| `UVW-KEM-256` | dec | 0.0% | 1.6% | drng 1.72e+03, pseudoXOF 5 |
| `UVW-KEM-512` | keygen | 0.0% | 0.7% | drng 3.42e+03 |
| `UVW-KEM-512` | enc | 3.5% | 2.0% | drng 5.37, pseudoXOF 4 |
| `UVW-KEM-512` | dec | 0.0% | 0.9% | drng 6.84e+03, pseudoXOF 5 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `UVW-KEM-128` | KAT log (sha256 `8ca83c17d929f4cd…`) | `kat/kem-38/UVW-KEM-128.log` |
| `UVW-KEM-128` | timing dec | `records/kem-38/UVW-KEM-128__dec.json` |
| `UVW-KEM-128` | timing enc | `records/kem-38/UVW-KEM-128__enc.json` |
| `UVW-KEM-128` | timing keygen | `records/kem-38/UVW-KEM-128__keygen.json` |
| `UVW-KEM-128` | hash profile dec | `profile/kem-38/UVW-KEM-128__dec.json` |
| `UVW-KEM-128` | hash profile enc | `profile/kem-38/UVW-KEM-128__enc.json` |
| `UVW-KEM-128` | hash profile keygen | `profile/kem-38/UVW-KEM-128__keygen.json` |
| `UVW-KEM-256` | KAT log (sha256 `bc3cf9b29dccd627…`) | `kat/kem-38/UVW-KEM-256.log` |
| `UVW-KEM-256` | timing dec | `records/kem-38/UVW-KEM-256__dec.json` |
| `UVW-KEM-256` | timing enc | `records/kem-38/UVW-KEM-256__enc.json` |
| `UVW-KEM-256` | timing keygen | `records/kem-38/UVW-KEM-256__keygen.json` |
| `UVW-KEM-256` | hash profile dec | `profile/kem-38/UVW-KEM-256__dec.json` |
| `UVW-KEM-256` | hash profile enc | `profile/kem-38/UVW-KEM-256__enc.json` |
| `UVW-KEM-256` | hash profile keygen | `profile/kem-38/UVW-KEM-256__keygen.json` |
| `UVW-KEM-512` | KAT log (sha256 `b98f468328ace035…`) | `kat/kem-38/UVW-KEM-512.log` |
| `UVW-KEM-512` | timing dec | `records/kem-38/UVW-KEM-512__dec.json` |
| `UVW-KEM-512` | timing enc | `records/kem-38/UVW-KEM-512__enc.json` |
| `UVW-KEM-512` | timing keygen | `records/kem-38/UVW-KEM-512__keygen.json` |
| `UVW-KEM-512` | hash profile dec | `profile/kem-38/UVW-KEM-512__dec.json` |
| `UVW-KEM-512` | hash profile enc | `profile/kem-38/UVW-KEM-512__enc.json` |
| `UVW-KEM-512` | hash profile keygen | `profile/kem-38/UVW-KEM-512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

