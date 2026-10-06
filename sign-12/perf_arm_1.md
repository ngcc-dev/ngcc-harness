# sign-12 Galas Signature Scheme — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-12` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561077713686528.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: Galas Signature Scheme
- Implementation versions measured: reference
- Parameter sets: `Galas-160F`, `Galas-160S`, `Galas-256F`, `Galas-256S`, `Galas-384F`, `Galas-384S`, `Galas-512F`, `Galas-512S`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-12/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Galas-160F` | harness-default | MISMATCH [1] |
| `Galas-160S` | harness-default | MISMATCH [1] |
| `Galas-256F` | harness-default | MISMATCH [1] |
| `Galas-256S` | harness-default | MISMATCH [1] |
| `Galas-384F` | harness-default | MISMATCH [1] |
| `Galas-384S` | harness-default | MISMATCH [1] |
| `Galas-512F` | harness-default | MISMATCH [1] |
| `Galas-512S` | harness-default | MISMATCH [1] |

[1] API deviation: sig_keygen does not draw from drng_algorithm but seeds a private DRNG from 32 zero bytes (or from a seed set by a non-API helper used only by the submitters' own KAT generator), so the official KAT flow yields different keys; with keygen drawing from the DRNG the first KAT records reproduce exactly (sign-12/Makefile). Galas-512S additionally exceeds the 900 s KAT time limit. Timed anyway; note that key generation always produces the same key pair. These instances are timed anyway; their output is not validated.

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Galas-160F` | keygen | 17.40 M | 6.45 ms | 155 | 6.46 ms | 485 (5 × 97) |
| `Galas-160F` | sign | 1.58 G | 585 ms | 1.71 | 589 ms | 100 (5 × 20) |
| `Galas-160F` | verify | 968.28 M | 359 ms | 2.78 | 360 ms | 100 (5 × 20) |
| `Galas-160S` | keygen | 17.39 M | 6.45 ms | 155 | 6.46 ms | 490 (5 × 98) |
| `Galas-160S` | sign | 2.04 G | 758 ms | 1.32 | 758 ms | 100 (5 × 20) |
| `Galas-160S` | verify | 1.40 G | 519 ms | 1.92 | 519 ms | 100 (5 × 20) |
| `Galas-256F` | keygen | 45.48 M | 16.9 ms | 59.3 | 16.9 ms | 190 (5 × 38) |
| `Galas-256F` | sign | 6.53 G | 2.42 s | 0.413 | 2.44 s | 100 (5 × 20) |
| `Galas-256F` | verify | 3.90 G | 1.45 s | 0.692 | 1.47 s | 100 (5 × 20) |
| `Galas-256S` | keygen | 45.24 M | 16.8 ms | 59.6 | 16.9 ms | 190 (5 × 38) |
| `Galas-256S` | sign | 10.48 G | 3.89 s | 0.257 | 3.89 s | 80 (5 × 16) |
| `Galas-256S` | verify | 7.81 G | 2.9 s | 0.345 | 2.9 s | 100 (5 × 20) |
| `Galas-384F` | keygen | 106.98 M | 39.7 ms | 25.2 | 39.7 ms | 100 (5 × 20) |
| `Galas-384F` | sign | 24.98 G | 9.27 s | 0.108 | 9.27 s | 30 (5 × 6) |
| `Galas-384F` | verify | 14.31 G | 5.31 s | 0.188 | 5.31 s | 65 (5 × 13) |
| `Galas-384S` | keygen | 107.01 M | 39.7 ms | 25.2 | 39.7 ms | 100 (5 × 20) |
| `Galas-384S` | sign | 33.17 G | 12.3 s | 0.0813 | 12.3 s | 25 (5 × 5) |
| `Galas-384S` | verify | 22.25 G | 8.26 s | 0.121 | 8.26 s | 40 (5 × 8) |
| `Galas-512F` | keygen | 195.39 M | 72.5 ms | 13.8 | 73 ms | 100 (5 × 20) |
| `Galas-512F` | sign | 66.84 G | 24.8 s | 0.0403 | 25 s | 10 (5 × 2) |
| `Galas-512F` | verify | 35.30 G | 13.1 s | 0.0763 | 13.1 s | 30 (5 × 6) |
| `Galas-512S` | keygen | 196.94 M | 73.1 ms | 13.7 | 73.1 ms | 100 (5 × 20) |
| `Galas-512S` | sign | 83.19 G | 30.9 s | 0.0324 | 30.8 s | 10 (5 × 2) |
| `Galas-512S` | verify | 50.62 G | 18.8 s | 0.0532 | 18.8 s | 15 (5 × 3) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Galas-160F` | keygen | 590452 | 3620 KiB | 3684 KiB |
| `Galas-160F` | sign | 590452 | 1592 KiB | 7388 KiB |
| `Galas-160F` | verify | 590452 | 6844 KiB | 7192 KiB |
| `Galas-160S` | keygen | 590452 | 1592 KiB | 1656 KiB |
| `Galas-160S` | sign | 590452 | 1592 KiB | 9596 KiB |
| `Galas-160S` | verify | 590452 | 7992 KiB | 9512 KiB |
| `Galas-256F` | keygen | 590452 | 1600 KiB | 1728 KiB |
| `Galas-256F` | sign | 590452 | 1664 KiB | 13848 KiB |
| `Galas-256F` | verify | 590452 | 13444 KiB | 14012 KiB |
| `Galas-256S` | keygen | 590452 | 1596 KiB | 1724 KiB |
| `Galas-256S` | sign | 590452 | 1660 KiB | 26608 KiB |
| `Galas-256S` | verify | 590452 | 12280 KiB | 26264 KiB |
| `Galas-384F` | keygen | 590452 | 1620 KiB | 1876 KiB |
| `Galas-384F` | sign | 590452 | 1812 KiB | 38896 KiB |
| `Galas-384F` | verify | 590452 | 35064 KiB | 38068 KiB |
| `Galas-384S` | keygen | 590452 | 1612 KiB | 1868 KiB |
| `Galas-384S` | sign | 590452 | 1804 KiB | 64592 KiB |
| `Galas-384S` | verify | 590452 | 36452 KiB | 64100 KiB |
| `Galas-512F` | keygen | 590452 | 1644 KiB | 2028 KiB |
| `Galas-512F` | sign | 590452 | 1964 KiB | 84780 KiB |
| `Galas-512F` | verify | 590452 | 78904 KiB | 83928 KiB |
| `Galas-512S` | keygen | 590452 | 1632 KiB | 2020 KiB |
| `Galas-512S` | sign | 590452 | 1956 KiB | 134496 KiB |
| `Galas-512S` | verify | 590452 | 80608 KiB | 134100 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `Galas-160F` | 40 | 40 | 5964 |
| `Galas-160S` | 40 | 40 | 4812 |
| `Galas-256F` | 64 | 64 | 15950 |
| `Galas-256S` | 64 | 64 | 12114 |
| `Galas-384F` | 96 | 96 | 33384 |
| `Galas-384S` | 96 | 96 | 27784 |
| `Galas-512F` | 128 | 128 | 59416 |
| `Galas-512S` | 128 | 128 | 49516 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Galas-160F` | keygen | 0.0% | 0.1% | drng 6 |
| `Galas-160F` | sign | 4.7% | 0.0% | pseudoXOF 3.69e+04, pseudohash 102 |
| `Galas-160F` | verify | 7.5% | 0.0% | pseudoXOF 3.65e+04, pseudohash 28 |
| `Galas-160S` | keygen | 0.0% | 0.1% | drng 6 |
| `Galas-160S` | sign | 25% | 0.0% | pseudoXOF 2.49e+05, pseudohash 1.85e+03 |
| `Galas-160S` | verify | 34% | 0.0% | pseudoXOF 2.49e+05, pseudohash 22 |
| `Galas-256F` | keygen | 0.0% | 0.1% | drng 6 |
| `Galas-256F` | sign | 3.6% | 0.0% | pseudoXOF 6.18e+04, pseudohash 297 |
| `Galas-256F` | verify | 5.7% | 0.0% | pseudoXOF 6.11e+04, pseudohash 44 |
| `Galas-256S` | keygen | 0.0% | 0.1% | drng 6 |
| `Galas-256S` | sign | 38% | 0.0% | pseudoXOF 1.12e+06, pseudohash 5.65e+03 |
| `Galas-256S` | verify | 50% | 0.0% | pseudoXOF 1.12e+06, pseudohash 30 |
| `Galas-384F` | keygen | 0.0% | 0.0% | drng 6 |
| `Galas-384F` | sign | 3.2% | 0.0% | pseudoXOF 2.3e+05, pseudohash 266 |
| `Galas-384F` | verify | 5.8% | 0.0% | pseudoXOF 2.29e+05, pseudohash 57 |
| `Galas-384S` | keygen | 0.0% | 0.0% | drng 6 |
| `Galas-384S` | sign | 25% | 0.0% | pseudoXOF 2.38e+06, pseudohash 3.04e+03 |
| `Galas-384S` | verify | 39% | 0.0% | pseudoXOF 2.37e+06, pseudohash 41 |
| `Galas-512F` | keygen | 0.0% | 0.0% | drng 6 |
| `Galas-512F` | sign | 2.3% | 0.0% | pseudoXOF 4.22e+05, pseudohash 178 |
| `Galas-512F` | verify | 4.4% | 0.0% | pseudoXOF 4.19e+05, pseudohash 73 |
| `Galas-512S` | keygen | 0.0% | 0.0% | drng 6 |
| `Galas-512S` | sign | 21% | 0.0% | pseudoXOF 4.64e+06, pseudohash 3.29e+03 |
| `Galas-512S` | verify | 34% | 0.0% | pseudoXOF 4.64e+06, pseudohash 51 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Galas-160F` | KAT log (sha256 `ce820a23f0eb5a0a…`) | `kat/sign-12/Galas-160F.log` |
| `Galas-160F` | timing keygen | `records/sign-12/Galas-160F__keygen.json` |
| `Galas-160F` | timing sign | `records/sign-12/Galas-160F__sign.json` |
| `Galas-160F` | timing verify | `records/sign-12/Galas-160F__verify.json` |
| `Galas-160F` | hash profile keygen | `profile/sign-12/Galas-160F__keygen.json` |
| `Galas-160F` | hash profile sign | `profile/sign-12/Galas-160F__sign.json` |
| `Galas-160F` | hash profile verify | `profile/sign-12/Galas-160F__verify.json` |
| `Galas-160S` | KAT log (sha256 `fe7595055ff70d19…`) | `kat/sign-12/Galas-160S.log` |
| `Galas-160S` | timing keygen | `records/sign-12/Galas-160S__keygen.json` |
| `Galas-160S` | timing sign | `records/sign-12/Galas-160S__sign.json` |
| `Galas-160S` | timing verify | `records/sign-12/Galas-160S__verify.json` |
| `Galas-160S` | hash profile keygen | `profile/sign-12/Galas-160S__keygen.json` |
| `Galas-160S` | hash profile sign | `profile/sign-12/Galas-160S__sign.json` |
| `Galas-160S` | hash profile verify | `profile/sign-12/Galas-160S__verify.json` |
| `Galas-256F` | KAT log (sha256 `047d8cb9dbadb4c6…`) | `kat/sign-12/Galas-256F.log` |
| `Galas-256F` | timing keygen | `records/sign-12/Galas-256F__keygen.json` |
| `Galas-256F` | timing sign | `records/sign-12/Galas-256F__sign.json` |
| `Galas-256F` | timing verify | `records/sign-12/Galas-256F__verify.json` |
| `Galas-256F` | hash profile keygen | `profile/sign-12/Galas-256F__keygen.json` |
| `Galas-256F` | hash profile sign | `profile/sign-12/Galas-256F__sign.json` |
| `Galas-256F` | hash profile verify | `profile/sign-12/Galas-256F__verify.json` |
| `Galas-256S` | KAT log (sha256 `5b3011221b86b9b0…`) | `kat/sign-12/Galas-256S.log` |
| `Galas-256S` | timing keygen | `records/sign-12/Galas-256S__keygen.json` |
| `Galas-256S` | timing sign | `records/sign-12/Galas-256S__sign.json` |
| `Galas-256S` | timing verify | `records/sign-12/Galas-256S__verify.json` |
| `Galas-256S` | hash profile keygen | `profile/sign-12/Galas-256S__keygen.json` |
| `Galas-256S` | hash profile sign | `profile/sign-12/Galas-256S__sign.json` |
| `Galas-256S` | hash profile verify | `profile/sign-12/Galas-256S__verify.json` |
| `Galas-384F` | KAT log (sha256 `119f01db3d3f3732…`) | `kat/sign-12/Galas-384F.log` |
| `Galas-384F` | timing keygen | `records/sign-12/Galas-384F__keygen.json` |
| `Galas-384F` | timing sign | `records/sign-12/Galas-384F__sign.json` |
| `Galas-384F` | timing verify | `records/sign-12/Galas-384F__verify.json` |
| `Galas-384F` | hash profile keygen | `profile/sign-12/Galas-384F__keygen.json` |
| `Galas-384F` | hash profile sign | `profile/sign-12/Galas-384F__sign.json` |
| `Galas-384F` | hash profile verify | `profile/sign-12/Galas-384F__verify.json` |
| `Galas-384S` | KAT log (sha256 `2d1a95e4e5f779f3…`) | `kat/sign-12/Galas-384S.log` |
| `Galas-384S` | timing keygen | `records/sign-12/Galas-384S__keygen.json` |
| `Galas-384S` | timing sign | `records/sign-12/Galas-384S__sign.json` |
| `Galas-384S` | timing verify | `records/sign-12/Galas-384S__verify.json` |
| `Galas-384S` | hash profile keygen | `profile/sign-12/Galas-384S__keygen.json` |
| `Galas-384S` | hash profile sign | `profile/sign-12/Galas-384S__sign.json` |
| `Galas-384S` | hash profile verify | `profile/sign-12/Galas-384S__verify.json` |
| `Galas-512F` | KAT log (sha256 `046f335400853e85…`) | `kat/sign-12/Galas-512F.log` |
| `Galas-512F` | timing keygen | `records/sign-12/Galas-512F__keygen.json` |
| `Galas-512F` | timing sign | `records/sign-12/Galas-512F__sign.json` |
| `Galas-512F` | timing verify | `records/sign-12/Galas-512F__verify.json` |
| `Galas-512F` | hash profile keygen | `profile/sign-12/Galas-512F__keygen.json` |
| `Galas-512F` | hash profile sign | `profile/sign-12/Galas-512F__sign.json` |
| `Galas-512F` | hash profile verify | `profile/sign-12/Galas-512F__verify.json` |
| `Galas-512S` | KAT log (sha256 `6fee394bdc94afbe…`) | `kat/sign-12/Galas-512S.log` |
| `Galas-512S` | timing keygen | `records/sign-12/Galas-512S__keygen.json` |
| `Galas-512S` | timing sign | `records/sign-12/Galas-512S__sign.json` |
| `Galas-512S` | timing verify | `records/sign-12/Galas-512S__verify.json` |
| `Galas-512S` | hash profile keygen | `profile/sign-12/Galas-512S__keygen.json` |
| `Galas-512S` | hash profile sign | `profile/sign-12/Galas-512S__sign.json` |
| `Galas-512S` | hash profile verify | `profile/sign-12/Galas-512S__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

