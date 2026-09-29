# sign-04 CEDRUSɑ — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-04` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561076644139008.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: CEDRUSɑ
- Implementation versions measured: reference
- Parameter sets: `CEDRUSALPHA-160f`, `CEDRUSALPHA-160s`, `CEDRUSALPHA-256f`, `CEDRUSALPHA-256s`, `CEDRUSALPHA-384f`, `CEDRUSALPHA-384s`, `CEDRUSALPHA-512f`, `CEDRUSALPHA-512s`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-04/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `CEDRUSALPHA-160f` | guide | PASS |
| `CEDRUSALPHA-160s` | guide | PASS |
| `CEDRUSALPHA-256f` | guide | PASS |
| `CEDRUSALPHA-256s` | guide | PASS |
| `CEDRUSALPHA-384f` | guide | PASS |
| `CEDRUSALPHA-384s` | guide | PASS |
| `CEDRUSALPHA-512f` | guide | PASS |
| `CEDRUSALPHA-512s` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `CEDRUSALPHA-160f` | keygen | 15.29 M | 5.67 ms | 176 | 5.63 ms | 540 (5 × 108) |
| `CEDRUSALPHA-160f` | sign | 539.58 M | 200 ms | 5 | 200 ms | 100 (5 × 20) |
| `CEDRUSALPHA-160f` | verify | 16.75 M | 6.21 ms | 161 | 6.21 ms | 495 (5 × 99) |
| `CEDRUSALPHA-160s` | keygen | 494.89 M | 184 ms | 5.45 | 184 ms | 100 (5 × 20) |
| `CEDRUSALPHA-160s` | sign | 7.35 G | 2.73 s | 0.367 | 2.73 s | 100 (5 × 20) |
| `CEDRUSALPHA-160s` | verify | 18.11 M | 6.72 ms | 149 | 6.72 ms | 470 (5 × 94) |
| `CEDRUSALPHA-256f` | keygen | 49.08 M | 18.2 ms | 54.9 | 18.2 ms | 140 (5 × 28) |
| `CEDRUSALPHA-256f` | sign | 1.27 G | 472 ms | 2.12 | 470 ms | 100 (5 × 20) |
| `CEDRUSALPHA-256f` | verify | 22.63 M | 8.4 ms | 119 | 8.39 ms | 380 (5 × 76) |
| `CEDRUSALPHA-256s` | keygen | 743.45 M | 276 ms | 3.63 | 276 ms | 100 (5 × 20) |
| `CEDRUSALPHA-256s` | sign | 11.33 G | 4.2 s | 0.238 | 4.19 s | 90 (5 × 18) |
| `CEDRUSALPHA-256s` | verify | 27.40 M | 10.2 ms | 98.4 | 10.2 ms | 315 (5 × 63) |
| `CEDRUSALPHA-384f` | keygen | 607.42 M | 225 ms | 4.44 | 224 ms | 100 (5 × 20) |
| `CEDRUSALPHA-384f` | sign | 11.81 G | 4.38 s | 0.228 | 4.37 s | 85 (5 × 17) |
| `CEDRUSALPHA-384f` | verify | 117.06 M | 43.4 ms | 23 | 43.4 ms | 100 (5 × 20) |
| `CEDRUSALPHA-384s` | keygen | 3.88 G | 1.44 s | 0.695 | 1.44 s | 100 (5 × 20) |
| `CEDRUSALPHA-384s` | sign | 42.44 G | 15.7 s | 0.0635 | 15.6 s | 20 (5 × 4) |
| `CEDRUSALPHA-384s` | verify | 65.29 M | 24.2 ms | 41.3 | 24 ms | 135 (5 × 27) |
| `CEDRUSALPHA-512f` | keygen | 1.51 G | 560 ms | 1.78 | 560 ms | 100 (5 × 20) |
| `CEDRUSALPHA-512f` | sign | 19.86 G | 7.37 s | 0.136 | 7.37 s | 50 (5 × 10) |
| `CEDRUSALPHA-512f` | verify | 137.72 M | 51.1 ms | 19.6 | 51.1 ms | 100 (5 × 20) |
| `CEDRUSALPHA-512s` | keygen | 7.30 G | 2.71 s | 0.369 | 2.71 s | 100 (5 × 20) |
| `CEDRUSALPHA-512s` | sign | 84.16 G | 31.2 s | 0.032 | 31.2 s | 10 (5 × 2) |
| `CEDRUSALPHA-512s` | verify | 121.91 M | 45.2 ms | 22.1 | 45.2 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `CEDRUSALPHA-160f` | keygen | 248864 | 1440 KiB | 1720 KiB |
| `CEDRUSALPHA-160f` | sign | 248864 | 3688 KiB | 3752 KiB |
| `CEDRUSALPHA-160f` | verify | 248864 | 1656 KiB | 1720 KiB |
| `CEDRUSALPHA-160s` | keygen | 377728 | 1432 KiB | 1828 KiB |
| `CEDRUSALPHA-160s` | sign | 377728 | 1764 KiB | 1828 KiB |
| `CEDRUSALPHA-160s` | verify | 377728 | 1764 KiB | 1828 KiB |
| `CEDRUSALPHA-256f` | keygen | 1125044 | 1464 KiB | 2592 KiB |
| `CEDRUSALPHA-256f` | sign | 1125044 | 2528 KiB | 2592 KiB |
| `CEDRUSALPHA-256f` | verify | 1125044 | 2524 KiB | 2588 KiB |
| `CEDRUSALPHA-256s` | keygen | 1689260 | 1448 KiB | 3112 KiB |
| `CEDRUSALPHA-256s` | sign | 1689260 | 3048 KiB | 3112 KiB |
| `CEDRUSALPHA-256s` | verify | 1689260 | 5064 KiB | 5128 KiB |
| `CEDRUSALPHA-384f` | keygen | 4502484 | 1496 KiB | 5896 KiB |
| `CEDRUSALPHA-384f` | sign | 4502484 | 7864 KiB | 7928 KiB |
| `CEDRUSALPHA-384f` | verify | 4502484 | 5824 KiB | 7936 KiB |
| `CEDRUSALPHA-384s` | keygen | 3940612 | 1480 KiB | 5344 KiB |
| `CEDRUSALPHA-384s` | sign | 3940612 | 5276 KiB | 7376 KiB |
| `CEDRUSALPHA-384s` | verify | 3940612 | 5280 KiB | 5348 KiB |
| `CEDRUSALPHA-512f` | keygen | 10101852 | 1548 KiB | 11388 KiB |
| `CEDRUSALPHA-512f` | sign | 10101852 | 11320 KiB | 11392 KiB |
| `CEDRUSALPHA-512f` | verify | 10101852 | 11320 KiB | 11392 KiB |
| `CEDRUSALPHA-512s` | keygen | 11399340 | 1516 KiB | 12608 KiB |
| `CEDRUSALPHA-512s` | sign | 11399340 | 12540 KiB | 12608 KiB |
| `CEDRUSALPHA-512s` | verify | 11399340 | 14452 KiB | 14516 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | signature |
|---|---|---|---|
| `CEDRUSALPHA-160f` | 40 | 80 | 19420 |
| `CEDRUSALPHA-160s` | 40 | 80 | 10300 |
| `CEDRUSALPHA-256f` | 64 | 128 | 43296 |
| `CEDRUSALPHA-256s` | 64 | 128 | 25568 |
| `CEDRUSALPHA-384f` | 96 | 192 | 76176 |
| `CEDRUSALPHA-384s` | 96 | 192 | 60672 |
| `CEDRUSALPHA-512f` | 128 | 256 | 127488 |
| `CEDRUSALPHA-512s` | 128 | 256 | 98048 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `CEDRUSALPHA-160f` | keygen | 97% | 0.0% | drng 1, sm3hash 5.74e+03 |
| `CEDRUSALPHA-160f` | sign | 97% | 0.0% | drng 1, pseudoXOF 1, sm3hash 2.05e+05 |
| `CEDRUSALPHA-160f` | verify | 97% | 0.0% | pseudoXOF 1, sm3hash 6.09e+03 |
| `CEDRUSALPHA-160s` | keygen | 97% | 0.0% | drng 1, sm3hash 1.85e+05 |
| `CEDRUSALPHA-160s` | sign | 97% | 0.0% | drng 1, pseudoXOF 1, sm3hash 2.75e+06 |
| `CEDRUSALPHA-160s` | verify | 97% | 0.0% | pseudoXOF 1, sm3hash 6.65e+03 |
| `CEDRUSALPHA-256f` | keygen | 97% | 0.0% | drng 1, sm3hash 1.82e+04 |
| `CEDRUSALPHA-256f` | sign | 97% | 0.0% | drng 1, pseudoXOF 1, sm3hash 4.64e+05 |
| `CEDRUSALPHA-256f` | verify | 96% | 0.0% | pseudoXOF 1, sm3hash 8.01e+03 |
| `CEDRUSALPHA-256s` | keygen | 97% | 0.0% | drng 1, sm3hash 2.78e+05 |
| `CEDRUSALPHA-256s` | sign | 97% | 0.0% | drng 1, pseudoXOF 1, sm3hash 4.17e+06 |
| `CEDRUSALPHA-256s` | verify | 96% | 0.0% | pseudoXOF 1, sm3hash 9.95e+03 |
| `CEDRUSALPHA-384f` | keygen | 99% | 0.0% | drng 1, pseudoXOF 7.63e+04 |
| `CEDRUSALPHA-384f` | sign | 99% | 0.0% | drng 1, pseudoXOF 1.51e+06 |
| `CEDRUSALPHA-384f` | verify | 99% | 0.0% | pseudoXOF 1.45e+04 |
| `CEDRUSALPHA-384s` | keygen | 99% | 0.0% | drng 1, pseudoXOF 4.92e+05 |
| `CEDRUSALPHA-384s` | sign | 99% | 0.0% | drng 1, pseudoXOF 5.36e+06 |
| `CEDRUSALPHA-384s` | verify | 98% | 0.0% | pseudoXOF 7.96e+03 |
| `CEDRUSALPHA-512f` | keygen | 99% | 0.0% | drng 1, pseudoXOF 1.9e+05 |
| `CEDRUSALPHA-512f` | sign | 99% | 0.0% | drng 1, pseudoXOF 2.5e+06 |
| `CEDRUSALPHA-512f` | verify | 98% | 0.0% | pseudoXOF 1.66e+04 |
| `CEDRUSALPHA-512s` | keygen | 99% | 0.0% | drng 1, pseudoXOF 9.18e+05 |
| `CEDRUSALPHA-512s` | sign | 99% | 0.0% | drng 1, pseudoXOF 1.06e+07 |
| `CEDRUSALPHA-512s` | verify | 98% | 0.0% | pseudoXOF 1.47e+04 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `CEDRUSALPHA-160f` | KAT log (sha256 `152206311d5445f1…`) | `kat/sign-04/CEDRUSALPHA-160f.log` |
| `CEDRUSALPHA-160f` | timing keygen | `records/sign-04/CEDRUSALPHA-160f__keygen.json` |
| `CEDRUSALPHA-160f` | timing sign | `records/sign-04/CEDRUSALPHA-160f__sign.json` |
| `CEDRUSALPHA-160f` | timing verify | `records/sign-04/CEDRUSALPHA-160f__verify.json` |
| `CEDRUSALPHA-160f` | hash profile keygen | `profile/sign-04/CEDRUSALPHA-160f__keygen.json` |
| `CEDRUSALPHA-160f` | hash profile sign | `profile/sign-04/CEDRUSALPHA-160f__sign.json` |
| `CEDRUSALPHA-160f` | hash profile verify | `profile/sign-04/CEDRUSALPHA-160f__verify.json` |
| `CEDRUSALPHA-160s` | KAT log (sha256 `6cee78969fb3159e…`) | `kat/sign-04/CEDRUSALPHA-160s.log` |
| `CEDRUSALPHA-160s` | timing keygen | `records/sign-04/CEDRUSALPHA-160s__keygen.json` |
| `CEDRUSALPHA-160s` | timing sign | `records/sign-04/CEDRUSALPHA-160s__sign.json` |
| `CEDRUSALPHA-160s` | timing verify | `records/sign-04/CEDRUSALPHA-160s__verify.json` |
| `CEDRUSALPHA-160s` | hash profile keygen | `profile/sign-04/CEDRUSALPHA-160s__keygen.json` |
| `CEDRUSALPHA-160s` | hash profile sign | `profile/sign-04/CEDRUSALPHA-160s__sign.json` |
| `CEDRUSALPHA-160s` | hash profile verify | `profile/sign-04/CEDRUSALPHA-160s__verify.json` |
| `CEDRUSALPHA-256f` | KAT log (sha256 `4e8cba962c6e2a49…`) | `kat/sign-04/CEDRUSALPHA-256f.log` |
| `CEDRUSALPHA-256f` | timing keygen | `records/sign-04/CEDRUSALPHA-256f__keygen.json` |
| `CEDRUSALPHA-256f` | timing sign | `records/sign-04/CEDRUSALPHA-256f__sign.json` |
| `CEDRUSALPHA-256f` | timing verify | `records/sign-04/CEDRUSALPHA-256f__verify.json` |
| `CEDRUSALPHA-256f` | hash profile keygen | `profile/sign-04/CEDRUSALPHA-256f__keygen.json` |
| `CEDRUSALPHA-256f` | hash profile sign | `profile/sign-04/CEDRUSALPHA-256f__sign.json` |
| `CEDRUSALPHA-256f` | hash profile verify | `profile/sign-04/CEDRUSALPHA-256f__verify.json` |
| `CEDRUSALPHA-256s` | KAT log (sha256 `925ebb5fafafd670…`) | `kat/sign-04/CEDRUSALPHA-256s.log` |
| `CEDRUSALPHA-256s` | timing keygen | `records/sign-04/CEDRUSALPHA-256s__keygen.json` |
| `CEDRUSALPHA-256s` | timing sign | `records/sign-04/CEDRUSALPHA-256s__sign.json` |
| `CEDRUSALPHA-256s` | timing verify | `records/sign-04/CEDRUSALPHA-256s__verify.json` |
| `CEDRUSALPHA-256s` | hash profile keygen | `profile/sign-04/CEDRUSALPHA-256s__keygen.json` |
| `CEDRUSALPHA-256s` | hash profile sign | `profile/sign-04/CEDRUSALPHA-256s__sign.json` |
| `CEDRUSALPHA-256s` | hash profile verify | `profile/sign-04/CEDRUSALPHA-256s__verify.json` |
| `CEDRUSALPHA-384f` | KAT log (sha256 `c1c610c80d5317b3…`) | `kat/sign-04/CEDRUSALPHA-384f.log` |
| `CEDRUSALPHA-384f` | timing keygen | `records/sign-04/CEDRUSALPHA-384f__keygen.json` |
| `CEDRUSALPHA-384f` | timing sign | `records/sign-04/CEDRUSALPHA-384f__sign.json` |
| `CEDRUSALPHA-384f` | timing verify | `records/sign-04/CEDRUSALPHA-384f__verify.json` |
| `CEDRUSALPHA-384f` | hash profile keygen | `profile/sign-04/CEDRUSALPHA-384f__keygen.json` |
| `CEDRUSALPHA-384f` | hash profile sign | `profile/sign-04/CEDRUSALPHA-384f__sign.json` |
| `CEDRUSALPHA-384f` | hash profile verify | `profile/sign-04/CEDRUSALPHA-384f__verify.json` |
| `CEDRUSALPHA-384s` | KAT log (sha256 `d49255fbcd986b11…`) | `kat/sign-04/CEDRUSALPHA-384s.log` |
| `CEDRUSALPHA-384s` | timing keygen | `records/sign-04/CEDRUSALPHA-384s__keygen.json` |
| `CEDRUSALPHA-384s` | timing sign | `records/sign-04/CEDRUSALPHA-384s__sign.json` |
| `CEDRUSALPHA-384s` | timing verify | `records/sign-04/CEDRUSALPHA-384s__verify.json` |
| `CEDRUSALPHA-384s` | hash profile keygen | `profile/sign-04/CEDRUSALPHA-384s__keygen.json` |
| `CEDRUSALPHA-384s` | hash profile sign | `profile/sign-04/CEDRUSALPHA-384s__sign.json` |
| `CEDRUSALPHA-384s` | hash profile verify | `profile/sign-04/CEDRUSALPHA-384s__verify.json` |
| `CEDRUSALPHA-512f` | KAT log (sha256 `3fe07db393a89dd2…`) | `kat/sign-04/CEDRUSALPHA-512f.log` |
| `CEDRUSALPHA-512f` | timing keygen | `records/sign-04/CEDRUSALPHA-512f__keygen.json` |
| `CEDRUSALPHA-512f` | timing sign | `records/sign-04/CEDRUSALPHA-512f__sign.json` |
| `CEDRUSALPHA-512f` | timing verify | `records/sign-04/CEDRUSALPHA-512f__verify.json` |
| `CEDRUSALPHA-512f` | hash profile keygen | `profile/sign-04/CEDRUSALPHA-512f__keygen.json` |
| `CEDRUSALPHA-512f` | hash profile sign | `profile/sign-04/CEDRUSALPHA-512f__sign.json` |
| `CEDRUSALPHA-512f` | hash profile verify | `profile/sign-04/CEDRUSALPHA-512f__verify.json` |
| `CEDRUSALPHA-512s` | KAT log (sha256 `e196c87b22c20f22…`) | `kat/sign-04/CEDRUSALPHA-512s.log` |
| `CEDRUSALPHA-512s` | timing keygen | `records/sign-04/CEDRUSALPHA-512s__keygen.json` |
| `CEDRUSALPHA-512s` | timing sign | `records/sign-04/CEDRUSALPHA-512s__sign.json` |
| `CEDRUSALPHA-512s` | timing verify | `records/sign-04/CEDRUSALPHA-512s__verify.json` |
| `CEDRUSALPHA-512s` | hash profile keygen | `profile/sign-04/CEDRUSALPHA-512s__keygen.json` |
| `CEDRUSALPHA-512s` | hash profile sign | `profile/sign-04/CEDRUSALPHA-512s__sign.json` |
| `CEDRUSALPHA-512s` | hash profile verify | `profile/sign-04/CEDRUSALPHA-512s__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

