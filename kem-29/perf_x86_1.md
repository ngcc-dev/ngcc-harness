# kem-29 Polar-KEM — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-29` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560872041795584.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: Polar-KEM
- Implementation versions measured: reference
- Parameter sets: `PolarKEM-128`, `PolarKEM-256`, `PolarKEM-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-29/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `PolarKEM-128` | guide | PASS |
| `PolarKEM-256` | guide | PASS |
| `PolarKEM-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `PolarKEM-128` | keygen | 166.5 k | 79.6 µs | 1.26e+04 | 79.5 µs | 57380 (5 × 11476) |
| `PolarKEM-128` | enc | 532.3 k | 254 µs | 3.93e+03 | 254 µs | 19005 (5 × 3801) |
| `PolarKEM-128` | dec | 1.04 M | 496 µs | 2.02e+03 | 496 µs | 9880 (5 × 1976) |
| `PolarKEM-256` | keygen | 325.7 k | 156 µs | 6.43e+03 | 155 µs | 30260 (5 × 6052) |
| `PolarKEM-256` | enc | 678.8 k | 324 µs | 3.08e+03 | 324 µs | 11520 (5 × 2304) |
| `PolarKEM-256` | dec | 1.32 M | 633 µs | 1.58e+03 | 633 µs | 7675 (5 × 1535) |
| `PolarKEM-512` | keygen | 644.7 k | 308 µs | 3.25e+03 | 308 µs | 15750 (5 × 3150) |
| `PolarKEM-512` | enc | 1.33 M | 637 µs | 1.57e+03 | 637 µs | 7680 (5 × 1536) |
| `PolarKEM-512` | dec | 2.61 M | 1.25 ms | 800 | 1.25 ms | 3935 (5 × 787) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `PolarKEM-128` | keygen | 26873 | 1680 KiB | 1744 KiB |
| `PolarKEM-128` | enc | 26873 | 1696 KiB | 1792 KiB |
| `PolarKEM-128` | dec | 26873 | 1720 KiB | 1788 KiB |
| `PolarKEM-256` | keygen | 27321 | 1692 KiB | 1784 KiB |
| `PolarKEM-256` | enc | 27321 | 1716 KiB | 1808 KiB |
| `PolarKEM-256` | dec | 27321 | 1724 KiB | 1796 KiB |
| `PolarKEM-512` | keygen | 27689 | 1696 KiB | 1764 KiB |
| `PolarKEM-512` | enc | 27689 | 1712 KiB | 1812 KiB |
| `PolarKEM-512` | dec | 27689 | 1744 KiB | 1812 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `PolarKEM-128` | 1024 | 2048 | 768 | 16 |
| `PolarKEM-256` | 2048 | 4096 | 1280 | 32 |
| `PolarKEM-512` | 4096 | 8192 | 2304 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `PolarKEM-128` | keygen | 94% | 5.5% | drng 2, pseudoXOF 2, sm3hash 1 |
| `PolarKEM-128` | enc | 91% | 0.9% | drng 1, pseudoXOF 6, sm3hash 3 |
| `PolarKEM-128` | dec | 95% | 0.0% | pseudoXOF 10, sm3hash 5 |
| `PolarKEM-256` | keygen | 97% | 2.8% | drng 2, pseudoXOF 2, sm3hash 1 |
| `PolarKEM-256` | enc | 88% | 0.7% | drng 1, pseudoXOF 6, sm3hash 3 |
| `PolarKEM-256` | dec | 92% | 0.0% | pseudoXOF 10, sm3hash 5 |
| `PolarKEM-512` | keygen | 98% | 1.4% | drng 2, pseudoXOF 2, sm3hash 1 |
| `PolarKEM-512` | enc | 88% | 0.5% | drng 1, pseudoXOF 7, sm3hash 3 |
| `PolarKEM-512` | dec | 93% | 0.0% | pseudoXOF 12, sm3hash 5 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `PolarKEM-128` | KAT log (sha256 `bd2bab0113c45032…`) | `kat/kem-29/PolarKEM-128.log` |
| `PolarKEM-128` | timing dec | `records/kem-29/PolarKEM-128__dec.json` |
| `PolarKEM-128` | timing enc | `records/kem-29/PolarKEM-128__enc.json` |
| `PolarKEM-128` | timing keygen | `records/kem-29/PolarKEM-128__keygen.json` |
| `PolarKEM-128` | hash profile dec | `profile/kem-29/PolarKEM-128__dec.json` |
| `PolarKEM-128` | hash profile enc | `profile/kem-29/PolarKEM-128__enc.json` |
| `PolarKEM-128` | hash profile keygen | `profile/kem-29/PolarKEM-128__keygen.json` |
| `PolarKEM-256` | KAT log (sha256 `f5d59e6dbf416b27…`) | `kat/kem-29/PolarKEM-256.log` |
| `PolarKEM-256` | timing dec | `records/kem-29/PolarKEM-256__dec.json` |
| `PolarKEM-256` | timing enc | `records/kem-29/PolarKEM-256__enc.json` |
| `PolarKEM-256` | timing keygen | `records/kem-29/PolarKEM-256__keygen.json` |
| `PolarKEM-256` | hash profile dec | `profile/kem-29/PolarKEM-256__dec.json` |
| `PolarKEM-256` | hash profile enc | `profile/kem-29/PolarKEM-256__enc.json` |
| `PolarKEM-256` | hash profile keygen | `profile/kem-29/PolarKEM-256__keygen.json` |
| `PolarKEM-512` | KAT log (sha256 `2c1bce2b649fc15f…`) | `kat/kem-29/PolarKEM-512.log` |
| `PolarKEM-512` | timing dec | `records/kem-29/PolarKEM-512__dec.json` |
| `PolarKEM-512` | timing enc | `records/kem-29/PolarKEM-512__enc.json` |
| `PolarKEM-512` | timing keygen | `records/kem-29/PolarKEM-512__keygen.json` |
| `PolarKEM-512` | hash profile dec | `profile/kem-29/PolarKEM-512__dec.json` |
| `PolarKEM-512` | hash profile enc | `profile/kem-29/PolarKEM-512__enc.json` |
| `PolarKEM-512` | hash profile keygen | `profile/kem-29/PolarKEM-512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

