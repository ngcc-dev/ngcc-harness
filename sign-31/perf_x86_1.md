# sign-31 TSUOV — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-31` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561105597419520.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: TSUOV
- Implementation versions measured: reference
- Parameter sets: `TSUOV_128`, `TSUOV_256`, `TSUOV_512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-31/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `TSUOV_128` | guide | PASS |
| `TSUOV_256` | guide | PASS |
| `TSUOV_512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `TSUOV_128` | keygen | 10.52 M | 5.02 ms | 199 | 5.02 ms | 995 (5 × 199) |
| `TSUOV_128` | sign | 14.54 M | 6.95 ms | 144 | 6.95 ms | 720 (5 × 144) |
| `TSUOV_128` | verify | 11.83 M | 5.65 ms | 177 | 5.65 ms | 880 (5 × 176) |
| `TSUOV_256` | keygen | 67.40 M | 32.2 ms | 31.1 | 32.2 ms | 160 (5 × 32) |
| `TSUOV_256` | sign | 94.37 M | 45.1 ms | 22.2 | 45.1 ms | 115 (5 × 23) |
| `TSUOV_256` | verify | 79.44 M | 37.9 ms | 26.4 | 37.9 ms | 135 (5 × 27) |
| `TSUOV_512` | keygen | 918.12 M | 441 ms | 2.27 | 439 ms | 100 (5 × 20) |
| `TSUOV_512` | sign | 862.97 M | 414 ms | 2.42 | 412 ms | 100 (5 × 20) |
| `TSUOV_512` | verify | 898.63 M | 431 ms | 2.32 | 429 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `TSUOV_128` | keygen | 41693 | 1712 KiB | 1792 KiB |
| `TSUOV_128` | sign | 41693 | 1720 KiB | 1880 KiB |
| `TSUOV_128` | verify | 41693 | 1776 KiB | 1868 KiB |
| `TSUOV_256` | keygen | 50789 | 1716 KiB | 1856 KiB |
| `TSUOV_256` | sign | 50789 | 1780 KiB | 2028 KiB |
| `TSUOV_256` | verify | 50789 | 1900 KiB | 1972 KiB |
| `TSUOV_512` | keygen | 166297 | 1728 KiB | 2128 KiB |
| `TSUOV_512` | sign | 166297 | 2044 KiB | 2324 KiB |
| `TSUOV_512` | verify | 166297 | 2252 KiB | 2384 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `TSUOV_128` | 779 | 32 | 896 |
| `TSUOV_256` | 2170 | 64 | 2412 |
| `TSUOV_512` | 21319 | 128 | 2939 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `TSUOV_128` | keygen | 77% | 0.1% | drng 2, pseudoXOF 123 |
| `TSUOV_128` | sign | 56% | 0.1% | drng 3, pseudoXOF 126, pseudohash 1, sm3hash 1.35 |
| `TSUOV_128` | verify | 69% | 0.0% | pseudoXOF 123, pseudohash 1 |
| `TSUOV_256` | keygen | 75% | 0.0% | drng 2, pseudoXOF 229 |
| `TSUOV_256` | sign | 54% | 0.0% | drng 3, pseudoXOF 234, pseudohash 1, sm3hash 4.17 |
| `TSUOV_256` | verify | 64% | 0.0% | pseudoXOF 229, pseudohash 1 |
| `TSUOV_512` | keygen | 74% | 0.0% | drng 2, pseudoXOF 437 |
| `TSUOV_512` | sign | 79% | 0.0% | drng 3, pseudoXOF 439, pseudohash 1, sm3hash 2 |
| `TSUOV_512` | verify | 75% | 0.0% | pseudoXOF 437, pseudohash 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `TSUOV_128` | KAT log (sha256 `2cadb0d4c4d3a9ee…`) | `kat/sign-31/TSUOV_128.log` |
| `TSUOV_128` | timing keygen | `records/sign-31/TSUOV_128__keygen.json` |
| `TSUOV_128` | timing sign | `records/sign-31/TSUOV_128__sign.json` |
| `TSUOV_128` | timing verify | `records/sign-31/TSUOV_128__verify.json` |
| `TSUOV_128` | hash profile keygen | `profile/sign-31/TSUOV_128__keygen.json` |
| `TSUOV_128` | hash profile sign | `profile/sign-31/TSUOV_128__sign.json` |
| `TSUOV_128` | hash profile verify | `profile/sign-31/TSUOV_128__verify.json` |
| `TSUOV_256` | KAT log (sha256 `d282e795801b7da9…`) | `kat/sign-31/TSUOV_256.log` |
| `TSUOV_256` | timing keygen | `records/sign-31/TSUOV_256__keygen.json` |
| `TSUOV_256` | timing sign | `records/sign-31/TSUOV_256__sign.json` |
| `TSUOV_256` | timing verify | `records/sign-31/TSUOV_256__verify.json` |
| `TSUOV_256` | hash profile keygen | `profile/sign-31/TSUOV_256__keygen.json` |
| `TSUOV_256` | hash profile sign | `profile/sign-31/TSUOV_256__sign.json` |
| `TSUOV_256` | hash profile verify | `profile/sign-31/TSUOV_256__verify.json` |
| `TSUOV_512` | KAT log (sha256 `78508e695ef81c4a…`) | `kat/sign-31/TSUOV_512.log` |
| `TSUOV_512` | timing keygen | `records/sign-31/TSUOV_512__keygen.json` |
| `TSUOV_512` | timing sign | `records/sign-31/TSUOV_512__sign.json` |
| `TSUOV_512` | timing verify | `records/sign-31/TSUOV_512__verify.json` |
| `TSUOV_512` | hash profile keygen | `profile/sign-31/TSUOV_512__keygen.json` |
| `TSUOV_512` | hash profile sign | `profile/sign-31/TSUOV_512__sign.json` |
| `TSUOV_512` | hash profile verify | `profile/sign-31/TSUOV_512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

