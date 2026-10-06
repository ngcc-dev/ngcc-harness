# sign-34 YuanYang.DSA — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-34` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561114459983872.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: YuanYang.DSA
- Implementation versions measured: reference
- Parameter sets: `yuanyang-512`, `yuanyang-1024`, `yuanyang-2048`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-34/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `yuanyang-512` | guide | PASS |
| `yuanyang-1024` | guide | PASS |
| `yuanyang-2048` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `yuanyang-512` | keygen | 13.52 M | 5.03 ms | 199 | 5.04 ms | 675 (5 × 135) |
| `yuanyang-512` | sign | 6.71 M | 2.49 ms | 402 | 2.48 ms | 1365 (5 × 273) |
| `yuanyang-512` | verify | 182.2 k | 67.6 µs | 1.48e+04 | 67.5 µs | 31435 (5 × 6287) |
| `yuanyang-1024` | keygen | 16.58 M | 6.16 ms | 162 | 6.16 ms | 400 (5 × 80) |
| `yuanyang-1024` | sign | 10.83 M | 4.02 ms | 249 | 4.02 ms | 650 (5 × 130) |
| `yuanyang-1024` | verify | 400.8 k | 149 µs | 6.72e+03 | 148 µs | 14870 (5 × 2974) |
| `yuanyang-2048` | keygen | 71.05 M | 26.7 ms | 37.5 | 26.7 ms | 100 (5 × 20) |
| `yuanyang-2048` | sign | 35.17 M | 13.1 ms | 76.6 | 13 ms | 305 (5 × 61) |
| `yuanyang-2048` | verify | 835.6 k | 310 µs | 3.23e+03 | 310 µs | 7160 (5 × 1432) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `yuanyang-512` | keygen | 171312 | 1696 KiB | 5044 KiB |
| `yuanyang-512` | sign | 171312 | 3344 KiB | 3432 KiB |
| `yuanyang-512` | verify | 171312 | 3364 KiB | 3428 KiB |
| `yuanyang-1024` | keygen | 226432 | 1748 KiB | 5372 KiB |
| `yuanyang-1024` | sign | 226432 | 4592 KiB | 4744 KiB |
| `yuanyang-1024` | verify | 226432 | 4720 KiB | 4784 KiB |
| `yuanyang-2048` | keygen | 348992 | 1856 KiB | 5884 KiB |
| `yuanyang-2048` | sign | 348992 | 3156 KiB | 5308 KiB |
| `yuanyang-2048` | verify | 348992 | 5468 KiB | 5532 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `yuanyang-512` | 738 | 15584 | 561 |
| `yuanyang-1024` | 1570 | 31264 | 1150 |
| `yuanyang-2048` | 3330 | 62720 | 2364 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `yuanyang-512` | keygen | 0.0% | 26% | drng 77.7 |
| `yuanyang-512` | sign | 2.4% | 47% | drng 70.1, pseudoXOF 1.48, sm3hash 1 |
| `yuanyang-512` | verify | 65% | 0.0% | pseudoXOF 1, sm3hash 1 |
| `yuanyang-1024` | keygen | 0.0% | 4.5% | drng 15.5 |
| `yuanyang-1024` | sign | 2.3% | 46% | drng 105, pseudoXOF 1.05, sm3hash 1 |
| `yuanyang-1024` | verify | 59% | 0.0% | pseudoXOF 1, sm3hash 1 |
| `yuanyang-2048` | keygen | 0.0% | 22% | drng 362 |
| `yuanyang-2048` | sign | 2.1% | 46% | drng 339, pseudoXOF 1.69, sm3hash 1 |
| `yuanyang-2048` | verify | 57% | 0.0% | pseudoXOF 1, sm3hash 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `yuanyang-512` | KAT log (sha256 `7fc99852b1bd2eda…`) | `kat/sign-34/yuanyang-512.log` |
| `yuanyang-512` | timing keygen | `records/sign-34/yuanyang-512__keygen.json` |
| `yuanyang-512` | timing sign | `records/sign-34/yuanyang-512__sign.json` |
| `yuanyang-512` | timing verify | `records/sign-34/yuanyang-512__verify.json` |
| `yuanyang-512` | hash profile keygen | `profile/sign-34/yuanyang-512__keygen.json` |
| `yuanyang-512` | hash profile sign | `profile/sign-34/yuanyang-512__sign.json` |
| `yuanyang-512` | hash profile verify | `profile/sign-34/yuanyang-512__verify.json` |
| `yuanyang-1024` | KAT log (sha256 `b400f563dbfabc97…`) | `kat/sign-34/yuanyang-1024.log` |
| `yuanyang-1024` | timing keygen | `records/sign-34/yuanyang-1024__keygen.json` |
| `yuanyang-1024` | timing sign | `records/sign-34/yuanyang-1024__sign.json` |
| `yuanyang-1024` | timing verify | `records/sign-34/yuanyang-1024__verify.json` |
| `yuanyang-1024` | hash profile keygen | `profile/sign-34/yuanyang-1024__keygen.json` |
| `yuanyang-1024` | hash profile sign | `profile/sign-34/yuanyang-1024__sign.json` |
| `yuanyang-1024` | hash profile verify | `profile/sign-34/yuanyang-1024__verify.json` |
| `yuanyang-2048` | KAT log (sha256 `b6ac4d22b8622f44…`) | `kat/sign-34/yuanyang-2048.log` |
| `yuanyang-2048` | timing keygen | `records/sign-34/yuanyang-2048__keygen.json` |
| `yuanyang-2048` | timing sign | `records/sign-34/yuanyang-2048__sign.json` |
| `yuanyang-2048` | timing verify | `records/sign-34/yuanyang-2048__verify.json` |
| `yuanyang-2048` | hash profile keygen | `profile/sign-34/yuanyang-2048__keygen.json` |
| `yuanyang-2048` | hash profile sign | `profile/sign-34/yuanyang-2048__sign.json` |
| `yuanyang-2048` | hash profile verify | `profile/sign-34/yuanyang-2048__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

