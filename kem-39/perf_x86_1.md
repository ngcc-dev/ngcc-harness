# kem-39 Weaver — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-39` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560890312183808.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: Weaver
- Implementation versions measured: reference
- Parameter sets: `WeaverKEM-128`, `WeaverKEM-256`, `WeaverKEM-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-39/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `WeaverKEM-128` | guide | PASS |
| `WeaverKEM-256` | guide | PASS |
| `WeaverKEM-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `WeaverKEM-128` | keygen | 545.1 k | 260 µs | 3.84e+03 | 260 µs | 17140 (5 × 3428) |
| `WeaverKEM-128` | enc | 585.8 k | 280 µs | 3.57e+03 | 280 µs | 17000 (5 × 3400) |
| `WeaverKEM-128` | dec | 610.9 k | 292 µs | 3.43e+03 | 292 µs | 16635 (5 × 3327) |
| `WeaverKEM-256` | keygen | 845.7 k | 404 µs | 2.48e+03 | 404 µs | 11670 (5 × 2334) |
| `WeaverKEM-256` | enc | 952.3 k | 455 µs | 2.2e+03 | 455 µs | 10645 (5 × 2129) |
| `WeaverKEM-256` | dec | 956.1 k | 457 µs | 2.19e+03 | 457 µs | 10465 (5 × 2093) |
| `WeaverKEM-512` | keygen | 2.74 M | 1.31 ms | 763 | 1.31 ms | 3710 (5 × 742) |
| `WeaverKEM-512` | enc | 3.18 M | 1.52 ms | 658 | 1.52 ms | 3225 (5 × 645) |
| `WeaverKEM-512` | dec | 3.23 M | 1.54 ms | 649 | 1.54 ms | 3135 (5 × 627) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `WeaverKEM-128` | keygen | 37173 | 1712 KiB | 1804 KiB |
| `WeaverKEM-128` | enc | 37173 | 1728 KiB | 1820 KiB |
| `WeaverKEM-128` | dec | 37173 | 1716 KiB | 1828 KiB |
| `WeaverKEM-256` | keygen | 42149 | 1704 KiB | 1816 KiB |
| `WeaverKEM-256` | enc | 42149 | 1748 KiB | 1816 KiB |
| `WeaverKEM-256` | dec | 42149 | 1736 KiB | 1812 KiB |
| `WeaverKEM-512` | keygen | 47821 | 1720 KiB | 1848 KiB |
| `WeaverKEM-512` | enc | 47821 | 1776 KiB | 1856 KiB |
| `WeaverKEM-512` | dec | 47821 | 1756 KiB | 1872 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `WeaverKEM-128` | 752 | 1776 | 816 | 16 |
| `WeaverKEM-256` | 1312 | 3072 | 1536 | 32 |
| `WeaverKEM-512` | 2880 | 6400 | 3392 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — grep hits for AES/Keccak/OpenSSL are not reachable in the built library

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `WeaverKEM-128` | keygen | 82% | 1.1% | drng 1, pseudoXOF 31, sm3hash 1 |
| `WeaverKEM-128` | enc | 81% | 0.8% | drng 1, pseudoXOF 36, sm3hash 1 |
| `WeaverKEM-128` | dec | 78% | 0.0% | pseudoXOF 37 |
| `WeaverKEM-256` | keygen | 76% | 0.7% | drng 1, pseudoXOF 21, pseudohash 1 |
| `WeaverKEM-256` | enc | 71% | 0.5% | drng 1, pseudoXOF 25, pseudohash 1 |
| `WeaverKEM-256` | dec | 68% | 0.0% | pseudoXOF 26 |
| `WeaverKEM-512` | keygen | 82% | 0.3% | drng 1, pseudoXOF 21, pseudohash 1 |
| `WeaverKEM-512` | enc | 77% | 0.2% | drng 1, pseudoXOF 29, pseudohash 1 |
| `WeaverKEM-512` | dec | 76% | 0.0% | pseudoXOF 30 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `WeaverKEM-128` | KAT log (sha256 `4c6584c21c5804b2…`) | `kat/kem-39/WeaverKEM-128.log` |
| `WeaverKEM-128` | timing dec | `records/kem-39/WeaverKEM-128__dec.json` |
| `WeaverKEM-128` | timing enc | `records/kem-39/WeaverKEM-128__enc.json` |
| `WeaverKEM-128` | timing keygen | `records/kem-39/WeaverKEM-128__keygen.json` |
| `WeaverKEM-128` | hash profile dec | `profile/kem-39/WeaverKEM-128__dec.json` |
| `WeaverKEM-128` | hash profile enc | `profile/kem-39/WeaverKEM-128__enc.json` |
| `WeaverKEM-128` | hash profile keygen | `profile/kem-39/WeaverKEM-128__keygen.json` |
| `WeaverKEM-256` | KAT log (sha256 `414608167f012e68…`) | `kat/kem-39/WeaverKEM-256.log` |
| `WeaverKEM-256` | timing dec | `records/kem-39/WeaverKEM-256__dec.json` |
| `WeaverKEM-256` | timing enc | `records/kem-39/WeaverKEM-256__enc.json` |
| `WeaverKEM-256` | timing keygen | `records/kem-39/WeaverKEM-256__keygen.json` |
| `WeaverKEM-256` | hash profile dec | `profile/kem-39/WeaverKEM-256__dec.json` |
| `WeaverKEM-256` | hash profile enc | `profile/kem-39/WeaverKEM-256__enc.json` |
| `WeaverKEM-256` | hash profile keygen | `profile/kem-39/WeaverKEM-256__keygen.json` |
| `WeaverKEM-512` | KAT log (sha256 `4571a836057380ec…`) | `kat/kem-39/WeaverKEM-512.log` |
| `WeaverKEM-512` | timing dec | `records/kem-39/WeaverKEM-512__dec.json` |
| `WeaverKEM-512` | timing enc | `records/kem-39/WeaverKEM-512__enc.json` |
| `WeaverKEM-512` | timing keygen | `records/kem-39/WeaverKEM-512__keygen.json` |
| `WeaverKEM-512` | hash profile dec | `profile/kem-39/WeaverKEM-512__dec.json` |
| `WeaverKEM-512` | hash profile enc | `profile/kem-39/WeaverKEM-512__enc.json` |
| `WeaverKEM-512` | hash profile keygen | `profile/kem-39/WeaverKEM-512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

