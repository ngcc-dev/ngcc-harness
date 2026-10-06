# kem-08 BW-KEM — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-08` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560843877044224.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: BW-KEM
- Implementation versions measured: reference
- Parameter sets: `BW_KEM_C128`, `BW_KEM_C256`, `BW_KEM_C512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-08/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `BW_KEM_C128` | guide | PASS |
| `BW_KEM_C256` | guide | PASS |
| `BW_KEM_C512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `BW_KEM_C128` | keygen | 256.8 k | 123 µs | 8.15e+03 | 123 µs | 36695 (5 × 7339) |
| `BW_KEM_C128` | enc | 288.5 k | 138 µs | 7.25e+03 | 138 µs | 34045 (5 × 6809) |
| `BW_KEM_C128` | dec | 338.7 k | 162 µs | 6.18e+03 | 162 µs | 28755 (5 × 5751) |
| `BW_KEM_C256` | keygen | 645.6 k | 308 µs | 3.24e+03 | 308 µs | 15405 (5 × 3081) |
| `BW_KEM_C256` | enc | 664.0 k | 317 µs | 3.15e+03 | 317 µs | 15235 (5 × 3047) |
| `BW_KEM_C256` | dec | 793.0 k | 379 µs | 2.64e+03 | 379 µs | 12960 (5 × 2592) |
| `BW_KEM_C512` | keygen | 2.14 M | 1.02 ms | 978 | 1.02 ms | 4780 (5 × 956) |
| `BW_KEM_C512` | enc | 2.16 M | 1.03 ms | 970 | 1.03 ms | 4760 (5 × 952) |
| `BW_KEM_C512` | dec | 2.51 M | 1.2 ms | 835 | 1.2 ms | 4115 (5 × 823) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `BW_KEM_C128` | keygen | 29209 | 1704 KiB | 1768 KiB |
| `BW_KEM_C128` | enc | 29209 | 1688 KiB | 1808 KiB |
| `BW_KEM_C128` | dec | 29209 | 1628 KiB | 1756 KiB |
| `BW_KEM_C256` | keygen | 37177 | 1724 KiB | 1812 KiB |
| `BW_KEM_C256` | enc | 37177 | 1728 KiB | 1792 KiB |
| `BW_KEM_C256` | dec | 37177 | 1740 KiB | 1828 KiB |
| `BW_KEM_C512` | keygen | 37761 | 1700 KiB | 1824 KiB |
| `BW_KEM_C512` | enc | 37761 | 1732 KiB | 1800 KiB |
| `BW_KEM_C512` | dec | 37761 | 1764 KiB | 1832 KiB |

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
| `BW_KEM_C128` | keygen | 65% | 1.8% | drng 1, pseudoXOF 8.02, sm3hash 1 |
| `BW_KEM_C128` | enc | 63% | 1.6% | drng 1, pseudoXOF 9, sm3hash 1 |
| `BW_KEM_C128` | dec | 59% | 0.0% | pseudoXOF 10, sm3hash 1 |
| `BW_KEM_C256` | keygen | 72% | 1.0% | drng 1, pseudoXOF 24.1, pseudohash 1 |
| `BW_KEM_C256` | enc | 71% | 0.7% | drng 1, pseudoXOF 25, pseudohash 1 |
| `BW_KEM_C256` | dec | 64% | 0.0% | pseudoXOF 26, pseudohash 1 |
| `BW_KEM_C512` | keygen | 77% | 0.4% | drng 1, pseudoXOF 24, pseudohash 1 |
| `BW_KEM_C512` | enc | 78% | 0.3% | drng 1, pseudoXOF 25, pseudohash 1 |
| `BW_KEM_C512` | dec | 72% | 0.0% | pseudoXOF 26, pseudohash 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `BW_KEM_C128` | KAT log (sha256 `2cac5db0705e2b71…`) | `kat/kem-08/BW_KEM_C128.log` |
| `BW_KEM_C128` | timing dec | `records/kem-08/BW_KEM_C128__dec.json` |
| `BW_KEM_C128` | timing enc | `records/kem-08/BW_KEM_C128__enc.json` |
| `BW_KEM_C128` | timing keygen | `records/kem-08/BW_KEM_C128__keygen.json` |
| `BW_KEM_C128` | hash profile dec | `profile/kem-08/BW_KEM_C128__dec.json` |
| `BW_KEM_C128` | hash profile enc | `profile/kem-08/BW_KEM_C128__enc.json` |
| `BW_KEM_C128` | hash profile keygen | `profile/kem-08/BW_KEM_C128__keygen.json` |
| `BW_KEM_C256` | KAT log (sha256 `73dc5b29806dd55c…`) | `kat/kem-08/BW_KEM_C256.log` |
| `BW_KEM_C256` | timing dec | `records/kem-08/BW_KEM_C256__dec.json` |
| `BW_KEM_C256` | timing enc | `records/kem-08/BW_KEM_C256__enc.json` |
| `BW_KEM_C256` | timing keygen | `records/kem-08/BW_KEM_C256__keygen.json` |
| `BW_KEM_C256` | hash profile dec | `profile/kem-08/BW_KEM_C256__dec.json` |
| `BW_KEM_C256` | hash profile enc | `profile/kem-08/BW_KEM_C256__enc.json` |
| `BW_KEM_C256` | hash profile keygen | `profile/kem-08/BW_KEM_C256__keygen.json` |
| `BW_KEM_C512` | KAT log (sha256 `7dae74ef633306b0…`) | `kat/kem-08/BW_KEM_C512.log` |
| `BW_KEM_C512` | timing dec | `records/kem-08/BW_KEM_C512__dec.json` |
| `BW_KEM_C512` | timing enc | `records/kem-08/BW_KEM_C512__enc.json` |
| `BW_KEM_C512` | timing keygen | `records/kem-08/BW_KEM_C512__keygen.json` |
| `BW_KEM_C512` | hash profile dec | `profile/kem-08/BW_KEM_C512__dec.json` |
| `BW_KEM_C512` | hash profile enc | `profile/kem-08/BW_KEM_C512__enc.json` |
| `BW_KEM_C512` | hash profile keygen | `profile/kem-08/BW_KEM_C512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

