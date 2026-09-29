# sign-30 TRINE — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-30` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561105463201792.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: TRINE
- Implementation versions measured: reference
- Parameter sets: `TRINE-128-Balanced`, `TRINE-128-ShortSig`, `TRINE-256-Balanced`, `TRINE-256-ShortSig`, `TRINE-512-Balanced`, `TRINE-512-ShortSig`

## 2. Assessment environment

| item | value |
|---|---|
| processor | Qualcomm Oryon (CPU 2, one core) |
| machine | ASUS Vivobook S 15 |
| clock | fixed 2.71 GHz (governor performance, minimum = maximum), boost off, SMT none |
| memory | 30562 MiB |
| OS / kernel | Ubuntu 26.04.1 LTS / 7.0.0-34-generic |
| compiler / build tool | gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0 / cmake version 4.2.3 |
| campaign start / end (UTC) | 2026-09-28T15:05:46 / 2026-09-29T08:43:52 |

## 3. Functional testing (KAT)

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-30/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `TRINE-128-Balanced` | guide | PASS |
| `TRINE-128-ShortSig` | guide | PASS |
| `TRINE-256-Balanced` | guide | PASS |
| `TRINE-256-ShortSig` | guide | PASS |
| `TRINE-512-Balanced` | harness-default | TIMEOUT [1] |
| `TRINE-512-ShortSig` | guide | PASS |

