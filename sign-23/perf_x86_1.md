# sign-23 Shuttle — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-23` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561096101515264.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: Shuttle
- Implementation versions measured: reference
- Parameter sets: `SHUTTLE-128`, `SHUTTLE-256`, `SHUTTLE-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-23/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `SHUTTLE-128` | guide | PASS |
| `SHUTTLE-256` | guide | PASS |
| `SHUTTLE-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `SHUTTLE-128` | keygen | 8.65 M | 4.13 ms | 242 | 4.13 ms | 3045 (5 × 609) |
| `SHUTTLE-128` | sign | 4.51 M | 2.15 ms | 465 | 2.15 ms | 2345 (5 × 469) |
| `SHUTTLE-128` | verify | 1.11 M | 532 µs | 1.88e+03 | 532 µs | 9195 (5 × 1839) |
| `SHUTTLE-256` | keygen | 9.11 M | 4.35 ms | 230 | 4.35 ms | 765 (5 × 153) |
| `SHUTTLE-256` | sign | 7.15 M | 3.42 ms | 293 | 3.42 ms | 1465 (5 × 293) |
| `SHUTTLE-256` | verify | 1.51 M | 723 µs | 1.38e+03 | 723 µs | 6740 (5 × 1348) |
| `SHUTTLE-512` | keygen | 20.92 M | 10 ms | 100 | 9.99 ms | 1185 (5 × 237) |
| `SHUTTLE-512` | sign | 14.60 M | 6.99 ms | 143 | 6.99 ms | 715 (5 × 143) |
| `SHUTTLE-512` | verify | 2.83 M | 1.35 ms | 741 | 1.35 ms | 3635 (5 × 727) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `SHUTTLE-128` | keygen | 82449 | 1704 KiB | 1868 KiB |
| `SHUTTLE-128` | sign | 82449 | 1760 KiB | 1936 KiB |
| `SHUTTLE-128` | verify | 82449 | 1856 KiB | 1940 KiB |
| `SHUTTLE-256` | keygen | 85213 | 1644 KiB | 1836 KiB |
| `SHUTTLE-256` | sign | 85213 | 1788 KiB | 1980 KiB |
| `SHUTTLE-256` | verify | 85213 | 1900 KiB | 2024 KiB |
| `SHUTTLE-512` | keygen | 86253 | 1732 KiB | 1956 KiB |
| `SHUTTLE-512` | sign | 86253 | 1856 KiB | 2168 KiB |
| `SHUTTLE-512` | verify | 86253 | 2068 KiB | 2156 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `SHUTTLE-128` | 1264 | 2288 | 1183 |
| `SHUTTLE-256` | 1952 | 3680 | 2417 |
| `SHUTTLE-512` | 3648 | 7104 | 5001 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — uses the ICCS SM3 DRBG (init/get_random_number) as its XOF; AVX2/AVX-512 SM3 DRBG in optimized

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `SHUTTLE-128` | keygen | 0.0% | 81% | drng 289 |
| `SHUTTLE-128` | sign | 0.0% | 68% | drng 90 |
| `SHUTTLE-128` | verify | 0.0% | 57% | drng 20 |
| `SHUTTLE-256` | keygen | 0.0% | 80% | drng 293 |
| `SHUTTLE-256` | sign | 0.0% | 68% | drng 135 |
| `SHUTTLE-256` | verify | 0.0% | 55% | drng 20 |
| `SHUTTLE-512` | keygen | 0.0% | 83% | drng 648 |
| `SHUTTLE-512` | sign | 0.0% | 64% | drng 230 |
| `SHUTTLE-512` | verify | 0.0% | 56% | drng 20 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `SHUTTLE-128` | KAT log (sha256 `b0dadc14b16b5c1f…`) | `kat/sign-23/SHUTTLE-128.log` |
| `SHUTTLE-128` | timing keygen | `records/sign-23/SHUTTLE-128__keygen.json` |
| `SHUTTLE-128` | timing sign | `records/sign-23/SHUTTLE-128__sign.json` |
| `SHUTTLE-128` | timing verify | `records/sign-23/SHUTTLE-128__verify.json` |
| `SHUTTLE-128` | hash profile keygen | `profile/sign-23/SHUTTLE-128__keygen.json` |
| `SHUTTLE-128` | hash profile sign | `profile/sign-23/SHUTTLE-128__sign.json` |
| `SHUTTLE-128` | hash profile verify | `profile/sign-23/SHUTTLE-128__verify.json` |
| `SHUTTLE-256` | KAT log (sha256 `e23c9ac1263bb8a4…`) | `kat/sign-23/SHUTTLE-256.log` |
| `SHUTTLE-256` | timing keygen | `records/sign-23/SHUTTLE-256__keygen.json` |
| `SHUTTLE-256` | timing sign | `records/sign-23/SHUTTLE-256__sign.json` |
| `SHUTTLE-256` | timing verify | `records/sign-23/SHUTTLE-256__verify.json` |
| `SHUTTLE-256` | hash profile keygen | `profile/sign-23/SHUTTLE-256__keygen.json` |
| `SHUTTLE-256` | hash profile sign | `profile/sign-23/SHUTTLE-256__sign.json` |
| `SHUTTLE-256` | hash profile verify | `profile/sign-23/SHUTTLE-256__verify.json` |
| `SHUTTLE-512` | KAT log (sha256 `5481f4e153835c12…`) | `kat/sign-23/SHUTTLE-512.log` |
| `SHUTTLE-512` | timing keygen | `records/sign-23/SHUTTLE-512__keygen.json` |
| `SHUTTLE-512` | timing sign | `records/sign-23/SHUTTLE-512__sign.json` |
| `SHUTTLE-512` | timing verify | `records/sign-23/SHUTTLE-512__verify.json` |
| `SHUTTLE-512` | hash profile keygen | `profile/sign-23/SHUTTLE-512__keygen.json` |
| `SHUTTLE-512` | hash profile sign | `profile/sign-23/SHUTTLE-512__sign.json` |
| `SHUTTLE-512` | hash profile verify | `profile/sign-23/SHUTTLE-512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

