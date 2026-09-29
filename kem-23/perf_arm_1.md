# kem-23 Mito — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-23` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560862776578048.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: Mito
- Implementation versions measured: reference
- Parameter sets: `Mito-1-128`, `Mito-1-256`, `Mito-1-512`, `Mito-1-E-128`, `Mito-1-E-256`, `Mito-1-E-512`, `Mito-2-E-128`, `Mito-2-E-256`, `Mito-2-E-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-23/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Mito-1-128` | guide | PASS |
| `Mito-1-256` | guide | PASS |
| `Mito-1-512` | guide | PASS |
| `Mito-1-E-128` | guide | PASS |
| `Mito-1-E-256` | guide | PASS |
| `Mito-1-E-512` | guide | PASS |
| `Mito-2-E-128` | guide | PASS |
| `Mito-2-E-256` | guide | PASS |
| `Mito-2-E-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Mito-1-128` | keygen | 7.89 M | 2.93 ms | 342 | 2.92 ms | 1070 (5 × 214) |
| `Mito-1-128` | enc | 12.99 M | 4.82 ms | 207 | 4.82 ms | 655 (5 × 131) |
| `Mito-1-128` | dec | 18.38 M | 6.82 ms | 147 | 6.81 ms | 465 (5 × 93) |
| `Mito-1-256` | keygen | 22.94 M | 8.51 ms | 117 | 8.51 ms | 370 (5 × 74) |
| `Mito-1-256` | enc | 38.08 M | 14.1 ms | 70.8 | 14.1 ms | 225 (5 × 45) |
| `Mito-1-256` | dec | 53.97 M | 20 ms | 49.9 | 20 ms | 160 (5 × 32) |
| `Mito-1-512` | keygen | 139.20 M | 51.6 ms | 19.4 | 51.6 ms | 100 (5 × 20) |
| `Mito-1-512` | enc | 231.64 M | 85.9 ms | 11.6 | 85.9 ms | 100 (5 × 20) |
| `Mito-1-512` | dec | 326.19 M | 121 ms | 8.26 | 121 ms | 100 (5 × 20) |
| `Mito-1-E-128` | keygen | 7.30 M | 2.71 ms | 369 | 2.71 ms | 1155 (5 × 231) |
| `Mito-1-E-128` | enc | 12.02 M | 4.46 ms | 224 | 4.46 ms | 700 (5 × 140) |
| `Mito-1-E-128` | dec | 16.99 M | 6.3 ms | 159 | 6.3 ms | 505 (5 × 101) |
| `Mito-1-E-256` | keygen | 24.34 M | 9.03 ms | 111 | 9.03 ms | 350 (5 × 70) |
| `Mito-1-E-256` | enc | 40.39 M | 15 ms | 66.7 | 15 ms | 215 (5 × 43) |
| `Mito-1-E-256` | dec | 57.10 M | 21.2 ms | 47.2 | 21.2 ms | 150 (5 × 30) |
| `Mito-1-E-512` | keygen | 133.45 M | 49.5 ms | 20.2 | 49.5 ms | 100 (5 × 20) |
| `Mito-1-E-512` | enc | 222.03 M | 82.4 ms | 12.1 | 82.4 ms | 100 (5 × 20) |
| `Mito-1-E-512` | dec | 312.57 M | 116 ms | 8.62 | 116 ms | 100 (5 × 20) |
| `Mito-2-E-128` | keygen | 10.44 M | 3.87 ms | 258 | 3.87 ms | 800 (5 × 160) |
| `Mito-2-E-128` | enc | 14.94 M | 5.54 ms | 180 | 5.54 ms | 570 (5 × 114) |
| `Mito-2-E-128` | dec | 19.93 M | 7.4 ms | 135 | 7.39 ms | 430 (5 × 86) |
| `Mito-2-E-256` | keygen | 32.59 M | 12.1 ms | 82.7 | 12.1 ms | 260 (5 × 52) |
| `Mito-2-E-256` | enc | 47.04 M | 17.5 ms | 57.3 | 17.4 ms | 185 (5 × 37) |
| `Mito-2-E-256` | dec | 62.44 M | 23.2 ms | 43.2 | 23.2 ms | 140 (5 × 28) |
| `Mito-2-E-512` | keygen | 208.48 M | 77.4 ms | 12.9 | 77.3 ms | 100 (5 × 20) |
| `Mito-2-E-512` | enc | 301.12 M | 112 ms | 8.95 | 112 ms | 100 (5 × 20) |
| `Mito-2-E-512` | dec | 396.73 M | 147 ms | 6.79 | 147 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Mito-1-128` | keygen | 51684 | 1456 KiB | 1580 KiB |
| `Mito-1-128` | enc | 51684 | 1524 KiB | 1596 KiB |
| `Mito-1-128` | dec | 51684 | 1540 KiB | 1612 KiB |
| `Mito-1-256` | keygen | 53628 | 1472 KiB | 1672 KiB |
| `Mito-1-256` | enc | 53628 | 1636 KiB | 1708 KiB |
| `Mito-1-256` | dec | 53628 | 1664 KiB | 1736 KiB |
| `Mito-1-512` | keygen | 54204 | 1532 KiB | 1988 KiB |
| `Mito-1-512` | enc | 54204 | 2036 KiB | 2112 KiB |
| `Mito-1-512` | dec | 54204 | 2144 KiB | 2212 KiB |
| `Mito-1-E-128` | keygen | 51796 | 1456 KiB | 1576 KiB |
| `Mito-1-E-128` | enc | 51796 | 1520 KiB | 1592 KiB |
| `Mito-1-E-128` | dec | 51796 | 1536 KiB | 1604 KiB |
| `Mito-1-E-256` | keygen | 53444 | 1468 KiB | 1660 KiB |
| `Mito-1-E-256` | enc | 53444 | 1624 KiB | 1696 KiB |
| `Mito-1-E-256` | dec | 53444 | 3632 KiB | 3696 KiB |
| `Mito-1-E-512` | keygen | 54044 | 1528 KiB | 1968 KiB |
| `Mito-1-E-512` | enc | 54044 | 2012 KiB | 2080 KiB |
| `Mito-1-E-512` | dec | 54044 | 2112 KiB | 2180 KiB |
| `Mito-2-E-128` | keygen | 51924 | 3464 KiB | 3588 KiB |
| `Mito-2-E-128` | enc | 51924 | 1528 KiB | 1600 KiB |
| `Mito-2-E-128` | dec | 51924 | 1544 KiB | 1616 KiB |
| `Mito-2-E-256` | keygen | 53724 | 1476 KiB | 1672 KiB |
| `Mito-2-E-256` | enc | 53724 | 1636 KiB | 1704 KiB |
| `Mito-2-E-256` | dec | 53724 | 3700 KiB | 3764 KiB |
| `Mito-2-E-512` | keygen | 54404 | 1544 KiB | 2000 KiB |
| `Mito-2-E-512` | enc | 54404 | 2036 KiB | 2108 KiB |
| `Mito-2-E-512` | dec | 54404 | 4020 KiB | 4084 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `Mito-1-128` | 3908 | 4052 | 5796 | 64 |
| `Mito-1-256` | 8522 | 8682 | 12714 | 64 |
| `Mito-1-512` | 26630 | 26822 | 39878 | 64 |
| `Mito-1-E-128` | 3720 | 3864 | 5512 | 64 |
| `Mito-1-E-256` | 8130 | 8290 | 12130 | 64 |
| `Mito-1-E-512` | 25674 | 25866 | 38442 | 64 |
| `Mito-2-E-128` | 5192 | 5336 | 6440 | 64 |
| `Mito-2-E-256` | 10828 | 10988 | 13484 | 64 |
| `Mito-2-E-512` | 32712 | 32904 | 40840 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Mito-1-128` | keygen | 0.4% | 12% | drng 177, pseudohash 1 |
| `Mito-1-128` | enc | 1.5% | 10% | drng 266, pseudohash 2 |
| `Mito-1-128` | dec | 2.4% | 9.0% | drng 351, pseudohash 3 |
| `Mito-1-256` | keygen | 0.1% | 6.7% | drng 275, pseudohash 1 |
| `Mito-1-256` | enc | 1.0% | 5.6% | drng 413, pseudohash 2 |
| `Mito-1-256` | dec | 1.7% | 5.0% | drng 552, pseudohash 3 |
| `Mito-1-512` | keygen | 0.0% | 2.5% | drng 536, pseudohash 1 |
| `Mito-1-512` | enc | 0.5% | 2.0% | drng 803, pseudohash 2 |
| `Mito-1-512` | dec | 0.8% | 1.8% | drng 1.08e+03, pseudohash 3 |
| `Mito-1-E-128` | keygen | 0.5% | 13% | drng 178, pseudohash 1 |
| `Mito-1-E-128` | enc | 1.6% | 11% | drng 267, pseudohash 2 |
| `Mito-1-E-128` | dec | 2.5% | 10% | drng 351, pseudohash 3 |
| `Mito-1-E-256` | keygen | 0.1% | 6.2% | drng 275, pseudohash 1 |
| `Mito-1-E-256` | enc | 0.9% | 5.2% | drng 411, pseudohash 2 |
| `Mito-1-E-256` | dec | 1.5% | 4.7% | drng 545, pseudohash 3 |
| `Mito-1-E-512` | keygen | 0.0% | 2.5% | drng 535, pseudohash 1 |
| `Mito-1-E-512` | enc | 0.5% | 2.0% | drng 802, pseudohash 2 |
| `Mito-1-E-512` | dec | 0.8% | 1.8% | drng 1.08e+03, pseudohash 3 |
| `Mito-2-E-128` | keygen | 0.3% | 10% | drng 194, pseudohash 1 |
| `Mito-2-E-128` | enc | 1.7% | 8.2% | drng 236, pseudohash 2 |
| `Mito-2-E-128` | dec | 2.6% | 8.1% | drng 327, pseudohash 3 |
| `Mito-2-E-256` | keygen | 0.1% | 5.2% | drng 292, pseudohash 1 |
| `Mito-2-E-256` | enc | 1.0% | 4.3% | drng 363, pseudohash 2 |
| `Mito-2-E-256` | dec | 1.7% | 4.2% | drng 504, pseudohash 3 |
| `Mito-2-E-512` | keygen | 0.0% | 1.9% | drng 555, pseudohash 1 |
| `Mito-2-E-512` | enc | 0.5% | 1.4% | drng 692, pseudohash 2 |
| `Mito-2-E-512` | dec | 0.8% | 1.4% | drng 964, pseudohash 3 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Mito-1-128` | KAT log (sha256 `727893e1e1356b3f…`) | `kat/kem-23/Mito-1-128.log` |
| `Mito-1-128` | timing dec | `records/kem-23/Mito-1-128__dec.json` |
| `Mito-1-128` | timing enc | `records/kem-23/Mito-1-128__enc.json` |
| `Mito-1-128` | timing keygen | `records/kem-23/Mito-1-128__keygen.json` |
| `Mito-1-128` | hash profile dec | `profile/kem-23/Mito-1-128__dec.json` |
| `Mito-1-128` | hash profile enc | `profile/kem-23/Mito-1-128__enc.json` |
| `Mito-1-128` | hash profile keygen | `profile/kem-23/Mito-1-128__keygen.json` |
| `Mito-1-256` | KAT log (sha256 `ed272f4a73ff71ef…`) | `kat/kem-23/Mito-1-256.log` |
| `Mito-1-256` | timing dec | `records/kem-23/Mito-1-256__dec.json` |
| `Mito-1-256` | timing enc | `records/kem-23/Mito-1-256__enc.json` |
| `Mito-1-256` | timing keygen | `records/kem-23/Mito-1-256__keygen.json` |
| `Mito-1-256` | hash profile dec | `profile/kem-23/Mito-1-256__dec.json` |
| `Mito-1-256` | hash profile enc | `profile/kem-23/Mito-1-256__enc.json` |
| `Mito-1-256` | hash profile keygen | `profile/kem-23/Mito-1-256__keygen.json` |
| `Mito-1-512` | KAT log (sha256 `efbd0a20dc081ab1…`) | `kat/kem-23/Mito-1-512.log` |
| `Mito-1-512` | timing dec | `records/kem-23/Mito-1-512__dec.json` |
| `Mito-1-512` | timing enc | `records/kem-23/Mito-1-512__enc.json` |
| `Mito-1-512` | timing keygen | `records/kem-23/Mito-1-512__keygen.json` |
| `Mito-1-512` | hash profile dec | `profile/kem-23/Mito-1-512__dec.json` |
| `Mito-1-512` | hash profile enc | `profile/kem-23/Mito-1-512__enc.json` |
| `Mito-1-512` | hash profile keygen | `profile/kem-23/Mito-1-512__keygen.json` |
| `Mito-1-E-128` | KAT log (sha256 `f5aded99c56a3d27…`) | `kat/kem-23/Mito-1-E-128.log` |
| `Mito-1-E-128` | timing dec | `records/kem-23/Mito-1-E-128__dec.json` |
| `Mito-1-E-128` | timing enc | `records/kem-23/Mito-1-E-128__enc.json` |
| `Mito-1-E-128` | timing keygen | `records/kem-23/Mito-1-E-128__keygen.json` |
| `Mito-1-E-128` | hash profile dec | `profile/kem-23/Mito-1-E-128__dec.json` |
| `Mito-1-E-128` | hash profile enc | `profile/kem-23/Mito-1-E-128__enc.json` |
| `Mito-1-E-128` | hash profile keygen | `profile/kem-23/Mito-1-E-128__keygen.json` |
| `Mito-1-E-256` | KAT log (sha256 `5ab0b26d29af48b9…`) | `kat/kem-23/Mito-1-E-256.log` |
| `Mito-1-E-256` | timing dec | `records/kem-23/Mito-1-E-256__dec.json` |
| `Mito-1-E-256` | timing enc | `records/kem-23/Mito-1-E-256__enc.json` |
| `Mito-1-E-256` | timing keygen | `records/kem-23/Mito-1-E-256__keygen.json` |
| `Mito-1-E-256` | hash profile dec | `profile/kem-23/Mito-1-E-256__dec.json` |
| `Mito-1-E-256` | hash profile enc | `profile/kem-23/Mito-1-E-256__enc.json` |
| `Mito-1-E-256` | hash profile keygen | `profile/kem-23/Mito-1-E-256__keygen.json` |
| `Mito-1-E-512` | KAT log (sha256 `9d46574d1d8d96ef…`) | `kat/kem-23/Mito-1-E-512.log` |
| `Mito-1-E-512` | timing dec | `records/kem-23/Mito-1-E-512__dec.json` |
| `Mito-1-E-512` | timing enc | `records/kem-23/Mito-1-E-512__enc.json` |
| `Mito-1-E-512` | timing keygen | `records/kem-23/Mito-1-E-512__keygen.json` |
| `Mito-1-E-512` | hash profile dec | `profile/kem-23/Mito-1-E-512__dec.json` |
| `Mito-1-E-512` | hash profile enc | `profile/kem-23/Mito-1-E-512__enc.json` |
| `Mito-1-E-512` | hash profile keygen | `profile/kem-23/Mito-1-E-512__keygen.json` |
| `Mito-2-E-128` | KAT log (sha256 `bdf8327e34e8df14…`) | `kat/kem-23/Mito-2-E-128.log` |
| `Mito-2-E-128` | timing dec | `records/kem-23/Mito-2-E-128__dec.json` |
| `Mito-2-E-128` | timing enc | `records/kem-23/Mito-2-E-128__enc.json` |
| `Mito-2-E-128` | timing keygen | `records/kem-23/Mito-2-E-128__keygen.json` |
| `Mito-2-E-128` | hash profile dec | `profile/kem-23/Mito-2-E-128__dec.json` |
| `Mito-2-E-128` | hash profile enc | `profile/kem-23/Mito-2-E-128__enc.json` |
| `Mito-2-E-128` | hash profile keygen | `profile/kem-23/Mito-2-E-128__keygen.json` |
| `Mito-2-E-256` | KAT log (sha256 `90d42ac325e942de…`) | `kat/kem-23/Mito-2-E-256.log` |
| `Mito-2-E-256` | timing dec | `records/kem-23/Mito-2-E-256__dec.json` |
| `Mito-2-E-256` | timing enc | `records/kem-23/Mito-2-E-256__enc.json` |
| `Mito-2-E-256` | timing keygen | `records/kem-23/Mito-2-E-256__keygen.json` |
| `Mito-2-E-256` | hash profile dec | `profile/kem-23/Mito-2-E-256__dec.json` |
| `Mito-2-E-256` | hash profile enc | `profile/kem-23/Mito-2-E-256__enc.json` |
| `Mito-2-E-256` | hash profile keygen | `profile/kem-23/Mito-2-E-256__keygen.json` |
| `Mito-2-E-512` | KAT log (sha256 `89d9bcc64af44277…`) | `kat/kem-23/Mito-2-E-512.log` |
| `Mito-2-E-512` | timing dec | `records/kem-23/Mito-2-E-512__dec.json` |
| `Mito-2-E-512` | timing enc | `records/kem-23/Mito-2-E-512__enc.json` |
| `Mito-2-E-512` | timing keygen | `records/kem-23/Mito-2-E-512__keygen.json` |
| `Mito-2-E-512` | hash profile dec | `profile/kem-23/Mito-2-E-512__dec.json` |
| `Mito-2-E-512` | hash profile enc | `profile/kem-23/Mito-2-E-512__enc.json` |
| `Mito-2-E-512` | hash profile keygen | `profile/kem-23/Mito-2-E-512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

