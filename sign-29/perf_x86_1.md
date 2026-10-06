# sign-29 Tins — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-29` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561105320595456.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: Tins
- Implementation versions measured: reference
- Parameter sets: `Tins128`, `Tins256`, `Tins512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-29/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Tins128` | harness-default | CRYPTOFAIL [1] |
| `Tins256` | guide | PASS |
| `Tins512` | guide | PASS |

[1] CRYPTOFAIL: the Tins128 verifier overwrites the h_piop parsed from the signature with the recomputed hash and then compares it with an uninitialized stack buffer (src/Tins128/SIG_TINS128.c:414-439; Tins256/Tins512 are correct), so its verdict depends on leftover stack contents and honest signatures are rejected in this build (sign-29/pseudocode.md, discrepancy 1). Key generation and signing are timed normally; verification is timed too, but every call rejects the honest signature (counted as verify_rejections), so it measures the flawed verifier. These instances are timed anyway; their output is not validated.

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Tins128` | keygen | 1.71 M | 818 µs | 1.22e+03 | 818 µs | 5850 (5 × 1170) |
| `Tins128` | sign | 1.35 G | 643 ms | 1.55 | 643 ms | 100 (5 × 20) |
| `Tins128` | verify | 1.25 G | 596 ms | 1.68 | 596 ms | 100 (5 × 20) |
| `Tins256` | keygen | 9.67 M | 4.62 ms | 216 | 4.62 ms | 1065 (5 × 213) |
| `Tins256` | sign | 4.77 G | 2.29 s | 0.437 | 2.28 s | 100 (5 × 20) |
| `Tins256` | verify | 4.64 G | 2.23 s | 0.449 | 2.23 s | 100 (5 × 20) |
| `Tins512` | keygen | 62.90 M | 30 ms | 33.3 | 30 ms | 170 (5 × 34) |
| `Tins512` | sign | 20.33 G | 9.77 s | 0.102 | 9.77 s | 90 (5 × 18) |
| `Tins512` | verify | 20.14 G | 9.67 s | 0.103 | 9.67 s | 90 (5 × 18) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Tins128` | keygen | – | 1732 KiB | 1816 KiB |
| `Tins128` | sign | – | 1744 KiB | 15116 KiB |
| `Tins128` | verify | – | 3280 KiB | 14760 KiB |
| `Tins256` | keygen | 44889 | 1720 KiB | 1884 KiB |
| `Tins256` | sign | 44889 | 1808 KiB | 45280 KiB |
| `Tins256` | verify | 44889 | 14152 KiB | 45056 KiB |
| `Tins512` | keygen | 49081 | 1720 KiB | 2160 KiB |
| `Tins512` | sign | 49081 | 2080 KiB | 213952 KiB |
| `Tins512` | verify | 49081 | 26316 KiB | 136500 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `Tins128` | 51 | 32 | 3284 (maximum) |
| `Tins256` | 98 | 64 | 13012 (maximum) |
| `Tins512` | 195 | 128 | 51187 (maximum) |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Tins128` | keygen | 0.0% | 28% | drng 6 |
| `Tins128` | sign | 20% | 24% | drng 5.25e+04, pseudoXOF 9.83e+04, sm3hash 4.92e+04 |
| `Tins128` | verify | 20% | 24% | drng 4.91e+04, pseudoXOF 9.8e+04, sm3hash 4.91e+04 |
| `Tins256` | keygen | 0.0% | 17% | drng 6 |
| `Tins256` | sign | 37% | 16% | drng 1.08e+05, pseudoXOF 1.97e+05, pseudohash 9.83e+04 |
| `Tins256` | verify | 39% | 16% | drng 9.83e+04, pseudoXOF 1.96e+05, pseudohash 9.83e+04 |
| `Tins512` | keygen | 0.0% | 10% | drng 6 |
| `Tins512` | sign | 54% | 10% | drng 1.93e+05, pseudoXOF 3.85e+05, pseudohash 1.93e+05 |
| `Tins512` | verify | 55% | 10% | drng 1.92e+05, pseudoXOF 3.84e+05, pseudohash 1.92e+05 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Tins128` | KAT log (sha256 `5b3e2d11ab37b331…`) | `kat/sign-29/Tins128.log` |
| `Tins128` | timing keygen | `records/sign-29/Tins128__keygen.json` |
| `Tins128` | timing sign | `records/sign-29/Tins128__sign.json` |
| `Tins128` | timing verify | `records/sign-29/Tins128__verify.json` |
| `Tins128` | hash profile keygen | `profile/sign-29/Tins128__keygen.json` |
| `Tins128` | hash profile sign | `profile/sign-29/Tins128__sign.json` |
| `Tins128` | hash profile verify | `profile/sign-29/Tins128__verify.json` |
| `Tins256` | KAT log (sha256 `c1d0be4cb3c51276…`) | `kat/sign-29/Tins256.log` |
| `Tins256` | timing keygen | `records/sign-29/Tins256__keygen.json` |
| `Tins256` | timing sign | `records/sign-29/Tins256__sign.json` |
| `Tins256` | timing verify | `records/sign-29/Tins256__verify.json` |
| `Tins256` | hash profile keygen | `profile/sign-29/Tins256__keygen.json` |
| `Tins256` | hash profile sign | `profile/sign-29/Tins256__sign.json` |
| `Tins256` | hash profile verify | `profile/sign-29/Tins256__verify.json` |
| `Tins512` | KAT log (sha256 `f4dca575f7eb4faa…`) | `kat/sign-29/Tins512.log` |
| `Tins512` | timing keygen | `records/sign-29/Tins512__keygen.json` |
| `Tins512` | timing sign | `records/sign-29/Tins512__sign.json` |
| `Tins512` | timing verify | `records/sign-29/Tins512__verify.json` |
| `Tins512` | hash profile keygen | `profile/sign-29/Tins512__keygen.json` |
| `Tins512` | hash profile sign | `profile/sign-29/Tins512__sign.json` |
| `Tins512` | hash profile verify | `profile/sign-29/Tins512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

