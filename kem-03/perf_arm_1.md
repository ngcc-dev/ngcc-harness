# kem-03 BAG-Loong — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-03` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560843247898624.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: BAG-Loong
- Implementation versions measured: reference
- Parameter sets: `BAG-Loong-128`, `BAG-Loong-256`, `BAG-Loong-384`, `BAG-Loong-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-03/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `BAG-Loong-128` | guide | PASS |
| `BAG-Loong-256` | guide | PASS |
| `BAG-Loong-384` | guide | PASS |
| `BAG-Loong-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `BAG-Loong-128` | keygen | 48.72 M | 18.1 ms | 55.3 | 18 ms | 170 (5 × 34) |
| `BAG-Loong-128` | enc | 49.37 M | 18.3 ms | 54.5 | 18.3 ms | 175 (5 × 35) |
| `BAG-Loong-128` | dec | 88.21 M | 32.7 ms | 30.6 | 32.5 ms | 100 (5 × 20) |
| `BAG-Loong-256` | keygen | 125.94 M | 46.7 ms | 21.4 | 46.5 ms | 100 (5 × 20) |
| `BAG-Loong-256` | enc | 111.06 M | 41.2 ms | 24.3 | 41.2 ms | 100 (5 × 20) |
| `BAG-Loong-256` | dec | 214.51 M | 79.6 ms | 12.6 | 79.5 ms | 100 (5 × 20) |
| `BAG-Loong-384` | keygen | 279.28 M | 104 ms | 9.65 | 103 ms | 100 (5 × 20) |
| `BAG-Loong-384` | enc | 228.43 M | 84.8 ms | 11.8 | 84.6 ms | 100 (5 × 20) |
| `BAG-Loong-384` | dec | 438.21 M | 163 ms | 6.15 | 162 ms | 100 (5 × 20) |
| `BAG-Loong-512` | keygen | 468.28 M | 174 ms | 5.76 | 174 ms | 100 (5 × 20) |
| `BAG-Loong-512` | enc | 337.55 M | 125 ms | 7.98 | 125 ms | 100 (5 × 20) |
| `BAG-Loong-512` | dec | 637.67 M | 237 ms | 4.23 | 236 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `BAG-Loong-128` | keygen | 50348 | 1448 KiB | 4324 KiB |
| `BAG-Loong-128` | enc | 50348 | 3748 KiB | 4316 KiB |
| `BAG-Loong-128` | dec | 50348 | 2324 KiB | 2388 KiB |
| `BAG-Loong-256` | keygen | 50580 | 1460 KiB | 5320 KiB |
| `BAG-Loong-256` | enc | 50580 | 4372 KiB | 5128 KiB |
| `BAG-Loong-256` | dec | 50580 | 3808 KiB | 5332 KiB |
| `BAG-Loong-384` | keygen | 50660 | 1484 KiB | 5844 KiB |
| `BAG-Loong-384` | enc | 50660 | 4740 KiB | 5880 KiB |
| `BAG-Loong-384` | dec | 50660 | 4708 KiB | 5868 KiB |
| `BAG-Loong-512` | keygen | 50540 | 1516 KiB | 6708 KiB |
| `BAG-Loong-512` | enc | 50540 | 5704 KiB | 7752 KiB |
| `BAG-Loong-512` | dec | 50540 | 4340 KiB | 6388 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `BAG-Loong-128` | 2500 | 5064 | 3071 | 16 |
| `BAG-Loong-256` | 6597 | 13258 | 8400 | 32 |
| `BAG-Loong-384` | 12152 | 24368 | 15112 | 48 |
| `BAG-Loong-512` | 19043 | 38150 | 21660 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — `loong_hash_tagged_sm3_256` wraps sm3hash

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `BAG-Loong-128` | keygen | 78% | 0.0% | drng 3, pseudoXOF 2 |
| `BAG-Loong-128` | enc | 82% | 0.0% | drng 2, pseudoXOF 3, pseudohash 1, sm3hash 1 |
| `BAG-Loong-128` | dec | 75% | 0.0% | pseudoXOF 4, pseudohash 1, sm3hash 1 |
| `BAG-Loong-256` | keygen | 55% | 0.0% | drng 3, pseudoXOF 2 |
| `BAG-Loong-256` | enc | 66% | 0.0% | drng 2, pseudoXOF 3, pseudohash 1, sm3hash 1 |
| `BAG-Loong-256` | dec | 60% | 0.0% | pseudoXOF 4, pseudohash 1, sm3hash 1 |
| `BAG-Loong-384` | keygen | 52% | 0.0% | drng 3, pseudoXOF 2 |
| `BAG-Loong-384` | enc | 63% | 0.0% | drng 2, pseudoXOF 3, pseudohash 1, sm3hash 1 |
| `BAG-Loong-384` | dec | 61% | 0.0% | pseudoXOF 4, pseudohash 1, sm3hash 1 |
| `BAG-Loong-512` | keygen | 42% | 0.0% | drng 3, pseudoXOF 2 |
| `BAG-Loong-512` | enc | 58% | 0.0% | drng 2, pseudoXOF 3, pseudohash 1, sm3hash 1 |
| `BAG-Loong-512` | dec | 58% | 0.0% | pseudoXOF 4, pseudohash 1, sm3hash 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `BAG-Loong-128` | KAT log (sha256 `9676867b0011052d…`) | `kat/kem-03/BAG-Loong-128.log` |
| `BAG-Loong-128` | timing dec | `records/kem-03/BAG-Loong-128__dec.json` |
| `BAG-Loong-128` | timing enc | `records/kem-03/BAG-Loong-128__enc.json` |
| `BAG-Loong-128` | timing keygen | `records/kem-03/BAG-Loong-128__keygen.json` |
| `BAG-Loong-128` | hash profile dec | `profile/kem-03/BAG-Loong-128__dec.json` |
| `BAG-Loong-128` | hash profile enc | `profile/kem-03/BAG-Loong-128__enc.json` |
| `BAG-Loong-128` | hash profile keygen | `profile/kem-03/BAG-Loong-128__keygen.json` |
| `BAG-Loong-256` | KAT log (sha256 `492a5418cad31b79…`) | `kat/kem-03/BAG-Loong-256.log` |
| `BAG-Loong-256` | timing dec | `records/kem-03/BAG-Loong-256__dec.json` |
| `BAG-Loong-256` | timing enc | `records/kem-03/BAG-Loong-256__enc.json` |
| `BAG-Loong-256` | timing keygen | `records/kem-03/BAG-Loong-256__keygen.json` |
| `BAG-Loong-256` | hash profile dec | `profile/kem-03/BAG-Loong-256__dec.json` |
| `BAG-Loong-256` | hash profile enc | `profile/kem-03/BAG-Loong-256__enc.json` |
| `BAG-Loong-256` | hash profile keygen | `profile/kem-03/BAG-Loong-256__keygen.json` |
| `BAG-Loong-384` | KAT log (sha256 `d6d0af3b3a3039d8…`) | `kat/kem-03/BAG-Loong-384.log` |
| `BAG-Loong-384` | timing dec | `records/kem-03/BAG-Loong-384__dec.json` |
| `BAG-Loong-384` | timing enc | `records/kem-03/BAG-Loong-384__enc.json` |
| `BAG-Loong-384` | timing keygen | `records/kem-03/BAG-Loong-384__keygen.json` |
| `BAG-Loong-384` | hash profile dec | `profile/kem-03/BAG-Loong-384__dec.json` |
| `BAG-Loong-384` | hash profile enc | `profile/kem-03/BAG-Loong-384__enc.json` |
| `BAG-Loong-384` | hash profile keygen | `profile/kem-03/BAG-Loong-384__keygen.json` |
| `BAG-Loong-512` | KAT log (sha256 `933fdc21732ef6e5…`) | `kat/kem-03/BAG-Loong-512.log` |
| `BAG-Loong-512` | timing dec | `records/kem-03/BAG-Loong-512__dec.json` |
| `BAG-Loong-512` | timing enc | `records/kem-03/BAG-Loong-512__enc.json` |
| `BAG-Loong-512` | timing keygen | `records/kem-03/BAG-Loong-512__keygen.json` |
| `BAG-Loong-512` | hash profile dec | `profile/kem-03/BAG-Loong-512__dec.json` |
| `BAG-Loong-512` | hash profile enc | `profile/kem-03/BAG-Loong-512__enc.json` |
| `BAG-Loong-512` | hash profile keygen | `profile/kem-03/BAG-Loong-512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

