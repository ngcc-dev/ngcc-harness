# kem-30 PolarLAC — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-30` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560872180207616.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: PolarLAC
- Implementation versions measured: reference
- Parameter sets: `POLARLAC-128`, `POLARLAC-256`, `POLARLAC-512`, `POLARLAC-512-Star`, `POLARLAC-Light`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-30/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `POLARLAC-128` | guide | PASS |
| `POLARLAC-256` | guide | PASS |
| `POLARLAC-512` | guide | PASS |
| `POLARLAC-512-Star` | guide | PASS |
| `POLARLAC-Light` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `POLARLAC-128` | keygen | 134.8 k | 50 µs | 2e+04 | 50 µs | 51560 (5 × 10312) |
| `POLARLAC-128` | enc | 176.6 k | 65.5 µs | 1.53e+04 | 65.2 µs | 46605 (5 × 9321) |
| `POLARLAC-128` | dec | 213.3 k | 79.2 µs | 1.26e+04 | 79.1 µs | 38070 (5 × 7614) |
| `POLARLAC-256` | keygen | 236.6 k | 87.8 µs | 1.14e+04 | 87 µs | 31190 (5 × 6238) |
| `POLARLAC-256` | enc | 327.6 k | 122 µs | 8.23e+03 | 121 µs | 25290 (5 × 5058) |
| `POLARLAC-256` | dec | 407.6 k | 151 µs | 6.61e+03 | 151 µs | 20325 (5 × 4065) |
| `POLARLAC-512` | keygen | 826.9 k | 307 µs | 3.26e+03 | 307 µs | 9820 (5 × 1964) |
| `POLARLAC-512` | enc | 1.12 M | 415 µs | 2.41e+03 | 414 µs | 7595 (5 × 1519) |
| `POLARLAC-512` | dec | 1.34 M | 498 µs | 2.01e+03 | 498 µs | 5915 (5 × 1183) |
| `POLARLAC-512-Star` | keygen | 809.2 k | 300 µs | 3.33e+03 | 300 µs | 9930 (5 × 1986) |
| `POLARLAC-512-Star` | enc | 1.10 M | 407 µs | 2.46e+03 | 406 µs | 7285 (5 × 1457) |
| `POLARLAC-512-Star` | dec | 1.31 M | 487 µs | 2.06e+03 | 487 µs | 6440 (5 × 1288) |
| `POLARLAC-Light` | keygen | 124.4 k | 46.2 µs | 2.17e+04 | 46 µs | 56940 (5 × 11388) |
| `POLARLAC-Light` | enc | 162.8 k | 60.4 µs | 1.66e+04 | 60.2 µs | 50745 (5 × 10149) |
| `POLARLAC-Light` | dec | 197.7 k | 73.3 µs | 1.36e+04 | 73.3 µs | 41135 (5 × 8227) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `POLARLAC-128` | keygen | 49940 | 1440 KiB | 1512 KiB |
| `POLARLAC-128` | enc | 49940 | 1448 KiB | 1512 KiB |
| `POLARLAC-128` | dec | 49940 | 1452 KiB | 1516 KiB |
| `POLARLAC-256` | keygen | 57692 | 1452 KiB | 1532 KiB |
| `POLARLAC-256` | enc | 57692 | 1472 KiB | 1536 KiB |
| `POLARLAC-256` | dec | 57692 | 1472 KiB | 1536 KiB |
| `POLARLAC-512` | keygen | 69020 | 1464 KiB | 1568 KiB |
| `POLARLAC-512` | enc | 69020 | 1504 KiB | 1572 KiB |
| `POLARLAC-512` | dec | 69020 | 1512 KiB | 1576 KiB |
| `POLARLAC-512-Star` | keygen | 73764 | 1464 KiB | 1572 KiB |
| `POLARLAC-512-Star` | enc | 73764 | 1516 KiB | 1580 KiB |
| `POLARLAC-512-Star` | dec | 73764 | 1516 KiB | 1584 KiB |
| `POLARLAC-Light` | keygen | 50116 | 1440 KiB | 1512 KiB |
| `POLARLAC-Light` | enc | 50116 | 1448 KiB | 1512 KiB |
| `POLARLAC-Light` | dec | 50116 | 1448 KiB | 1512 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `POLARLAC-128` | 530 | 1570 | 640 | 16 |
| `POLARLAC-256` | 1060 | 3140 | 1280 | 32 |
| `POLARLAC-512` | 2116 | 6276 | 2560 | 64 |
| `POLARLAC-512-Star` | 2522 | 6682 | 2970 | 64 |
| `POLARLAC-Light` | 530 | 1570 | 608 | 16 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — default BIT_USE_SHAKE=0

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `POLARLAC-128` | keygen | 71% | 7.2% | drng 2, pseudoXOF 9.05 |
| `POLARLAC-128` | enc | 76% | 2.4% | drng 1, pseudoXOF 10 |
| `POLARLAC-128` | dec | 69% | 0.0% | pseudoXOF 11 |
| `POLARLAC-256` | keygen | 70% | 4.2% | drng 2, pseudoXOF 9.06 |
| `POLARLAC-256` | enc | 73% | 1.3% | drng 1, pseudoXOF 10 |
| `POLARLAC-256` | dec | 65% | 0.0% | pseudoXOF 11 |
| `POLARLAC-512` | keygen | 81% | 1.4% | drng 2, pseudoXOF 9.1 |
| `POLARLAC-512` | enc | 81% | 0.5% | drng 1, pseudoXOF 10.1 |
| `POLARLAC-512` | dec | 75% | 0.0% | pseudoXOF 11 |
| `POLARLAC-512-Star` | keygen | 82% | 1.4% | drng 2, pseudoXOF 13.1 |
| `POLARLAC-512-Star` | enc | 83% | 0.5% | drng 1, pseudoXOF 14.1 |
| `POLARLAC-512-Star` | dec | 78% | 0.0% | pseudoXOF 15 |
| `POLARLAC-Light` | keygen | 69% | 7.8% | drng 2, pseudoXOF 9.07 |
| `POLARLAC-Light` | enc | 74% | 2.6% | drng 1, pseudoXOF 10.1 |
| `POLARLAC-Light` | dec | 67% | 0.0% | pseudoXOF 11 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `POLARLAC-128` | KAT log (sha256 `f1b545b1d65e1726…`) | `kat/kem-30/POLARLAC-128.log` |
| `POLARLAC-128` | timing dec | `records/kem-30/POLARLAC-128__dec.json` |
| `POLARLAC-128` | timing enc | `records/kem-30/POLARLAC-128__enc.json` |
| `POLARLAC-128` | timing keygen | `records/kem-30/POLARLAC-128__keygen.json` |
| `POLARLAC-128` | hash profile dec | `profile/kem-30/POLARLAC-128__dec.json` |
| `POLARLAC-128` | hash profile enc | `profile/kem-30/POLARLAC-128__enc.json` |
| `POLARLAC-128` | hash profile keygen | `profile/kem-30/POLARLAC-128__keygen.json` |
| `POLARLAC-256` | KAT log (sha256 `8cee581f35067201…`) | `kat/kem-30/POLARLAC-256.log` |
| `POLARLAC-256` | timing dec | `records/kem-30/POLARLAC-256__dec.json` |
| `POLARLAC-256` | timing enc | `records/kem-30/POLARLAC-256__enc.json` |
| `POLARLAC-256` | timing keygen | `records/kem-30/POLARLAC-256__keygen.json` |
| `POLARLAC-256` | hash profile dec | `profile/kem-30/POLARLAC-256__dec.json` |
| `POLARLAC-256` | hash profile enc | `profile/kem-30/POLARLAC-256__enc.json` |
| `POLARLAC-256` | hash profile keygen | `profile/kem-30/POLARLAC-256__keygen.json` |
| `POLARLAC-512` | KAT log (sha256 `9ddba137c1b066df…`) | `kat/kem-30/POLARLAC-512.log` |
| `POLARLAC-512` | timing dec | `records/kem-30/POLARLAC-512__dec.json` |
| `POLARLAC-512` | timing enc | `records/kem-30/POLARLAC-512__enc.json` |
| `POLARLAC-512` | timing keygen | `records/kem-30/POLARLAC-512__keygen.json` |
| `POLARLAC-512` | hash profile dec | `profile/kem-30/POLARLAC-512__dec.json` |
| `POLARLAC-512` | hash profile enc | `profile/kem-30/POLARLAC-512__enc.json` |
| `POLARLAC-512` | hash profile keygen | `profile/kem-30/POLARLAC-512__keygen.json` |
| `POLARLAC-512-Star` | KAT log (sha256 `e542bf2d1e4c1da0…`) | `kat/kem-30/POLARLAC-512-Star.log` |
| `POLARLAC-512-Star` | timing dec | `records/kem-30/POLARLAC-512-Star__dec.json` |
| `POLARLAC-512-Star` | timing enc | `records/kem-30/POLARLAC-512-Star__enc.json` |
| `POLARLAC-512-Star` | timing keygen | `records/kem-30/POLARLAC-512-Star__keygen.json` |
| `POLARLAC-512-Star` | hash profile dec | `profile/kem-30/POLARLAC-512-Star__dec.json` |
| `POLARLAC-512-Star` | hash profile enc | `profile/kem-30/POLARLAC-512-Star__enc.json` |
| `POLARLAC-512-Star` | hash profile keygen | `profile/kem-30/POLARLAC-512-Star__keygen.json` |
| `POLARLAC-Light` | KAT log (sha256 `b06e208be907c37a…`) | `kat/kem-30/POLARLAC-Light.log` |
| `POLARLAC-Light` | timing dec | `records/kem-30/POLARLAC-Light__dec.json` |
| `POLARLAC-Light` | timing enc | `records/kem-30/POLARLAC-Light__enc.json` |
| `POLARLAC-Light` | timing keygen | `records/kem-30/POLARLAC-Light__keygen.json` |
| `POLARLAC-Light` | hash profile dec | `profile/kem-30/POLARLAC-Light__dec.json` |
| `POLARLAC-Light` | hash profile enc | `profile/kem-30/POLARLAC-Light__enc.json` |
| `POLARLAC-Light` | hash profile keygen | `profile/kem-30/POLARLAC-Light__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

