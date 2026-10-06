# sign-02 BIT: Bimodal Triangular distribution based lattice signatures — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-02` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561076358926336.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: BIT: Bimodal Triangular distribution based lattice signatures
- Implementation versions measured: optimized (AVX2), reference
- Parameter sets: `BiT-128`, `BiT-128-avx2`, `BiT-256`, `BiT-256-avx2`, `BiT-512`, `BiT-512-avx2`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-02/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `BiT-128` | guide | PASS |
| `BiT-128-avx2` | guide-performance | PASS |
| `BiT-256` | guide | PASS |
| `BiT-256-avx2` | guide-performance | PASS |
| `BiT-512` | guide | PASS |
| `BiT-512-avx2` | guide-performance | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `BiT-128` | keygen | 460.7 k | 220 µs | 4.54e+03 | 220 µs | 20975 (5 × 4195) |
| `BiT-128` | sign | 2.25 M | 1.07 ms | 931 | 1.07 ms | 4125 (5 × 825) |
| `BiT-128` | verify | 443.8 k | 212 µs | 4.72e+03 | 212 µs | 22870 (5 × 4574) |
| `BiT-128-avx2` | keygen | 383.4 k | 183 µs | 5.46e+03 | 183 µs | 24345 (5 × 4869) |
| `BiT-128-avx2` | sign | 1.60 M | 764 µs | 1.31e+03 | 764 µs | 11880 (5 × 2376) |
| `BiT-128-avx2` | verify | 340.5 k | 163 µs | 6.12e+03 | 162 µs | 29505 (5 × 5901) |
| `BiT-256` | keygen | 1.24 M | 594 µs | 1.68e+03 | 594 µs | 7940 (5 × 1588) |
| `BiT-256` | sign | 2.71 M | 1.29 ms | 774 | 1.29 ms | 4855 (5 × 971) |
| `BiT-256` | verify | 1.28 M | 609 µs | 1.64e+03 | 609 µs | 7910 (5 × 1582) |
| `BiT-256-avx2` | keygen | 1.07 M | 513 µs | 1.95e+03 | 513 µs | 9070 (5 × 1814) |
| `BiT-256-avx2` | sign | 2.12 M | 1.02 ms | 984 | 1.02 ms | 3865 (5 × 773) |
| `BiT-256-avx2` | verify | 1.07 M | 511 µs | 1.96e+03 | 511 µs | 9175 (5 × 1835) |
| `BiT-512` | keygen | 3.59 M | 1.71 ms | 583 | 1.71 ms | 2815 (5 × 563) |
| `BiT-512` | sign | 10.97 M | 5.24 ms | 191 | 5.24 ms | 1480 (5 × 296) |
| `BiT-512` | verify | 3.54 M | 1.69 ms | 591 | 1.69 ms | 2900 (5 × 580) |
| `BiT-512-avx2` | keygen | 3.28 M | 1.57 ms | 637 | 1.57 ms | 2960 (5 × 592) |
| `BiT-512-avx2` | sign | 9.37 M | 4.48 ms | 223 | 4.48 ms | 1255 (5 × 251) |
| `BiT-512-avx2` | verify | 3.16 M | 1.51 ms | 663 | 1.51 ms | 3240 (5 × 648) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `BiT-128` | keygen | 54005 | 1744 KiB | 1832 KiB |
| `BiT-128` | sign | 54005 | 1752 KiB | 1844 KiB |
| `BiT-128` | verify | 54005 | 1768 KiB | 1864 KiB |
| `BiT-128-avx2` | keygen | 107801 | 1760 KiB | 1880 KiB |
| `BiT-128-avx2` | sign | 107801 | 1776 KiB | 1872 KiB |
| `BiT-128-avx2` | verify | 107801 | 1832 KiB | 1896 KiB |
| `BiT-256` | keygen | 54253 | 1740 KiB | 1896 KiB |
| `BiT-256` | sign | 54253 | 1780 KiB | 1976 KiB |
| `BiT-256` | verify | 54253 | 1892 KiB | 1976 KiB |
| `BiT-256-avx2` | keygen | 124845 | 1760 KiB | 1952 KiB |
| `BiT-256-avx2` | sign | 124845 | 1860 KiB | 2044 KiB |
| `BiT-256-avx2` | verify | 124845 | 1960 KiB | 2048 KiB |
| `BiT-512` | keygen | 61237 | 1740 KiB | 1960 KiB |
| `BiT-512` | sign | 61237 | 1888 KiB | 2132 KiB |
| `BiT-512` | verify | 61237 | 2056 KiB | 2144 KiB |
| `BiT-512-avx2` | keygen | 228521 | 1788 KiB | 2124 KiB |
| `BiT-512-avx2` | sign | 228521 | 2008 KiB | 2316 KiB |
| `BiT-512-avx2` | verify | 228521 | 2212 KiB | 2312 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `BiT-128` | 1048 | 1864 | 1504 |
| `BiT-128-avx2` | 1048 | 1864 | 1504 |
| `BiT-256` | 2144 | 4160 | 3456 |
| `BiT-256-avx2` | 2144 | 4160 | 3456 |
| `BiT-512` | 5056 | 9024 | 6695 |
| `BiT-512-avx2` | 5056 | 9024 | 6695 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — own fips202.c compiled but unreachable (BIT_USE_SHAKE=0)

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `BiT-128` | keygen | 74% | 2.0% | drng 2, pseudoXOF 15, sm3hash 1 |
| `BiT-128` | sign | 58% | 0.4% | drng 2, pseudoXOF 69.6, sm3hash 6.91 |
| `BiT-128` | verify | 72% | 0.0% | pseudoXOF 10, sm3hash 3 |
| `BiT-256` | keygen | 81% | 0.7% | drng 2, pseudoXOF 33, pseudohash 1 |
| `BiT-256` | sign | 65% | 0.4% | drng 2, pseudoXOF 56.3, pseudohash 2.89 |
| `BiT-256` | verify | 79% | 0.0% | pseudoXOF 31, pseudohash 3 |
| `BiT-512` | keygen | 88% | 0.3% | drng 2, pseudoXOF 51, pseudohash 1 |
| `BiT-512` | sign | 70% | 0.1% | drng 2, pseudoXOF 95.9, pseudohash 3.87 |
| `BiT-512` | verify | 85% | 0.0% | pseudoXOF 53, pseudohash 3 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `BiT-128` | KAT log (sha256 `981ff634de76dcf0…`) | `kat/sign-02/BiT-128.log` |
| `BiT-128` | timing keygen | `records/sign-02/BiT-128__keygen.json` |
| `BiT-128` | timing sign | `records/sign-02/BiT-128__sign.json` |
| `BiT-128` | timing verify | `records/sign-02/BiT-128__verify.json` |
| `BiT-128` | hash profile keygen | `profile/sign-02/BiT-128__keygen.json` |
| `BiT-128` | hash profile sign | `profile/sign-02/BiT-128__sign.json` |
| `BiT-128` | hash profile verify | `profile/sign-02/BiT-128__verify.json` |
| `BiT-128-avx2` | KAT log (sha256 `715ad5e8e3ae186a…`) | `kat/sign-02/BiT-128-avx2.log` |
| `BiT-128-avx2` | timing keygen | `records/sign-02/BiT-128-avx2__keygen.json` |
| `BiT-128-avx2` | timing sign | `records/sign-02/BiT-128-avx2__sign.json` |
| `BiT-128-avx2` | timing verify | `records/sign-02/BiT-128-avx2__verify.json` |
| `BiT-256` | KAT log (sha256 `73f254a612d9d300…`) | `kat/sign-02/BiT-256.log` |
| `BiT-256` | timing keygen | `records/sign-02/BiT-256__keygen.json` |
| `BiT-256` | timing sign | `records/sign-02/BiT-256__sign.json` |
| `BiT-256` | timing verify | `records/sign-02/BiT-256__verify.json` |
| `BiT-256` | hash profile keygen | `profile/sign-02/BiT-256__keygen.json` |
| `BiT-256` | hash profile sign | `profile/sign-02/BiT-256__sign.json` |
| `BiT-256` | hash profile verify | `profile/sign-02/BiT-256__verify.json` |
| `BiT-256-avx2` | KAT log (sha256 `4bda1004f13b1d1b…`) | `kat/sign-02/BiT-256-avx2.log` |
| `BiT-256-avx2` | timing keygen | `records/sign-02/BiT-256-avx2__keygen.json` |
| `BiT-256-avx2` | timing sign | `records/sign-02/BiT-256-avx2__sign.json` |
| `BiT-256-avx2` | timing verify | `records/sign-02/BiT-256-avx2__verify.json` |
| `BiT-512` | KAT log (sha256 `81e635e7d76ec82d…`) | `kat/sign-02/BiT-512.log` |
| `BiT-512` | timing keygen | `records/sign-02/BiT-512__keygen.json` |
| `BiT-512` | timing sign | `records/sign-02/BiT-512__sign.json` |
| `BiT-512` | timing verify | `records/sign-02/BiT-512__verify.json` |
| `BiT-512` | hash profile keygen | `profile/sign-02/BiT-512__keygen.json` |
| `BiT-512` | hash profile sign | `profile/sign-02/BiT-512__sign.json` |
| `BiT-512` | hash profile verify | `profile/sign-02/BiT-512__verify.json` |
| `BiT-512-avx2` | KAT log (sha256 `1f5d42f73645ad9f…`) | `kat/sign-02/BiT-512-avx2.log` |
| `BiT-512-avx2` | timing keygen | `records/sign-02/BiT-512-avx2__keygen.json` |
| `BiT-512-avx2` | timing sign | `records/sign-02/BiT-512-avx2__sign.json` |
| `BiT-512-avx2` | timing verify | `records/sign-02/BiT-512-avx2__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

