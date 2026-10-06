# sign-16 Octarine — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-16` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561078258946048.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: Octarine
- Implementation versions measured: reference
- Parameter sets: `Octarine-128`, `Octarine-256`, `Octarine-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-16/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Octarine-128` | guide | PASS |
| `Octarine-256` | guide | PASS |
| `Octarine-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Octarine-128` | keygen | 767.9 k | 367 µs | 2.73e+03 | 367 µs | 12610 (5 × 2522) |
| `Octarine-128` | sign | 2.09 M | 999 µs | 1e+03 | 999 µs | 3190 (5 × 638) |
| `Octarine-128` | verify | 879.7 k | 420 µs | 2.38e+03 | 420 µs | 11670 (5 × 2334) |
| `Octarine-256` | keygen | 1.48 M | 705 µs | 1.42e+03 | 705 µs | 6675 (5 × 1335) |
| `Octarine-256` | sign | 4.58 M | 2.19 ms | 457 | 2.19 ms | 3985 (5 × 797) |
| `Octarine-256` | verify | 1.71 M | 817 µs | 1.22e+03 | 817 µs | 6040 (5 × 1208) |
| `Octarine-512` | keygen | 3.79 M | 1.81 ms | 553 | 1.81 ms | 2640 (5 × 528) |
| `Octarine-512` | sign | 17.20 M | 8.22 ms | 122 | 8.21 ms | 1545 (5 × 309) |
| `Octarine-512` | verify | 4.35 M | 2.08 ms | 481 | 2.08 ms | 2395 (5 × 479) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Octarine-128` | keygen | 54217 | 1740 KiB | 1848 KiB |
| `Octarine-128` | sign | 54217 | 1784 KiB | 1876 KiB |
| `Octarine-128` | verify | 54217 | 1812 KiB | 1880 KiB |
| `Octarine-256` | keygen | 56113 | 1748 KiB | 1908 KiB |
| `Octarine-256` | sign | 56113 | 1832 KiB | 1956 KiB |
| `Octarine-256` | verify | 56113 | 1852 KiB | 1960 KiB |
| `Octarine-512` | keygen | 56249 | 1756 KiB | 2012 KiB |
| `Octarine-512` | sign | 56249 | 1956 KiB | 2120 KiB |
| `Octarine-512` | verify | 56249 | 2052 KiB | 2144 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `Octarine-128` | 1344 | 2432 | 2564 |
| `Octarine-256` | 2368 | 4608 | 5449 |
| `Octarine-512` | 5184 | 11776 | 14713 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Octarine-128` | keygen | 60% | 0.8% | drng 1, pseudoXOF 7 |
| `Octarine-128` | sign | 52% | 0.3% | drng 1, pseudoXOF 21.2 |
| `Octarine-128` | verify | 51% | 0.0% | pseudoXOF 9 |
| `Octarine-256` | keygen | 57% | 0.4% | drng 1, pseudoXOF 12 |
| `Octarine-256` | sign | 53% | 0.1% | drng 1, pseudoXOF 37.1 |
| `Octarine-256` | verify | 50% | 0.0% | pseudoXOF 13 |
| `Octarine-512` | keygen | 53% | 0.2% | drng 1, pseudoXOF 27 |
| `Octarine-512` | sign | 55% | 0.0% | drng 1, pseudoXOF 115 |
| `Octarine-512` | verify | 47% | 0.0% | pseudoXOF 26 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Octarine-128` | KAT log (sha256 `eb1ad326a3768e2d…`) | `kat/sign-16/Octarine-128.log` |
| `Octarine-128` | timing keygen | `records/sign-16/Octarine-128__keygen.json` |
| `Octarine-128` | timing sign | `records/sign-16/Octarine-128__sign.json` |
| `Octarine-128` | timing verify | `records/sign-16/Octarine-128__verify.json` |
| `Octarine-128` | hash profile keygen | `profile/sign-16/Octarine-128__keygen.json` |
| `Octarine-128` | hash profile sign | `profile/sign-16/Octarine-128__sign.json` |
| `Octarine-128` | hash profile verify | `profile/sign-16/Octarine-128__verify.json` |
| `Octarine-256` | KAT log (sha256 `908f9d4dd5d4b62b…`) | `kat/sign-16/Octarine-256.log` |
| `Octarine-256` | timing keygen | `records/sign-16/Octarine-256__keygen.json` |
| `Octarine-256` | timing sign | `records/sign-16/Octarine-256__sign.json` |
| `Octarine-256` | timing verify | `records/sign-16/Octarine-256__verify.json` |
| `Octarine-256` | hash profile keygen | `profile/sign-16/Octarine-256__keygen.json` |
| `Octarine-256` | hash profile sign | `profile/sign-16/Octarine-256__sign.json` |
| `Octarine-256` | hash profile verify | `profile/sign-16/Octarine-256__verify.json` |
| `Octarine-512` | KAT log (sha256 `ad0d23d697962d09…`) | `kat/sign-16/Octarine-512.log` |
| `Octarine-512` | timing keygen | `records/sign-16/Octarine-512__keygen.json` |
| `Octarine-512` | timing sign | `records/sign-16/Octarine-512__sign.json` |
| `Octarine-512` | timing verify | `records/sign-16/Octarine-512__verify.json` |
| `Octarine-512` | hash profile keygen | `profile/sign-16/Octarine-512__keygen.json` |
| `Octarine-512` | hash profile sign | `profile/sign-16/Octarine-512__sign.json` |
| `Octarine-512` | hash profile verify | `profile/sign-16/Octarine-512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

