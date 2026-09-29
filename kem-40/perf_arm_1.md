# kem-40 YuanYang.KEM — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-40` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560890442207232.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: YuanYang.KEM
- Implementation versions measured: reference
- Parameter sets: `yuanyang-512`, `yuanyang-1024`, `yuanyang-2048`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-40/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `yuanyang-512` | guide | PASS |
| `yuanyang-1024` | guide | PASS |
| `yuanyang-2048` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `yuanyang-512` | keygen | 405.7 k | 151 µs | 6.64e+03 | 151 µs | 15795 (5 × 3159) |
| `yuanyang-512` | enc | 239.7 k | 89 µs | 1.12e+04 | 88.9 µs | 34860 (5 × 6972) |
| `yuanyang-512` | dec | 325.0 k | 121 µs | 8.29e+03 | 121 µs | 25710 (5 × 5142) |
| `yuanyang-1024` | keygen | 867.0 k | 322 µs | 3.11e+03 | 322 µs | 8960 (5 × 1792) |
| `yuanyang-1024` | enc | 411.7 k | 153 µs | 6.55e+03 | 153 µs | 20255 (5 × 4051) |
| `yuanyang-1024` | dec | 603.2 k | 224 µs | 4.47e+03 | 223 µs | 13905 (5 × 2781) |
| `yuanyang-2048` | keygen | 1.63 M | 605 µs | 1.65e+03 | 605 µs | 4270 (5 × 854) |
| `yuanyang-2048` | enc | 805.3 k | 299 µs | 3.35e+03 | 299 µs | 10485 (5 × 2097) |
| `yuanyang-2048` | dec | 1.23 M | 457 µs | 2.19e+03 | 457 µs | 6770 (5 × 1354) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `yuanyang-512` | keygen | 42256 | 1508 KiB | 1704 KiB |
| `yuanyang-512` | enc | 42256 | 1648 KiB | 1712 KiB |
| `yuanyang-512` | dec | 42256 | 3688 KiB | 3752 KiB |
| `yuanyang-1024` | keygen | 47240 | 1516 KiB | 1724 KiB |
| `yuanyang-1024` | enc | 47240 | 1664 KiB | 1732 KiB |
| `yuanyang-1024` | dec | 47240 | 1684 KiB | 1748 KiB |
| `yuanyang-2048` | keygen | 55928 | 3560 KiB | 3780 KiB |
| `yuanyang-2048` | enc | 55928 | 1696 KiB | 1764 KiB |
| `yuanyang-2048` | dec | 55928 | 1732 KiB | 1800 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `yuanyang-512` | 736 | 1536 | 656 | 16 |
| `yuanyang-1024` | 1568 | 3168 | 1344 | 32 |
| `yuanyang-2048` | 3328 | 6528 | 2880 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `yuanyang-512` | keygen | 9.2% | 27% | drng 2.35, pseudohash 1 |
| `yuanyang-512` | enc | 47% | 23% | drng 3, pseudohash 4 |
| `yuanyang-512` | dec | 26% | 16% | drng 2, pseudohash 4 |
| `yuanyang-1024` | keygen | 8.2% | 22% | drng 4, pseudohash 1 |
| `yuanyang-1024` | enc | 43% | 25% | drng 4, pseudohash 4 |
| `yuanyang-1024` | dec | 19% | 16% | drng 3, pseudohash 4 |
| `yuanyang-2048` | keygen | 8.6% | 20% | drng 6.88, pseudohash 1 |
| `yuanyang-2048` | enc | 39% | 25% | drng 6, pseudohash 4 |
| `yuanyang-2048` | dec | 15% | 16% | drng 5, pseudohash 4 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `yuanyang-512` | KAT log (sha256 `dc41ee1af227da4f…`) | `kat/kem-40/yuanyang-512.log` |
| `yuanyang-512` | timing dec | `records/kem-40/yuanyang-512__dec.json` |
| `yuanyang-512` | timing enc | `records/kem-40/yuanyang-512__enc.json` |
| `yuanyang-512` | timing keygen | `records/kem-40/yuanyang-512__keygen.json` |
| `yuanyang-512` | hash profile dec | `profile/kem-40/yuanyang-512__dec.json` |
| `yuanyang-512` | hash profile enc | `profile/kem-40/yuanyang-512__enc.json` |
| `yuanyang-512` | hash profile keygen | `profile/kem-40/yuanyang-512__keygen.json` |
| `yuanyang-1024` | KAT log (sha256 `583ffabb0bbcfc5f…`) | `kat/kem-40/yuanyang-1024.log` |
| `yuanyang-1024` | timing dec | `records/kem-40/yuanyang-1024__dec.json` |
| `yuanyang-1024` | timing enc | `records/kem-40/yuanyang-1024__enc.json` |
| `yuanyang-1024` | timing keygen | `records/kem-40/yuanyang-1024__keygen.json` |
| `yuanyang-1024` | hash profile dec | `profile/kem-40/yuanyang-1024__dec.json` |
| `yuanyang-1024` | hash profile enc | `profile/kem-40/yuanyang-1024__enc.json` |
| `yuanyang-1024` | hash profile keygen | `profile/kem-40/yuanyang-1024__keygen.json` |
| `yuanyang-2048` | KAT log (sha256 `e622a8d320cb8bf0…`) | `kat/kem-40/yuanyang-2048.log` |
| `yuanyang-2048` | timing dec | `records/kem-40/yuanyang-2048__dec.json` |
| `yuanyang-2048` | timing enc | `records/kem-40/yuanyang-2048__enc.json` |
| `yuanyang-2048` | timing keygen | `records/kem-40/yuanyang-2048__keygen.json` |
| `yuanyang-2048` | hash profile dec | `profile/kem-40/yuanyang-2048__dec.json` |
| `yuanyang-2048` | hash profile enc | `profile/kem-40/yuanyang-2048__enc.json` |
| `yuanyang-2048` | hash profile keygen | `profile/kem-40/yuanyang-2048__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

