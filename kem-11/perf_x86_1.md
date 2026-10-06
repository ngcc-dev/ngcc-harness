# kem-11 COMPASS-KEM — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-11` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560844283891712.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: COMPASS-KEM
- Implementation versions measured: reference
- Parameter sets: `COMPASS-KEM-128`, `COMPASS-KEM-256`, `COMPASS-KEM-384`, `COMPASS-KEM-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-11/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `COMPASS-KEM-128` | guide | PASS |
| `COMPASS-KEM-256` | guide | PASS |
| `COMPASS-KEM-384` | guide | PASS |
| `COMPASS-KEM-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `COMPASS-KEM-128` | keygen | 1.61 M | 770 µs | 1.3e+03 | 769 µs | 6265 (5 × 1253) |
| `COMPASS-KEM-128` | enc | 1.64 M | 782 µs | 1.28e+03 | 781 µs | 6210 (5 × 1242) |
| `COMPASS-KEM-128` | dec | 1.66 M | 794 µs | 1.26e+03 | 794 µs | 6220 (5 × 1244) |
| `COMPASS-KEM-256` | keygen | 6.24 M | 2.98 ms | 335 | 2.98 ms | 1660 (5 × 332) |
| `COMPASS-KEM-256` | enc | 6.25 M | 2.99 ms | 335 | 2.99 ms | 1670 (5 × 334) |
| `COMPASS-KEM-256` | dec | 6.30 M | 3.01 ms | 332 | 3.01 ms | 1650 (5 × 330) |
| `COMPASS-KEM-384` | keygen | 3.81 M | 1.82 ms | 550 | 1.82 ms | 2670 (5 × 534) |
| `COMPASS-KEM-384` | enc | 3.97 M | 1.89 ms | 528 | 1.89 ms | 2535 (5 × 507) |
| `COMPASS-KEM-384` | dec | 4.15 M | 1.98 ms | 505 | 1.98 ms | 2520 (5 × 504) |
| `COMPASS-KEM-512` | keygen | 6.63 M | 3.17 ms | 316 | 3.17 ms | 1550 (5 × 310) |
| `COMPASS-KEM-512` | enc | 6.83 M | 3.26 ms | 307 | 3.26 ms | 1530 (5 × 306) |
| `COMPASS-KEM-512` | dec | 7.07 M | 3.38 ms | 296 | 3.38 ms | 1480 (5 × 296) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `COMPASS-KEM-128` | keygen | 26865 | 1692 KiB | 1776 KiB |
| `COMPASS-KEM-128` | enc | 26865 | 1716 KiB | 1784 KiB |
| `COMPASS-KEM-128` | dec | 26865 | 1704 KiB | 1816 KiB |
| `COMPASS-KEM-256` | keygen | 27137 | 1708 KiB | 1820 KiB |
| `COMPASS-KEM-256` | enc | 27137 | 1700 KiB | 1832 KiB |
| `COMPASS-KEM-256` | dec | 27137 | 1716 KiB | 1812 KiB |
| `COMPASS-KEM-384` | keygen | 29121 | 1712 KiB | 1812 KiB |
| `COMPASS-KEM-384` | enc | 29121 | 1728 KiB | 1800 KiB |
| `COMPASS-KEM-384` | dec | 29121 | 1736 KiB | 1808 KiB |
| `COMPASS-KEM-512` | keygen | 29081 | 1700 KiB | 1832 KiB |
| `COMPASS-KEM-512` | enc | 29081 | 1752 KiB | 1820 KiB |
| `COMPASS-KEM-512` | dec | 29081 | 1764 KiB | 1832 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `COMPASS-KEM-128` | 672 | 1504 | 768 | 32 |
| `COMPASS-KEM-256` | 1312 | 2912 | 1472 | 32 |
| `COMPASS-KEM-384` | 2144 | 4704 | 2432 | 32 |
| `COMPASS-KEM-512` | 2848 | 6240 | 3264 | 32 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — `shake*` names are shims over pseudoXOF

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `COMPASS-KEM-128` | keygen | 96% | 0.4% | drng 1, pseudoXOF 6, pseudohash 1, sm3hash 1 |
| `COMPASS-KEM-128` | enc | 95% | 0.3% | drng 1, pseudoXOF 6, pseudohash 1, sm3hash 1 |
| `COMPASS-KEM-128` | dec | 93% | 0.0% | pseudoXOF 7, pseudohash 1 |
| `COMPASS-KEM-256` | keygen | 97% | 0.1% | drng 1, pseudoXOF 20, pseudohash 1, sm3hash 1 |
| `COMPASS-KEM-256` | enc | 97% | 0.1% | drng 1, pseudoXOF 20, pseudohash 1, sm3hash 1 |
| `COMPASS-KEM-256` | dec | 96% | 0.0% | pseudoXOF 21, pseudohash 1 |
| `COMPASS-KEM-384` | keygen | 91% | 0.2% | drng 1, pseudoXOF 12, pseudohash 1, sm3hash 1 |
| `COMPASS-KEM-384` | enc | 88% | 0.1% | drng 1, pseudoXOF 12, pseudohash 1, sm3hash 1 |
| `COMPASS-KEM-384` | dec | 84% | 0.0% | pseudoXOF 13, pseudohash 1 |
| `COMPASS-KEM-512` | keygen | 93% | 0.1% | drng 1, pseudoXOF 20, pseudohash 1, sm3hash 1 |
| `COMPASS-KEM-512` | enc | 90% | 0.1% | drng 1, pseudoXOF 20, pseudohash 1, sm3hash 1 |
| `COMPASS-KEM-512` | dec | 87% | 0.0% | pseudoXOF 21, pseudohash 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `COMPASS-KEM-128` | KAT log (sha256 `05aa7e888c5a2bea…`) | `kat/kem-11/COMPASS-KEM-128.log` |
| `COMPASS-KEM-128` | timing dec | `records/kem-11/COMPASS-KEM-128__dec.json` |
| `COMPASS-KEM-128` | timing enc | `records/kem-11/COMPASS-KEM-128__enc.json` |
| `COMPASS-KEM-128` | timing keygen | `records/kem-11/COMPASS-KEM-128__keygen.json` |
| `COMPASS-KEM-128` | hash profile dec | `profile/kem-11/COMPASS-KEM-128__dec.json` |
| `COMPASS-KEM-128` | hash profile enc | `profile/kem-11/COMPASS-KEM-128__enc.json` |
| `COMPASS-KEM-128` | hash profile keygen | `profile/kem-11/COMPASS-KEM-128__keygen.json` |
| `COMPASS-KEM-256` | KAT log (sha256 `aa4d37b589294bb8…`) | `kat/kem-11/COMPASS-KEM-256.log` |
| `COMPASS-KEM-256` | timing dec | `records/kem-11/COMPASS-KEM-256__dec.json` |
| `COMPASS-KEM-256` | timing enc | `records/kem-11/COMPASS-KEM-256__enc.json` |
| `COMPASS-KEM-256` | timing keygen | `records/kem-11/COMPASS-KEM-256__keygen.json` |
| `COMPASS-KEM-256` | hash profile dec | `profile/kem-11/COMPASS-KEM-256__dec.json` |
| `COMPASS-KEM-256` | hash profile enc | `profile/kem-11/COMPASS-KEM-256__enc.json` |
| `COMPASS-KEM-256` | hash profile keygen | `profile/kem-11/COMPASS-KEM-256__keygen.json` |
| `COMPASS-KEM-384` | KAT log (sha256 `abd315e709b6f9a0…`) | `kat/kem-11/COMPASS-KEM-384.log` |
| `COMPASS-KEM-384` | timing dec | `records/kem-11/COMPASS-KEM-384__dec.json` |
| `COMPASS-KEM-384` | timing enc | `records/kem-11/COMPASS-KEM-384__enc.json` |
| `COMPASS-KEM-384` | timing keygen | `records/kem-11/COMPASS-KEM-384__keygen.json` |
| `COMPASS-KEM-384` | hash profile dec | `profile/kem-11/COMPASS-KEM-384__dec.json` |
| `COMPASS-KEM-384` | hash profile enc | `profile/kem-11/COMPASS-KEM-384__enc.json` |
| `COMPASS-KEM-384` | hash profile keygen | `profile/kem-11/COMPASS-KEM-384__keygen.json` |
| `COMPASS-KEM-512` | KAT log (sha256 `42977d1c10a5a3f0…`) | `kat/kem-11/COMPASS-KEM-512.log` |
| `COMPASS-KEM-512` | timing dec | `records/kem-11/COMPASS-KEM-512__dec.json` |
| `COMPASS-KEM-512` | timing enc | `records/kem-11/COMPASS-KEM-512__enc.json` |
| `COMPASS-KEM-512` | timing keygen | `records/kem-11/COMPASS-KEM-512__keygen.json` |
| `COMPASS-KEM-512` | hash profile dec | `profile/kem-11/COMPASS-KEM-512__dec.json` |
| `COMPASS-KEM-512` | hash profile enc | `profile/kem-11/COMPASS-KEM-512__enc.json` |
| `COMPASS-KEM-512` | hash profile keygen | `profile/kem-11/COMPASS-KEM-512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

