# sign-18 Origami — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-18` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561086978904064.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: Origami
- Implementation versions measured: reference
- Parameter sets: `Origami-128`, `Origami-256`, `Origami-384`, `Origami-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-18/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Origami-128` | guide | PASS [1] |
| `Origami-256` | guide | PASS [1] |
| `Origami-384` | guide | PASS [1] |
| `Origami-512` | guide | PASS [1] |

[1] Not a KAT problem (KATs pass): sig_sign fails with return code -4 for about 8% of (message, salt) pairs — for some targets every one of the MAX_SIGN_ATTEMPTS = 8192 zone solves in origami_ref.c fails — and the API does not retry. The benchmark retries signing with a fresh salt, as an application would; retries are included in the signing time and counted in each record (sign_failures_retried). These instances are timed anyway; their output is not validated.

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Origami-128` | keygen | 2.36 M | 877 µs | 1.14e+03 | 877 µs | 3525 (5 × 705) |
| `Origami-128` | sign | 816.33 M | 303 ms | 3.3 | 300 ms | 80 (5 × 16); 10 failed signing attempts retried |
| `Origami-128` | verify | 23.04 M | 8.55 ms | 117 | 8.54 ms | 370 (5 × 74); 5 failed signing attempts retried |
| `Origami-256` | keygen | 85.41 M | 31.7 ms | 31.6 | 31.7 ms | 100 (5 × 20) |
| `Origami-256` | sign | 1.38 G | 513 ms | 1.95 | 513 ms | 100 (5 × 20) |
| `Origami-256` | verify | 1.25 G | 464 ms | 2.15 | 461 ms | 100 (5 × 20) |
| `Origami-384` | keygen | 291.60 M | 108 ms | 9.24 | 107 ms | 100 (5 × 20) |
| `Origami-384` | sign | 12.12 G | 4.5 s | 0.222 | 4.5 s | 100 (5 × 20); 10 failed signing attempts retried |
| `Origami-384` | verify | 5.82 G | 2.16 s | 0.463 | 2.13 s | 100 (5 × 20) |
| `Origami-512` | keygen | 469.27 M | 174 ms | 5.74 | 174 ms | 100 (5 × 20) |
| `Origami-512` | sign | 11.43 G | 4.24 s | 0.236 | 4.24 s | 90 (5 × 18) |
| `Origami-512` | verify | 10.54 G | 3.91 s | 0.256 | 3.89 s | 95 (5 × 19) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Origami-128` | keygen | 37016 | 1432 KiB | 1516 KiB |
| `Origami-128` | sign | 37016 | 3456 KiB | 3532 KiB |
| `Origami-128` | verify | 37016 | 1456 KiB | 1524 KiB |
| `Origami-256` | keygen | 37040 | 1444 KiB | 1616 KiB |
| `Origami-256` | sign | 37040 | 1548 KiB | 1644 KiB |
| `Origami-256` | verify | 37040 | 1576 KiB | 1640 KiB |
| `Origami-384` | keygen | 37176 | 1456 KiB | 1700 KiB |
| `Origami-384` | sign | 37176 | 3528 KiB | 3640 KiB |
| `Origami-384` | verify | 37176 | 1680 KiB | 1744 KiB |
| `Origami-512` | keygen | 37400 | 1464 KiB | 1752 KiB |
| `Origami-512` | sign | 37400 | 3616 KiB | 3740 KiB |
| `Origami-512` | verify | 37400 | 3668 KiB | 3732 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | signature |
|---|---|---|---|
| `Origami-128` | 2996 | 16 | 116 |
| `Origami-256` | 14968 | 32 | 516 |
| `Origami-384` | 27940 | 48 | 948 |
| `Origami-512` | 35924 | 64 | 1220 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — `shake*` names are shims over pseudoXOF

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Origami-128` | keygen | 97% | 0.2% | drng 1, pseudoXOF 349 |
| `Origami-128` | sign | 86% | 0.0% | drng 1, pseudoXOF 452, pseudohash 1 |
| `Origami-128` | verify | 88% | 0.0% | pseudoXOF 304, pseudohash 1 |
| `Origami-256` | keygen | 99% | 0.0% | drng 1, pseudoXOF 1.67e+03 |
| `Origami-256` | sign | 85% | 0.0% | drng 1, pseudoXOF 4.78e+03, pseudohash 1 |
| `Origami-256` | verify | 87% | 0.0% | pseudoXOF 3.86e+03, pseudohash 1 |
| `Origami-384` | keygen | 100% | 0.0% | drng 1, pseudoXOF 3.11e+03 |
| `Origami-384` | sign | 85% | 0.0% | drng 1.33, pseudoXOF 6.69e+04, pseudohash 1.33 |
| `Origami-384` | verify | 86% | 0.0% | pseudoXOF 1.56e+04, pseudohash 1 |
| `Origami-512` | keygen | 100% | 0.0% | drng 1, pseudoXOF 4e+03 |
| `Origami-512` | sign | 84% | 0.0% | drng 1, pseudoXOF 2.94e+04, pseudohash 1 |
| `Origami-512` | verify | 86% | 0.0% | pseudoXOF 2.77e+04, pseudohash 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Origami-128` | KAT log (sha256 `6ba57e0847541400…`) | `kat/sign-18/Origami-128.log` |
| `Origami-128` | timing keygen | `records/sign-18/Origami-128__keygen.json` |
| `Origami-128` | timing sign | `records/sign-18/Origami-128__sign.json` |
| `Origami-128` | timing verify | `records/sign-18/Origami-128__verify.json` |
| `Origami-128` | hash profile keygen | `profile/sign-18/Origami-128__keygen.json` |
| `Origami-128` | hash profile sign | `profile/sign-18/Origami-128__sign.json` |
| `Origami-128` | hash profile verify | `profile/sign-18/Origami-128__verify.json` |
| `Origami-256` | KAT log (sha256 `d700ce0ab237b67c…`) | `kat/sign-18/Origami-256.log` |
| `Origami-256` | timing keygen | `records/sign-18/Origami-256__keygen.json` |
| `Origami-256` | timing sign | `records/sign-18/Origami-256__sign.json` |
| `Origami-256` | timing verify | `records/sign-18/Origami-256__verify.json` |
| `Origami-256` | hash profile keygen | `profile/sign-18/Origami-256__keygen.json` |
| `Origami-256` | hash profile sign | `profile/sign-18/Origami-256__sign.json` |
| `Origami-256` | hash profile verify | `profile/sign-18/Origami-256__verify.json` |
| `Origami-384` | KAT log (sha256 `8a2fdf6828cf60d5…`) | `kat/sign-18/Origami-384.log` |
| `Origami-384` | timing keygen | `records/sign-18/Origami-384__keygen.json` |
| `Origami-384` | timing sign | `records/sign-18/Origami-384__sign.json` |
| `Origami-384` | timing verify | `records/sign-18/Origami-384__verify.json` |
| `Origami-384` | hash profile keygen | `profile/sign-18/Origami-384__keygen.json` |
| `Origami-384` | hash profile sign | `profile/sign-18/Origami-384__sign.json` |
| `Origami-384` | hash profile verify | `profile/sign-18/Origami-384__verify.json` |
| `Origami-512` | KAT log (sha256 `8b741e0ce4742c54…`) | `kat/sign-18/Origami-512.log` |
| `Origami-512` | timing keygen | `records/sign-18/Origami-512__keygen.json` |
| `Origami-512` | timing sign | `records/sign-18/Origami-512__sign.json` |
| `Origami-512` | timing verify | `records/sign-18/Origami-512__verify.json` |
| `Origami-512` | hash profile keygen | `profile/sign-18/Origami-512__keygen.json` |
| `Origami-512` | hash profile sign | `profile/sign-18/Origami-512__sign.json` |
| `Origami-512` | hash profile verify | `profile/sign-18/Origami-512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

