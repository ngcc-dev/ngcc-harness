# kem-08 BW-KEM — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-08` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560843877044224.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: BW-KEM
- Implementation versions measured: reference
- Parameter sets: `BW_KEM_C128`, `BW_KEM_C256`, `BW_KEM_C512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-08/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `BW_KEM_C128` | guide | PASS |
| `BW_KEM_C256` | guide | PASS |
| `BW_KEM_C512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `BW_KEM_C128` | keygen | 219.7 k | 81.5 µs | 1.23e+04 | 81.1 µs | 35480 (5 × 7096) |
| `BW_KEM_C128` | enc | 248.0 k | 92 µs | 1.09e+04 | 91.8 µs | 33175 (5 × 6635) |
| `BW_KEM_C128` | dec | 286.9 k | 106 µs | 9.39e+03 | 106 µs | 28990 (5 × 5798) |
| `BW_KEM_C256` | keygen | 559.9 k | 208 µs | 4.81e+03 | 208 µs | 13730 (5 × 2746) |
| `BW_KEM_C256` | enc | 570.2 k | 212 µs | 4.73e+03 | 211 µs | 14670 (5 × 2934) |
| `BW_KEM_C256` | dec | 654.4 k | 243 µs | 4.12e+03 | 243 µs | 12730 (5 × 2546) |
| `BW_KEM_C512` | keygen | 1.94 M | 718 µs | 1.39e+03 | 716 µs | 4225 (5 × 845) |
| `BW_KEM_C512` | enc | 1.97 M | 730 µs | 1.37e+03 | 730 µs | 4255 (5 × 851) |
| `BW_KEM_C512` | dec | 2.23 M | 828 µs | 1.21e+03 | 827 µs | 3810 (5 × 762) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `BW_KEM_C128` | keygen | 27136 | 1420 KiB | 1492 KiB |
| `BW_KEM_C128` | enc | 27136 | 1428 KiB | 1492 KiB |
| `BW_KEM_C128` | dec | 27136 | 1428 KiB | 1492 KiB |
| `BW_KEM_C256` | keygen | 33472 | 1428 KiB | 1504 KiB |
| `BW_KEM_C256` | enc | 33472 | 1444 KiB | 1508 KiB |
| `BW_KEM_C256` | dec | 33472 | 1444 KiB | 1512 KiB |
| `BW_KEM_C512` | keygen | 38936 | 1440 KiB | 1536 KiB |
| `BW_KEM_C512` | enc | 38936 | 3516 KiB | 3580 KiB |
| `BW_KEM_C512` | dec | 38936 | 1480 KiB | 1544 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `BW_KEM_C128` | 784 | 1585 | 768 | 16 |
| `BW_KEM_C256` | 1568 | 3169 | 1440 | 32 |
| `BW_KEM_C512` | 3136 | 6337 | 2944 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — optimized AVX2 auxfunc.c adds an SM3 counter-block fast path

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `BW_KEM_C128` | keygen | 72% | 2.0% | drng 1, pseudoXOF 8.02, sm3hash 1 |
| `BW_KEM_C128` | enc | 69% | 1.8% | drng 1, pseudoXOF 9, sm3hash 1 |
| `BW_KEM_C128` | dec | 66% | 0.0% | pseudoXOF 10, sm3hash 1 |
| `BW_KEM_C256` | keygen | 78% | 1.0% | drng 1, pseudoXOF 24.1, pseudohash 1 |
| `BW_KEM_C256` | enc | 78% | 0.7% | drng 1, pseudoXOF 25, pseudohash 1 |
| `BW_KEM_C256` | dec | 72% | 0.0% | pseudoXOF 26, pseudohash 1 |
| `BW_KEM_C512` | keygen | 83% | 0.4% | drng 1, pseudoXOF 24, pseudohash 1 |
| `BW_KEM_C512` | enc | 83% | 0.3% | drng 1, pseudoXOF 25, pseudohash 1 |
| `BW_KEM_C512` | dec | 78% | 0.0% | pseudoXOF 26, pseudohash 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `BW_KEM_C128` | KAT log (sha256 `8c140920e969690b…`) | `kat/kem-08/BW_KEM_C128.log` |
| `BW_KEM_C128` | timing dec | `records/kem-08/BW_KEM_C128__dec.json` |
| `BW_KEM_C128` | timing enc | `records/kem-08/BW_KEM_C128__enc.json` |
| `BW_KEM_C128` | timing keygen | `records/kem-08/BW_KEM_C128__keygen.json` |
| `BW_KEM_C128` | hash profile dec | `profile/kem-08/BW_KEM_C128__dec.json` |
| `BW_KEM_C128` | hash profile enc | `profile/kem-08/BW_KEM_C128__enc.json` |
| `BW_KEM_C128` | hash profile keygen | `profile/kem-08/BW_KEM_C128__keygen.json` |
| `BW_KEM_C256` | KAT log (sha256 `560e3227b48fc50a…`) | `kat/kem-08/BW_KEM_C256.log` |
| `BW_KEM_C256` | timing dec | `records/kem-08/BW_KEM_C256__dec.json` |
| `BW_KEM_C256` | timing enc | `records/kem-08/BW_KEM_C256__enc.json` |
| `BW_KEM_C256` | timing keygen | `records/kem-08/BW_KEM_C256__keygen.json` |
| `BW_KEM_C256` | hash profile dec | `profile/kem-08/BW_KEM_C256__dec.json` |
| `BW_KEM_C256` | hash profile enc | `profile/kem-08/BW_KEM_C256__enc.json` |
| `BW_KEM_C256` | hash profile keygen | `profile/kem-08/BW_KEM_C256__keygen.json` |
| `BW_KEM_C512` | KAT log (sha256 `d45e123027089069…`) | `kat/kem-08/BW_KEM_C512.log` |
| `BW_KEM_C512` | timing dec | `records/kem-08/BW_KEM_C512__dec.json` |
| `BW_KEM_C512` | timing enc | `records/kem-08/BW_KEM_C512__enc.json` |
| `BW_KEM_C512` | timing keygen | `records/kem-08/BW_KEM_C512__keygen.json` |
| `BW_KEM_C512` | hash profile dec | `profile/kem-08/BW_KEM_C512__dec.json` |
| `BW_KEM_C512` | hash profile enc | `profile/kem-08/BW_KEM_C512__enc.json` |
| `BW_KEM_C512` | hash profile keygen | `profile/kem-08/BW_KEM_C512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

