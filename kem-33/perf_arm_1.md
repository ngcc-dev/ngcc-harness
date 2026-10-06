# kem-33 QUBE — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-33` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560881000828928.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: QUBE
- Implementation versions measured: reference
- Parameter sets: `qube-128`, `qube-192`, `qube-256`, `qube-384`, `qube-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-33/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `qube-128` | harness-default | MISMATCH [1] |
| `qube-192` | harness-default | NOKAT [1] |
| `qube-256` | harness-default | MISMATCH [1] |
| `qube-384` | harness-default | MISMATCH [1] |
| `qube-512` | harness-default | MISMATCH [1] |

[1] The reference sources reproduce their own in-tree KAT files byte for byte, but the top-level Test_Vectors carry the optimized implementation's key and ciphertext sizes at four levels, and no top-level qube-192 vector was submitted (kem-33/README.md, pseudocode.md). These instances are timed anyway; their output is not validated.

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `qube-128` | keygen | 551.6 k | 205 µs | 4.89e+03 | 205 µs | 14535 (5 × 2907) |
| `qube-128` | enc | 600.8 k | 223 µs | 4.49e+03 | 223 µs | 13560 (5 × 2712) |
| `qube-128` | dec | 1.03 M | 381 µs | 2.62e+03 | 379 µs | 8255 (5 × 1651) |
| `qube-192` | keygen | 1.25 M | 466 µs | 2.15e+03 | 462 µs | 6410 (5 × 1282) |
| `qube-192` | enc | 1.32 M | 489 µs | 2.04e+03 | 489 µs | 6340 (5 × 1268) |
| `qube-192` | dec | 2.36 M | 877 µs | 1.14e+03 | 876 µs | 3585 (5 × 717) |
| `qube-256` | keygen | 2.55 M | 947 µs | 1.06e+03 | 947 µs | 3250 (5 × 650) |
| `qube-256` | enc | 2.60 M | 965 µs | 1.04e+03 | 962 µs | 3340 (5 × 668) |
| `qube-256` | dec | 4.78 M | 1.77 ms | 563 | 1.78 ms | 1785 (5 × 357) |
| `qube-384` | keygen | 6.15 M | 2.28 ms | 438 | 2.28 ms | 1355 (5 × 271) |
| `qube-384` | enc | 6.26 M | 2.32 ms | 430 | 2.31 ms | 1340 (5 × 268) |
| `qube-384` | dec | 11.10 M | 4.12 ms | 243 | 4.11 ms | 760 (5 × 152) |
| `qube-512` | keygen | 16.78 M | 6.23 ms | 161 | 6.22 ms | 490 (5 × 98) |
| `qube-512` | enc | 16.76 M | 6.22 ms | 161 | 6.21 ms | 510 (5 × 102) |
| `qube-512` | dec | 25.27 M | 9.38 ms | 107 | 9.37 ms | 340 (5 × 68) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `qube-128` | keygen | 37348 | 1436 KiB | 3552 KiB |
| `qube-128` | enc | 37348 | 1460 KiB | 1536 KiB |
| `qube-128` | dec | 37348 | 1468 KiB | 1548 KiB |
| `qube-192` | keygen | 38996 | 1448 KiB | 1572 KiB |
| `qube-192` | enc | 38996 | 3508 KiB | 3572 KiB |
| `qube-192` | dec | 38996 | 1516 KiB | 1604 KiB |
| `qube-256` | keygen | 39132 | 1460 KiB | 1604 KiB |
| `qube-256` | enc | 39132 | 1552 KiB | 1636 KiB |
| `qube-256` | dec | 39132 | 1572 KiB | 1648 KiB |
| `qube-384` | keygen | 39020 | 1492 KiB | 1712 KiB |
| `qube-384` | enc | 39020 | 3716 KiB | 3780 KiB |
| `qube-384` | dec | 39020 | 1740 KiB | 1824 KiB |
| `qube-512` | keygen | 39420 | 3468 KiB | 3680 KiB |
| `qube-512` | enc | 39420 | 1872 KiB | 1940 KiB |
| `qube-512` | dec | 39420 | 3848 KiB | 3912 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `qube-128` | 3294 | 3326 | 3287 | 16 |
| `qube-192` | 6364 | 6412 | 6362 | 24 |
| `qube-256` | 10446 | 10510 | 10423 | 32 |
| `qube-384` | 20738 | 20834 | 20633 | 48 |
| `qube-512` | 34028 | 34156 | 33846 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `qube-128` | keygen | 57% | 1.6% | drng 2, pseudoXOF 7.07, sm3hash 1 |
| `qube-128` | enc | 62% | 1.4% | drng 2, pseudoXOF 7, sm3hash 2 |
| `qube-128` | dec | 52% | 0.0% | pseudoXOF 11, sm3hash 4 |
| `qube-192` | keygen | 50% | 0.7% | drng 2, pseudoXOF 8, pseudohash 1 |
| `qube-192` | enc | 57% | 0.7% | drng 2, pseudoXOF 8, pseudohash 1, sm3hash 1 |
| `qube-192` | dec | 45% | 0.0% | pseudoXOF 13, pseudohash 1, sm3hash 3 |
| `qube-256` | keygen | 49% | 0.3% | drng 2, pseudoXOF 9, pseudohash 1 |
| `qube-256` | enc | 55% | 0.3% | drng 2, pseudoXOF 8.57, pseudohash 1, sm3hash 1 |
| `qube-256` | dec | 40% | 0.0% | pseudoXOF 14, pseudohash 1, sm3hash 3 |
| `qube-384` | keygen | 41% | 0.2% | drng 2, pseudoXOF 10, pseudohash 1 |
| `qube-384` | enc | 51% | 0.2% | drng 2, pseudoXOF 9, pseudohash 2 |
| `qube-384` | dec | 44% | 0.0% | pseudoXOF 15, pseudohash 4 |
| `qube-512` | keygen | 54% | 0.1% | drng 2, pseudoXOF 10, pseudohash 1 |
| `qube-512` | enc | 63% | 0.1% | drng 2, pseudoXOF 10, pseudohash 2 |
| `qube-512` | dec | 53% | 0.0% | pseudoXOF 17, pseudohash 4 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `qube-128` | KAT log (sha256 `430d6f37988fdac0…`) | `kat/kem-33/qube-128.log` |
| `qube-128` | timing dec | `records/kem-33/qube-128__dec.json` |
| `qube-128` | timing enc | `records/kem-33/qube-128__enc.json` |
| `qube-128` | timing keygen | `records/kem-33/qube-128__keygen.json` |
| `qube-128` | hash profile dec | `profile/kem-33/qube-128__dec.json` |
| `qube-128` | hash profile enc | `profile/kem-33/qube-128__enc.json` |
| `qube-128` | hash profile keygen | `profile/kem-33/qube-128__keygen.json` |
| `qube-192` | KAT log (sha256 `8e39d7098e70e062…`) | `kat/kem-33/qube-192.log` |
| `qube-192` | timing dec | `records/kem-33/qube-192__dec.json` |
| `qube-192` | timing enc | `records/kem-33/qube-192__enc.json` |
| `qube-192` | timing keygen | `records/kem-33/qube-192__keygen.json` |
| `qube-192` | hash profile dec | `profile/kem-33/qube-192__dec.json` |
| `qube-192` | hash profile enc | `profile/kem-33/qube-192__enc.json` |
| `qube-192` | hash profile keygen | `profile/kem-33/qube-192__keygen.json` |
| `qube-256` | KAT log (sha256 `8572024e2a5eee13…`) | `kat/kem-33/qube-256.log` |
| `qube-256` | timing dec | `records/kem-33/qube-256__dec.json` |
| `qube-256` | timing enc | `records/kem-33/qube-256__enc.json` |
| `qube-256` | timing keygen | `records/kem-33/qube-256__keygen.json` |
| `qube-256` | hash profile dec | `profile/kem-33/qube-256__dec.json` |
| `qube-256` | hash profile enc | `profile/kem-33/qube-256__enc.json` |
| `qube-256` | hash profile keygen | `profile/kem-33/qube-256__keygen.json` |
| `qube-384` | KAT log (sha256 `f55f27e3a733b7ae…`) | `kat/kem-33/qube-384.log` |
| `qube-384` | timing dec | `records/kem-33/qube-384__dec.json` |
| `qube-384` | timing enc | `records/kem-33/qube-384__enc.json` |
| `qube-384` | timing keygen | `records/kem-33/qube-384__keygen.json` |
| `qube-384` | hash profile dec | `profile/kem-33/qube-384__dec.json` |
| `qube-384` | hash profile enc | `profile/kem-33/qube-384__enc.json` |
| `qube-384` | hash profile keygen | `profile/kem-33/qube-384__keygen.json` |
| `qube-512` | KAT log (sha256 `8447864d076f8758…`) | `kat/kem-33/qube-512.log` |
| `qube-512` | timing dec | `records/kem-33/qube-512__dec.json` |
| `qube-512` | timing enc | `records/kem-33/qube-512__enc.json` |
| `qube-512` | timing keygen | `records/kem-33/qube-512__keygen.json` |
| `qube-512` | hash profile dec | `profile/kem-33/qube-512__dec.json` |
| `qube-512` | hash profile enc | `profile/kem-33/qube-512__enc.json` |
| `qube-512` | hash profile keygen | `profile/kem-33/qube-512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

