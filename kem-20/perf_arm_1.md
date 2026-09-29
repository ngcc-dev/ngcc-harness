# kem-20 MAMBA-Frost — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-20` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560853918208000.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: MAMBA-Frost
- Implementation versions measured: reference
- Parameter sets: `MAMBA-Frost-128`, `MAMBA-Frost-192`, `MAMBA-Frost-256`, `MAMBA-Frost-384`, `MAMBA-Frost-512`, `MAMBA-Frost-CC-128`, `MAMBA-Frost-CC-192`, `MAMBA-Frost-CC-256`, `MAMBA-Frost-CC-384`, `MAMBA-Frost-CC-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-20/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `MAMBA-Frost-128` | guide | PASS |
| `MAMBA-Frost-192` | guide | PASS |
| `MAMBA-Frost-256` | guide | PASS |
| `MAMBA-Frost-384` | guide | PASS |
| `MAMBA-Frost-512` | guide | PASS |
| `MAMBA-Frost-CC-128` | guide | PASS |
| `MAMBA-Frost-CC-192` | guide | PASS |
| `MAMBA-Frost-CC-256` | guide | PASS |
| `MAMBA-Frost-CC-384` | guide | PASS |
| `MAMBA-Frost-CC-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `MAMBA-Frost-128` | keygen | 29.13 M | 10.8 ms | 92.5 | 10.8 ms | 285 (5 × 57) |
| `MAMBA-Frost-128` | enc | 34.64 M | 12.9 ms | 77.8 | 12.9 ms | 245 (5 × 49) |
| `MAMBA-Frost-128` | dec | 35.07 M | 13 ms | 76.9 | 13 ms | 245 (5 × 49) |
| `MAMBA-Frost-192` | keygen | 85.02 M | 31.5 ms | 31.7 | 31.5 ms | 100 (5 × 20) |
| `MAMBA-Frost-192` | enc | 88.78 M | 32.9 ms | 30.4 | 32.9 ms | 100 (5 × 20) |
| `MAMBA-Frost-192` | dec | 89.54 M | 33.2 ms | 30.1 | 33.2 ms | 100 (5 × 20) |
| `MAMBA-Frost-256` | keygen | 182.50 M | 67.7 ms | 14.8 | 67.8 ms | 100 (5 × 20) |
| `MAMBA-Frost-256` | enc | 195.71 M | 72.6 ms | 13.8 | 72.6 ms | 100 (5 × 20) |
| `MAMBA-Frost-256` | dec | 196.76 M | 73 ms | 13.7 | 73 ms | 100 (5 × 20) |
| `MAMBA-Frost-384` | keygen | 407.04 M | 151 ms | 6.62 | 151 ms | 100 (5 × 20) |
| `MAMBA-Frost-384` | enc | 485.86 M | 180 ms | 5.55 | 180 ms | 100 (5 × 20) |
| `MAMBA-Frost-384` | dec | 488.43 M | 181 ms | 5.52 | 181 ms | 100 (5 × 20) |
| `MAMBA-Frost-512` | keygen | 742.05 M | 275 ms | 3.63 | 275 ms | 100 (5 × 20) |
| `MAMBA-Frost-512` | enc | 741.58 M | 275 ms | 3.63 | 275 ms | 100 (5 × 20) |
| `MAMBA-Frost-512` | dec | 745.51 M | 277 ms | 3.62 | 277 ms | 100 (5 × 20) |
| `MAMBA-Frost-CC-128` | keygen | 29.07 M | 10.8 ms | 92.7 | 10.8 ms | 290 (5 × 58) |
| `MAMBA-Frost-CC-128` | enc | 34.67 M | 12.9 ms | 77.7 | 12.9 ms | 245 (5 × 49) |
| `MAMBA-Frost-CC-128` | dec | 35.10 M | 13 ms | 76.8 | 13 ms | 245 (5 × 49) |
| `MAMBA-Frost-CC-192` | keygen | 85.03 M | 31.6 ms | 31.7 | 31.6 ms | 100 (5 × 20) |
| `MAMBA-Frost-CC-192` | enc | 88.88 M | 33 ms | 30.3 | 33 ms | 100 (5 × 20) |
| `MAMBA-Frost-CC-192` | dec | 89.58 M | 33.2 ms | 30.1 | 33.2 ms | 100 (5 × 20) |
| `MAMBA-Frost-CC-256` | keygen | 182.58 M | 67.8 ms | 14.8 | 67.8 ms | 100 (5 × 20) |
| `MAMBA-Frost-CC-256` | enc | 195.74 M | 72.6 ms | 13.8 | 72.6 ms | 100 (5 × 20) |
| `MAMBA-Frost-CC-256` | dec | 196.85 M | 73 ms | 13.7 | 73 ms | 100 (5 × 20) |
| `MAMBA-Frost-CC-384` | keygen | 414.77 M | 154 ms | 6.5 | 154 ms | 100 (5 × 20) |
| `MAMBA-Frost-CC-384` | enc | 454.92 M | 169 ms | 5.92 | 169 ms | 100 (5 × 20) |
| `MAMBA-Frost-CC-384` | dec | 456.61 M | 169 ms | 5.9 | 169 ms | 100 (5 × 20) |
| `MAMBA-Frost-CC-512` | keygen | 767.15 M | 285 ms | 3.51 | 285 ms | 100 (5 × 20) |
| `MAMBA-Frost-CC-512` | enc | 729.13 M | 270 ms | 3.7 | 270 ms | 100 (5 × 20) |
| `MAMBA-Frost-CC-512` | dec | 730.34 M | 271 ms | 3.69 | 271 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `MAMBA-Frost-128` | keygen | 28856 | 1436 KiB | 2040 KiB |
| `MAMBA-Frost-128` | enc | 28856 | 2504 KiB | 2568 KiB |
| `MAMBA-Frost-128` | dec | 28856 | 4088 KiB | 4152 KiB |
| `MAMBA-Frost-192` | keygen | 29080 | 3452 KiB | 4156 KiB |
| `MAMBA-Frost-192` | enc | 29080 | 4556 KiB | 4620 KiB |
| `MAMBA-Frost-192` | dec | 29080 | 3112 KiB | 3180 KiB |
| `MAMBA-Frost-256` | keygen | 29168 | 1464 KiB | 6888 KiB |
| `MAMBA-Frost-256` | enc | 29168 | 6832 KiB | 6896 KiB |
| `MAMBA-Frost-256` | dec | 29168 | 4936 KiB | 5004 KiB |
| `MAMBA-Frost-384` | keygen | 29512 | 1508 KiB | 10988 KiB |
| `MAMBA-Frost-384` | enc | 29512 | 11144 KiB | 11208 KiB |
| `MAMBA-Frost-384` | dec | 29512 | 9284 KiB | 9348 KiB |
| `MAMBA-Frost-512` | keygen | 30944 | 1568 KiB | 16456 KiB |
| `MAMBA-Frost-512` | enc | 30944 | 4132 KiB | 14840 KiB |
| `MAMBA-Frost-512` | dec | 30944 | 2484 KiB | 14840 KiB |
| `MAMBA-Frost-CC-128` | keygen | 28888 | 1436 KiB | 3804 KiB |
| `MAMBA-Frost-CC-128` | enc | 28888 | 4136 KiB | 4200 KiB |
| `MAMBA-Frost-CC-128` | dec | 28888 | 2524 KiB | 2588 KiB |
| `MAMBA-Frost-CC-192` | keygen | 29112 | 1444 KiB | 3992 KiB |
| `MAMBA-Frost-CC-192` | enc | 29112 | 5128 KiB | 5192 KiB |
| `MAMBA-Frost-CC-192` | dec | 29112 | 4580 KiB | 4644 KiB |
| `MAMBA-Frost-CC-256` | keygen | 29200 | 1468 KiB | 6840 KiB |
| `MAMBA-Frost-CC-256` | enc | 29200 | 4876 KiB | 4944 KiB |
| `MAMBA-Frost-CC-256` | dec | 29200 | 4940 KiB | 5008 KiB |
| `MAMBA-Frost-CC-384` | keygen | 33848 | 1524 KiB | 11000 KiB |
| `MAMBA-Frost-CC-384` | enc | 33848 | 9104 KiB | 9192 KiB |
| `MAMBA-Frost-CC-384` | dec | 33848 | 9236 KiB | 9300 KiB |
| `MAMBA-Frost-CC-512` | keygen | 29920 | 3624 KiB | 17216 KiB |
| `MAMBA-Frost-CC-512` | enc | 29920 | 3840 KiB | 16784 KiB |
| `MAMBA-Frost-CC-512` | dec | 29920 | 4128 KiB | 16936 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `MAMBA-Frost-128` | 5152 | 6736 | 5192 | 16 |
| `MAMBA-Frost-192` | 9712 | 11528 | 9760 | 24 |
| `MAMBA-Frost-256` | 16776 | 19416 | 15552 | 32 |
| `MAMBA-Frost-384` | 25096 | 29032 | 37736 | 48 |
| `MAMBA-Frost-512` | 36432 | 41728 | 72944 | 64 |
| `MAMBA-Frost-CC-128` | 5152 | 6736 | 5192 | 16 |
| `MAMBA-Frost-CC-192` | 9712 | 11528 | 9760 | 24 |
| `MAMBA-Frost-CC-256` | 16776 | 19416 | 15552 | 32 |
| `MAMBA-Frost-CC-384` | 37628 | 43492 | 25204 | 48 |
| `MAMBA-Frost-CC-512` | 72832 | 83328 | 36544 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **mixed** — all SHAKE calls are pseudoXOF shims; matrix A via own AES-128 with -D_AES128_FOR_A_

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `MAMBA-Frost-128` | keygen | 2.8% | 0.0% | drng 1, pseudoXOF 4 |
| `MAMBA-Frost-128` | enc | 3.7% | 0.0% | drng 1, pseudoXOF 7 |
| `MAMBA-Frost-128` | dec | 4.4% | 0.0% | pseudoXOF 8 |
| `MAMBA-Frost-192` | keygen | 1.7% | 0.0% | drng 1, pseudoXOF 4 |
| `MAMBA-Frost-192` | enc | 2.5% | 0.0% | drng 1, pseudoXOF 7 |
| `MAMBA-Frost-192` | dec | 3.0% | 0.0% | pseudoXOF 8 |
| `MAMBA-Frost-256` | keygen | 1.6% | 0.0% | drng 1, pseudoXOF 4 |
| `MAMBA-Frost-256` | enc | 2.1% | 0.0% | drng 1, pseudoXOF 7 |
| `MAMBA-Frost-256` | dec | 2.4% | 0.0% | pseudoXOF 8 |
| `MAMBA-Frost-384` | keygen | 1.1% | 0.0% | drng 1, pseudoXOF 4 |
| `MAMBA-Frost-384` | enc | 2.0% | 0.0% | drng 1, pseudoXOF 7 |
| `MAMBA-Frost-384` | dec | 2.2% | 0.0% | pseudoXOF 8 |
| `MAMBA-Frost-512` | keygen | 1.0% | 0.0% | drng 1, pseudoXOF 4 |
| `MAMBA-Frost-512` | enc | 2.6% | 0.0% | drng 1, pseudoXOF 7 |
| `MAMBA-Frost-512` | dec | 3.0% | 0.0% | pseudoXOF 8 |
| `MAMBA-Frost-CC-128` | keygen | 2.8% | 0.0% | drng 1, pseudoXOF 4 |
| `MAMBA-Frost-CC-128` | enc | 3.7% | 0.0% | drng 1, pseudoXOF 7 |
| `MAMBA-Frost-CC-128` | dec | 4.4% | 0.0% | pseudoXOF 8 |
| `MAMBA-Frost-CC-192` | keygen | 1.7% | 0.0% | drng 1, pseudoXOF 4 |
| `MAMBA-Frost-CC-192` | enc | 2.5% | 0.0% | drng 1, pseudoXOF 7 |
| `MAMBA-Frost-CC-192` | dec | 3.0% | 0.0% | pseudoXOF 8 |
| `MAMBA-Frost-CC-256` | keygen | 1.7% | 0.0% | drng 1, pseudoXOF 4 |
| `MAMBA-Frost-CC-256` | enc | 2.1% | 0.0% | drng 1, pseudoXOF 7 |
| `MAMBA-Frost-CC-256` | dec | 2.4% | 0.0% | pseudoXOF 8 |
| `MAMBA-Frost-CC-384` | keygen | 1.6% | 0.0% | drng 1, pseudoXOF 4 |
| `MAMBA-Frost-CC-384` | enc | 1.7% | 0.0% | drng 1, pseudoXOF 7 |
| `MAMBA-Frost-CC-384` | dec | 1.8% | 0.0% | pseudoXOF 8 |
| `MAMBA-Frost-CC-512` | keygen | 2.0% | 0.0% | drng 1, pseudoXOF 4 |
| `MAMBA-Frost-CC-512` | enc | 1.9% | 0.0% | drng 1, pseudoXOF 7 |
| `MAMBA-Frost-CC-512` | dec | 1.9% | 0.0% | pseudoXOF 8 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `MAMBA-Frost-128` | KAT log (sha256 `a0181e4c74257514…`) | `kat/kem-20/MAMBA-Frost-128.log` |
| `MAMBA-Frost-128` | timing dec | `records/kem-20/MAMBA-Frost-128__dec.json` |
| `MAMBA-Frost-128` | timing enc | `records/kem-20/MAMBA-Frost-128__enc.json` |
| `MAMBA-Frost-128` | timing keygen | `records/kem-20/MAMBA-Frost-128__keygen.json` |
| `MAMBA-Frost-128` | hash profile dec | `profile/kem-20/MAMBA-Frost-128__dec.json` |
| `MAMBA-Frost-128` | hash profile enc | `profile/kem-20/MAMBA-Frost-128__enc.json` |
| `MAMBA-Frost-128` | hash profile keygen | `profile/kem-20/MAMBA-Frost-128__keygen.json` |
| `MAMBA-Frost-192` | KAT log (sha256 `e2b266a519c7f08a…`) | `kat/kem-20/MAMBA-Frost-192.log` |
| `MAMBA-Frost-192` | timing dec | `records/kem-20/MAMBA-Frost-192__dec.json` |
| `MAMBA-Frost-192` | timing enc | `records/kem-20/MAMBA-Frost-192__enc.json` |
| `MAMBA-Frost-192` | timing keygen | `records/kem-20/MAMBA-Frost-192__keygen.json` |
| `MAMBA-Frost-192` | hash profile dec | `profile/kem-20/MAMBA-Frost-192__dec.json` |
| `MAMBA-Frost-192` | hash profile enc | `profile/kem-20/MAMBA-Frost-192__enc.json` |
| `MAMBA-Frost-192` | hash profile keygen | `profile/kem-20/MAMBA-Frost-192__keygen.json` |
| `MAMBA-Frost-256` | KAT log (sha256 `7b6f9ad7f27ad951…`) | `kat/kem-20/MAMBA-Frost-256.log` |
| `MAMBA-Frost-256` | timing dec | `records/kem-20/MAMBA-Frost-256__dec.json` |
| `MAMBA-Frost-256` | timing enc | `records/kem-20/MAMBA-Frost-256__enc.json` |
| `MAMBA-Frost-256` | timing keygen | `records/kem-20/MAMBA-Frost-256__keygen.json` |
| `MAMBA-Frost-256` | hash profile dec | `profile/kem-20/MAMBA-Frost-256__dec.json` |
| `MAMBA-Frost-256` | hash profile enc | `profile/kem-20/MAMBA-Frost-256__enc.json` |
| `MAMBA-Frost-256` | hash profile keygen | `profile/kem-20/MAMBA-Frost-256__keygen.json` |
| `MAMBA-Frost-384` | KAT log (sha256 `9151d36d7553f9a2…`) | `kat/kem-20/MAMBA-Frost-384.log` |
| `MAMBA-Frost-384` | timing dec | `records/kem-20/MAMBA-Frost-384__dec.json` |
| `MAMBA-Frost-384` | timing enc | `records/kem-20/MAMBA-Frost-384__enc.json` |
| `MAMBA-Frost-384` | timing keygen | `records/kem-20/MAMBA-Frost-384__keygen.json` |
| `MAMBA-Frost-384` | hash profile dec | `profile/kem-20/MAMBA-Frost-384__dec.json` |
| `MAMBA-Frost-384` | hash profile enc | `profile/kem-20/MAMBA-Frost-384__enc.json` |
| `MAMBA-Frost-384` | hash profile keygen | `profile/kem-20/MAMBA-Frost-384__keygen.json` |
| `MAMBA-Frost-512` | KAT log (sha256 `da260aaff809e0e8…`) | `kat/kem-20/MAMBA-Frost-512.log` |
| `MAMBA-Frost-512` | timing dec | `records/kem-20/MAMBA-Frost-512__dec.json` |
| `MAMBA-Frost-512` | timing enc | `records/kem-20/MAMBA-Frost-512__enc.json` |
| `MAMBA-Frost-512` | timing keygen | `records/kem-20/MAMBA-Frost-512__keygen.json` |
| `MAMBA-Frost-512` | hash profile dec | `profile/kem-20/MAMBA-Frost-512__dec.json` |
| `MAMBA-Frost-512` | hash profile enc | `profile/kem-20/MAMBA-Frost-512__enc.json` |
| `MAMBA-Frost-512` | hash profile keygen | `profile/kem-20/MAMBA-Frost-512__keygen.json` |
| `MAMBA-Frost-CC-128` | KAT log (sha256 `1ca55a17eb22c090…`) | `kat/kem-20/MAMBA-Frost-CC-128.log` |
| `MAMBA-Frost-CC-128` | timing dec | `records/kem-20/MAMBA-Frost-CC-128__dec.json` |
| `MAMBA-Frost-CC-128` | timing enc | `records/kem-20/MAMBA-Frost-CC-128__enc.json` |
| `MAMBA-Frost-CC-128` | timing keygen | `records/kem-20/MAMBA-Frost-CC-128__keygen.json` |
| `MAMBA-Frost-CC-128` | hash profile dec | `profile/kem-20/MAMBA-Frost-CC-128__dec.json` |
| `MAMBA-Frost-CC-128` | hash profile enc | `profile/kem-20/MAMBA-Frost-CC-128__enc.json` |
| `MAMBA-Frost-CC-128` | hash profile keygen | `profile/kem-20/MAMBA-Frost-CC-128__keygen.json` |
| `MAMBA-Frost-CC-192` | KAT log (sha256 `744d516f834716cd…`) | `kat/kem-20/MAMBA-Frost-CC-192.log` |
| `MAMBA-Frost-CC-192` | timing dec | `records/kem-20/MAMBA-Frost-CC-192__dec.json` |
| `MAMBA-Frost-CC-192` | timing enc | `records/kem-20/MAMBA-Frost-CC-192__enc.json` |
| `MAMBA-Frost-CC-192` | timing keygen | `records/kem-20/MAMBA-Frost-CC-192__keygen.json` |
| `MAMBA-Frost-CC-192` | hash profile dec | `profile/kem-20/MAMBA-Frost-CC-192__dec.json` |
| `MAMBA-Frost-CC-192` | hash profile enc | `profile/kem-20/MAMBA-Frost-CC-192__enc.json` |
| `MAMBA-Frost-CC-192` | hash profile keygen | `profile/kem-20/MAMBA-Frost-CC-192__keygen.json` |
| `MAMBA-Frost-CC-256` | KAT log (sha256 `c40927b6852d2571…`) | `kat/kem-20/MAMBA-Frost-CC-256.log` |
| `MAMBA-Frost-CC-256` | timing dec | `records/kem-20/MAMBA-Frost-CC-256__dec.json` |
| `MAMBA-Frost-CC-256` | timing enc | `records/kem-20/MAMBA-Frost-CC-256__enc.json` |
| `MAMBA-Frost-CC-256` | timing keygen | `records/kem-20/MAMBA-Frost-CC-256__keygen.json` |
| `MAMBA-Frost-CC-256` | hash profile dec | `profile/kem-20/MAMBA-Frost-CC-256__dec.json` |
| `MAMBA-Frost-CC-256` | hash profile enc | `profile/kem-20/MAMBA-Frost-CC-256__enc.json` |
| `MAMBA-Frost-CC-256` | hash profile keygen | `profile/kem-20/MAMBA-Frost-CC-256__keygen.json` |
| `MAMBA-Frost-CC-384` | KAT log (sha256 `d9da2a8a8ea88de0…`) | `kat/kem-20/MAMBA-Frost-CC-384.log` |
| `MAMBA-Frost-CC-384` | timing dec | `records/kem-20/MAMBA-Frost-CC-384__dec.json` |
| `MAMBA-Frost-CC-384` | timing enc | `records/kem-20/MAMBA-Frost-CC-384__enc.json` |
| `MAMBA-Frost-CC-384` | timing keygen | `records/kem-20/MAMBA-Frost-CC-384__keygen.json` |
| `MAMBA-Frost-CC-384` | hash profile dec | `profile/kem-20/MAMBA-Frost-CC-384__dec.json` |
| `MAMBA-Frost-CC-384` | hash profile enc | `profile/kem-20/MAMBA-Frost-CC-384__enc.json` |
| `MAMBA-Frost-CC-384` | hash profile keygen | `profile/kem-20/MAMBA-Frost-CC-384__keygen.json` |
| `MAMBA-Frost-CC-512` | KAT log (sha256 `373ce1303f91ebef…`) | `kat/kem-20/MAMBA-Frost-CC-512.log` |
| `MAMBA-Frost-CC-512` | timing dec | `records/kem-20/MAMBA-Frost-CC-512__dec.json` |
| `MAMBA-Frost-CC-512` | timing enc | `records/kem-20/MAMBA-Frost-CC-512__enc.json` |
| `MAMBA-Frost-CC-512` | timing keygen | `records/kem-20/MAMBA-Frost-CC-512__keygen.json` |
| `MAMBA-Frost-CC-512` | hash profile dec | `profile/kem-20/MAMBA-Frost-CC-512__dec.json` |
| `MAMBA-Frost-CC-512` | hash profile enc | `profile/kem-20/MAMBA-Frost-CC-512__enc.json` |
| `MAMBA-Frost-CC-512` | hash profile keygen | `profile/kem-20/MAMBA-Frost-CC-512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

