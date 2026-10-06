# kem-22 Mithril — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-22` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560862650748928.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: Mithril
- Implementation versions measured: optimized (AVX2), reference
- Parameter sets: `Mithril-128`, `Mithril-128-avx2`, `Mithril-256`, `Mithril-256-avx2`, `Mithril-512`, `Mithril-512-avx2`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-22/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Mithril-128` | guide | PASS |
| `Mithril-128-avx2` | guide-performance | PASS |
| `Mithril-256` | guide | PASS |
| `Mithril-256-avx2` | guide-performance | PASS |
| `Mithril-512` | guide | PASS |
| `Mithril-512-avx2` | guide-performance | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Mithril-128` | keygen | 340.6 k | 163 µs | 6.13e+03 | 163 µs | 28495 (5 × 5699) |
| `Mithril-128` | enc | 376.6 k | 180 µs | 5.54e+03 | 180 µs | 27000 (5 × 5400) |
| `Mithril-128` | dec | 408.2 k | 196 µs | 5.11e+03 | 196 µs | 24760 (5 × 4952) |
| `Mithril-128-avx2` | keygen | 160.4 k | 76.6 µs | 1.3e+04 | 76.6 µs | 50425 (5 × 10085) |
| `Mithril-128-avx2` | enc | 160.5 k | 76.7 µs | 1.3e+04 | 76.7 µs | 60470 (5 × 12094) |
| `Mithril-128-avx2` | dec | 160.3 k | 76.6 µs | 1.31e+04 | 76.6 µs | 60805 (5 × 12161) |
| `Mithril-256` | keygen | 865.5 k | 415 µs | 2.41e+03 | 415 µs | 11800 (5 × 2360) |
| `Mithril-256` | enc | 994.3 k | 477 µs | 2.1e+03 | 477 µs | 9625 (5 × 1925) |
| `Mithril-256` | dec | 1.12 M | 537 µs | 1.86e+03 | 537 µs | 9235 (5 × 1847) |
| `Mithril-256-avx2` | keygen | 272.4 k | 130 µs | 7.68e+03 | 130 µs | 33930 (5 × 6786) |
| `Mithril-256-avx2` | enc | 269.0 k | 129 µs | 7.78e+03 | 129 µs | 37185 (5 × 7437) |
| `Mithril-256-avx2` | dec | 275.3 k | 132 µs | 7.6e+03 | 132 µs | 36525 (5 × 7305) |
| `Mithril-512` | keygen | 2.66 M | 1.27 ms | 786 | 1.27 ms | 3955 (5 × 791) |
| `Mithril-512` | enc | 3.16 M | 1.51 ms | 661 | 1.51 ms | 3190 (5 × 638) |
| `Mithril-512` | dec | 3.66 M | 1.75 ms | 571 | 1.75 ms | 2830 (5 × 566) |
| `Mithril-512-avx2` | keygen | 560.5 k | 268 µs | 3.73e+03 | 268 µs | 17225 (5 × 3445) |
| `Mithril-512-avx2` | enc | 576.9 k | 276 µs | 3.63e+03 | 276 µs | 17705 (5 × 3541) |
| `Mithril-512-avx2` | dec | 610.7 k | 292 µs | 3.43e+03 | 292 µs | 16730 (5 × 3346) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Mithril-128` | keygen | 32401 | 1712 KiB | 1784 KiB |
| `Mithril-128` | enc | 32401 | 1696 KiB | 1760 KiB |
| `Mithril-128` | dec | 32401 | 1692 KiB | 1756 KiB |
| `Mithril-128-avx2` | keygen | 64741 | 1728 KiB | 1836 KiB |
| `Mithril-128-avx2` | enc | 64741 | 1736 KiB | 1844 KiB |
| `Mithril-128-avx2` | dec | 64741 | 1720 KiB | 1844 KiB |
| `Mithril-256` | keygen | 33105 | 1696 KiB | 1796 KiB |
| `Mithril-256` | enc | 33105 | 1704 KiB | 1768 KiB |
| `Mithril-256` | dec | 33105 | 1716 KiB | 1800 KiB |
| `Mithril-256-avx2` | keygen | 70881 | 1744 KiB | 1840 KiB |
| `Mithril-256-avx2` | enc | 70881 | 1776 KiB | 1848 KiB |
| `Mithril-256-avx2` | dec | 70881 | 1772 KiB | 1836 KiB |
| `Mithril-512` | keygen | 32865 | 1700 KiB | 1828 KiB |
| `Mithril-512` | enc | 32865 | 1712 KiB | 1840 KiB |
| `Mithril-512` | dec | 32865 | 1744 KiB | 1812 KiB |
| `Mithril-512-avx2` | keygen | 73289 | 1740 KiB | 1876 KiB |
| `Mithril-512-avx2` | enc | 73289 | 1800 KiB | 1904 KiB |
| `Mithril-512-avx2` | dec | 73289 | 1820 KiB | 1908 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `Mithril-128` | 944 | 1136 | 928 | 16 |
| `Mithril-128-avx2` | 944 | 1136 | 928 | 16 |
| `Mithril-256` | 1648 | 2000 | 1680 | 32 |
| `Mithril-256-avx2` | 1648 | 2000 | 1680 | 32 |
| `Mithril-512` | 3056 | 3728 | 3504 | 64 |
| `Mithril-512-avx2` | 3056 | 3728 | 3504 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Mithril-128` | keygen | 40% | 4.9% | drng 3, pseudoXOF 7 |
| `Mithril-128` | enc | 38% | 1.3% | drng 1, pseudoXOF 8 |
| `Mithril-128` | dec | 35% | 0.0% | pseudoXOF 8 |
| `Mithril-256` | keygen | 28% | 1.9% | drng 3, pseudoXOF 11 |
| `Mithril-256` | enc | 25% | 0.5% | drng 1, pseudoXOF 12 |
| `Mithril-256` | dec | 23% | 0.0% | pseudoXOF 12 |
| `Mithril-512` | keygen | 20% | 0.7% | drng 3, pseudoXOF 19 |
| `Mithril-512` | enc | 17% | 0.2% | drng 1, pseudoXOF 20 |
| `Mithril-512` | dec | 15% | 0.0% | pseudoXOF 20 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Mithril-128` | KAT log (sha256 `3a88a33a0c5b5bac…`) | `kat/kem-22/Mithril-128.log` |
| `Mithril-128` | timing dec | `records/kem-22/Mithril-128__dec.json` |
| `Mithril-128` | timing enc | `records/kem-22/Mithril-128__enc.json` |
| `Mithril-128` | timing keygen | `records/kem-22/Mithril-128__keygen.json` |
| `Mithril-128` | hash profile dec | `profile/kem-22/Mithril-128__dec.json` |
| `Mithril-128` | hash profile enc | `profile/kem-22/Mithril-128__enc.json` |
| `Mithril-128` | hash profile keygen | `profile/kem-22/Mithril-128__keygen.json` |
| `Mithril-128-avx2` | KAT log (sha256 `9ff90ed4d8b9d11f…`) | `kat/kem-22/Mithril-128-avx2.log` |
| `Mithril-128-avx2` | timing dec | `records/kem-22/Mithril-128-avx2__dec.json` |
| `Mithril-128-avx2` | timing enc | `records/kem-22/Mithril-128-avx2__enc.json` |
| `Mithril-128-avx2` | timing keygen | `records/kem-22/Mithril-128-avx2__keygen.json` |
| `Mithril-256` | KAT log (sha256 `6618fcf364f6ef34…`) | `kat/kem-22/Mithril-256.log` |
| `Mithril-256` | timing dec | `records/kem-22/Mithril-256__dec.json` |
| `Mithril-256` | timing enc | `records/kem-22/Mithril-256__enc.json` |
| `Mithril-256` | timing keygen | `records/kem-22/Mithril-256__keygen.json` |
| `Mithril-256` | hash profile dec | `profile/kem-22/Mithril-256__dec.json` |
| `Mithril-256` | hash profile enc | `profile/kem-22/Mithril-256__enc.json` |
| `Mithril-256` | hash profile keygen | `profile/kem-22/Mithril-256__keygen.json` |
| `Mithril-256-avx2` | KAT log (sha256 `7522a8f004213a5c…`) | `kat/kem-22/Mithril-256-avx2.log` |
| `Mithril-256-avx2` | timing dec | `records/kem-22/Mithril-256-avx2__dec.json` |
| `Mithril-256-avx2` | timing enc | `records/kem-22/Mithril-256-avx2__enc.json` |
| `Mithril-256-avx2` | timing keygen | `records/kem-22/Mithril-256-avx2__keygen.json` |
| `Mithril-512` | KAT log (sha256 `9ba8ea80c538090a…`) | `kat/kem-22/Mithril-512.log` |
| `Mithril-512` | timing dec | `records/kem-22/Mithril-512__dec.json` |
| `Mithril-512` | timing enc | `records/kem-22/Mithril-512__enc.json` |
| `Mithril-512` | timing keygen | `records/kem-22/Mithril-512__keygen.json` |
| `Mithril-512` | hash profile dec | `profile/kem-22/Mithril-512__dec.json` |
| `Mithril-512` | hash profile enc | `profile/kem-22/Mithril-512__enc.json` |
| `Mithril-512` | hash profile keygen | `profile/kem-22/Mithril-512__keygen.json` |
| `Mithril-512-avx2` | KAT log (sha256 `13f02d381ca0ee94…`) | `kat/kem-22/Mithril-512-avx2.log` |
| `Mithril-512-avx2` | timing dec | `records/kem-22/Mithril-512-avx2__dec.json` |
| `Mithril-512-avx2` | timing enc | `records/kem-22/Mithril-512-avx2__enc.json` |
| `Mithril-512-avx2` | timing keygen | `records/kem-22/Mithril-512-avx2__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

