# kem-24 MORNING-Scabbard — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-24` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560862906601472.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: MORNING-Scabbard
- Implementation versions measured: reference
- Parameter sets: `scabbard128`, `scabbard256`, `scabbard512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-24/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `scabbard128` | guide | PASS |
| `scabbard256` | guide | PASS |
| `scabbard512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `scabbard128` | keygen | 741.6 k | 355 µs | 2.82e+03 | 355 µs | 13510 (5 × 2702) |
| `scabbard128` | enc | 775.8 k | 371 µs | 2.69e+03 | 371 µs | 13215 (5 × 2643) |
| `scabbard128` | dec | 779.7 k | 373 µs | 2.68e+03 | 373 µs | 13150 (5 × 2630) |
| `scabbard256` | keygen | 1.72 M | 825 µs | 1.21e+03 | 825 µs | 5955 (5 × 1191) |
| `scabbard256` | enc | 1.84 M | 882 µs | 1.13e+03 | 880 µs | 5625 (5 × 1125) |
| `scabbard256` | dec | 1.89 M | 906 µs | 1.1e+03 | 906 µs | 5450 (5 × 1090) |
| `scabbard512` | keygen | 5.12 M | 2.45 ms | 409 | 2.45 ms | 2010 (5 × 402) |
| `scabbard512` | enc | 5.57 M | 2.66 ms | 375 | 2.66 ms | 1875 (5 × 375) |
| `scabbard512` | dec | 5.77 M | 2.76 ms | 363 | 2.76 ms | 1810 (5 × 362) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `scabbard128` | keygen | 26213 | 1684 KiB | 1800 KiB |
| `scabbard128` | enc | 26213 | 1704 KiB | 1808 KiB |
| `scabbard128` | dec | 26213 | 1716 KiB | 1792 KiB |
| `scabbard256` | keygen | 26577 | 1712 KiB | 1808 KiB |
| `scabbard256` | enc | 26577 | 1652 KiB | 1780 KiB |
| `scabbard256` | dec | 26577 | 1740 KiB | 1804 KiB |
| `scabbard512` | keygen | 26733 | 1692 KiB | 1844 KiB |
| `scabbard512` | enc | 26733 | 1756 KiB | 1868 KiB |
| `scabbard512` | dec | 26733 | 1748 KiB | 1816 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `scabbard128` | 736 | 1056 | 760 | 16 |
| `scabbard256` | 1616 | 2256 | 1648 | 32 |
| `scabbard512` | 2880 | 4032 | 3072 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `scabbard128` | keygen | 70% | 1.8% | drng 3, pseudoXOF 92 |
| `scabbard128` | enc | 69% | 0.6% | drng 1, pseudoXOF 95 |
| `scabbard128` | dec | 67% | 0.0% | pseudoXOF 93 |
| `scabbard256` | keygen | 53% | 0.8% | drng 3, pseudoXOF 92 |
| `scabbard256` | enc | 52% | 0.3% | drng 1, pseudoXOF 95 |
| `scabbard256` | dec | 49% | 0.0% | pseudoXOF 93 |
| `scabbard512` | keygen | 51% | 0.4% | drng 3, pseudoXOF 74 |
| `scabbard512` | enc | 49% | 0.1% | drng 1, pseudoXOF 77 |
| `scabbard512` | dec | 45% | 0.0% | pseudoXOF 75 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `scabbard128` | KAT log (sha256 `aed56a053af9471c…`) | `kat/kem-24/scabbard128.log` |
| `scabbard128` | timing dec | `records/kem-24/scabbard128__dec.json` |
| `scabbard128` | timing enc | `records/kem-24/scabbard128__enc.json` |
| `scabbard128` | timing keygen | `records/kem-24/scabbard128__keygen.json` |
| `scabbard128` | hash profile dec | `profile/kem-24/scabbard128__dec.json` |
| `scabbard128` | hash profile enc | `profile/kem-24/scabbard128__enc.json` |
| `scabbard128` | hash profile keygen | `profile/kem-24/scabbard128__keygen.json` |
| `scabbard256` | KAT log (sha256 `cad16277ac56bc8f…`) | `kat/kem-24/scabbard256.log` |
| `scabbard256` | timing dec | `records/kem-24/scabbard256__dec.json` |
| `scabbard256` | timing enc | `records/kem-24/scabbard256__enc.json` |
| `scabbard256` | timing keygen | `records/kem-24/scabbard256__keygen.json` |
| `scabbard256` | hash profile dec | `profile/kem-24/scabbard256__dec.json` |
| `scabbard256` | hash profile enc | `profile/kem-24/scabbard256__enc.json` |
| `scabbard256` | hash profile keygen | `profile/kem-24/scabbard256__keygen.json` |
| `scabbard512` | KAT log (sha256 `e92c5a159bb1f547…`) | `kat/kem-24/scabbard512.log` |
| `scabbard512` | timing dec | `records/kem-24/scabbard512__dec.json` |
| `scabbard512` | timing enc | `records/kem-24/scabbard512__enc.json` |
| `scabbard512` | timing keygen | `records/kem-24/scabbard512__keygen.json` |
| `scabbard512` | hash profile dec | `profile/kem-24/scabbard512__dec.json` |
| `scabbard512` | hash profile enc | `profile/kem-24/scabbard512__enc.json` |
| `scabbard512` | hash profile keygen | `profile/kem-24/scabbard512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

