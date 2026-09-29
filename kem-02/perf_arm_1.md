# kem-02 Amoeba — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-02` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560843117875200.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: Amoeba
- Implementation versions measured: reference
- Parameter sets: `Amoeba128`, `Amoeba192`, `Amoeba256`, `Amoeba384`, `Amoeba512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-02/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Amoeba128` | guide | PASS |
| `Amoeba192` | guide | PASS |
| `Amoeba256` | guide | PASS |
| `Amoeba384` | guide | PASS |
| `Amoeba512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Amoeba128` | keygen | 329.5 k | 122 µs | 8.18e+03 | 122 µs | 24430 (5 × 4886) |
| `Amoeba128` | enc | 448.5 k | 166 µs | 6.01e+03 | 166 µs | 18630 (5 × 3726) |
| `Amoeba128` | dec | 492.2 k | 183 µs | 5.48e+03 | 182 µs | 16990 (5 × 3398) |
| `Amoeba192` | keygen | 448.4 k | 166 µs | 6.01e+03 | 166 µs | 18100 (5 × 3620) |
| `Amoeba192` | enc | 622.2 k | 231 µs | 4.33e+03 | 231 µs | 13560 (5 × 2712) |
| `Amoeba192` | dec | 689.0 k | 256 µs | 3.91e+03 | 255 µs | 12230 (5 × 2446) |
| `Amoeba256` | keygen | 541.3 k | 201 µs | 4.98e+03 | 199 µs | 14235 (5 × 2847) |
| `Amoeba256` | enc | 721.2 k | 268 µs | 3.74e+03 | 267 µs | 11700 (5 × 2340) |
| `Amoeba256` | dec | 808.7 k | 300 µs | 3.33e+03 | 300 µs | 7865 (5 × 1573) |
| `Amoeba384` | keygen | 799.6 k | 297 µs | 3.37e+03 | 295 µs | 9950 (5 × 1990) |
| `Amoeba384` | enc | 1.07 M | 398 µs | 2.51e+03 | 397 µs | 7830 (5 × 1566) |
| `Amoeba384` | dec | 1.21 M | 447 µs | 2.24e+03 | 447 µs | 6890 (5 × 1378) |
| `Amoeba512` | keygen | 1.05 M | 388 µs | 2.58e+03 | 388 µs | 7770 (5 × 1554) |
| `Amoeba512` | enc | 1.40 M | 520 µs | 1.92e+03 | 519 µs | 5820 (5 × 1164) |
| `Amoeba512` | dec | 1.59 M | 589 µs | 1.7e+03 | 589 µs | 5310 (5 × 1062) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Amoeba128` | keygen | 46780 | 1436 KiB | 1508 KiB |
| `Amoeba128` | enc | 46780 | 1448 KiB | 1516 KiB |
| `Amoeba128` | dec | 46780 | 1452 KiB | 1520 KiB |
| `Amoeba192` | keygen | 46956 | 1440 KiB | 1512 KiB |
| `Amoeba192` | enc | 46956 | 1460 KiB | 1524 KiB |
| `Amoeba192` | dec | 46956 | 1460 KiB | 1524 KiB |
| `Amoeba256` | keygen | 46940 | 1440 KiB | 1520 KiB |
| `Amoeba256` | enc | 46940 | 1468 KiB | 1536 KiB |
| `Amoeba256` | dec | 46940 | 1468 KiB | 1536 KiB |
| `Amoeba384` | keygen | 47100 | 1444 KiB | 1532 KiB |
| `Amoeba384` | enc | 47100 | 1484 KiB | 1552 KiB |
| `Amoeba384` | dec | 47100 | 1488 KiB | 1556 KiB |
| `Amoeba512` | keygen | 47148 | 1448 KiB | 1544 KiB |
| `Amoeba512` | enc | 47148 | 1504 KiB | 1572 KiB |
| `Amoeba512` | dec | 47148 | 1508 KiB | 1576 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `Amoeba128` | 784 | 1712 | 1047 | 64 |
| `Amoeba192` | 1144 | 2504 | 1581 | 64 |
| `Amoeba256` | 1648 | 3440 | 1833 | 64 |
| `Amoeba384` | 2440 | 5096 | 2769 | 64 |
| `Amoeba512` | 3520 | 7040 | 3914 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Amoeba128` | keygen | 81% | 3.4% | drng 2, pseudoXOF 20.5 |
| `Amoeba128` | enc | 79% | 1.2% | drng 1, pseudoXOF 23, sm3hash 4 |
| `Amoeba128` | dec | 73% | 0.0% | pseudoXOF 23, sm3hash 4 |
| `Amoeba192` | keygen | 80% | 2.5% | drng 2, pseudoXOF 23.5 |
| `Amoeba192` | enc | 79% | 0.9% | drng 1, pseudoXOF 26, sm3hash 4 |
| `Amoeba192` | dec | 71% | 0.0% | pseudoXOF 26, sm3hash 4 |
| `Amoeba256` | keygen | 79% | 2.0% | drng 2, pseudoXOF 26.5 |
| `Amoeba256` | enc | 76% | 0.8% | drng 1, pseudoXOF 28, sm3hash 4 |
| `Amoeba256` | dec | 68% | 0.0% | pseudoXOF 28, sm3hash 4 |
| `Amoeba384` | keygen | 79% | 1.4% | drng 2, pseudoXOF 36.5 |
| `Amoeba384` | enc | 76% | 0.5% | drng 1, pseudoXOF 38, sm3hash 4 |
| `Amoeba384` | dec | 67% | 0.0% | pseudoXOF 38, sm3hash 4 |
| `Amoeba512` | keygen | 78% | 1.1% | drng 2, pseudoXOF 46.6 |
| `Amoeba512` | enc | 75% | 0.4% | drng 1, pseudoXOF 47, sm3hash 4 |
| `Amoeba512` | dec | 66% | 0.0% | pseudoXOF 47, sm3hash 4 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Amoeba128` | KAT log (sha256 `c069c1acff6568af…`) | `kat/kem-02/Amoeba128.log` |
| `Amoeba128` | timing dec | `records/kem-02/Amoeba128__dec.json` |
| `Amoeba128` | timing enc | `records/kem-02/Amoeba128__enc.json` |
| `Amoeba128` | timing keygen | `records/kem-02/Amoeba128__keygen.json` |
| `Amoeba128` | hash profile dec | `profile/kem-02/Amoeba128__dec.json` |
| `Amoeba128` | hash profile enc | `profile/kem-02/Amoeba128__enc.json` |
| `Amoeba128` | hash profile keygen | `profile/kem-02/Amoeba128__keygen.json` |
| `Amoeba192` | KAT log (sha256 `ac9ab896ac650fc9…`) | `kat/kem-02/Amoeba192.log` |
| `Amoeba192` | timing dec | `records/kem-02/Amoeba192__dec.json` |
| `Amoeba192` | timing enc | `records/kem-02/Amoeba192__enc.json` |
| `Amoeba192` | timing keygen | `records/kem-02/Amoeba192__keygen.json` |
| `Amoeba192` | hash profile dec | `profile/kem-02/Amoeba192__dec.json` |
| `Amoeba192` | hash profile enc | `profile/kem-02/Amoeba192__enc.json` |
| `Amoeba192` | hash profile keygen | `profile/kem-02/Amoeba192__keygen.json` |
| `Amoeba256` | KAT log (sha256 `11335a6ea68b71c8…`) | `kat/kem-02/Amoeba256.log` |
| `Amoeba256` | timing dec | `records/kem-02/Amoeba256__dec.json` |
| `Amoeba256` | timing enc | `records/kem-02/Amoeba256__enc.json` |
| `Amoeba256` | timing keygen | `records/kem-02/Amoeba256__keygen.json` |
| `Amoeba256` | hash profile dec | `profile/kem-02/Amoeba256__dec.json` |
| `Amoeba256` | hash profile enc | `profile/kem-02/Amoeba256__enc.json` |
| `Amoeba256` | hash profile keygen | `profile/kem-02/Amoeba256__keygen.json` |
| `Amoeba384` | KAT log (sha256 `e13fb3e66bab01a0…`) | `kat/kem-02/Amoeba384.log` |
| `Amoeba384` | timing dec | `records/kem-02/Amoeba384__dec.json` |
| `Amoeba384` | timing enc | `records/kem-02/Amoeba384__enc.json` |
| `Amoeba384` | timing keygen | `records/kem-02/Amoeba384__keygen.json` |
| `Amoeba384` | hash profile dec | `profile/kem-02/Amoeba384__dec.json` |
| `Amoeba384` | hash profile enc | `profile/kem-02/Amoeba384__enc.json` |
| `Amoeba384` | hash profile keygen | `profile/kem-02/Amoeba384__keygen.json` |
| `Amoeba512` | KAT log (sha256 `4aeac966c6f66723…`) | `kat/kem-02/Amoeba512.log` |
| `Amoeba512` | timing dec | `records/kem-02/Amoeba512__dec.json` |
| `Amoeba512` | timing enc | `records/kem-02/Amoeba512__enc.json` |
| `Amoeba512` | timing keygen | `records/kem-02/Amoeba512__keygen.json` |
| `Amoeba512` | hash profile dec | `profile/kem-02/Amoeba512__dec.json` |
| `Amoeba512` | hash profile enc | `profile/kem-02/Amoeba512__enc.json` |
| `Amoeba512` | hash profile keygen | `profile/kem-02/Amoeba512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

