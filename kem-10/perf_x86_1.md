# kem-10 C-Multi-UR-AG — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-10` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560844149673984.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: C-Multi-UR-AG
- Implementation versions measured: reference
- Parameter sets: `CMultiURAG-128`, `CMultiURAG-256`, `CMultiURAG-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-10/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `CMultiURAG-128` | guide | PASS |
| `CMultiURAG-256` | guide | PASS |
| `CMultiURAG-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `CMultiURAG-128` | keygen | 12.19 M | 5.82 ms | 172 | 5.82 ms | 850 (5 × 170) |
| `CMultiURAG-128` | enc | 23.60 M | 11.3 ms | 88.7 | 11.3 ms | 445 (5 × 89) |
| `CMultiURAG-128` | dec | 128.36 M | 61.3 ms | 16.3 | 61.3 ms | 100 (5 × 20) |
| `CMultiURAG-256` | keygen | 37.54 M | 17.9 ms | 55.7 | 17.9 ms | 280 (5 × 56) |
| `CMultiURAG-256` | enc | 55.27 M | 26.4 ms | 37.9 | 26.4 ms | 190 (5 × 38) |
| `CMultiURAG-256` | dec | 339.59 M | 164 ms | 6.12 | 162 ms | 100 (5 × 20) |
| `CMultiURAG-512` | keygen | 165.29 M | 79 ms | 12.7 | 79 ms | 100 (5 × 20) |
| `CMultiURAG-512` | enc | 240.27 M | 115 ms | 8.71 | 115 ms | 100 (5 × 20) |
| `CMultiURAG-512` | dec | 1.45 G | 693 ms | 1.44 | 693 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `CMultiURAG-128` | keygen | 142445 | 1732 KiB | 1944 KiB |
| `CMultiURAG-128` | enc | 142445 | 1940 KiB | 2032 KiB |
| `CMultiURAG-128` | dec | 142445 | 1948 KiB | 2024 KiB |
| `CMultiURAG-256` | keygen | 142273 | 1776 KiB | 2012 KiB |
| `CMultiURAG-256` | enc | 142273 | 2032 KiB | 2140 KiB |
| `CMultiURAG-256` | dec | 142273 | 2088 KiB | 2196 KiB |
| `CMultiURAG-512` | keygen | 143369 | 1836 KiB | 2236 KiB |
| `CMultiURAG-512` | enc | 143369 | 2376 KiB | 2496 KiB |
| `CMultiURAG-512` | dec | 143369 | 2476 KiB | 2628 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `CMultiURAG-128` | 3866 | 3960 | 7332 | 64 |
| `CMultiURAG-256` | 10780 | 10892 | 15304 | 64 |
| `CMultiURAG-512` | 28866 | 29021 | 40926 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — own fips202.c compiled but unreachable from the API

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `CMultiURAG-128` | keygen | 0.0% | 6.8% | drng 44 |
| `CMultiURAG-128` | enc | 2.8% | 3.7% | drng 45, pseudohash 2 |
| `CMultiURAG-128` | dec | 0.5% | 1.4% | drng 85, pseudohash 2 |
| `CMultiURAG-256` | keygen | 0.0% | 5.2% | drng 54 |
| `CMultiURAG-256` | enc | 2.9% | 3.6% | drng 55, pseudohash 2 |
| `CMultiURAG-256` | dec | 0.5% | 1.2% | drng 105, pseudohash 2 |
| `CMultiURAG-512` | keygen | 0.0% | 3.4% | drng 76 |
| `CMultiURAG-512` | enc | 1.8% | 2.4% | drng 77, pseudohash 2 |
| `CMultiURAG-512` | dec | 0.3% | 0.8% | drng 149, pseudohash 2 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `CMultiURAG-128` | KAT log (sha256 `b9d9f1686a50cd41…`) | `kat/kem-10/CMultiURAG-128.log` |
| `CMultiURAG-128` | timing dec | `records/kem-10/CMultiURAG-128__dec.json` |
| `CMultiURAG-128` | timing enc | `records/kem-10/CMultiURAG-128__enc.json` |
| `CMultiURAG-128` | timing keygen | `records/kem-10/CMultiURAG-128__keygen.json` |
| `CMultiURAG-128` | hash profile dec | `profile/kem-10/CMultiURAG-128__dec.json` |
| `CMultiURAG-128` | hash profile enc | `profile/kem-10/CMultiURAG-128__enc.json` |
| `CMultiURAG-128` | hash profile keygen | `profile/kem-10/CMultiURAG-128__keygen.json` |
| `CMultiURAG-256` | KAT log (sha256 `e4dbd28c00cff934…`) | `kat/kem-10/CMultiURAG-256.log` |
| `CMultiURAG-256` | timing dec | `records/kem-10/CMultiURAG-256__dec.json` |
| `CMultiURAG-256` | timing enc | `records/kem-10/CMultiURAG-256__enc.json` |
| `CMultiURAG-256` | timing keygen | `records/kem-10/CMultiURAG-256__keygen.json` |
| `CMultiURAG-256` | hash profile dec | `profile/kem-10/CMultiURAG-256__dec.json` |
| `CMultiURAG-256` | hash profile enc | `profile/kem-10/CMultiURAG-256__enc.json` |
| `CMultiURAG-256` | hash profile keygen | `profile/kem-10/CMultiURAG-256__keygen.json` |
| `CMultiURAG-512` | KAT log (sha256 `1e04ca9f582e5fc1…`) | `kat/kem-10/CMultiURAG-512.log` |
| `CMultiURAG-512` | timing dec | `records/kem-10/CMultiURAG-512__dec.json` |
| `CMultiURAG-512` | timing enc | `records/kem-10/CMultiURAG-512__enc.json` |
| `CMultiURAG-512` | timing keygen | `records/kem-10/CMultiURAG-512__keygen.json` |
| `CMultiURAG-512` | hash profile dec | `profile/kem-10/CMultiURAG-512__dec.json` |
| `CMultiURAG-512` | hash profile enc | `profile/kem-10/CMultiURAG-512__enc.json` |
| `CMultiURAG-512` | hash profile keygen | `profile/kem-10/CMultiURAG-512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

