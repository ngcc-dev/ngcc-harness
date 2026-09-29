# kem-05 BIKE-MLThre — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-05` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560843495362560.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: BIKE-MLThre
- Implementation versions measured: reference
- Parameter sets: `BIKE_v2_128`, `BIKE_v2_256`, `BIKE_v2_512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-05/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `BIKE_v2_128` | guide | PASS |
| `BIKE_v2_256` | guide | PASS |
| `BIKE_v2_512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `BIKE_v2_128` | keygen | 1.04 G | 385 ms | 2.6 | 385 ms | 100 (5 × 20) |
| `BIKE_v2_128` | enc | 9.17 M | 3.4 ms | 294 | 3.47 ms | 895 (5 × 179) |
| `BIKE_v2_128` | dec | 51.48 M | 19.1 ms | 52.4 | 19.1 ms | 165 (5 × 33) |
| `BIKE_v2_256` | keygen | 11.94 G | 4.43 s | 0.226 | 4.43 s | 85 (5 × 17) |
| `BIKE_v2_256` | enc | 65.07 M | 24.1 ms | 41.4 | 24.2 ms | 135 (5 × 27) |
| `BIKE_v2_256` | dec | 361.68 M | 134 ms | 7.45 | 134 ms | 100 (5 × 20) |
| `BIKE_v2_512` | keygen | 160.30 G | 59.5 s | 0.0168 | 59.5 s | 5 (5 × 1) |
| `BIKE_v2_512` | enc | 476.64 M | 177 ms | 5.66 | 177 ms | 100 (5 × 20) |
| `BIKE_v2_512` | dec | 2.58 G | 956 ms | 1.05 | 956 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `BIKE_v2_128` | keygen | 67564 | 3300 KiB | 6188 KiB |
| `BIKE_v2_128` | enc | 67564 | 8120 KiB | 8184 KiB |
| `BIKE_v2_128` | dec | 67564 | 8072 KiB | 8136 KiB |
| `BIKE_v2_256` | keygen | 125188 | 3312 KiB | 8216 KiB |
| `BIKE_v2_256` | enc | 125188 | 8188 KiB | 8252 KiB |
| `BIKE_v2_256` | dec | 125188 | 6608 KiB | 6672 KiB |
| `BIKE_v2_512` | keygen | 343756 | 3368 KiB | 6380 KiB |
| `BIKE_v2_512` | enc | 343756 | 8064 KiB | 8128 KiB |
| `BIKE_v2_512` | dec | 343756 | 10380 KiB | 10444 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `BIKE_v2_128` | 1541 | 3114 | 1573 | 32 |
| `BIKE_v2_256` | 5122 | 10276 | 5154 | 32 |
| `BIKE_v2_512` | 18751 | 37566 | 18815 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **mixed** — hash/XOF via ICCS; seeds from NIST AES-256-CTR-DRBG (OpenSSL) only with -DNIST_RAND, otherwise libc rand()

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `BIKE_v2_128` | keygen | 0.0% | 0.0% | drng 1, pseudoXOF 1 |
| `BIKE_v2_128` | enc | 1.3% | 0.1% | drng 1, pseudoXOF 1, sm3hash 2 |
| `BIKE_v2_128` | dec | 0.2% | 0.0% | pseudoXOF 1, sm3hash 2 |
| `BIKE_v2_256` | keygen | 0.0% | 0.0% | drng 1, pseudoXOF 1 |
| `BIKE_v2_256` | enc | 0.6% | 0.0% | drng 1, pseudoXOF 1, sm3hash 2 |
| `BIKE_v2_256` | dec | 0.1% | 0.0% | pseudoXOF 1, sm3hash 2 |
| `BIKE_v2_512` | keygen | 0.0% | 0.0% | drng 1, pseudoXOF 1 |
| `BIKE_v2_512` | enc | 0.5% | 0.0% | drng 1, pseudoXOF 1, pseudohash 2 |
| `BIKE_v2_512` | dec | 0.1% | 0.0% | pseudoXOF 1, pseudohash 2 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `BIKE_v2_128` | KAT log (sha256 `78d3649e557ea9da…`) | `kat/kem-05/BIKE_v2_128.log` |
| `BIKE_v2_128` | timing dec | `records/kem-05/BIKE_v2_128__dec.json` |
| `BIKE_v2_128` | timing enc | `records/kem-05/BIKE_v2_128__enc.json` |
| `BIKE_v2_128` | timing keygen | `records/kem-05/BIKE_v2_128__keygen.json` |
| `BIKE_v2_128` | hash profile dec | `profile/kem-05/BIKE_v2_128__dec.json` |
| `BIKE_v2_128` | hash profile enc | `profile/kem-05/BIKE_v2_128__enc.json` |
| `BIKE_v2_128` | hash profile keygen | `profile/kem-05/BIKE_v2_128__keygen.json` |
| `BIKE_v2_256` | KAT log (sha256 `42c0169c38aae801…`) | `kat/kem-05/BIKE_v2_256.log` |
| `BIKE_v2_256` | timing dec | `records/kem-05/BIKE_v2_256__dec.json` |
| `BIKE_v2_256` | timing enc | `records/kem-05/BIKE_v2_256__enc.json` |
| `BIKE_v2_256` | timing keygen | `records/kem-05/BIKE_v2_256__keygen.json` |
| `BIKE_v2_256` | hash profile dec | `profile/kem-05/BIKE_v2_256__dec.json` |
| `BIKE_v2_256` | hash profile enc | `profile/kem-05/BIKE_v2_256__enc.json` |
| `BIKE_v2_256` | hash profile keygen | `profile/kem-05/BIKE_v2_256__keygen.json` |
| `BIKE_v2_512` | KAT log (sha256 `6c32a470a4783776…`) | `kat/kem-05/BIKE_v2_512.log` |
| `BIKE_v2_512` | timing dec | `records/kem-05/BIKE_v2_512__dec.json` |
| `BIKE_v2_512` | timing enc | `records/kem-05/BIKE_v2_512__enc.json` |
| `BIKE_v2_512` | timing keygen | `records/kem-05/BIKE_v2_512__keygen.json` |
| `BIKE_v2_512` | hash profile dec | `profile/kem-05/BIKE_v2_512__dec.json` |
| `BIKE_v2_512` | hash profile enc | `profile/kem-05/BIKE_v2_512__enc.json` |
| `BIKE_v2_512` | hash profile keygen | `profile/kem-05/BIKE_v2_512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

