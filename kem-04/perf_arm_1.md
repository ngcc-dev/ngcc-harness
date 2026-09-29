# kem-04 BAG-Piglet — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-04` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560843373727744.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: BAG-Piglet
- Implementation versions measured: reference
- Parameter sets: `bag_piglet_128`, `bag_piglet_256`, `bag_piglet_384`, `bag_piglet_512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-04/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `bag_piglet_128` | guide | PASS |
| `bag_piglet_256` | guide | PASS |
| `bag_piglet_384` | guide | PASS |
| `bag_piglet_512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `bag_piglet_128` | keygen | 1.47 M | 546 µs | 1.83e+03 | 547 µs | 5510 (5 × 1102) |
| `bag_piglet_128` | enc | 3.12 M | 1.16 ms | 864 | 1.15 ms | 2730 (5 × 546) |
| `bag_piglet_128` | dec | 10.84 M | 4.02 ms | 249 | 4.02 ms | 750 (5 × 150) |
| `bag_piglet_256` | keygen | 7.00 M | 2.6 ms | 385 | 2.6 ms | 1045 (5 × 209) |
| `bag_piglet_256` | enc | 22.44 M | 8.32 ms | 120 | 8.32 ms | 385 (5 × 77) |
| `bag_piglet_256` | dec | 78.47 M | 29.1 ms | 34.4 | 29.1 ms | 105 (5 × 21) |
| `bag_piglet_384` | keygen | 19.41 M | 7.2 ms | 139 | 7.21 ms | 440 (5 × 88) |
| `bag_piglet_384` | enc | 53.70 M | 19.9 ms | 50.2 | 20 ms | 155 (5 × 31) |
| `bag_piglet_384` | dec | 207.37 M | 76.9 ms | 13 | 76.7 ms | 100 (5 × 20) |
| `bag_piglet_512` | keygen | 92.55 M | 34.3 ms | 29.1 | 34.3 ms | 100 (5 × 20) |
| `bag_piglet_512` | enc | 205.64 M | 76.3 ms | 13.1 | 76.3 ms | 100 (5 × 20) |
| `bag_piglet_512` | dec | 690.75 M | 256 ms | 3.9 | 256 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `bag_piglet_128` | keygen | 65984 | 1456 KiB | 1564 KiB |
| `bag_piglet_128` | enc | 65984 | 1496 KiB | 1572 KiB |
| `bag_piglet_128` | dec | 65984 | 1516 KiB | 1592 KiB |
| `bag_piglet_256` | keygen | 67416 | 1456 KiB | 1576 KiB |
| `bag_piglet_256` | enc | 67416 | 1512 KiB | 1580 KiB |
| `bag_piglet_256` | dec | 67416 | 3616 KiB | 3680 KiB |
| `bag_piglet_384` | keygen | 71848 | 1460 KiB | 1684 KiB |
| `bag_piglet_384` | enc | 71848 | 3572 KiB | 3636 KiB |
| `bag_piglet_384` | dec | 71848 | 1680 KiB | 1784 KiB |
| `bag_piglet_512` | keygen | 70592 | 1464 KiB | 3968 KiB |
| `bag_piglet_512` | enc | 70592 | 4040 KiB | 4104 KiB |
| `bag_piglet_512` | dec | 70592 | 2296 KiB | 2360 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `bag_piglet_128` | 522 | 16 | 1027 | 16 |
| `bag_piglet_256` | 1573 | 32 | 3097 | 32 |
| `bag_piglet_384` | 2778 | 48 | 5475 | 48 |
| `bag_piglet_512` | 4158 | 64 | 8204 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — `*_using_shake` names parse pseudoXOF output; AES-256-CTR dead; DRNG only with -DBAG_PIGLET_KAT

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `bag_piglet_128` | keygen | 54% | 0.3% | drng 1, pseudoXOF 4 |
| `bag_piglet_128` | enc | 18% | 0.6% | drng 4, pseudoXOF 5 |
| `bag_piglet_128` | dec | 15% | 0.0% | pseudoXOF 10 |
| `bag_piglet_256` | keygen | 23% | 0.1% | drng 1, pseudoXOF 4 |
| `bag_piglet_256` | enc | 5.2% | 0.1% | drng 5, pseudoXOF 5 |
| `bag_piglet_256` | dec | 4.2% | 0.0% | pseudoXOF 10 |
| `bag_piglet_384` | keygen | 33% | 0.0% | drng 1, pseudoXOF 4 |
| `bag_piglet_384` | enc | 8.7% | 0.0% | drng 5, pseudoXOF 5 |
| `bag_piglet_384` | dec | 6.4% | 0.0% | pseudoXOF 10 |
| `bag_piglet_512` | keygen | 53% | 0.0% | drng 1, pseudoXOF 4 |
| `bag_piglet_512` | enc | 16% | 0.0% | drng 5, pseudoXOF 5 |
| `bag_piglet_512` | dec | 14% | 0.0% | pseudoXOF 10 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `bag_piglet_128` | KAT log (sha256 `d578dd0c28ef010d…`) | `kat/kem-04/bag_piglet_128.log` |
| `bag_piglet_128` | timing dec | `records/kem-04/bag_piglet_128__dec.json` |
| `bag_piglet_128` | timing enc | `records/kem-04/bag_piglet_128__enc.json` |
| `bag_piglet_128` | timing keygen | `records/kem-04/bag_piglet_128__keygen.json` |
| `bag_piglet_128` | hash profile dec | `profile/kem-04/bag_piglet_128__dec.json` |
| `bag_piglet_128` | hash profile enc | `profile/kem-04/bag_piglet_128__enc.json` |
| `bag_piglet_128` | hash profile keygen | `profile/kem-04/bag_piglet_128__keygen.json` |
| `bag_piglet_256` | KAT log (sha256 `d6120009f7d2f065…`) | `kat/kem-04/bag_piglet_256.log` |
| `bag_piglet_256` | timing dec | `records/kem-04/bag_piglet_256__dec.json` |
| `bag_piglet_256` | timing enc | `records/kem-04/bag_piglet_256__enc.json` |
| `bag_piglet_256` | timing keygen | `records/kem-04/bag_piglet_256__keygen.json` |
| `bag_piglet_256` | hash profile dec | `profile/kem-04/bag_piglet_256__dec.json` |
| `bag_piglet_256` | hash profile enc | `profile/kem-04/bag_piglet_256__enc.json` |
| `bag_piglet_256` | hash profile keygen | `profile/kem-04/bag_piglet_256__keygen.json` |
| `bag_piglet_384` | KAT log (sha256 `36f552ecddbeb3ca…`) | `kat/kem-04/bag_piglet_384.log` |
| `bag_piglet_384` | timing dec | `records/kem-04/bag_piglet_384__dec.json` |
| `bag_piglet_384` | timing enc | `records/kem-04/bag_piglet_384__enc.json` |
| `bag_piglet_384` | timing keygen | `records/kem-04/bag_piglet_384__keygen.json` |
| `bag_piglet_384` | hash profile dec | `profile/kem-04/bag_piglet_384__dec.json` |
| `bag_piglet_384` | hash profile enc | `profile/kem-04/bag_piglet_384__enc.json` |
| `bag_piglet_384` | hash profile keygen | `profile/kem-04/bag_piglet_384__keygen.json` |
| `bag_piglet_512` | KAT log (sha256 `4b5a94e407d287a8…`) | `kat/kem-04/bag_piglet_512.log` |
| `bag_piglet_512` | timing dec | `records/kem-04/bag_piglet_512__dec.json` |
| `bag_piglet_512` | timing enc | `records/kem-04/bag_piglet_512__enc.json` |
| `bag_piglet_512` | timing keygen | `records/kem-04/bag_piglet_512__keygen.json` |
| `bag_piglet_512` | hash profile dec | `profile/kem-04/bag_piglet_512__dec.json` |
| `bag_piglet_512` | hash profile enc | `profile/kem-04/bag_piglet_512__enc.json` |
| `bag_piglet_512` | hash profile keygen | `profile/kem-04/bag_piglet_512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

