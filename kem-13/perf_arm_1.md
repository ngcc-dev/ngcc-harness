# kem-13 DKEM (Ding Key Encapsulation) — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-13` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560844552327168.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: DKEM (Ding Key Encapsulation)
- Implementation versions measured: reference
- Parameter sets: `DKEM-128`, `DKEM-256`, `DKEM-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-13/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `DKEM-128` | guide | PASS |
| `DKEM-256` | guide | PASS |
| `DKEM-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `DKEM-128` | keygen | 173.4 k | 64.3 µs | 1.55e+04 | 64.2 µs | 44570 (5 × 8914) |
| `DKEM-128` | enc | 198.4 k | 73.6 µs | 1.36e+04 | 73.2 µs | 41890 (5 × 8378) |
| `DKEM-128` | dec | 220.3 k | 81.8 µs | 1.22e+04 | 81.7 µs | 37770 (5 × 7554) |
| `DKEM-256` | keygen | 531.5 k | 197 µs | 5.07e+03 | 196 µs | 15360 (5 × 3072) |
| `DKEM-256` | enc | 543.1 k | 202 µs | 4.96e+03 | 201 µs | 15035 (5 × 3007) |
| `DKEM-256` | dec | 589.8 k | 219 µs | 4.57e+03 | 216 µs | 14515 (5 × 2903) |
| `DKEM-512` | keygen | 1.86 M | 691 µs | 1.45e+03 | 691 µs | 4445 (5 × 889) |
| `DKEM-512` | enc | 1.95 M | 725 µs | 1.38e+03 | 723 µs | 4360 (5 × 872) |
| `DKEM-512` | dec | 2.05 M | 760 µs | 1.32e+03 | 758 µs | 4170 (5 × 834) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `DKEM-128` | keygen | 28040 | 1420 KiB | 1488 KiB |
| `DKEM-128` | enc | 28040 | 1424 KiB | 1488 KiB |
| `DKEM-128` | dec | 28040 | 1424 KiB | 1488 KiB |
| `DKEM-256` | keygen | 28848 | 1424 KiB | 1500 KiB |
| `DKEM-256` | enc | 28848 | 1440 KiB | 1504 KiB |
| `DKEM-256` | dec | 28848 | 1440 KiB | 1504 KiB |
| `DKEM-512` | keygen | 30008 | 1432 KiB | 1520 KiB |
| `DKEM-512` | enc | 30008 | 1464 KiB | 1528 KiB |
| `DKEM-512` | dec | 30008 | 1468 KiB | 1532 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `DKEM-128` | 800 | 1600 | 800 | 32 |
| `DKEM-256` | 1568 | 3136 | 1600 | 32 |
| `DKEM-512` | 3392 | 6784 | 3136 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — default DKE_HASH=0; own SM3/HMAC compiled but unreachable; DKE_HASH=2 would switch to SHAKE

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `DKEM-128` | keygen | 70% | 3.1% | drng 1, pseudoXOF 1, sm3hash 88.2 |
| `DKEM-128` | enc | 68% | 2.1% | drng 1, pseudoXOF 1, sm3hash 95 |
| `DKEM-128` | dec | 62% | 0.0% | pseudoXOF 1, sm3hash 96 |
| `DKEM-256` | keygen | 74% | 1.0% | drng 1, pseudoXOF 1, sm3hash 289 |
| `DKEM-256` | enc | 73% | 0.8% | drng 1, pseudoXOF 1, sm3hash 293 |
| `DKEM-256` | dec | 69% | 0.0% | pseudoXOF 1, sm3hash 294 |
| `DKEM-512` | keygen | 85% | 0.4% | drng 1, pseudoXOF 1, sm3hash 608 |
| `DKEM-512` | enc | 84% | 0.3% | drng 1, pseudoXOF 1, pseudohash 1, sm3hash 620 |
| `DKEM-512` | dec | 81% | 0.0% | pseudoXOF 1, pseudohash 2, sm3hash 620 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `DKEM-128` | KAT log (sha256 `e6d1a9a6da0ce836…`) | `kat/kem-13/DKEM-128.log` |
| `DKEM-128` | timing dec | `records/kem-13/DKEM-128__dec.json` |
| `DKEM-128` | timing enc | `records/kem-13/DKEM-128__enc.json` |
| `DKEM-128` | timing keygen | `records/kem-13/DKEM-128__keygen.json` |
| `DKEM-128` | hash profile dec | `profile/kem-13/DKEM-128__dec.json` |
| `DKEM-128` | hash profile enc | `profile/kem-13/DKEM-128__enc.json` |
| `DKEM-128` | hash profile keygen | `profile/kem-13/DKEM-128__keygen.json` |
| `DKEM-256` | KAT log (sha256 `3c62f1da1dfb4fff…`) | `kat/kem-13/DKEM-256.log` |
| `DKEM-256` | timing dec | `records/kem-13/DKEM-256__dec.json` |
| `DKEM-256` | timing enc | `records/kem-13/DKEM-256__enc.json` |
| `DKEM-256` | timing keygen | `records/kem-13/DKEM-256__keygen.json` |
| `DKEM-256` | hash profile dec | `profile/kem-13/DKEM-256__dec.json` |
| `DKEM-256` | hash profile enc | `profile/kem-13/DKEM-256__enc.json` |
| `DKEM-256` | hash profile keygen | `profile/kem-13/DKEM-256__keygen.json` |
| `DKEM-512` | KAT log (sha256 `8d589a553e0eda70…`) | `kat/kem-13/DKEM-512.log` |
| `DKEM-512` | timing dec | `records/kem-13/DKEM-512__dec.json` |
| `DKEM-512` | timing enc | `records/kem-13/DKEM-512__enc.json` |
| `DKEM-512` | timing keygen | `records/kem-13/DKEM-512__keygen.json` |
| `DKEM-512` | hash profile dec | `profile/kem-13/DKEM-512__dec.json` |
| `DKEM-512` | hash profile enc | `profile/kem-13/DKEM-512__enc.json` |
| `DKEM-512` | hash profile keygen | `profile/kem-13/DKEM-512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

