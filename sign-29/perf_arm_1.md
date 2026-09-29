# sign-29 Tins — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-29` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561105320595456.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: Tins
- Implementation versions measured: reference
- Parameter sets: `Tins128`, `Tins256`, `Tins512`

## 2. Assessment environment

| item | value |
|---|---|
| processor | Qualcomm Oryon (CPU 2, one core) |
| machine | ASUS Vivobook S 15 |
| clock | fixed 2.71 GHz (governor performance, minimum = maximum), boost off, SMT none |
| memory | 30562 MiB |
| OS / kernel | Ubuntu 26.04.1 LTS / 7.0.0-34-generic |
| compiler / build tool | gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0 / cmake version 4.2.3 |
| campaign start / end (UTC) | 2026-09-28T15:05:46 / 2026-09-29T02:47:54 |

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
| `Tins128` | keygen | 1.45 M | 539 µs | 1.86e+03 | 537 µs | 5575 (5 × 1115) |
| `Tins128` | sign | 1.18 G | 440 ms | 2.27 | 440 ms | 100 (5 × 20) |
| `Tins128` | verify | 1.09 G | 406 ms | 2.46 | 406 ms | 100 (5 × 20) |
| `Tins256` | keygen | 8.17 M | 3.03 ms | 330 | 3.03 ms | 1030 (5 × 206) |
| `Tins256` | sign | 4.26 G | 1.58 s | 0.632 | 1.58 s | 100 (5 × 20) |
| `Tins256` | verify | 4.19 G | 1.55 s | 0.644 | 1.55 s | 100 (5 × 20) |
| `Tins512` | keygen | 52.77 M | 19.6 ms | 51.1 | 19.6 ms | 165 (5 × 33) |
| `Tins512` | sign | 18.40 G | 6.83 s | 0.146 | 6.81 s | 55 (5 × 11) |
| `Tins512` | verify | 18.24 G | 6.78 s | 0.148 | 6.77 s | 55 (5 × 11) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Tins128` | keygen | 41136 | 1436 KiB | 1520 KiB |
| `Tins128` | sign | 41136 | 1456 KiB | 16436 KiB |
| `Tins128` | verify | 41136 | 4520 KiB | 14588 KiB |
| `Tins256` | keygen | 41576 | 1448 KiB | 1592 KiB |
| `Tins256` | sign | 41576 | 3568 KiB | 46984 KiB |
| `Tins256` | verify | 41576 | 15872 KiB | 46724 KiB |
| `Tins512` | keygen | 43668 | 1488 KiB | 1864 KiB |
| `Tins512` | sign | 43668 | 1800 KiB | 177820 KiB |
| `Tins512` | verify | 43668 | 28020 KiB | 138216 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | signature |
|---|---|---|---|
| `Tins128` | 51 | 32 | 3284 |
| `Tins256` | 98 | 64 | 13012 |
| `Tins512` | 195 | 128 | 51187 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Tins128` | keygen | 0.0% | 30% | drng 6 |
| `Tins128` | sign | 21% | 25% | drng 5.25e+04, pseudoXOF 9.83e+04, sm3hash 4.92e+04 |
| `Tins128` | verify | 22% | 25% | drng 4.91e+04, pseudoXOF 9.8e+04, sm3hash 4.91e+04 |
| `Tins256` | keygen | 0.0% | 18% | drng 6 |
| `Tins256` | sign | 39% | 17% | drng 1.08e+05, pseudoXOF 1.97e+05, pseudohash 9.83e+04 |
| `Tins256` | verify | 41% | 16% | drng 9.83e+04, pseudoXOF 1.96e+05, pseudohash 9.83e+04 |
| `Tins512` | keygen | 0.0% | 11% | drng 6 |
| `Tins512` | sign | 57% | 10% | drng 1.93e+05, pseudoXOF 3.85e+05, pseudohash 1.93e+05 |
| `Tins512` | verify | 57% | 10% | drng 1.92e+05, pseudoXOF 3.84e+05, pseudohash 1.92e+05 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Tins128` | KAT log (sha256 `e93e2f9f45b071cf…`) | `kat/sign-29/Tins128.log` |
| `Tins128` | timing keygen | `records/sign-29/Tins128__keygen.json` |
| `Tins128` | timing sign | `records/sign-29/Tins128__sign.json` |
| `Tins128` | timing verify | `records/sign-29/Tins128__verify.json` |
| `Tins128` | hash profile keygen | `profile/sign-29/Tins128__keygen.json` |
| `Tins128` | hash profile sign | `profile/sign-29/Tins128__sign.json` |
| `Tins128` | hash profile verify | `profile/sign-29/Tins128__verify.json` |
| `Tins256` | KAT log (sha256 `c70f3adbbc00a648…`) | `kat/sign-29/Tins256.log` |
| `Tins256` | timing keygen | `records/sign-29/Tins256__keygen.json` |
| `Tins256` | timing sign | `records/sign-29/Tins256__sign.json` |
| `Tins256` | timing verify | `records/sign-29/Tins256__verify.json` |
| `Tins256` | hash profile keygen | `profile/sign-29/Tins256__keygen.json` |
| `Tins256` | hash profile sign | `profile/sign-29/Tins256__sign.json` |
| `Tins256` | hash profile verify | `profile/sign-29/Tins256__verify.json` |
| `Tins512` | KAT log (sha256 `ffceb53c234fb1e9…`) | `kat/sign-29/Tins512.log` |
| `Tins512` | timing keygen | `records/sign-29/Tins512__keygen.json` |
| `Tins512` | timing sign | `records/sign-29/Tins512__sign.json` |
| `Tins512` | timing verify | `records/sign-29/Tins512__verify.json` |
| `Tins512` | hash profile keygen | `profile/sign-29/Tins512__keygen.json` |
| `Tins512` | hash profile sign | `profile/sign-29/Tins512__sign.json` |
| `Tins512` | hash profile verify | `profile/sign-29/Tins512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

