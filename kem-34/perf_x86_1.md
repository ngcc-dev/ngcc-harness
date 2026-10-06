# kem-34 Rudraksh2 — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-34` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560881139240960.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: Rudraksh2
- Implementation versions measured: reference
- Parameter sets: `lwekem128`, `lwekem256`, `lwekem512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-34/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `lwekem128` | guide | PASS |
| `lwekem256` | guide | PASS |
| `lwekem512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `lwekem128` | keygen | 3.19 M | 1.52 ms | 657 | 1.52 ms | 3215 (5 × 643) |
| `lwekem128` | enc | 3.13 M | 1.5 ms | 669 | 1.49 ms | 3290 (5 × 658) |
| `lwekem128` | dec | 3.17 M | 1.51 ms | 660 | 1.51 ms | 3225 (5 × 645) |
| `lwekem256` | keygen | 6.31 M | 3.01 ms | 332 | 3.01 ms | 1650 (5 × 330) |
| `lwekem256` | enc | 6.25 M | 3.01 ms | 332 | 2.98 ms | 1660 (5 × 332) |
| `lwekem256` | dec | 6.33 M | 3.02 ms | 331 | 3.02 ms | 1655 (5 × 331) |
| `lwekem512` | keygen | 19.96 M | 9.53 ms | 105 | 9.53 ms | 520 (5 × 104) |
| `lwekem512` | enc | 19.99 M | 9.55 ms | 105 | 9.55 ms | 525 (5 × 105) |
| `lwekem512` | dec | 20.15 M | 9.63 ms | 104 | 9.63 ms | 520 (5 × 104) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `lwekem128` | keygen | 27929 | 1696 KiB | 1804 KiB |
| `lwekem128` | enc | 27929 | 1708 KiB | 1772 KiB |
| `lwekem128` | dec | 27929 | 1700 KiB | 1800 KiB |
| `lwekem256` | keygen | 29217 | 1716 KiB | 1828 KiB |
| `lwekem256` | enc | 29217 | 1720 KiB | 1832 KiB |
| `lwekem256` | dec | 29217 | 1708 KiB | 1832 KiB |
| `lwekem512` | keygen | 31193 | 1708 KiB | 1820 KiB |
| `lwekem512` | enc | 31193 | 1772 KiB | 1868 KiB |
| `lwekem512` | dec | 31193 | 1776 KiB | 1860 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `lwekem128` | 880 | 1776 | 912 | 16 |
| `lwekem256` | 1760 | 3552 | 1728 | 32 |
| `lwekem512` | 3392 | 6848 | 3552 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `lwekem128` | keygen | 92% | 0.3% | drng 2, pseudoXOF 101 |
| `lwekem128` | enc | 93% | 0.1% | drng 1, pseudoXOF 102 |
| `lwekem128` | dec | 91% | 0.0% | pseudoXOF 102 |
| `lwekem256` | keygen | 92% | 0.1% | drng 2, pseudoXOF 101 |
| `lwekem256` | enc | 93% | 0.1% | drng 1, pseudoXOF 102 |
| `lwekem256` | dec | 92% | 0.0% | pseudoXOF 102 |
| `lwekem512` | keygen | 96% | 0.1% | drng 2, pseudoXOF 82 |
| `lwekem512` | enc | 96% | 0.0% | drng 1, pseudoXOF 83 |
| `lwekem512` | dec | 95% | 0.0% | pseudoXOF 83 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `lwekem128` | KAT log (sha256 `377d59381a43b118…`) | `kat/kem-34/lwekem128.log` |
| `lwekem128` | timing dec | `records/kem-34/lwekem128__dec.json` |
| `lwekem128` | timing enc | `records/kem-34/lwekem128__enc.json` |
| `lwekem128` | timing keygen | `records/kem-34/lwekem128__keygen.json` |
| `lwekem128` | hash profile dec | `profile/kem-34/lwekem128__dec.json` |
| `lwekem128` | hash profile enc | `profile/kem-34/lwekem128__enc.json` |
| `lwekem128` | hash profile keygen | `profile/kem-34/lwekem128__keygen.json` |
| `lwekem256` | KAT log (sha256 `e6a8535cd995e1f3…`) | `kat/kem-34/lwekem256.log` |
| `lwekem256` | timing dec | `records/kem-34/lwekem256__dec.json` |
| `lwekem256` | timing enc | `records/kem-34/lwekem256__enc.json` |
| `lwekem256` | timing keygen | `records/kem-34/lwekem256__keygen.json` |
| `lwekem256` | hash profile dec | `profile/kem-34/lwekem256__dec.json` |
| `lwekem256` | hash profile enc | `profile/kem-34/lwekem256__enc.json` |
| `lwekem256` | hash profile keygen | `profile/kem-34/lwekem256__keygen.json` |
| `lwekem512` | KAT log (sha256 `cc8c40eebda05c93…`) | `kat/kem-34/lwekem512.log` |
| `lwekem512` | timing dec | `records/kem-34/lwekem512__dec.json` |
| `lwekem512` | timing enc | `records/kem-34/lwekem512__enc.json` |
| `lwekem512` | timing keygen | `records/kem-34/lwekem512__keygen.json` |
| `lwekem512` | hash profile dec | `profile/kem-34/lwekem512__dec.json` |
| `lwekem512` | hash profile enc | `profile/kem-34/lwekem512__enc.json` |
| `lwekem512` | hash profile keygen | `profile/kem-34/lwekem512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

