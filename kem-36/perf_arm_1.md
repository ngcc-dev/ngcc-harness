# kem-36 TRIKE — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-36` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560881424453632.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: TRIKE
- Implementation versions measured: reference
- Parameter sets: `TRIKE-2`, `TRIKE-5`, `TRIKE-7`, `TRIKE-9`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-36/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `TRIKE-2` | guide | PASS |
| `TRIKE-5` | guide | PASS |
| `TRIKE-7` | guide | PASS |
| `TRIKE-9` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `TRIKE-2` | keygen | 63.90 M | 23.7 ms | 42.2 | 23.5 ms | 135 (5 × 27) |
| `TRIKE-2` | enc | 6.65 M | 2.47 ms | 405 | 2.46 ms | 1285 (5 × 257) |
| `TRIKE-2` | dec | 35.18 M | 13.1 ms | 76.6 | 13.1 ms | 245 (5 × 49) |
| `TRIKE-5` | keygen | 474.51 M | 176 ms | 5.68 | 176 ms | 100 (5 × 20) |
| `TRIKE-5` | enc | 45.93 M | 17 ms | 58.7 | 17 ms | 190 (5 × 38) |
| `TRIKE-5` | dec | 133.26 M | 49.4 ms | 20.2 | 49.4 ms | 100 (5 × 20) |
| `TRIKE-7` | keygen | 1.54 G | 572 ms | 1.75 | 572 ms | 100 (5 × 20) |
| `TRIKE-7` | enc | 133.31 M | 49.5 ms | 20.2 | 49.5 ms | 100 (5 × 20) |
| `TRIKE-7` | dec | 405.06 M | 150 ms | 6.65 | 150 ms | 100 (5 × 20) |
| `TRIKE-9` | keygen | 1.94 G | 720 ms | 1.39 | 720 ms | 100 (5 × 20) |
| `TRIKE-9` | enc | 136.51 M | 50.6 ms | 19.7 | 50.6 ms | 100 (5 × 20) |
| `TRIKE-9` | dec | 796.85 M | 296 ms | 3.38 | 296 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `TRIKE-2` | keygen | 27408 | 1424 KiB | 1548 KiB |
| `TRIKE-2` | enc | 27408 | 1492 KiB | 1560 KiB |
| `TRIKE-2` | dec | 27408 | 1660 KiB | 3840 KiB |
| `TRIKE-5` | keygen | 27840 | 1444 KiB | 3740 KiB |
| `TRIKE-5` | enc | 27840 | 3556 KiB | 3620 KiB |
| `TRIKE-5` | dec | 27840 | 3656 KiB | 3996 KiB |
| `TRIKE-7` | keygen | 27736 | 3468 KiB | 3848 KiB |
| `TRIKE-7` | enc | 27736 | 3648 KiB | 3712 KiB |
| `TRIKE-7` | dec | 27736 | 3600 KiB | 4028 KiB |
| `TRIKE-9` | keygen | 27576 | 1504 KiB | 3772 KiB |
| `TRIKE-9` | enc | 27576 | 3848 KiB | 3912 KiB |
| `TRIKE-9` | dec | 27576 | 3900 KiB | 5592 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `TRIKE-2` | 1980 | 6328 | 3928 | 32 |
| `TRIKE-5` | 4453 | 13988 | 8874 | 32 |
| `TRIKE-7` | 8776 | 27260 | 17488 | 64 |
| `TRIKE-9` | 14320 | 44228 | 28576 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `TRIKE-2` | keygen | 0.0% | 1.2% | drng 113 |
| `TRIKE-2` | enc | 6.3% | 21% | drng 267, pseudohash 2 |
| `TRIKE-2` | dec | 1.2% | 3.9% | drng 266, pseudohash 2 |
| `TRIKE-5` | keygen | 0.0% | 0.3% | drng 171 |
| `TRIKE-5` | enc | 2.0% | 5.3% | drng 433, pseudohash 2 |
| `TRIKE-5` | dec | 0.7% | 1.8% | drng 432, pseudohash 2 |
| `TRIKE-7` | keygen | 0.0% | 0.1% | drng 255 |
| `TRIKE-7` | enc | 1.3% | 3.0% | drng 663, pseudohash 2 |
| `TRIKE-7` | dec | 0.4% | 1.0% | drng 662, pseudohash 2 |
| `TRIKE-9` | keygen | 0.0% | 0.2% | drng 339 |
| `TRIKE-9` | enc | 2.1% | 4.1% | drng 881, pseudohash 2 |
| `TRIKE-9` | dec | 0.4% | 0.7% | drng 880, pseudohash 2 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `TRIKE-2` | KAT log (sha256 `45ce672140b72918…`) | `kat/kem-36/TRIKE-2.log` |
| `TRIKE-2` | timing dec | `records/kem-36/TRIKE-2__dec.json` |
| `TRIKE-2` | timing enc | `records/kem-36/TRIKE-2__enc.json` |
| `TRIKE-2` | timing keygen | `records/kem-36/TRIKE-2__keygen.json` |
| `TRIKE-2` | hash profile dec | `profile/kem-36/TRIKE-2__dec.json` |
| `TRIKE-2` | hash profile enc | `profile/kem-36/TRIKE-2__enc.json` |
| `TRIKE-2` | hash profile keygen | `profile/kem-36/TRIKE-2__keygen.json` |
| `TRIKE-5` | KAT log (sha256 `f03ad615d441c702…`) | `kat/kem-36/TRIKE-5.log` |
| `TRIKE-5` | timing dec | `records/kem-36/TRIKE-5__dec.json` |
| `TRIKE-5` | timing enc | `records/kem-36/TRIKE-5__enc.json` |
| `TRIKE-5` | timing keygen | `records/kem-36/TRIKE-5__keygen.json` |
| `TRIKE-5` | hash profile dec | `profile/kem-36/TRIKE-5__dec.json` |
| `TRIKE-5` | hash profile enc | `profile/kem-36/TRIKE-5__enc.json` |
| `TRIKE-5` | hash profile keygen | `profile/kem-36/TRIKE-5__keygen.json` |
| `TRIKE-7` | KAT log (sha256 `62cc4313b653022e…`) | `kat/kem-36/TRIKE-7.log` |
| `TRIKE-7` | timing dec | `records/kem-36/TRIKE-7__dec.json` |
| `TRIKE-7` | timing enc | `records/kem-36/TRIKE-7__enc.json` |
| `TRIKE-7` | timing keygen | `records/kem-36/TRIKE-7__keygen.json` |
| `TRIKE-7` | hash profile dec | `profile/kem-36/TRIKE-7__dec.json` |
| `TRIKE-7` | hash profile enc | `profile/kem-36/TRIKE-7__enc.json` |
| `TRIKE-7` | hash profile keygen | `profile/kem-36/TRIKE-7__keygen.json` |
| `TRIKE-9` | KAT log (sha256 `ca06ad9161d1d573…`) | `kat/kem-36/TRIKE-9.log` |
| `TRIKE-9` | timing dec | `records/kem-36/TRIKE-9__dec.json` |
| `TRIKE-9` | timing enc | `records/kem-36/TRIKE-9__enc.json` |
| `TRIKE-9` | timing keygen | `records/kem-36/TRIKE-9__keygen.json` |
| `TRIKE-9` | hash profile dec | `profile/kem-36/TRIKE-9__dec.json` |
| `TRIKE-9` | hash profile enc | `profile/kem-36/TRIKE-9__enc.json` |
| `TRIKE-9` | hash profile keygen | `profile/kem-36/TRIKE-9__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

