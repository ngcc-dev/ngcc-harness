# sign-03 CEDRUS+C — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-03` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561076497338368.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: CEDRUS+C
- Implementation versions measured: reference
- Parameter sets: `CEDRUSC-160f`, `CEDRUSC-160s`, `CEDRUSC-256f`, `CEDRUSC-256s`, `CEDRUSC-384f`, `CEDRUSC-384s`, `CEDRUSC-512f`, `CEDRUSC-512s`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-03/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `CEDRUSC-160f` | guide | PASS |
| `CEDRUSC-160s` | guide | PASS |
| `CEDRUSC-256f` | guide | PASS |
| `CEDRUSC-256s` | guide | PASS |
| `CEDRUSC-384f` | guide | PASS |
| `CEDRUSC-384s` | guide | PASS |
| `CEDRUSC-512f` | guide | PASS |
| `CEDRUSC-512s` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `CEDRUSC-160f` | keygen | 13.62 M | 5.05 ms | 198 | 5.05 ms | 625 (5 × 125) |
| `CEDRUSC-160f` | sign | 488.05 M | 181 ms | 5.52 | 181 ms | 100 (5 × 20) |
| `CEDRUSC-160f` | verify | 14.58 M | 5.41 ms | 185 | 5.41 ms | 585 (5 × 117) |
| `CEDRUSC-160s` | keygen | 560.03 M | 208 ms | 4.81 | 206 ms | 100 (5 × 20) |
| `CEDRUSC-160s` | sign | 8.23 G | 3.06 s | 0.327 | 3.05 s | 100 (5 × 20) |
| `CEDRUSC-160s` | verify | 19.88 M | 7.38 ms | 136 | 7.38 ms | 430 (5 × 86) |
| `CEDRUSC-256f` | keygen | 43.98 M | 16.3 ms | 61.3 | 16.2 ms | 195 (5 × 39) |
| `CEDRUSC-256f` | sign | 1.19 G | 441 ms | 2.27 | 437 ms | 100 (5 × 20) |
| `CEDRUSC-256f` | verify | 20.06 M | 7.44 ms | 134 | 7.44 ms | 425 (5 × 85) |
| `CEDRUSC-256s` | keygen | 909.66 M | 338 ms | 2.96 | 338 ms | 100 (5 × 20) |
| `CEDRUSC-256s` | sign | 12.03 G | 4.46 s | 0.224 | 4.45 s | 80 (5 × 16) |
| `CEDRUSC-256s` | verify | 33.22 M | 12.3 ms | 81.1 | 12.3 ms | 260 (5 × 52) |
| `CEDRUSC-384f` | keygen | 624.24 M | 232 ms | 4.32 | 231 ms | 100 (5 × 20) |
| `CEDRUSC-384f` | sign | 11.81 G | 4.38 s | 0.228 | 4.38 s | 85 (5 × 17) |
| `CEDRUSC-384f` | verify | 119.41 M | 44.3 ms | 22.6 | 44.3 ms | 100 (5 × 20) |
| `CEDRUSC-384s` | keygen | 3.11 G | 1.15 s | 0.868 | 1.15 s | 100 (5 × 20) |
| `CEDRUSC-384s` | sign | 46.73 G | 17.3 s | 0.0577 | 17.3 s | 20 (5 × 4) |
| `CEDRUSC-384s` | verify | 50.58 M | 18.8 ms | 53.3 | 18.8 ms | 165 (5 × 33) |
| `CEDRUSC-512f` | keygen | 1.67 G | 619 ms | 1.62 | 619 ms | 100 (5 × 20) |
| `CEDRUSC-512f` | sign | 23.46 G | 8.7 s | 0.115 | 8.7 s | 40 (5 × 8) |
| `CEDRUSC-512f` | verify | 150.81 M | 56 ms | 17.9 | 56 ms | 100 (5 × 20) |
| `CEDRUSC-512s` | keygen | 6.64 G | 2.46 s | 0.406 | 2.46 s | 100 (5 × 20) |
| `CEDRUSC-512s` | sign | 83.37 G | 30.9 s | 0.0323 | 30.6 s | 10 (5 × 2) |
| `CEDRUSC-512s` | verify | 109.23 M | 40.5 ms | 24.7 | 40.5 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `CEDRUSC-160f` | keygen | 28016 | 1440 KiB | 1504 KiB |
| `CEDRUSC-160f` | sign | 28016 | 1440 KiB | 1504 KiB |
| `CEDRUSC-160f` | verify | 28016 | 1440 KiB | 1504 KiB |
| `CEDRUSC-160s` | keygen | 27800 | 1432 KiB | 1496 KiB |
| `CEDRUSC-160s` | sign | 27800 | 3436 KiB | 3500 KiB |
| `CEDRUSC-160s` | verify | 27800 | 1432 KiB | 1496 KiB |
| `CEDRUSC-256f` | keygen | 28012 | 1464 KiB | 1532 KiB |
| `CEDRUSC-256f` | sign | 28012 | 1468 KiB | 1532 KiB |
| `CEDRUSC-256f` | verify | 28012 | 1468 KiB | 1532 KiB |
| `CEDRUSC-256s` | keygen | 27908 | 1444 KiB | 1508 KiB |
| `CEDRUSC-256s` | sign | 27908 | 1444 KiB | 1508 KiB |
| `CEDRUSC-256s` | verify | 27908 | 1444 KiB | 1508 KiB |
| `CEDRUSC-384f` | keygen | 28212 | 1496 KiB | 1572 KiB |
| `CEDRUSC-384f` | sign | 28212 | 3536 KiB | 3600 KiB |
| `CEDRUSC-384f` | verify | 28212 | 1504 KiB | 1572 KiB |
| `CEDRUSC-384s` | keygen | 28356 | 1480 KiB | 1564 KiB |
| `CEDRUSC-384s` | sign | 28356 | 1496 KiB | 1564 KiB |
| `CEDRUSC-384s` | verify | 28356 | 1496 KiB | 1564 KiB |
| `CEDRUSC-512f` | keygen | 28540 | 1540 KiB | 1632 KiB |
| `CEDRUSC-512f` | sign | 28540 | 1560 KiB | 1632 KiB |
| `CEDRUSC-512f` | verify | 28540 | 1560 KiB | 1628 KiB |
| `CEDRUSC-512s` | keygen | 28540 | 1512 KiB | 1604 KiB |
| `CEDRUSC-512s` | sign | 28540 | 1536 KiB | 1604 KiB |
| `CEDRUSC-512s` | verify | 28540 | 1540 KiB | 1608 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | signature |
|---|---|---|---|
| `CEDRUSC-160f` | 40 | 80 | 19812 |
| `CEDRUSC-160s` | 40 | 80 | 9460 |
| `CEDRUSC-256f` | 64 | 128 | 43548 |
| `CEDRUSC-256s` | 64 | 128 | 24104 |
| `CEDRUSC-384f` | 96 | 192 | 75988 |
| `CEDRUSC-384s` | 96 | 192 | 61572 |
| `CEDRUSC-512f` | 128 | 256 | 121520 |
| `CEDRUSC-512s` | 128 | 256 | 98212 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `CEDRUSC-160f` | keygen | 97% | 0.0% | drng 1, sm3hash 5.01e+03 |
| `CEDRUSC-160f` | sign | 97% | 0.0% | drng 1, pseudoXOF 348, sm3hash 1.79e+05 |
| `CEDRUSC-160f` | verify | 97% | 0.0% | pseudoXOF 1, sm3hash 5.3e+03 |
| `CEDRUSC-160s` | keygen | 97% | 0.0% | drng 1, sm3hash 2.05e+05 |
| `CEDRUSC-160s` | sign | 97% | 0.0% | drng 1, pseudoXOF 3.32e+04, sm3hash 2.99e+06 |
| `CEDRUSC-160s` | verify | 97% | 0.0% | pseudoXOF 1, sm3hash 7.34e+03 |
| `CEDRUSC-256f` | keygen | 97% | 0.0% | drng 1, sm3hash 1.62e+04 |
| `CEDRUSC-256f` | sign | 97% | 0.0% | drng 1, pseudoXOF 708, sm3hash 4.25e+05 |
| `CEDRUSC-256f` | verify | 97% | 0.0% | pseudoXOF 1, sm3hash 7.1e+03 |
| `CEDRUSC-256s` | keygen | 97% | 0.0% | drng 1, sm3hash 3.4e+05 |
| `CEDRUSC-256s` | sign | 97% | 0.0% | drng 1, pseudoXOF 1.93e+04, sm3hash 4.34e+06 |
| `CEDRUSC-256s` | verify | 97% | 0.0% | pseudoXOF 1, sm3hash 1.22e+04 |
| `CEDRUSC-384f` | keygen | 99% | 0.0% | drng 1, pseudoXOF 7.79e+04 |
| `CEDRUSC-384f` | sign | 99% | 0.0% | drng 1, pseudoXOF 1.47e+06 |
| `CEDRUSC-384f` | verify | 99% | 0.0% | pseudoXOF 1.48e+04 |
| `CEDRUSC-384s` | keygen | 99% | 0.0% | drng 1, pseudoXOF 3.86e+05 |
| `CEDRUSC-384s` | sign | 99% | 0.0% | drng 1, pseudoXOF 5.46e+06 |
| `CEDRUSC-384s` | verify | 99% | 0.0% | pseudoXOF 6.19e+03 |
| `CEDRUSC-512f` | keygen | 99% | 0.0% | drng 1, pseudoXOF 2.08e+05 |
| `CEDRUSC-512f` | sign | 99% | 0.0% | drng 1, pseudoXOF 2.88e+06 |
| `CEDRUSC-512f` | verify | 99% | 0.0% | pseudoXOF 1.81e+04 |
| `CEDRUSC-512s` | keygen | 99% | 0.0% | drng 1, pseudoXOF 8.28e+05 |
| `CEDRUSC-512s` | sign | 99% | 0.0% | drng 1, pseudoXOF 9.97e+06 |
| `CEDRUSC-512s` | verify | 99% | 0.0% | pseudoXOF 1.33e+04 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `CEDRUSC-160f` | KAT log (sha256 `582b1a79a961bd03…`) | `kat/sign-03/CEDRUSC-160f.log` |
| `CEDRUSC-160f` | timing keygen | `records/sign-03/CEDRUSC-160f__keygen.json` |
| `CEDRUSC-160f` | timing sign | `records/sign-03/CEDRUSC-160f__sign.json` |
| `CEDRUSC-160f` | timing verify | `records/sign-03/CEDRUSC-160f__verify.json` |
| `CEDRUSC-160f` | hash profile keygen | `profile/sign-03/CEDRUSC-160f__keygen.json` |
| `CEDRUSC-160f` | hash profile sign | `profile/sign-03/CEDRUSC-160f__sign.json` |
| `CEDRUSC-160f` | hash profile verify | `profile/sign-03/CEDRUSC-160f__verify.json` |
| `CEDRUSC-160s` | KAT log (sha256 `36424750ef9791d5…`) | `kat/sign-03/CEDRUSC-160s.log` |
| `CEDRUSC-160s` | timing keygen | `records/sign-03/CEDRUSC-160s__keygen.json` |
| `CEDRUSC-160s` | timing sign | `records/sign-03/CEDRUSC-160s__sign.json` |
| `CEDRUSC-160s` | timing verify | `records/sign-03/CEDRUSC-160s__verify.json` |
| `CEDRUSC-160s` | hash profile keygen | `profile/sign-03/CEDRUSC-160s__keygen.json` |
| `CEDRUSC-160s` | hash profile sign | `profile/sign-03/CEDRUSC-160s__sign.json` |
| `CEDRUSC-160s` | hash profile verify | `profile/sign-03/CEDRUSC-160s__verify.json` |
| `CEDRUSC-256f` | KAT log (sha256 `01406bbcff9dcbd3…`) | `kat/sign-03/CEDRUSC-256f.log` |
| `CEDRUSC-256f` | timing keygen | `records/sign-03/CEDRUSC-256f__keygen.json` |
| `CEDRUSC-256f` | timing sign | `records/sign-03/CEDRUSC-256f__sign.json` |
| `CEDRUSC-256f` | timing verify | `records/sign-03/CEDRUSC-256f__verify.json` |
| `CEDRUSC-256f` | hash profile keygen | `profile/sign-03/CEDRUSC-256f__keygen.json` |
| `CEDRUSC-256f` | hash profile sign | `profile/sign-03/CEDRUSC-256f__sign.json` |
| `CEDRUSC-256f` | hash profile verify | `profile/sign-03/CEDRUSC-256f__verify.json` |
| `CEDRUSC-256s` | KAT log (sha256 `2bcc2ccc9dd12780…`) | `kat/sign-03/CEDRUSC-256s.log` |
| `CEDRUSC-256s` | timing keygen | `records/sign-03/CEDRUSC-256s__keygen.json` |
| `CEDRUSC-256s` | timing sign | `records/sign-03/CEDRUSC-256s__sign.json` |
| `CEDRUSC-256s` | timing verify | `records/sign-03/CEDRUSC-256s__verify.json` |
| `CEDRUSC-256s` | hash profile keygen | `profile/sign-03/CEDRUSC-256s__keygen.json` |
| `CEDRUSC-256s` | hash profile sign | `profile/sign-03/CEDRUSC-256s__sign.json` |
| `CEDRUSC-256s` | hash profile verify | `profile/sign-03/CEDRUSC-256s__verify.json` |
| `CEDRUSC-384f` | KAT log (sha256 `4761a33e8eb4e390…`) | `kat/sign-03/CEDRUSC-384f.log` |
| `CEDRUSC-384f` | timing keygen | `records/sign-03/CEDRUSC-384f__keygen.json` |
| `CEDRUSC-384f` | timing sign | `records/sign-03/CEDRUSC-384f__sign.json` |
| `CEDRUSC-384f` | timing verify | `records/sign-03/CEDRUSC-384f__verify.json` |
| `CEDRUSC-384f` | hash profile keygen | `profile/sign-03/CEDRUSC-384f__keygen.json` |
| `CEDRUSC-384f` | hash profile sign | `profile/sign-03/CEDRUSC-384f__sign.json` |
| `CEDRUSC-384f` | hash profile verify | `profile/sign-03/CEDRUSC-384f__verify.json` |
| `CEDRUSC-384s` | KAT log (sha256 `7ca9a520bc929d9f…`) | `kat/sign-03/CEDRUSC-384s.log` |
| `CEDRUSC-384s` | timing keygen | `records/sign-03/CEDRUSC-384s__keygen.json` |
| `CEDRUSC-384s` | timing sign | `records/sign-03/CEDRUSC-384s__sign.json` |
| `CEDRUSC-384s` | timing verify | `records/sign-03/CEDRUSC-384s__verify.json` |
| `CEDRUSC-384s` | hash profile keygen | `profile/sign-03/CEDRUSC-384s__keygen.json` |
| `CEDRUSC-384s` | hash profile sign | `profile/sign-03/CEDRUSC-384s__sign.json` |
| `CEDRUSC-384s` | hash profile verify | `profile/sign-03/CEDRUSC-384s__verify.json` |
| `CEDRUSC-512f` | KAT log (sha256 `3d55b1c089da2493…`) | `kat/sign-03/CEDRUSC-512f.log` |
| `CEDRUSC-512f` | timing keygen | `records/sign-03/CEDRUSC-512f__keygen.json` |
| `CEDRUSC-512f` | timing sign | `records/sign-03/CEDRUSC-512f__sign.json` |
| `CEDRUSC-512f` | timing verify | `records/sign-03/CEDRUSC-512f__verify.json` |
| `CEDRUSC-512f` | hash profile keygen | `profile/sign-03/CEDRUSC-512f__keygen.json` |
| `CEDRUSC-512f` | hash profile sign | `profile/sign-03/CEDRUSC-512f__sign.json` |
| `CEDRUSC-512f` | hash profile verify | `profile/sign-03/CEDRUSC-512f__verify.json` |
| `CEDRUSC-512s` | KAT log (sha256 `600a4468fc4626e2…`) | `kat/sign-03/CEDRUSC-512s.log` |
| `CEDRUSC-512s` | timing keygen | `records/sign-03/CEDRUSC-512s__keygen.json` |
| `CEDRUSC-512s` | timing sign | `records/sign-03/CEDRUSC-512s__sign.json` |
| `CEDRUSC-512s` | timing verify | `records/sign-03/CEDRUSC-512s__verify.json` |
| `CEDRUSC-512s` | hash profile keygen | `profile/sign-03/CEDRUSC-512s__keygen.json` |
| `CEDRUSC-512s` | hash profile sign | `profile/sign-03/CEDRUSC-512s__sign.json` |
| `CEDRUSC-512s` | hash profile verify | `profile/sign-03/CEDRUSC-512s__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

