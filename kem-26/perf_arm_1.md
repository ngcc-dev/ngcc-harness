# kem-26 NSS-HQC — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-26` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560863191814144.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: NSS-HQC
- Implementation versions measured: reference
- Parameter sets: `HQC-128`, `HQC-256`, `HQC-384`, `HQC-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-26/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `HQC-128` | guide | PASS |
| `HQC-256` | guide | PASS |
| `HQC-384` | guide | PASS |
| `HQC-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `HQC-128` | keygen | 26.00 M | 9.65 ms | 104 | 9.65 ms | 345 (5 × 69) |
| `HQC-128` | enc | 46.47 M | 17.2 ms | 58 | 17.3 ms | 185 (5 × 37) |
| `HQC-128` | dec | 92.01 M | 34.1 ms | 29.3 | 34.1 ms | 100 (5 × 20) |
| `HQC-256` | keygen | 76.97 M | 28.6 ms | 35 | 28.6 ms | 115 (5 × 23) |
| `HQC-256` | enc | 139.39 M | 51.7 ms | 19.3 | 51.8 ms | 100 (5 × 20) |
| `HQC-256` | dec | 246.34 M | 91.4 ms | 10.9 | 91.4 ms | 100 (5 × 20) |
| `HQC-384` | keygen | 258.98 M | 96.1 ms | 10.4 | 96.1 ms | 100 (5 × 20) |
| `HQC-384` | enc | 469.63 M | 174 ms | 5.74 | 174 ms | 100 (5 × 20) |
| `HQC-384` | dec | 775.08 M | 288 ms | 3.48 | 288 ms | 100 (5 × 20) |
| `HQC-512` | keygen | 614.26 M | 228 ms | 4.39 | 228 ms | 100 (5 × 20) |
| `HQC-512` | enc | 1.12 G | 416 ms | 2.4 | 416 ms | 100 (5 × 20) |
| `HQC-512` | dec | 1.80 G | 667 ms | 1.5 | 667 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `HQC-128` | keygen | 39852 | 3468 KiB | 3532 KiB |
| `HQC-128` | enc | 39852 | 3484 KiB | 3548 KiB |
| `HQC-128` | dec | 39852 | 3584 KiB | 3648 KiB |
| `HQC-256` | keygen | 44408 | 1452 KiB | 3680 KiB |
| `HQC-256` | enc | 44408 | 1708 KiB | 3760 KiB |
| `HQC-256` | dec | 44408 | 1740 KiB | 1804 KiB |
| `HQC-384` | keygen | 40932 | 1472 KiB | 4004 KiB |
| `HQC-384` | enc | 40932 | 3688 KiB | 3976 KiB |
| `HQC-384` | dec | 40932 | 3548 KiB | 3924 KiB |
| `HQC-512` | keygen | 41076 | 1508 KiB | 3888 KiB |
| `HQC-512` | enc | 41076 | 2516 KiB | 4456 KiB |
| `HQC-512` | dec | 41076 | 3680 KiB | 4604 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `HQC-128` | 3713 | 3777 | 5185 | 32 |
| `HQC-256` | 6844 | 6908 | 9084 | 32 |
| `HQC-384` | 15371 | 15467 | 18571 | 48 |
| `HQC-512` | 27302 | 27430 | 31462 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **bypass** — own inline Keccak (SHA3-512, SHAKE256) for everything; DRNG for randomness

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `HQC-128` | keygen | 0.0% | 0.0% | drng 3 |
| `HQC-128` | enc | 0.0% | 0.0% | drng 2 |
| `HQC-128` | dec | 0.0% | 0.0% | – |
| `HQC-256` | keygen | 0.0% | 0.0% | drng 3 |
| `HQC-256` | enc | 0.0% | 0.0% | drng 2 |
| `HQC-256` | dec | 0.0% | 0.0% | – |
| `HQC-384` | keygen | 0.0% | 0.0% | drng 3 |
| `HQC-384` | enc | 0.0% | 0.0% | drng 2 |
| `HQC-384` | dec | 0.0% | 0.0% | – |
| `HQC-512` | keygen | 0.0% | 0.0% | drng 3 |
| `HQC-512` | enc | 0.0% | 0.0% | drng 2 |
| `HQC-512` | dec | 0.0% | 0.0% | – |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `HQC-128` | KAT log (sha256 `c620e4db85598389…`) | `kat/kem-26/HQC-128.log` |
| `HQC-128` | timing dec | `records/kem-26/HQC-128__dec.json` |
| `HQC-128` | timing enc | `records/kem-26/HQC-128__enc.json` |
| `HQC-128` | timing keygen | `records/kem-26/HQC-128__keygen.json` |
| `HQC-128` | hash profile dec | `profile/kem-26/HQC-128__dec.json` |
| `HQC-128` | hash profile enc | `profile/kem-26/HQC-128__enc.json` |
| `HQC-128` | hash profile keygen | `profile/kem-26/HQC-128__keygen.json` |
| `HQC-256` | KAT log (sha256 `5db1c6724d4a0538…`) | `kat/kem-26/HQC-256.log` |
| `HQC-256` | timing dec | `records/kem-26/HQC-256__dec.json` |
| `HQC-256` | timing enc | `records/kem-26/HQC-256__enc.json` |
| `HQC-256` | timing keygen | `records/kem-26/HQC-256__keygen.json` |
| `HQC-256` | hash profile dec | `profile/kem-26/HQC-256__dec.json` |
| `HQC-256` | hash profile enc | `profile/kem-26/HQC-256__enc.json` |
| `HQC-256` | hash profile keygen | `profile/kem-26/HQC-256__keygen.json` |
| `HQC-384` | KAT log (sha256 `2d5f3b2fd30882ea…`) | `kat/kem-26/HQC-384.log` |
| `HQC-384` | timing dec | `records/kem-26/HQC-384__dec.json` |
| `HQC-384` | timing enc | `records/kem-26/HQC-384__enc.json` |
| `HQC-384` | timing keygen | `records/kem-26/HQC-384__keygen.json` |
| `HQC-384` | hash profile dec | `profile/kem-26/HQC-384__dec.json` |
| `HQC-384` | hash profile enc | `profile/kem-26/HQC-384__enc.json` |
| `HQC-384` | hash profile keygen | `profile/kem-26/HQC-384__keygen.json` |
| `HQC-512` | KAT log (sha256 `d581449c9afccafb…`) | `kat/kem-26/HQC-512.log` |
| `HQC-512` | timing dec | `records/kem-26/HQC-512__dec.json` |
| `HQC-512` | timing enc | `records/kem-26/HQC-512__enc.json` |
| `HQC-512` | timing keygen | `records/kem-26/HQC-512__keygen.json` |
| `HQC-512` | hash profile dec | `profile/kem-26/HQC-512__dec.json` |
| `HQC-512` | hash profile enc | `profile/kem-26/HQC-512__enc.json` |
| `HQC-512` | hash profile keygen | `profile/kem-26/HQC-512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

