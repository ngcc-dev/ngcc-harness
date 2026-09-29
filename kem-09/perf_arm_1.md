# kem-09 CheetahKEM — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-09` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560844002873344.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: CheetahKEM
- Implementation versions measured: reference
- Parameter sets: `Cheetah128`, `Cheetah256`, `Cheetah384`, `Cheetah512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-09/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Cheetah128` | guide | PASS |
| `Cheetah256` | guide | PASS |
| `Cheetah384` | guide | PASS |
| `Cheetah512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Cheetah128` | keygen | 243.4 k | 90.3 µs | 1.11e+04 | 90.2 µs | 30250 (5 × 6050) |
| `Cheetah128` | enc | 318.8 k | 118 µs | 8.45e+03 | 118 µs | 26090 (5 × 5218) |
| `Cheetah128` | dec | 350.7 k | 130 µs | 7.69e+03 | 130 µs | 24100 (5 × 4820) |
| `Cheetah256` | keygen | 650.6 k | 241 µs | 4.14e+03 | 241 µs | 12135 (5 × 2427) |
| `Cheetah256` | enc | 749.3 k | 278 µs | 3.6e+03 | 278 µs | 11265 (5 × 2253) |
| `Cheetah256` | dec | 820.0 k | 304 µs | 3.29e+03 | 304 µs | 10300 (5 × 2060) |
| `Cheetah384` | keygen | 2.03 M | 754 µs | 1.33e+03 | 752 µs | 4055 (5 × 811) |
| `Cheetah384` | enc | 2.16 M | 802 µs | 1.25e+03 | 800 µs | 3915 (5 × 783) |
| `Cheetah384` | dec | 2.34 M | 867 µs | 1.15e+03 | 859 µs | 3655 (5 × 731) |
| `Cheetah512` | keygen | 3.26 M | 1.21 ms | 827 | 1.21 ms | 2530 (5 × 506) |
| `Cheetah512` | enc | 3.41 M | 1.26 ms | 791 | 1.26 ms | 2495 (5 × 499) |
| `Cheetah512` | dec | 3.64 M | 1.35 ms | 741 | 1.35 ms | 2335 (5 × 467) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Cheetah128` | keygen | 29880 | 1420 KiB | 1500 KiB |
| `Cheetah128` | enc | 29880 | 1436 KiB | 1504 KiB |
| `Cheetah128` | dec | 29880 | 1448 KiB | 1512 KiB |
| `Cheetah256` | keygen | 30256 | 1424 KiB | 1524 KiB |
| `Cheetah256` | enc | 30256 | 1464 KiB | 1532 KiB |
| `Cheetah256` | dec | 30256 | 1480 KiB | 1548 KiB |
| `Cheetah384` | keygen | 30628 | 1428 KiB | 1560 KiB |
| `Cheetah384` | enc | 30628 | 1504 KiB | 1576 KiB |
| `Cheetah384` | dec | 30628 | 1524 KiB | 1596 KiB |
| `Cheetah512` | keygen | 30752 | 1432 KiB | 3616 KiB |
| `Cheetah512` | enc | 30752 | 3564 KiB | 3628 KiB |
| `Cheetah512` | dec | 30752 | 3564 KiB | 3628 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `Cheetah128` | 832 | 1936 | 864 | 16 |
| `Cheetah256` | 1648 | 3808 | 1728 | 32 |
| `Cheetah384` | 2704 | 5920 | 2832 | 48 |
| `Cheetah512` | 3600 | 7872 | 4032 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Cheetah128` | keygen | 70% | 4.6% | drng 2, pseudoXOF 3, sm3hash 1 |
| `Cheetah128` | enc | 66% | 1.4% | drng 1, pseudoXOF 5, sm3hash 1 |
| `Cheetah128` | dec | 60% | 0.0% | pseudoXOF 6 |
| `Cheetah256` | keygen | 74% | 2.4% | drng 2, pseudoXOF 3, sm3hash 1 |
| `Cheetah256` | enc | 70% | 0.6% | drng 1, pseudoXOF 5, sm3hash 1 |
| `Cheetah256` | dec | 64% | 0.0% | pseudoXOF 6 |
| `Cheetah384` | keygen | 85% | 0.8% | drng 2, pseudoXOF 3, sm3hash 1 |
| `Cheetah384` | enc | 83% | 0.3% | drng 1, pseudoXOF 5, sm3hash 1 |
| `Cheetah384` | dec | 80% | 0.0% | pseudoXOF 6 |
| `Cheetah512` | keygen | 86% | 0.6% | drng 2, pseudoXOF 3, sm3hash 1 |
| `Cheetah512` | enc | 84% | 0.2% | drng 1, pseudoXOF 5, sm3hash 1 |
| `Cheetah512` | dec | 82% | 0.0% | pseudoXOF 6 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Cheetah128` | KAT log (sha256 `a5fd7876557c6d8c…`) | `kat/kem-09/Cheetah128.log` |
| `Cheetah128` | timing dec | `records/kem-09/Cheetah128__dec.json` |
| `Cheetah128` | timing enc | `records/kem-09/Cheetah128__enc.json` |
| `Cheetah128` | timing keygen | `records/kem-09/Cheetah128__keygen.json` |
| `Cheetah128` | hash profile dec | `profile/kem-09/Cheetah128__dec.json` |
| `Cheetah128` | hash profile enc | `profile/kem-09/Cheetah128__enc.json` |
| `Cheetah128` | hash profile keygen | `profile/kem-09/Cheetah128__keygen.json` |
| `Cheetah256` | KAT log (sha256 `e7c36c385c9da71f…`) | `kat/kem-09/Cheetah256.log` |
| `Cheetah256` | timing dec | `records/kem-09/Cheetah256__dec.json` |
| `Cheetah256` | timing enc | `records/kem-09/Cheetah256__enc.json` |
| `Cheetah256` | timing keygen | `records/kem-09/Cheetah256__keygen.json` |
| `Cheetah256` | hash profile dec | `profile/kem-09/Cheetah256__dec.json` |
| `Cheetah256` | hash profile enc | `profile/kem-09/Cheetah256__enc.json` |
| `Cheetah256` | hash profile keygen | `profile/kem-09/Cheetah256__keygen.json` |
| `Cheetah384` | KAT log (sha256 `70219eb970296033…`) | `kat/kem-09/Cheetah384.log` |
| `Cheetah384` | timing dec | `records/kem-09/Cheetah384__dec.json` |
| `Cheetah384` | timing enc | `records/kem-09/Cheetah384__enc.json` |
| `Cheetah384` | timing keygen | `records/kem-09/Cheetah384__keygen.json` |
| `Cheetah384` | hash profile dec | `profile/kem-09/Cheetah384__dec.json` |
| `Cheetah384` | hash profile enc | `profile/kem-09/Cheetah384__enc.json` |
| `Cheetah384` | hash profile keygen | `profile/kem-09/Cheetah384__keygen.json` |
| `Cheetah512` | KAT log (sha256 `a9a15762e21cbe12…`) | `kat/kem-09/Cheetah512.log` |
| `Cheetah512` | timing dec | `records/kem-09/Cheetah512__dec.json` |
| `Cheetah512` | timing enc | `records/kem-09/Cheetah512__enc.json` |
| `Cheetah512` | timing keygen | `records/kem-09/Cheetah512__keygen.json` |
| `Cheetah512` | hash profile dec | `profile/kem-09/Cheetah512__dec.json` |
| `Cheetah512` | hash profile enc | `profile/kem-09/Cheetah512__enc.json` |
| `Cheetah512` | hash profile keygen | `profile/kem-09/Cheetah512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

