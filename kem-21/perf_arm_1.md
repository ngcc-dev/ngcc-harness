# kem-21 MAMBA-Viper — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-21` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560854052425728.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: MAMBA-Viper
- Implementation versions measured: reference
- Parameter sets: `MAMBA-Viper-128`, `MAMBA-Viper-192`, `MAMBA-Viper-256`, `MAMBA-Viper-384`, `MAMBA-Viper-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-21/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `MAMBA-Viper-128` | guide | PASS |
| `MAMBA-Viper-192` | guide | PASS |
| `MAMBA-Viper-256` | guide | PASS |
| `MAMBA-Viper-384` | guide | PASS |
| `MAMBA-Viper-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `MAMBA-Viper-128` | keygen | 174.9 k | 64.9 µs | 1.54e+04 | 64.9 µs | 41380 (5 × 8276) |
| `MAMBA-Viper-128` | enc | 271.4 k | 101 µs | 9.93e+03 | 101 µs | 30890 (5 × 6178) |
| `MAMBA-Viper-128` | dec | 319.1 k | 118 µs | 8.45e+03 | 118 µs | 24595 (5 × 4919) |
| `MAMBA-Viper-192` | keygen | 375.1 k | 139 µs | 7.18e+03 | 139 µs | 19090 (5 × 3818) |
| `MAMBA-Viper-192` | enc | 506.2 k | 188 µs | 5.32e+03 | 188 µs | 16430 (5 × 3286) |
| `MAMBA-Viper-192` | dec | 568.6 k | 211 µs | 4.74e+03 | 211 µs | 14535 (5 × 2907) |
| `MAMBA-Viper-256` | keygen | 625.2 k | 232 µs | 4.31e+03 | 231 µs | 12965 (5 × 2593) |
| `MAMBA-Viper-256` | enc | 781.4 k | 290 µs | 3.45e+03 | 290 µs | 10615 (5 × 2123) |
| `MAMBA-Viper-256` | dec | 862.3 k | 320 µs | 3.13e+03 | 316 µs | 9805 (5 × 1961) |
| `MAMBA-Viper-384` | keygen | 1.83 M | 678 µs | 1.48e+03 | 677 µs | 4355 (5 × 871) |
| `MAMBA-Viper-384` | enc | 2.10 M | 780 µs | 1.28e+03 | 779 µs | 4065 (5 × 813) |
| `MAMBA-Viper-384` | dec | 2.23 M | 826 µs | 1.21e+03 | 823 µs | 3800 (5 × 760) |
| `MAMBA-Viper-512` | keygen | 2.94 M | 1.09 ms | 915 | 1.09 ms | 2585 (5 × 517) |
| `MAMBA-Viper-512` | enc | 3.29 M | 1.22 ms | 818 | 1.22 ms | 2565 (5 × 513) |
| `MAMBA-Viper-512` | dec | 3.43 M | 1.27 ms | 786 | 1.27 ms | 2470 (5 × 494) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `MAMBA-Viper-128` | keygen | 36344 | 1428 KiB | 1504 KiB |
| `MAMBA-Viper-128` | enc | 36344 | 1440 KiB | 1504 KiB |
| `MAMBA-Viper-128` | dec | 36344 | 1444 KiB | 1508 KiB |
| `MAMBA-Viper-192` | keygen | 36432 | 1428 KiB | 1508 KiB |
| `MAMBA-Viper-192` | enc | 36432 | 1448 KiB | 1516 KiB |
| `MAMBA-Viper-192` | dec | 36432 | 1448 KiB | 1516 KiB |
| `MAMBA-Viper-256` | keygen | 35984 | 1432 KiB | 1520 KiB |
| `MAMBA-Viper-256` | enc | 35984 | 1456 KiB | 1524 KiB |
| `MAMBA-Viper-256` | dec | 35984 | 1456 KiB | 1524 KiB |
| `MAMBA-Viper-384` | keygen | 36192 | 1436 KiB | 1556 KiB |
| `MAMBA-Viper-384` | enc | 36192 | 1496 KiB | 1568 KiB |
| `MAMBA-Viper-384` | dec | 36192 | 1496 KiB | 1568 KiB |
| `MAMBA-Viper-512` | keygen | 36376 | 1440 KiB | 1592 KiB |
| `MAMBA-Viper-512` | enc | 36376 | 1536 KiB | 1608 KiB |
| `MAMBA-Viper-512` | dec | 36376 | 1536 KiB | 1608 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `MAMBA-Viper-128` | 608 | 1424 | 736 | 16 |
| `MAMBA-Viper-192` | 992 | 2200 | 1088 | 24 |
| `MAMBA-Viper-256` | 1312 | 2912 | 1472 | 32 |
| `MAMBA-Viper-384` | 2496 | 5488 | 2656 | 48 |
| `MAMBA-Viper-512` | 3200 | 7040 | 3456 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — `shake*` names are shims over pseudoXOF

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `MAMBA-Viper-128` | keygen | 55% | 7.3% | drng 3, pseudoXOF 8 |
| `MAMBA-Viper-128` | enc | 59% | 1.6% | drng 1, pseudoXOF 14 |
| `MAMBA-Viper-128` | dec | 57% | 0.0% | pseudoXOF 12 |
| `MAMBA-Viper-192` | keygen | 57% | 3.4% | drng 3, pseudoXOF 14 |
| `MAMBA-Viper-192` | enc | 57% | 0.8% | drng 1, pseudoXOF 20 |
| `MAMBA-Viper-192` | dec | 54% | 0.0% | pseudoXOF 18 |
| `MAMBA-Viper-256` | keygen | 56% | 2.0% | drng 3, pseudoXOF 22 |
| `MAMBA-Viper-256` | enc | 55% | 0.5% | drng 1, pseudoXOF 28 |
| `MAMBA-Viper-256` | dec | 52% | 0.0% | pseudoXOF 26 |
| `MAMBA-Viper-384` | keygen | 57% | 0.8% | drng 3, pseudoXOF 58 |
| `MAMBA-Viper-384` | enc | 56% | 0.3% | drng 1, pseudoXOF 64 |
| `MAMBA-Viper-384` | dec | 53% | 0.0% | pseudoXOF 62 |
| `MAMBA-Viper-512` | keygen | 56% | 0.5% | drng 3, pseudoXOF 92 |
| `MAMBA-Viper-512` | enc | 55% | 0.2% | drng 1, pseudoXOF 98 |
| `MAMBA-Viper-512` | dec | 53% | 0.0% | pseudoXOF 96 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `MAMBA-Viper-128` | KAT log (sha256 `3d04b7000fb8fbd8…`) | `kat/kem-21/MAMBA-Viper-128.log` |
| `MAMBA-Viper-128` | timing dec | `records/kem-21/MAMBA-Viper-128__dec.json` |
| `MAMBA-Viper-128` | timing enc | `records/kem-21/MAMBA-Viper-128__enc.json` |
| `MAMBA-Viper-128` | timing keygen | `records/kem-21/MAMBA-Viper-128__keygen.json` |
| `MAMBA-Viper-128` | hash profile dec | `profile/kem-21/MAMBA-Viper-128__dec.json` |
| `MAMBA-Viper-128` | hash profile enc | `profile/kem-21/MAMBA-Viper-128__enc.json` |
| `MAMBA-Viper-128` | hash profile keygen | `profile/kem-21/MAMBA-Viper-128__keygen.json` |
| `MAMBA-Viper-192` | KAT log (sha256 `a449fd93f0139985…`) | `kat/kem-21/MAMBA-Viper-192.log` |
| `MAMBA-Viper-192` | timing dec | `records/kem-21/MAMBA-Viper-192__dec.json` |
| `MAMBA-Viper-192` | timing enc | `records/kem-21/MAMBA-Viper-192__enc.json` |
| `MAMBA-Viper-192` | timing keygen | `records/kem-21/MAMBA-Viper-192__keygen.json` |
| `MAMBA-Viper-192` | hash profile dec | `profile/kem-21/MAMBA-Viper-192__dec.json` |
| `MAMBA-Viper-192` | hash profile enc | `profile/kem-21/MAMBA-Viper-192__enc.json` |
| `MAMBA-Viper-192` | hash profile keygen | `profile/kem-21/MAMBA-Viper-192__keygen.json` |
| `MAMBA-Viper-256` | KAT log (sha256 `38cc69f4ca95b0f0…`) | `kat/kem-21/MAMBA-Viper-256.log` |
| `MAMBA-Viper-256` | timing dec | `records/kem-21/MAMBA-Viper-256__dec.json` |
| `MAMBA-Viper-256` | timing enc | `records/kem-21/MAMBA-Viper-256__enc.json` |
| `MAMBA-Viper-256` | timing keygen | `records/kem-21/MAMBA-Viper-256__keygen.json` |
| `MAMBA-Viper-256` | hash profile dec | `profile/kem-21/MAMBA-Viper-256__dec.json` |
| `MAMBA-Viper-256` | hash profile enc | `profile/kem-21/MAMBA-Viper-256__enc.json` |
| `MAMBA-Viper-256` | hash profile keygen | `profile/kem-21/MAMBA-Viper-256__keygen.json` |
| `MAMBA-Viper-384` | KAT log (sha256 `1b41d53bacd29e46…`) | `kat/kem-21/MAMBA-Viper-384.log` |
| `MAMBA-Viper-384` | timing dec | `records/kem-21/MAMBA-Viper-384__dec.json` |
| `MAMBA-Viper-384` | timing enc | `records/kem-21/MAMBA-Viper-384__enc.json` |
| `MAMBA-Viper-384` | timing keygen | `records/kem-21/MAMBA-Viper-384__keygen.json` |
| `MAMBA-Viper-384` | hash profile dec | `profile/kem-21/MAMBA-Viper-384__dec.json` |
| `MAMBA-Viper-384` | hash profile enc | `profile/kem-21/MAMBA-Viper-384__enc.json` |
| `MAMBA-Viper-384` | hash profile keygen | `profile/kem-21/MAMBA-Viper-384__keygen.json` |
| `MAMBA-Viper-512` | KAT log (sha256 `cce2cbae77f855c5…`) | `kat/kem-21/MAMBA-Viper-512.log` |
| `MAMBA-Viper-512` | timing dec | `records/kem-21/MAMBA-Viper-512__dec.json` |
| `MAMBA-Viper-512` | timing enc | `records/kem-21/MAMBA-Viper-512__enc.json` |
| `MAMBA-Viper-512` | timing keygen | `records/kem-21/MAMBA-Viper-512__keygen.json` |
| `MAMBA-Viper-512` | hash profile dec | `profile/kem-21/MAMBA-Viper-512__dec.json` |
| `MAMBA-Viper-512` | hash profile enc | `profile/kem-21/MAMBA-Viper-512__enc.json` |
| `MAMBA-Viper-512` | hash profile keygen | `profile/kem-21/MAMBA-Viper-512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

