# sign-27 SQIsignTriangle — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-27` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561096613220352.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: SQIsignTriangle
- Implementation versions measured: reference
- Parameter sets: `SQIsignTriangle_lvl1`, `SQIsignTriangle_lvl2`, `SQIsignTriangle_lvl5`, `SQIsignTriangle_lvl6`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-27/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `SQIsignTriangle_lvl1` | guide | PASS |
| `SQIsignTriangle_lvl2` | guide | PASS |
| `SQIsignTriangle_lvl5` | guide | PASS |
| `SQIsignTriangle_lvl6` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `SQIsignTriangle_lvl1` | keygen | 108.98 M | 52.1 ms | 19.2 | 52.1 ms | 100 (5 × 20) |
| `SQIsignTriangle_lvl1` | sign | 203.28 M | 97.1 ms | 10.3 | 97.1 ms | 100 (5 × 20) |
| `SQIsignTriangle_lvl1` | verify | 19.98 M | 9.54 ms | 105 | 9.49 ms | 525 (5 × 105) |
| `SQIsignTriangle_lvl2` | keygen | 164.50 M | 78.6 ms | 12.7 | 78.6 ms | 100 (5 × 20) |
| `SQIsignTriangle_lvl2` | sign | 373.20 M | 178 ms | 5.61 | 178 ms | 100 (5 × 20) |
| `SQIsignTriangle_lvl2` | verify | 32.79 M | 15.7 ms | 63.9 | 15.7 ms | 320 (5 × 64) |
| `SQIsignTriangle_lvl5` | keygen | 533.79 M | 255 ms | 3.92 | 255 ms | 100 (5 × 20) |
| `SQIsignTriangle_lvl5` | sign | 1.06 G | 508 ms | 1.97 | 506 ms | 100 (5 × 20) |
| `SQIsignTriangle_lvl5` | verify | 155.70 M | 74.4 ms | 13.4 | 74.4 ms | 100 (5 × 20) |
| `SQIsignTriangle_lvl6` | keygen | 4.16 G | 2 s | 0.501 | 2 s | 100 (5 × 20) |
| `SQIsignTriangle_lvl6` | sign | 10.05 G | 4.83 s | 0.207 | 4.83 s | 100 (5 × 20) |
| `SQIsignTriangle_lvl6` | verify | 3.31 G | 1.59 s | 0.63 | 1.59 s | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `SQIsignTriangle_lvl1` | keygen | 275681 | 2208 KiB | 3028 KiB |
| `SQIsignTriangle_lvl1` | sign | 275681 | 2900 KiB | 3160 KiB |
| `SQIsignTriangle_lvl1` | verify | 275681 | 3008 KiB | 3072 KiB |
| `SQIsignTriangle_lvl2` | keygen | 267729 | 2232 KiB | 3084 KiB |
| `SQIsignTriangle_lvl2` | sign | 267729 | 2900 KiB | 3196 KiB |
| `SQIsignTriangle_lvl2` | verify | 267729 | 3012 KiB | 3080 KiB |
| `SQIsignTriangle_lvl5` | keygen | 268609 | 2200 KiB | 3180 KiB |
| `SQIsignTriangle_lvl5` | sign | 268609 | 2964 KiB | 3280 KiB |
| `SQIsignTriangle_lvl5` | verify | 268609 | 3140 KiB | 3208 KiB |
| `SQIsignTriangle_lvl6` | keygen | 284229 | 2240 KiB | 3372 KiB |
| `SQIsignTriangle_lvl6` | sign | 284229 | 3172 KiB | 3492 KiB |
| `SQIsignTriangle_lvl6` | verify | 284229 | 3364 KiB | 3452 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `SQIsignTriangle_lvl1` | 65 | 353 | 204 |
| `SQIsignTriangle_lvl2` | 81 | 437 | 255 |
| `SQIsignTriangle_lvl5` | 129 | 701 | 408 |
| `SQIsignTriangle_lvl6` | 257 | 1409 | 816 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — `shake*` names are shims over pseudoXOF

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `SQIsignTriangle_lvl1` | keygen | 0.0% | 7.2% | drng 1.76e+03 |
| `SQIsignTriangle_lvl1` | sign | 1.6% | 7.9% | drng 3.5e+03, pseudoXOF 37.4 |
| `SQIsignTriangle_lvl1` | verify | 3.4% | 0.0% | pseudoXOF 21 |
| `SQIsignTriangle_lvl2` | keygen | 0.0% | 4.4% | drng 1.54e+03 |
| `SQIsignTriangle_lvl2` | sign | 3.4% | 4.9% | drng 3.63e+03, pseudoXOF 50.9 |
| `SQIsignTriangle_lvl2` | verify | 0.4% | 0.0% | pseudoXOF 7 |
| `SQIsignTriangle_lvl5` | keygen | 0.0% | 3.5% | drng 3.79e+03 |
| `SQIsignTriangle_lvl5` | sign | 4.7% | 2.9% | drng 7.84e+03, pseudoXOF 79.5 |
| `SQIsignTriangle_lvl5` | verify | 39% | 0.0% | pseudoXOF 121 |
| `SQIsignTriangle_lvl6` | keygen | 0.0% | 0.6% | drng 4.72e+03 |
| `SQIsignTriangle_lvl6` | sign | 1.7% | 0.6% | drng 1.09e+04, pseudoXOF 106 |
| `SQIsignTriangle_lvl6` | verify | 83% | 0.0% | pseudoXOF 506 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `SQIsignTriangle_lvl1` | KAT log (sha256 `ce1a0bde8562c494…`) | `kat/sign-27/SQIsignTriangle_lvl1.log` |
| `SQIsignTriangle_lvl1` | timing keygen | `records/sign-27/SQIsignTriangle_lvl1__keygen.json` |
| `SQIsignTriangle_lvl1` | timing sign | `records/sign-27/SQIsignTriangle_lvl1__sign.json` |
| `SQIsignTriangle_lvl1` | timing verify | `records/sign-27/SQIsignTriangle_lvl1__verify.json` |
| `SQIsignTriangle_lvl1` | hash profile keygen | `profile/sign-27/SQIsignTriangle_lvl1__keygen.json` |
| `SQIsignTriangle_lvl1` | hash profile sign | `profile/sign-27/SQIsignTriangle_lvl1__sign.json` |
| `SQIsignTriangle_lvl1` | hash profile verify | `profile/sign-27/SQIsignTriangle_lvl1__verify.json` |
| `SQIsignTriangle_lvl2` | KAT log (sha256 `a6394a9518e2a6d2…`) | `kat/sign-27/SQIsignTriangle_lvl2.log` |
| `SQIsignTriangle_lvl2` | timing keygen | `records/sign-27/SQIsignTriangle_lvl2__keygen.json` |
| `SQIsignTriangle_lvl2` | timing sign | `records/sign-27/SQIsignTriangle_lvl2__sign.json` |
| `SQIsignTriangle_lvl2` | timing verify | `records/sign-27/SQIsignTriangle_lvl2__verify.json` |
| `SQIsignTriangle_lvl2` | hash profile keygen | `profile/sign-27/SQIsignTriangle_lvl2__keygen.json` |
| `SQIsignTriangle_lvl2` | hash profile sign | `profile/sign-27/SQIsignTriangle_lvl2__sign.json` |
| `SQIsignTriangle_lvl2` | hash profile verify | `profile/sign-27/SQIsignTriangle_lvl2__verify.json` |
| `SQIsignTriangle_lvl5` | KAT log (sha256 `212314496d10cd49…`) | `kat/sign-27/SQIsignTriangle_lvl5.log` |
| `SQIsignTriangle_lvl5` | timing keygen | `records/sign-27/SQIsignTriangle_lvl5__keygen.json` |
| `SQIsignTriangle_lvl5` | timing sign | `records/sign-27/SQIsignTriangle_lvl5__sign.json` |
| `SQIsignTriangle_lvl5` | timing verify | `records/sign-27/SQIsignTriangle_lvl5__verify.json` |
| `SQIsignTriangle_lvl5` | hash profile keygen | `profile/sign-27/SQIsignTriangle_lvl5__keygen.json` |
| `SQIsignTriangle_lvl5` | hash profile sign | `profile/sign-27/SQIsignTriangle_lvl5__sign.json` |
| `SQIsignTriangle_lvl5` | hash profile verify | `profile/sign-27/SQIsignTriangle_lvl5__verify.json` |
| `SQIsignTriangle_lvl6` | KAT log (sha256 `c9a77f77a6b42ff4…`) | `kat/sign-27/SQIsignTriangle_lvl6.log` |
| `SQIsignTriangle_lvl6` | timing keygen | `records/sign-27/SQIsignTriangle_lvl6__keygen.json` |
| `SQIsignTriangle_lvl6` | timing sign | `records/sign-27/SQIsignTriangle_lvl6__sign.json` |
| `SQIsignTriangle_lvl6` | timing verify | `records/sign-27/SQIsignTriangle_lvl6__verify.json` |
| `SQIsignTriangle_lvl6` | hash profile keygen | `profile/sign-27/SQIsignTriangle_lvl6__keygen.json` |
| `SQIsignTriangle_lvl6` | hash profile sign | `profile/sign-27/SQIsignTriangle_lvl6__sign.json` |
| `SQIsignTriangle_lvl6` | hash profile verify | `profile/sign-27/SQIsignTriangle_lvl6__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

