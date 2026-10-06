# kem-38 UVW Key Encapsulation Mechanism — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-38` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560881697083392.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: UVW Key Encapsulation Mechanism
- Implementation versions measured: reference
- Parameter sets: `UVW-KEM-128`, `UVW-KEM-256`, `UVW-KEM-512`

## 2. Assessment environment

| item | value |
|---|---|
| processor | 12th Gen Intel(R) Core(TM) i7-12700 (CPU 2, one core) |
| clock | max 2.10 GHz, governor performance, turbo off, SMT off |
| memory | 31788 MiB |
| OS / kernel | Debian GNU/Linux 13 (trixie) / 6.12.107+deb13-amd64 |
| compiler / build tool | gcc (Debian 14.2.0-19) 14.2.0 / cmake version 3.31.6 |
| campaign start / end (UTC) | 2026-09-25T10:02:21 / 2026-09-28T10:51:21 |

## 3. Functional testing (KAT)

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-38/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `UVW-KEM-128` | guide | PASS |
| `UVW-KEM-256` | guide | PASS |
| `UVW-KEM-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `UVW-KEM-128` | keygen | 627.34 M | 301 ms | 3.33 | 300 ms | 100 (5 × 20) |
| `UVW-KEM-128` | enc | 987.6 k | 472 µs | 2.12e+03 | 471 µs | 11280 (5 × 2256) |
| `UVW-KEM-128` | dec | 1.36 G | 650 ms | 1.54 | 650 ms | 100 (5 × 20) |
| `UVW-KEM-256` | keygen | 4.82 G | 2.32 s | 0.431 | 2.32 s | 100 (5 × 20) |
| `UVW-KEM-256` | enc | 2.88 M | 1.38 ms | 727 | 1.38 ms | 3950 (5 × 790) |
| `UVW-KEM-256` | dec | 9.76 G | 4.69 s | 0.213 | 4.69 s | 100 (5 × 20) |
| `UVW-KEM-512` | keygen | 37.96 G | 18.2 s | 0.0549 | 18.2 s | 45 (5 × 9) |
| `UVW-KEM-512` | enc | 18.93 M | 9.08 ms | 110 | 9.08 ms | 555 (5 × 111) |
| `UVW-KEM-512` | dec | 62.33 G | 29.8 s | 0.0336 | 29.8 s | 30 (5 × 6) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `UVW-KEM-128` | keygen | 225041 | 1772 KiB | 4804 KiB |
| `UVW-KEM-128` | enc | 225041 | 2440 KiB | 4088 KiB |
| `UVW-KEM-128` | dec | 225041 | 7300 KiB | 7400 KiB |
| `UVW-KEM-256` | keygen | 769869 | 1760 KiB | 13988 KiB |
| `UVW-KEM-256` | enc | 769869 | 4732 KiB | 12176 KiB |
| `UVW-KEM-256` | dec | 769869 | 10728 KiB | 43984 KiB |
| `UVW-KEM-512` | keygen | 2952809 | 1784 KiB | 51212 KiB |
| `UVW-KEM-512` | enc | 2952809 | 14180 KiB | 44204 KiB |
| `UVW-KEM-512` | dec | 2952809 | 32464 KiB | 117524 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `UVW-KEM-128` | 208013 | 35 | 1032 | 64 |
| `UVW-KEM-256` | 911645 | 35 | 2199 | 64 |
| `UVW-KEM-512` | 4001850 | 67 | 4820 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `UVW-KEM-128` | keygen | 0.0% | 3.4% | drng 218 |
| `UVW-KEM-128` | enc | 14% | 21% | drng 3, pseudoXOF 4 |
| `UVW-KEM-128` | dec | 0.0% | 3.1% | drng 437, pseudoXOF 5 |
| `UVW-KEM-256` | keygen | 0.0% | 1.6% | drng 858 |
| `UVW-KEM-256` | enc | 9.5% | 8.6% | drng 3.4, pseudoXOF 4 |
| `UVW-KEM-256` | dec | 0.0% | 1.7% | drng 1.72e+03, pseudoXOF 5 |
| `UVW-KEM-512` | keygen | 0.0% | 0.8% | drng 3.42e+03 |
| `UVW-KEM-512` | enc | 3.9% | 2.3% | drng 5.37, pseudoXOF 4 |
| `UVW-KEM-512` | dec | 0.0% | 1.1% | drng 6.84e+03, pseudoXOF 5 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `UVW-KEM-128` | KAT log (sha256 `2c07ed6cab2aeb5e…`) | `kat/kem-38/UVW-KEM-128.log` |
| `UVW-KEM-128` | timing dec | `records/kem-38/UVW-KEM-128__dec.json` |
| `UVW-KEM-128` | timing enc | `records/kem-38/UVW-KEM-128__enc.json` |
| `UVW-KEM-128` | timing keygen | `records/kem-38/UVW-KEM-128__keygen.json` |
| `UVW-KEM-128` | hash profile dec | `profile/kem-38/UVW-KEM-128__dec.json` |
| `UVW-KEM-128` | hash profile enc | `profile/kem-38/UVW-KEM-128__enc.json` |
| `UVW-KEM-128` | hash profile keygen | `profile/kem-38/UVW-KEM-128__keygen.json` |
| `UVW-KEM-256` | KAT log (sha256 `67212b8474761006…`) | `kat/kem-38/UVW-KEM-256.log` |
| `UVW-KEM-256` | timing dec | `records/kem-38/UVW-KEM-256__dec.json` |
| `UVW-KEM-256` | timing enc | `records/kem-38/UVW-KEM-256__enc.json` |
| `UVW-KEM-256` | timing keygen | `records/kem-38/UVW-KEM-256__keygen.json` |
| `UVW-KEM-256` | hash profile dec | `profile/kem-38/UVW-KEM-256__dec.json` |
| `UVW-KEM-256` | hash profile enc | `profile/kem-38/UVW-KEM-256__enc.json` |
| `UVW-KEM-256` | hash profile keygen | `profile/kem-38/UVW-KEM-256__keygen.json` |
| `UVW-KEM-512` | KAT log (sha256 `7407063586e93524…`) | `kat/kem-38/UVW-KEM-512.log` |
| `UVW-KEM-512` | timing dec | `records/kem-38/UVW-KEM-512__dec.json` |
| `UVW-KEM-512` | timing enc | `records/kem-38/UVW-KEM-512__enc.json` |
| `UVW-KEM-512` | timing keygen | `records/kem-38/UVW-KEM-512__keygen.json` |
| `UVW-KEM-512` | hash profile dec | `profile/kem-38/UVW-KEM-512__dec.json` |
| `UVW-KEM-512` | hash profile enc | `profile/kem-38/UVW-KEM-512__enc.json` |
| `UVW-KEM-512` | hash profile keygen | `profile/kem-38/UVW-KEM-512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

