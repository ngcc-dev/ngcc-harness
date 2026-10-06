# sign-30 TRINE — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-30` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561105463201792.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: TRINE
- Implementation versions measured: reference
- Parameter sets: `TRINE-128-Balanced`, `TRINE-128-ShortSig`, `TRINE-256-Balanced`, `TRINE-256-ShortSig`, `TRINE-512-Balanced`, `TRINE-512-ShortSig`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-30/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `TRINE-128-Balanced` | guide | PASS |
| `TRINE-128-ShortSig` | guide | PASS |
| `TRINE-256-Balanced` | harness-default | TIMEOUT [1] |
| `TRINE-256-ShortSig` | guide | PASS |
| `TRINE-512-Balanced` | harness-default | TIMEOUT [1] |
| `TRINE-512-ShortSig` | harness-default | TIMEOUT [1] |

[1] KAT TIMEOUT only: the ICCS build buffers the whole signing transcript and hashes it at the end (hundreds of MB per signature at level 512, sign-30/pseudocode.md), so even the reduced KAT runs exceed their time limits on a loaded machine; the other three TRINE instances pass. One-record checks with a 4-hour limit reproduced the first submitted KAT record of all three (PASS; logs in performance/data/x86_1/katcheck/). On arm_1 only TRINE-512-Balanced exceeds its limit; the same one-record check passes there too (performance/data/arm_1/katcheck/). These instances are timed anyway; their output is not validated.

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `TRINE-128-Balanced` | keygen | 9.54 M | 4.56 ms | 219 | 4.56 ms | 1070 (5 × 214) |
| `TRINE-128-Balanced` | sign | 39.46 G | 18.9 s | 0.0528 | 18.9 s | 45 (5 × 9) |
| `TRINE-128-Balanced` | verify | 25.46 G | 12.2 s | 0.0819 | 12.2 s | 70 (5 × 14) |
| `TRINE-128-ShortSig` | keygen | 19.87 M | 9.49 ms | 105 | 9.49 ms | 520 (5 × 104) |
| `TRINE-128-ShortSig` | sign | 18.23 G | 8.74 s | 0.114 | 8.74 s | 95 (5 × 19) |
| `TRINE-128-ShortSig` | verify | 8.77 G | 4.21 s | 0.238 | 4.21 s | 100 (5 × 20) |
| `TRINE-256-Balanced` | keygen | 56.62 M | 27.1 ms | 36.9 | 27.1 ms | 185 (5 × 37) |
| `TRINE-256-Balanced` | sign | 534.99 G | 256 s | 0.00391 | 247 s | 3 (3 × 1) |
| `TRINE-256-Balanced` | verify | 329.94 G | 158 s | 0.00635 | 158 s | 5 (5 × 1) |
| `TRINE-256-ShortSig` | keygen | 125.49 M | 60.4 ms | 16.5 | 60.4 ms | 100 (5 × 20) |
| `TRINE-256-ShortSig` | sign | 158.21 G | 75.9 s | 0.0132 | 75.9 s | 10 (5 × 2) |
| `TRINE-256-ShortSig` | verify | 58.42 G | 28 s | 0.0357 | 28 s | 30 (5 × 6) |
| `TRINE-512-Balanced` | keygen | 991.87 M | 475 ms | 2.1 | 475 ms | 100 (5 × 20) |
| `TRINE-512-Balanced` | sign | 8385.08 G | 4e+03 s | 0.00025 | 4e+03 s | 1 (1 × 1) |
| `TRINE-512-Balanced` | verify | 5525.78 G | 2.64e+03 s | 0.000379 | 2.64e+03 s | 1 (1 × 1) |
| `TRINE-512-ShortSig` | keygen | 2.25 G | 1.08 s | 0.926 | 1.08 s | 100 (5 × 20) |
| `TRINE-512-ShortSig` | sign | 3367.79 G | 1.61e+03 s | 0.000622 | 1.61e+03 s | 1 (1 × 1) |
| `TRINE-512-ShortSig` | verify | 1560.87 G | 745 s | 0.00134 | 745 s | 1 (1 × 1) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `TRINE-128-Balanced` | keygen | 50845 | 1712 KiB | 1968 KiB |
| `TRINE-128-Balanced` | sign | 50845 | 1852 KiB | 8584 KiB |
| `TRINE-128-Balanced` | verify | 50845 | 2828 KiB | 7232 KiB |
| `TRINE-128-ShortSig` | keygen | 51205 | 1772 KiB | 2080 KiB |
| `TRINE-128-ShortSig` | sign | 51205 | 2004 KiB | 6624 KiB |
| `TRINE-128-ShortSig` | verify | 51205 | 2080 KiB | 5208 KiB |
| `TRINE-256-Balanced` | keygen | – | 1844 KiB | 2412 KiB |
| `TRINE-256-Balanced` | sign | – | 2196 KiB | 87828 KiB |
| `TRINE-256-Balanced` | verify | – | 27624 KiB | 87436 KiB |
| `TRINE-256-ShortSig` | keygen | 53829 | 1744 KiB | 3372 KiB |
| `TRINE-256-ShortSig` | sign | 53829 | 2372 KiB | 41740 KiB |
| `TRINE-256-ShortSig` | verify | 53829 | 13284 KiB | 41268 KiB |
| `TRINE-512-Balanced` | keygen | – | 1760 KiB | 9640 KiB |
| `TRINE-512-Balanced` | sign | – | 2764 KiB | 1370616 KiB |
| `TRINE-512-Balanced` | verify | – | 15008 KiB | 1374708 KiB |
| `TRINE-512-ShortSig` | keygen | – | 1724 KiB | 17352 KiB |
| `TRINE-512-ShortSig` | sign | – | 5088 KiB | 633364 KiB |
| `TRINE-512-ShortSig` | verify | – | 9992 KiB | 637392 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

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
| `TRINE-128-Balanced` | keygen | 64% | 0.1% | drng 1, pseudoXOF 29 |
| `TRINE-128-Balanced` | sign | 16% | 0.0% | drng 2, pseudoXOF 1.7e+03, sm3hash 1 |
| `TRINE-128-Balanced` | verify | 16% | 0.0% | pseudoXOF 1.06e+03, sm3hash 1 |
| `TRINE-128-ShortSig` | keygen | 35% | 0.0% | drng 1, pseudoXOF 83 |
| `TRINE-128-ShortSig` | sign | 16% | 0.0% | drng 2, pseudoXOF 775, sm3hash 1 |
| `TRINE-128-ShortSig` | verify | 16% | 0.0% | pseudoXOF 326, sm3hash 1 |
| `TRINE-256-Balanced` | keygen | 46% | 0.0% | drng 1, pseudoXOF 37 |
| `TRINE-256-Balanced` | sign | 7.1% | 0.0% | drng 2, pseudoXOF 3.54e+03, pseudohash 1 |
| `TRINE-256-Balanced` | verify | 6.8% | 0.0% | pseudoXOF 2.19e+03, pseudohash 1 |
| `TRINE-256-ShortSig` | keygen | 25% | 0.0% | drng 1, pseudoXOF 109 |
| `TRINE-256-ShortSig` | sign | 9.2% | 0.0% | drng 2, pseudoXOF 1.54e+03, pseudohash 1 |
| `TRINE-256-ShortSig` | verify | 8.8% | 0.0% | pseudoXOF 533, pseudohash 1 |
| `TRINE-512-Balanced` | keygen | 57% | 0.0% | drng 1, pseudoXOF 47 |
| `TRINE-512-Balanced` | sign | 2.5% | 0.0% | drng 2, pseudoXOF 7.56e+03, pseudohash 1 |
| `TRINE-512-Balanced` | verify | 2.5% | 0.0% | pseudoXOF 4.6e+03, pseudohash 1 |
| `TRINE-512-ShortSig` | keygen | 27% | 0.0% | drng 1, pseudoXOF 137 |
| `TRINE-512-ShortSig` | sign | 2.5% | 0.0% | drng 2, pseudoXOF 3.31e+03, pseudohash 1 |
| `TRINE-512-ShortSig` | verify | 2.5% | 0.0% | pseudoXOF 1.21e+03, pseudohash 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `TRINE-128-Balanced` | KAT log (sha256 `de04dec3db84fdb9…`) | `kat/sign-30/TRINE-128-Balanced.log` |
| `TRINE-128-Balanced` | timing keygen | `records/sign-30/TRINE-128-Balanced__keygen.json` |
| `TRINE-128-Balanced` | timing sign | `records/sign-30/TRINE-128-Balanced__sign.json` |
| `TRINE-128-Balanced` | timing verify | `records/sign-30/TRINE-128-Balanced__verify.json` |
| `TRINE-128-Balanced` | hash profile keygen | `profile/sign-30/TRINE-128-Balanced__keygen.json` |
| `TRINE-128-Balanced` | hash profile sign | `profile/sign-30/TRINE-128-Balanced__sign.json` |
| `TRINE-128-Balanced` | hash profile verify | `profile/sign-30/TRINE-128-Balanced__verify.json` |
| `TRINE-128-ShortSig` | KAT log (sha256 `441f4563c576ff3b…`) | `kat/sign-30/TRINE-128-ShortSig.log` |
| `TRINE-128-ShortSig` | timing keygen | `records/sign-30/TRINE-128-ShortSig__keygen.json` |
| `TRINE-128-ShortSig` | timing sign | `records/sign-30/TRINE-128-ShortSig__sign.json` |
| `TRINE-128-ShortSig` | timing verify | `records/sign-30/TRINE-128-ShortSig__verify.json` |
| `TRINE-128-ShortSig` | hash profile keygen | `profile/sign-30/TRINE-128-ShortSig__keygen.json` |
| `TRINE-128-ShortSig` | hash profile sign | `profile/sign-30/TRINE-128-ShortSig__sign.json` |
| `TRINE-128-ShortSig` | hash profile verify | `profile/sign-30/TRINE-128-ShortSig__verify.json` |
| `TRINE-256-Balanced` | KAT log (sha256 `8a9d645ae0473cbe…`) | `kat/sign-30/TRINE-256-Balanced.log` |
| `TRINE-256-Balanced` | timing keygen | `records/sign-30/TRINE-256-Balanced__keygen.json` |
| `TRINE-256-Balanced` | timing sign | `records/sign-30/TRINE-256-Balanced__sign.json` |
| `TRINE-256-Balanced` | timing verify | `records/sign-30/TRINE-256-Balanced__verify.json` |
| `TRINE-256-Balanced` | hash profile keygen | `profile/sign-30/TRINE-256-Balanced__keygen.json` |
| `TRINE-256-Balanced` | hash profile sign | `profile/sign-30/TRINE-256-Balanced__sign.json` |
| `TRINE-256-Balanced` | hash profile verify | `profile/sign-30/TRINE-256-Balanced__verify.json` |
| `TRINE-256-ShortSig` | KAT log (sha256 `e5f2432da30f4a9f…`) | `kat/sign-30/TRINE-256-ShortSig.log` |
| `TRINE-256-ShortSig` | timing keygen | `records/sign-30/TRINE-256-ShortSig__keygen.json` |
| `TRINE-256-ShortSig` | timing sign | `records/sign-30/TRINE-256-ShortSig__sign.json` |
| `TRINE-256-ShortSig` | timing verify | `records/sign-30/TRINE-256-ShortSig__verify.json` |
| `TRINE-256-ShortSig` | hash profile keygen | `profile/sign-30/TRINE-256-ShortSig__keygen.json` |
| `TRINE-256-ShortSig` | hash profile sign | `profile/sign-30/TRINE-256-ShortSig__sign.json` |
| `TRINE-256-ShortSig` | hash profile verify | `profile/sign-30/TRINE-256-ShortSig__verify.json` |
| `TRINE-512-Balanced` | KAT log (sha256 `bfa4cfcf64503204…`) | `kat/sign-30/TRINE-512-Balanced.log` |
| `TRINE-512-Balanced` | timing keygen | `records/sign-30/TRINE-512-Balanced__keygen.json` |
| `TRINE-512-Balanced` | timing sign | `records/sign-30/TRINE-512-Balanced__sign.json` |
| `TRINE-512-Balanced` | timing verify | `records/sign-30/TRINE-512-Balanced__verify.json` |
| `TRINE-512-Balanced` | hash profile keygen | `profile/sign-30/TRINE-512-Balanced__keygen.json` |
| `TRINE-512-Balanced` | hash profile sign | `profile/sign-30/TRINE-512-Balanced__sign.json` |
| `TRINE-512-Balanced` | hash profile verify | `profile/sign-30/TRINE-512-Balanced__verify.json` |
| `TRINE-512-ShortSig` | KAT log (sha256 `3db85609c95c76ac…`) | `kat/sign-30/TRINE-512-ShortSig.log` |
| `TRINE-512-ShortSig` | timing keygen | `records/sign-30/TRINE-512-ShortSig__keygen.json` |
| `TRINE-512-ShortSig` | timing sign | `records/sign-30/TRINE-512-ShortSig__sign.json` |
| `TRINE-512-ShortSig` | timing verify | `records/sign-30/TRINE-512-ShortSig__verify.json` |
| `TRINE-512-ShortSig` | hash profile keygen | `profile/sign-30/TRINE-512-ShortSig__keygen.json` |
| `TRINE-512-ShortSig` | hash profile sign | `profile/sign-30/TRINE-512-ShortSig__sign.json` |
| `TRINE-512-ShortSig` | hash profile verify | `profile/sign-30/TRINE-512-ShortSig__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