[1] KAT TIMEOUT only: the ICCS build buffers the whole signing transcript and hashes it at the end (hundreds of MB per signature at level 512, sign-30/pseudocode.md), so even the reduced KAT runs exceed their time limits on a loaded machine; the other three TRINE instances pass. One-record checks with a 4-hour limit reproduced the first submitted KAT record of all three (PASS; logs in performance/data/x86_1/katcheck/). On arm_1 only TRINE-512-Balanced exceeds its limit; the same one-record check passes there too (performance/data/arm_1/katcheck/). These instances are timed anyway; their output is not validated.

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `TRINE-128-Balanced` | keygen | 8.49 M | 3.15 ms | 318 | 3.15 ms | 965 (5 × 193) |
| `TRINE-128-Balanced` | sign | 31.87 G | 11.8 s | 0.0846 | 11.8 s | 30 (5 × 6) |
| `TRINE-128-Balanced` | verify | 20.38 G | 7.56 s | 0.132 | 7.56 s | 50 (5 × 10) |
| `TRINE-128-ShortSig` | keygen | 16.64 M | 6.17 ms | 162 | 6.15 ms | 495 (5 × 99) |
| `TRINE-128-ShortSig` | sign | 14.43 G | 5.36 s | 0.187 | 5.33 s | 60 (5 × 12) |
| `TRINE-128-ShortSig` | verify | 7.05 G | 2.62 s | 0.382 | 2.61 s | 100 (5 × 20) |
| `TRINE-256-Balanced` | keygen | 45.79 M | 17 ms | 58.8 | 17 ms | 185 (5 × 37) |
| `TRINE-256-Balanced` | sign | 330.30 G | 123 s | 0.00816 | 123 s | 2 (2 × 1) |
| `TRINE-256-Balanced` | verify | 213.24 G | 79.1 s | 0.0126 | 79.1 s | 3 (3 × 1) |
| `TRINE-256-ShortSig` | keygen | 111.66 M | 41.4 ms | 24.1 | 41.4 ms | 100 (5 × 20) |
| `TRINE-256-ShortSig` | sign | 134.44 G | 49.9 s | 0.02 | 49.9 s | 5 (5 × 1) |
| `TRINE-256-ShortSig` | verify | 50.92 G | 18.9 s | 0.0529 | 18.9 s | 15 (5 × 3) |
| `TRINE-512-Balanced` | keygen | 862.57 M | 320 ms | 3.12 | 321 ms | 100 (5 × 20) |
| `TRINE-512-Balanced` | sign | 6234.95 G | 2.31e+03 s | 0.000432 | 2.31e+03 s | 1 (1 × 1) |
| `TRINE-512-Balanced` | verify | 4120.90 G | 1.53e+03 s | 0.000654 | 1.53e+03 s | 1 (1 × 1) |
| `TRINE-512-ShortSig` | keygen | 1.85 G | 688 ms | 1.45 | 687 ms | 100 (5 × 20) |
| `TRINE-512-ShortSig` | sign | 2508.52 G | 931 s | 0.00107 | 931 s | 1 (1 × 1) |
| `TRINE-512-ShortSig` | verify | 1150.27 G | 427 s | 0.00234 | 427 s | 1 (1 × 1) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `TRINE-128-Balanced` | keygen | 49048 | 1456 KiB | 3632 KiB |
| `TRINE-128-Balanced` | sign | 49048 | 1584 KiB | 9580 KiB |
| `TRINE-128-Balanced` | verify | 49048 | 3060 KiB | 8816 KiB |
| `TRINE-128-ShortSig` | keygen | 53848 | 3480 KiB | 3548 KiB |
| `TRINE-128-ShortSig` | sign | 53848 | 1740 KiB | 7884 KiB |
| `TRINE-128-ShortSig` | verify | 53848 | 3384 KiB | 6480 KiB |
| `TRINE-256-Balanced` | keygen | 49488 | 1548 KiB | 4152 KiB |
| `TRINE-256-Balanced` | sign | 49488 | 2160 KiB | 88408 KiB |
| `TRINE-256-Balanced` | verify | 49488 | 28244 KiB | 89068 KiB |
| `TRINE-256-ShortSig` | keygen | 50168 | 1824 KiB | 4892 KiB |
| `TRINE-256-ShortSig` | sign | 50168 | 4412 KiB | 42856 KiB |
| `TRINE-256-ShortSig` | verify | 50168 | 15496 KiB | 43224 KiB |
| `TRINE-512-Balanced` | keygen | 49672 | 2264 KiB | 11188 KiB |
| `TRINE-512-Balanced` | sign | 49672 | 2476 KiB | 1372492 KiB |
| `TRINE-512-Balanced` | verify | 49672 | 16944 KiB | 1374600 KiB |
| `TRINE-512-ShortSig` | keygen | 50720 | 1468 KiB | 18428 KiB |
| `TRINE-512-ShortSig` | sign | 50720 | 6212 KiB | 633708 KiB |
| `TRINE-512-ShortSig` | verify | 50720 | 11724 KiB | 639188 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | signature |
|---|---|---|---|
| `TRINE-128-Balanced` | 16004 | 32 | 3156 |
| `TRINE-128-ShortSig` | 63920 | 32 | 1652 |
| `TRINE-256-Balanced` | 96064 | 64 | 11712 |
| `TRINE-256-ShortSig` | 384064 | 64 | 5968 |
| `TRINE-512-Balanced` | 797290 | 128 | 46565 |
| `TRINE-512-ShortSig` | 3188776 | 128 | 23672 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — with -DUSE_ICCS (SHA3 build selectable)

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `TRINE-128-Balanced` | keygen | 68% | 0.1% | drng 1, pseudoXOF 29 |
| `TRINE-128-Balanced` | sign | 19% | 0.0% | drng 2, pseudoXOF 1.7e+03, sm3hash 1 |
| `TRINE-128-Balanced` | verify | 19% | 0.0% | pseudoXOF 1.06e+03, sm3hash 1 |
| `TRINE-128-ShortSig` | keygen | 40% | 0.0% | drng 1, pseudoXOF 83 |
| `TRINE-128-ShortSig` | sign | 20% | 0.0% | drng 2, pseudoXOF 772, sm3hash 1 |
| `TRINE-128-ShortSig` | verify | 19% | 0.0% | pseudoXOF 326, sm3hash 1 |
| `TRINE-256-Balanced` | keygen | 53% | 0.0% | drng 1, pseudoXOF 37 |
| `TRINE-256-Balanced` | sign | 10% | 0.0% | drng 2, pseudoXOF 3.54e+03, pseudohash 1 |
| `TRINE-256-Balanced` | verify | 10% | 0.0% | pseudoXOF 2.19e+03, pseudohash 1 |
| `TRINE-256-ShortSig` | keygen | 27% | 0.0% | drng 1, pseudoXOF 109 |
| `TRINE-256-ShortSig` | sign | 10% | 0.0% | drng 2, pseudoXOF 1.58e+03, pseudohash 1 |
| `TRINE-256-ShortSig` | verify | 10% | 0.0% | pseudoXOF 533, pseudohash 1 |
| `TRINE-512-Balanced` | keygen | 61% | 0.0% | drng 1, pseudoXOF 47 |
| `TRINE-512-Balanced` | sign | 3.0% | 0.0% | drng 2, pseudoXOF 7.56e+03, pseudohash 1 |
| `TRINE-512-Balanced` | verify | 3.0% | 0.0% | pseudoXOF 4.6e+03, pseudohash 1 |
| `TRINE-512-ShortSig` | keygen | 30% | 0.0% | drng 1, pseudoXOF 137 |
| `TRINE-512-ShortSig` | sign | 3.1% | 0.0% | drng 2, pseudoXOF 3.31e+03, pseudohash 1 |
| `TRINE-512-ShortSig` | verify | 3.1% | 0.0% | pseudoXOF 1.21e+03, pseudohash 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `TRINE-128-Balanced` | KAT log (sha256 `51c2205aeca42e5f…`) | `kat/sign-30/TRINE-128-Balanced.log` |
| `TRINE-128-Balanced` | timing keygen | `records/sign-30/TRINE-128-Balanced__keygen.json` |
| `TRINE-128-Balanced` | timing sign | `records/sign-30/TRINE-128-Balanced__sign.json` |
| `TRINE-128-Balanced` | timing verify | `records/sign-30/TRINE-128-Balanced__verify.json` |
| `TRINE-128-Balanced` | hash profile keygen | `profile/sign-30/TRINE-128-Balanced__keygen.json` |
| `TRINE-128-Balanced` | hash profile sign | `profile/sign-30/TRINE-128-Balanced__sign.json` |
| `TRINE-128-Balanced` | hash profile verify | `profile/sign-30/TRINE-128-Balanced__verify.json` |
| `TRINE-128-ShortSig` | KAT log (sha256 `90c834996d3377b2…`) | `kat/sign-30/TRINE-128-ShortSig.log` |
| `TRINE-128-ShortSig` | timing keygen | `records/sign-30/TRINE-128-ShortSig__keygen.json` |
| `TRINE-128-ShortSig` | timing sign | `records/sign-30/TRINE-128-ShortSig__sign.json` |
| `TRINE-128-ShortSig` | timing verify | `records/sign-30/TRINE-128-ShortSig__verify.json` |
| `TRINE-128-ShortSig` | hash profile keygen | `profile/sign-30/TRINE-128-ShortSig__keygen.json` |
| `TRINE-128-ShortSig` | hash profile sign | `profile/sign-30/TRINE-128-ShortSig__sign.json` |
| `TRINE-128-ShortSig` | hash profile verify | `profile/sign-30/TRINE-128-ShortSig__verify.json` |
| `TRINE-256-Balanced` | KAT log (sha256 `109e6e055d9448b2…`) | `kat/sign-30/TRINE-256-Balanced.log` |
| `TRINE-256-Balanced` | timing keygen | `records/sign-30/TRINE-256-Balanced__keygen.json` |
| `TRINE-256-Balanced` | timing sign | `records/sign-30/TRINE-256-Balanced__sign.json` |
| `TRINE-256-Balanced` | timing verify | `records/sign-30/TRINE-256-Balanced__verify.json` |
| `TRINE-256-Balanced` | hash profile keygen | `profile/sign-30/TRINE-256-Balanced__keygen.json` |
| `TRINE-256-Balanced` | hash profile sign | `profile/sign-30/TRINE-256-Balanced__sign.json` |
| `TRINE-256-Balanced` | hash profile verify | `profile/sign-30/TRINE-256-Balanced__verify.json` |
| `TRINE-256-ShortSig` | KAT log (sha256 `b2e38ee0bf2612d7…`) | `kat/sign-30/TRINE-256-ShortSig.log` |
| `TRINE-256-ShortSig` | timing keygen | `records/sign-30/TRINE-256-ShortSig__keygen.json` |
| `TRINE-256-ShortSig` | timing sign | `records/sign-30/TRINE-256-ShortSig__sign.json` |
| `TRINE-256-ShortSig` | timing verify | `records/sign-30/TRINE-256-ShortSig__verify.json` |
| `TRINE-256-ShortSig` | hash profile keygen | `profile/sign-30/TRINE-256-ShortSig__keygen.json` |
| `TRINE-256-ShortSig` | hash profile sign | `profile/sign-30/TRINE-256-ShortSig__sign.json` |
| `TRINE-256-ShortSig` | hash profile verify | `profile/sign-30/TRINE-256-ShortSig__verify.json` |
| `TRINE-512-Balanced` | KAT log (sha256 `3c856110aefec347…`) | `kat/sign-30/TRINE-512-Balanced.log` |
| `TRINE-512-Balanced` | timing keygen | `records/sign-30/TRINE-512-Balanced__keygen.json` |
| `TRINE-512-Balanced` | timing sign | `records/sign-30/TRINE-512-Balanced__sign.json` |
| `TRINE-512-Balanced` | timing verify | `records/sign-30/TRINE-512-Balanced__verify.json` |
| `TRINE-512-Balanced` | hash profile keygen | `profile/sign-30/TRINE-512-Balanced__keygen.json` |
| `TRINE-512-Balanced` | hash profile sign | `profile/sign-30/TRINE-512-Balanced__sign.json` |
| `TRINE-512-Balanced` | hash profile verify | `profile/sign-30/TRINE-512-Balanced__verify.json` |
| `TRINE-512-ShortSig` | KAT log (sha256 `017d43d0061955ce…`) | `kat/sign-30/TRINE-512-ShortSig.log` |
| `TRINE-512-ShortSig` | timing keygen | `records/sign-30/TRINE-512-ShortSig__keygen.json` |
| `TRINE-512-ShortSig` | timing sign | `records/sign-30/TRINE-512-ShortSig__sign.json` |
| `TRINE-512-ShortSig` | timing verify | `records/sign-30/TRINE-512-ShortSig__verify.json` |
| `TRINE-512-ShortSig` | hash profile keygen | `profile/sign-30/TRINE-512-ShortSig__keygen.json` |
| `TRINE-512-ShortSig` | hash profile sign | `profile/sign-30/TRINE-512-ShortSig__sign.json` |
| `TRINE-512-ShortSig` | hash profile verify | `profile/sign-30/TRINE-512-ShortSig__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

