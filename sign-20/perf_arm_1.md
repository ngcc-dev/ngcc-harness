# sign-20 Qing Luan — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-20` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561087243145216.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: Qing Luan
- Implementation versions measured: reference
- Parameter sets: `QingLuan-128`, `QingLuan-256`, `QingLuan-384`, `QingLuan-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-20/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `QingLuan-128` | guide | PASS |
| `QingLuan-256` | guide | PASS |
| `QingLuan-384` | guide | PASS |
| `QingLuan-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `QingLuan-128` | keygen | 259.4 k | 96.2 µs | 1.04e+04 | 95.5 µs | 30190 (5 × 6038) |
| `QingLuan-128` | sign | 14.93 M | 5.54 ms | 180 | 5.54 ms | 570 (5 × 114) |
| `QingLuan-128` | verify | 6.83 M | 2.53 ms | 395 | 2.53 ms | 1240 (5 × 248) |
| `QingLuan-256` | keygen | 1.60 M | 595 µs | 1.68e+03 | 595 µs | 5195 (5 × 1039) |
| `QingLuan-256` | sign | 93.84 M | 34.8 ms | 28.7 | 34.7 ms | 100 (5 × 20) |
| `QingLuan-256` | verify | 43.03 M | 16 ms | 62.6 | 16 ms | 200 (5 × 40) |
| `QingLuan-384` | keygen | 3.44 M | 1.28 ms | 783 | 1.28 ms | 2435 (5 × 487) |
| `QingLuan-384` | sign | 227.75 M | 84.5 ms | 11.8 | 84.5 ms | 100 (5 × 20) |
| `QingLuan-384` | verify | 108.13 M | 40.1 ms | 24.9 | 40.1 ms | 100 (5 × 20) |
| `QingLuan-512` | keygen | 8.44 M | 3.13 ms | 319 | 3.13 ms | 1000 (5 × 200) |
| `QingLuan-512` | sign | 538.64 M | 200 ms | 5 | 200 ms | 100 (5 × 20) |
| `QingLuan-512` | verify | 255.28 M | 94.7 ms | 10.6 | 94.6 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `QingLuan-128` | keygen | 31884 | 1444 KiB | 1512 KiB |
| `QingLuan-128` | sign | 31884 | 1448 KiB | 1628 KiB |
| `QingLuan-128` | verify | 31884 | 3560 KiB | 3624 KiB |
| `QingLuan-256` | keygen | 32288 | 1500 KiB | 1576 KiB |
| `QingLuan-256` | sign | 32288 | 1508 KiB | 3692 KiB |
| `QingLuan-256` | verify | 32288 | 3772 KiB | 3836 KiB |
| `QingLuan-384` | keygen | 32412 | 1588 KiB | 1684 KiB |
| `QingLuan-384` | sign | 32412 | 1616 KiB | 4924 KiB |
| `QingLuan-384` | verify | 32412 | 4396 KiB | 4704 KiB |
| `QingLuan-512` | keygen | 32672 | 3508 KiB | 3572 KiB |
| `QingLuan-512` | sign | 32672 | 3556 KiB | 6076 KiB |
| `QingLuan-512` | verify | 32672 | 4000 KiB | 5748 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `QingLuan-128` | 77 | 32 | 18720 |
| `QingLuan-256` | 153 | 64 | 74248 |
| `QingLuan-384` | 227 | 96 | 164940 |
| `QingLuan-512` | 302 | 128 | 292816 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — builds wide hash and counter XOF on sm3hash, not pseudohash/pseudoXOF

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `QingLuan-128` | keygen | 68% | 1.6% | drng 1, sm3hash 133 |
| `QingLuan-128` | sign | 63% | 0.1% | drng 2, sm3hash 5.93e+03 |
| `QingLuan-128` | verify | 64% | 0.0% | sm3hash 2.54e+03 |
| `QingLuan-256` | keygen | 81% | 0.3% | drng 1, sm3hash 497 |
| `QingLuan-256` | sign | 76% | 0.0% | drng 2, sm3hash 2.28e+04 |
| `QingLuan-256` | verify | 77% | 0.0% | sm3hash 9.73e+03 |
| `QingLuan-384` | keygen | 82% | 0.2% | drng 1, sm3hash 1.07e+03 |
| `QingLuan-384` | sign | 77% | 0.0% | drng 2, sm3hash 5.01e+04 |
| `QingLuan-384` | verify | 80% | 0.0% | sm3hash 2.13e+04 |
| `QingLuan-512` | keygen | 86% | 0.1% | drng 1, sm3hash 1.87e+03 |
| `QingLuan-512` | sign | 82% | 0.0% | drng 2, sm3hash 8.85e+04 |
| `QingLuan-512` | verify | 84% | 0.0% | sm3hash 3.76e+04 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `QingLuan-128` | KAT log (sha256 `27641702af1bac89…`) | `kat/sign-20/QingLuan-128.log` |
| `QingLuan-128` | timing keygen | `records/sign-20/QingLuan-128__keygen.json` |
| `QingLuan-128` | timing sign | `records/sign-20/QingLuan-128__sign.json` |
| `QingLuan-128` | timing verify | `records/sign-20/QingLuan-128__verify.json` |
| `QingLuan-128` | hash profile keygen | `profile/sign-20/QingLuan-128__keygen.json` |
| `QingLuan-128` | hash profile sign | `profile/sign-20/QingLuan-128__sign.json` |
| `QingLuan-128` | hash profile verify | `profile/sign-20/QingLuan-128__verify.json` |
| `QingLuan-256` | KAT log (sha256 `92604f0ea56dafe9…`) | `kat/sign-20/QingLuan-256.log` |
| `QingLuan-256` | timing keygen | `records/sign-20/QingLuan-256__keygen.json` |
| `QingLuan-256` | timing sign | `records/sign-20/QingLuan-256__sign.json` |
| `QingLuan-256` | timing verify | `records/sign-20/QingLuan-256__verify.json` |
| `QingLuan-256` | hash profile keygen | `profile/sign-20/QingLuan-256__keygen.json` |
| `QingLuan-256` | hash profile sign | `profile/sign-20/QingLuan-256__sign.json` |
| `QingLuan-256` | hash profile verify | `profile/sign-20/QingLuan-256__verify.json` |
| `QingLuan-384` | KAT log (sha256 `74a73c25081b5b42…`) | `kat/sign-20/QingLuan-384.log` |
| `QingLuan-384` | timing keygen | `records/sign-20/QingLuan-384__keygen.json` |
| `QingLuan-384` | timing sign | `records/sign-20/QingLuan-384__sign.json` |
| `QingLuan-384` | timing verify | `records/sign-20/QingLuan-384__verify.json` |
| `QingLuan-384` | hash profile keygen | `profile/sign-20/QingLuan-384__keygen.json` |
| `QingLuan-384` | hash profile sign | `profile/sign-20/QingLuan-384__sign.json` |
| `QingLuan-384` | hash profile verify | `profile/sign-20/QingLuan-384__verify.json` |
| `QingLuan-512` | KAT log (sha256 `ed2031c69e9759cb…`) | `kat/sign-20/QingLuan-512.log` |
| `QingLuan-512` | timing keygen | `records/sign-20/QingLuan-512__keygen.json` |
| `QingLuan-512` | timing sign | `records/sign-20/QingLuan-512__sign.json` |
| `QingLuan-512` | timing verify | `records/sign-20/QingLuan-512__verify.json` |
| `QingLuan-512` | hash profile keygen | `profile/sign-20/QingLuan-512__keygen.json` |
| `QingLuan-512` | hash profile sign | `profile/sign-20/QingLuan-512__sign.json` |
| `QingLuan-512` | hash profile verify | `profile/sign-20/QingLuan-512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

