# sign-22 Rhyme — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-22` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561095950520320.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: Rhyme
- Implementation versions measured: reference
- Parameter sets: `Rhyme-SHAKE-128`, `Rhyme-SHAKE-256`, `Rhyme-SHAKE-384`, `Rhyme-SHAKE-512`, `Rhyme-SM3-128`, `Rhyme-SM3-256`, `Rhyme-SM3-384`, `Rhyme-SM3-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-22/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Rhyme-SHAKE-128` | guide | PASS |
| `Rhyme-SHAKE-256` | guide | PASS |
| `Rhyme-SHAKE-384` | guide | PASS |
| `Rhyme-SHAKE-512` | guide | PASS |
| `Rhyme-SM3-128` | guide | PASS |
| `Rhyme-SM3-256` | guide | PASS |
| `Rhyme-SM3-384` | guide | PASS |
| `Rhyme-SM3-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Rhyme-SHAKE-128` | keygen | 391.43 M | 145 ms | 6.89 | 145 ms | 100 (5 × 20) |
| `Rhyme-SHAKE-128` | sign | 1.60 M | 592 µs | 1.69e+03 | 592 µs | 4590 (5 × 918) |
| `Rhyme-SHAKE-128` | verify | 139.2 k | 51.7 µs | 1.94e+04 | 51.4 µs | 38370 (5 × 7674) |
| `Rhyme-SHAKE-256` | keygen | 866.01 M | 321 ms | 3.11 | 321 ms | 100 (5 × 20) |
| `Rhyme-SHAKE-256` | sign | 7.33 M | 2.72 ms | 368 | 2.72 ms | 1105 (5 × 221) |
| `Rhyme-SHAKE-256` | verify | 372.4 k | 138 µs | 7.24e+03 | 136 µs | 16945 (5 × 3389) |
| `Rhyme-SHAKE-384` | keygen | 3.40 G | 1.26 s | 0.793 | 1.26 s | 100 (5 × 20) |
| `Rhyme-SHAKE-384` | sign | 3.47 M | 1.29 ms | 776 | 1.29 ms | 2100 (5 × 420) |
| `Rhyme-SHAKE-384` | verify | 628.9 k | 233 µs | 4.28e+03 | 234 µs | 10175 (5 × 2035) |
| `Rhyme-SHAKE-512` | keygen | 4.25 G | 1.58 s | 0.634 | 1.58 s | 80 (5 × 16) |
| `Rhyme-SHAKE-512` | sign | 7.25 M | 2.69 ms | 372 | 2.69 ms | 1045 (5 × 209) |
| `Rhyme-SHAKE-512` | verify | 930.1 k | 345 µs | 2.9e+03 | 343 µs | 7045 (5 × 1409) |
| `Rhyme-SM3-128` | keygen | 420.26 M | 156 ms | 6.41 | 156 ms | 100 (5 × 20) |
| `Rhyme-SM3-128` | sign | 3.80 M | 1.41 ms | 709 | 1.41 ms | 2085 (5 × 417) |
| `Rhyme-SM3-128` | verify | 274.1 k | 102 µs | 9.82e+03 | 102 µs | 23955 (5 × 4791) |
| `Rhyme-SM3-256` | keygen | 1.10 G | 408 ms | 2.45 | 408 ms | 100 (5 × 20) |
| `Rhyme-SM3-256` | sign | 14.01 M | 5.2 ms | 192 | 5.19 ms | 565 (5 × 113) |
| `Rhyme-SM3-256` | verify | 785.3 k | 292 µs | 3.43e+03 | 290 µs | 8755 (5 × 1751) |
| `Rhyme-SM3-384` | keygen | 2.96 G | 1.1 s | 0.91 | 1.1 s | 100 (5 × 20) |
| `Rhyme-SM3-384` | sign | 20.72 M | 7.69 ms | 130 | 7.68 ms | 405 (5 × 81) |
| `Rhyme-SM3-384` | verify | 1.40 M | 520 µs | 1.92e+03 | 519 µs | 5550 (5 × 1110) |
| `Rhyme-SM3-512` | keygen | 3.28 G | 1.22 s | 0.823 | 1.22 s | 100 (5 × 20) |
| `Rhyme-SM3-512` | sign | 29.31 M | 10.9 ms | 91.9 | 10.8 ms | 285 (5 × 57) |
| `Rhyme-SM3-512` | verify | 1.89 M | 702 µs | 1.42e+03 | 703 µs | 4180 (5 × 836) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Rhyme-SHAKE-128` | keygen | 158848 | 1588 KiB | 2556 KiB |
| `Rhyme-SHAKE-128` | sign | 158848 | 3876 KiB | 4020 KiB |
| `Rhyme-SHAKE-128` | verify | 158848 | 4412 KiB | 4476 KiB |
| `Rhyme-SHAKE-256` | keygen | 206680 | 1608 KiB | 4028 KiB |
| `Rhyme-SHAKE-256` | sign | 206680 | 4460 KiB | 4688 KiB |
| `Rhyme-SHAKE-256` | verify | 206680 | 4712 KiB | 4776 KiB |
| `Rhyme-SHAKE-384` | keygen | 298080 | 1636 KiB | 5708 KiB |
| `Rhyme-SHAKE-384` | sign | 298080 | 5660 KiB | 6012 KiB |
| `Rhyme-SHAKE-384` | verify | 298080 | 5692 KiB | 5756 KiB |
| `Rhyme-SHAKE-512` | keygen | 296568 | 1644 KiB | 5760 KiB |
| `Rhyme-SHAKE-512` | sign | 296568 | 4988 KiB | 5388 KiB |
| `Rhyme-SHAKE-512` | verify | 296568 | 5760 KiB | 5824 KiB |
| `Rhyme-SM3-128` | keygen | 155424 | 1592 KiB | 5304 KiB |
| `Rhyme-SM3-128` | sign | 155424 | 4244 KiB | 19916 KiB |
| `Rhyme-SM3-128` | verify | 155424 | 2508 KiB | 18184 KiB |
| `Rhyme-SM3-256` | keygen | 207072 | 1608 KiB | 6704 KiB |
| `Rhyme-SM3-256` | sign | 207072 | 4180 KiB | 20100 KiB |
| `Rhyme-SM3-256` | verify | 207072 | 4852 KiB | 18040 KiB |
| `Rhyme-SM3-384` | keygen | 294536 | 1636 KiB | 10568 KiB |
| `Rhyme-SM3-384` | sign | 294536 | 5552 KiB | 21208 KiB |
| `Rhyme-SM3-384` | verify | 294536 | 5440 KiB | 19864 KiB |
| `Rhyme-SM3-512` | keygen | 292912 | 1644 KiB | 9904 KiB |
| `Rhyme-SM3-512` | sign | 292912 | 5448 KiB | 20856 KiB |
| `Rhyme-SM3-512` | verify | 292912 | 5900 KiB | 17148 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | signature |
|---|---|---|---|
| `Rhyme-SHAKE-128` | 800 | 11072 | 5156 |
| `Rhyme-SHAKE-256` | 1824 | 22336 | 10308 |
| `Rhyme-SHAKE-384` | 2720 | 45760 | 14436 |
| `Rhyme-SHAKE-512` | 3872 | 44864 | 20612 |
| `Rhyme-SM3-128` | 800 | 11072 | 5156 |
| `Rhyme-SM3-256` | 1824 | 22336 | 10308 |
| `Rhyme-SM3-384` | 2720 | 45760 | 14436 |
| `Rhyme-SM3-512` | 3872 | 44864 | 20612 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **instance-dependent** — SM3 sets: counter XOF on sm3hash; SHAKE sets: own FIPS 202 (bypass)

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Rhyme-SHAKE-128` | keygen | 0.0% | 0.0% | drng 1 |
| `Rhyme-SHAKE-128` | sign | 0.0% | 0.0% | – |
| `Rhyme-SHAKE-128` | verify | 0.0% | 0.0% | – |
| `Rhyme-SHAKE-256` | keygen | 0.0% | 0.0% | drng 1 |
| `Rhyme-SHAKE-256` | sign | 0.0% | 0.0% | – |
| `Rhyme-SHAKE-256` | verify | 0.0% | 0.0% | – |
| `Rhyme-SHAKE-384` | keygen | 0.0% | 0.0% | drng 1 |
| `Rhyme-SHAKE-384` | sign | 0.0% | 0.0% | – |
| `Rhyme-SHAKE-384` | verify | 0.0% | 0.0% | – |
| `Rhyme-SHAKE-512` | keygen | 0.0% | 0.0% | drng 1 |
| `Rhyme-SHAKE-512` | sign | 0.0% | 0.0% | – |
| `Rhyme-SHAKE-512` | verify | 0.0% | 0.0% | – |
| `Rhyme-SM3-128` | keygen | 0.7% | 0.0% | drng 1, sm3hash 953 |
| `Rhyme-SM3-128` | sign | 81% | 0.0% | sm3hash 1.18e+03 |
| `Rhyme-SM3-128` | verify | 64% | 0.0% | sm3hash 92 |
| `Rhyme-SM3-256` | keygen | 0.9% | 0.0% | drng 1, sm3hash 1.77e+03 |
| `Rhyme-SM3-256` | sign | 82% | 0.0% | sm3hash 4.35e+03 |
| `Rhyme-SM3-256` | verify | 66% | 0.0% | sm3hash 241 |
| `Rhyme-SM3-384` | keygen | 0.5% | 0.0% | drng 1, sm3hash 1.38e+04 |
| `Rhyme-SM3-384` | sign | 80% | 0.0% | sm3hash 6.26e+03 |
| `Rhyme-SM3-384` | verify | 65% | 0.0% | sm3hash 446 |
| `Rhyme-SM3-512` | keygen | 0.6% | 0.0% | drng 1, sm3hash 6.59e+03 |
| `Rhyme-SM3-512` | sign | 81% | 0.0% | sm3hash 8.85e+03 |
| `Rhyme-SM3-512` | verify | 62% | 0.0% | sm3hash 474 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Rhyme-SHAKE-128` | KAT log (sha256 `fb31d19acdd3d901…`) | `kat/sign-22/Rhyme-SHAKE-128.log` |
| `Rhyme-SHAKE-128` | timing keygen | `records/sign-22/Rhyme-SHAKE-128__keygen.json` |
| `Rhyme-SHAKE-128` | timing sign | `records/sign-22/Rhyme-SHAKE-128__sign.json` |
| `Rhyme-SHAKE-128` | timing verify | `records/sign-22/Rhyme-SHAKE-128__verify.json` |
| `Rhyme-SHAKE-128` | hash profile keygen | `profile/sign-22/Rhyme-SHAKE-128__keygen.json` |
| `Rhyme-SHAKE-128` | hash profile sign | `profile/sign-22/Rhyme-SHAKE-128__sign.json` |
| `Rhyme-SHAKE-128` | hash profile verify | `profile/sign-22/Rhyme-SHAKE-128__verify.json` |
| `Rhyme-SHAKE-256` | KAT log (sha256 `1e0baf52abf29bb1…`) | `kat/sign-22/Rhyme-SHAKE-256.log` |
| `Rhyme-SHAKE-256` | timing keygen | `records/sign-22/Rhyme-SHAKE-256__keygen.json` |
| `Rhyme-SHAKE-256` | timing sign | `records/sign-22/Rhyme-SHAKE-256__sign.json` |
| `Rhyme-SHAKE-256` | timing verify | `records/sign-22/Rhyme-SHAKE-256__verify.json` |
| `Rhyme-SHAKE-256` | hash profile keygen | `profile/sign-22/Rhyme-SHAKE-256__keygen.json` |
| `Rhyme-SHAKE-256` | hash profile sign | `profile/sign-22/Rhyme-SHAKE-256__sign.json` |
| `Rhyme-SHAKE-256` | hash profile verify | `profile/sign-22/Rhyme-SHAKE-256__verify.json` |
| `Rhyme-SHAKE-384` | KAT log (sha256 `960929cd2bbdd68c…`) | `kat/sign-22/Rhyme-SHAKE-384.log` |
| `Rhyme-SHAKE-384` | timing keygen | `records/sign-22/Rhyme-SHAKE-384__keygen.json` |
| `Rhyme-SHAKE-384` | timing sign | `records/sign-22/Rhyme-SHAKE-384__sign.json` |
| `Rhyme-SHAKE-384` | timing verify | `records/sign-22/Rhyme-SHAKE-384__verify.json` |
| `Rhyme-SHAKE-384` | hash profile keygen | `profile/sign-22/Rhyme-SHAKE-384__keygen.json` |
| `Rhyme-SHAKE-384` | hash profile sign | `profile/sign-22/Rhyme-SHAKE-384__sign.json` |
| `Rhyme-SHAKE-384` | hash profile verify | `profile/sign-22/Rhyme-SHAKE-384__verify.json` |
| `Rhyme-SHAKE-512` | KAT log (sha256 `e3a85502f0ae9b7f…`) | `kat/sign-22/Rhyme-SHAKE-512.log` |
| `Rhyme-SHAKE-512` | timing keygen | `records/sign-22/Rhyme-SHAKE-512__keygen.json` |
| `Rhyme-SHAKE-512` | timing sign | `records/sign-22/Rhyme-SHAKE-512__sign.json` |
| `Rhyme-SHAKE-512` | timing verify | `records/sign-22/Rhyme-SHAKE-512__verify.json` |
| `Rhyme-SHAKE-512` | hash profile keygen | `profile/sign-22/Rhyme-SHAKE-512__keygen.json` |
| `Rhyme-SHAKE-512` | hash profile sign | `profile/sign-22/Rhyme-SHAKE-512__sign.json` |
| `Rhyme-SHAKE-512` | hash profile verify | `profile/sign-22/Rhyme-SHAKE-512__verify.json` |
| `Rhyme-SM3-128` | KAT log (sha256 `6bab502cc5fbb099…`) | `kat/sign-22/Rhyme-SM3-128.log` |
| `Rhyme-SM3-128` | timing keygen | `records/sign-22/Rhyme-SM3-128__keygen.json` |
| `Rhyme-SM3-128` | timing sign | `records/sign-22/Rhyme-SM3-128__sign.json` |
| `Rhyme-SM3-128` | timing verify | `records/sign-22/Rhyme-SM3-128__verify.json` |
| `Rhyme-SM3-128` | hash profile keygen | `profile/sign-22/Rhyme-SM3-128__keygen.json` |
| `Rhyme-SM3-128` | hash profile sign | `profile/sign-22/Rhyme-SM3-128__sign.json` |
| `Rhyme-SM3-128` | hash profile verify | `profile/sign-22/Rhyme-SM3-128__verify.json` |
| `Rhyme-SM3-256` | KAT log (sha256 `75ec3acd7b40e127…`) | `kat/sign-22/Rhyme-SM3-256.log` |
| `Rhyme-SM3-256` | timing keygen | `records/sign-22/Rhyme-SM3-256__keygen.json` |
| `Rhyme-SM3-256` | timing sign | `records/sign-22/Rhyme-SM3-256__sign.json` |
| `Rhyme-SM3-256` | timing verify | `records/sign-22/Rhyme-SM3-256__verify.json` |
| `Rhyme-SM3-256` | hash profile keygen | `profile/sign-22/Rhyme-SM3-256__keygen.json` |
| `Rhyme-SM3-256` | hash profile sign | `profile/sign-22/Rhyme-SM3-256__sign.json` |
| `Rhyme-SM3-256` | hash profile verify | `profile/sign-22/Rhyme-SM3-256__verify.json` |
| `Rhyme-SM3-384` | KAT log (sha256 `0d25194d08819cf7…`) | `kat/sign-22/Rhyme-SM3-384.log` |
| `Rhyme-SM3-384` | timing keygen | `records/sign-22/Rhyme-SM3-384__keygen.json` |
| `Rhyme-SM3-384` | timing sign | `records/sign-22/Rhyme-SM3-384__sign.json` |
| `Rhyme-SM3-384` | timing verify | `records/sign-22/Rhyme-SM3-384__verify.json` |
| `Rhyme-SM3-384` | hash profile keygen | `profile/sign-22/Rhyme-SM3-384__keygen.json` |
| `Rhyme-SM3-384` | hash profile sign | `profile/sign-22/Rhyme-SM3-384__sign.json` |
| `Rhyme-SM3-384` | hash profile verify | `profile/sign-22/Rhyme-SM3-384__verify.json` |
| `Rhyme-SM3-512` | KAT log (sha256 `7f6a15f4e5f4530f…`) | `kat/sign-22/Rhyme-SM3-512.log` |
| `Rhyme-SM3-512` | timing keygen | `records/sign-22/Rhyme-SM3-512__keygen.json` |
| `Rhyme-SM3-512` | timing sign | `records/sign-22/Rhyme-SM3-512__sign.json` |
| `Rhyme-SM3-512` | timing verify | `records/sign-22/Rhyme-SM3-512__verify.json` |
| `Rhyme-SM3-512` | hash profile keygen | `profile/sign-22/Rhyme-SM3-512__keygen.json` |
| `Rhyme-SM3-512` | hash profile sign | `profile/sign-22/Rhyme-SM3-512__sign.json` |
| `Rhyme-SM3-512` | hash profile verify | `profile/sign-22/Rhyme-SM3-512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

