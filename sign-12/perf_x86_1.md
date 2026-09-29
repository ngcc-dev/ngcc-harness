# sign-12 Galas Signature Scheme — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-12` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561077713686528.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: Galas Signature Scheme
- Implementation versions measured: reference
- Parameter sets: `Galas-160F`, `Galas-160S`, `Galas-256F`, `Galas-256S`, `Galas-384F`, `Galas-384S`, `Galas-512F`, `Galas-512S`

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
| `Galas-512S` | harness-default | TIMEOUT [1] |

[1] API deviation: sig_keygen does not draw from drng_algorithm but seeds a private DRNG from 32 zero bytes (or from a seed set by a non-API helper used only by the submitters' own KAT generator), so the official KAT flow yields different keys; with keygen drawing from the DRNG the first KAT records reproduce exactly (sign-12/Makefile). Galas-512S additionally exceeds the 900 s KAT time limit. Timed anyway; note that key generation always produces the same key pair. These instances are timed anyway; their output is not validated.

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Galas-160F` | keygen | 22.14 M | 10.6 ms | 94.3 | 10.6 ms | 475 (5 × 95) |
| `Galas-160F` | sign | 1.88 G | 901 ms | 1.11 | 903 ms | 100 (5 × 20) |
| `Galas-160F` | verify | 1.20 G | 571 ms | 1.75 | 571 ms | 100 (5 × 20) |
| `Galas-160S` | keygen | 22.13 M | 10.6 ms | 94.3 | 10.6 ms | 475 (5 × 95) |
| `Galas-160S` | sign | 2.39 G | 1.14 s | 0.874 | 1.15 s | 100 (5 × 20) |
| `Galas-160S` | verify | 1.68 G | 805 ms | 1.24 | 802 ms | 100 (5 × 20) |
| `Galas-256F` | keygen | 57.79 M | 27.7 ms | 36.1 | 27.7 ms | 185 (5 × 37) |
| `Galas-256F` | sign | 7.73 G | 3.7 s | 0.271 | 3.69 s | 100 (5 × 20) |
| `Galas-256F` | verify | 4.89 G | 2.34 s | 0.427 | 2.34 s | 100 (5 × 20) |
| `Galas-256S` | keygen | 57.82 M | 27.7 ms | 36.1 | 27.7 ms | 185 (5 × 37) |
| `Galas-256S` | sign | 12.12 G | 5.82 s | 0.172 | 5.82 s | 100 (5 × 20) |
| `Galas-256S` | verify | 9.06 G | 4.33 s | 0.231 | 4.33 s | 100 (5 × 20) |
| `Galas-384F` | keygen | 136.91 M | 65.5 ms | 15.3 | 65.5 ms | 100 (5 × 20) |
| `Galas-384F` | sign | 29.61 G | 14.2 s | 0.0704 | 14.2 s | 60 (5 × 12) |
| `Galas-384F` | verify | 17.90 G | 8.59 s | 0.116 | 8.59 s | 100 (5 × 20) |
| `Galas-384S` | keygen | 137.02 M | 65.5 ms | 15.3 | 65.5 ms | 100 (5 × 20) |
| `Galas-384S` | sign | 38.44 G | 18.4 s | 0.0544 | 18.4 s | 50 (5 × 10) |
| `Galas-384S` | verify | 26.58 G | 12.7 s | 0.0785 | 12.7 s | 70 (5 × 14) |
| `Galas-512F` | keygen | 254.11 M | 121 ms | 8.23 | 121 ms | 100 (5 × 20) |
| `Galas-512F` | sign | 75.51 G | 36.2 s | 0.0276 | 35.9 s | 25 (5 × 5) |
| `Galas-512F` | verify | 44.76 G | 21.5 s | 0.0466 | 21.5 s | 40 (5 × 8) |
| `Galas-512S` | keygen | 254.15 M | 121 ms | 8.23 | 121 ms | 100 (5 × 20) |
| `Galas-512S` | sign | 92.54 G | 44.4 s | 0.0225 | 44 s | 20 (5 × 4) |
| `Galas-512S` | verify | 61.40 G | 29.4 s | 0.034 | 29.4 s | 30 (5 × 6) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Galas-160F` | keygen | – | 1920 KiB | 2076 KiB |
| `Galas-160F` | sign | – | 1968 KiB | 5740 KiB |
| `Galas-160F` | verify | – | 4904 KiB | 5448 KiB |
| `Galas-160S` | keygen | – | 1896 KiB | 2024 KiB |
| `Galas-160S` | sign | – | 1960 KiB | 8068 KiB |
| `Galas-160S` | verify | – | 5280 KiB | 7636 KiB |
| `Galas-256F` | keygen | – | 1924 KiB | 2096 KiB |
| `Galas-256F` | sign | – | 2040 KiB | 12968 KiB |
| `Galas-256F` | verify | – | 11448 KiB | 12712 KiB |
| `Galas-256S` | keygen | – | 1908 KiB | 2100 KiB |
| `Galas-256S` | sign | – | 2032 KiB | 25216 KiB |
| `Galas-256S` | verify | – | 12216 KiB | 24456 KiB |
| `Galas-384F` | keygen | – | 1916 KiB | 2172 KiB |
| `Galas-384F` | sign | – | 2140 KiB | 37536 KiB |
| `Galas-384F` | verify | – | 33688 KiB | 37372 KiB |
| `Galas-384S` | keygen | – | 1916 KiB | 2196 KiB |
| `Galas-384S` | sign | – | 2116 KiB | 63708 KiB |
| `Galas-384S` | verify | – | 35868 KiB | 63552 KiB |
| `Galas-512F` | keygen | – | 1976 KiB | 2236 KiB |
| `Galas-512F` | sign | – | 2100 KiB | 82948 KiB |
| `Galas-512F` | verify | – | 76224 KiB | 82596 KiB |
| `Galas-512S` | keygen | – | 1928 KiB | 2200 KiB |
| `Galas-512S` | sign | – | 2048 KiB | 132640 KiB |
| `Galas-512S` | verify | – | 78712 KiB | 132376 KiB |

## 6. Transmission and storage overhead

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
| `Galas-160F` | sign | 4.1% | 0.0% | pseudoXOF 3.69e+04, pseudohash 102 |
| `Galas-160F` | verify | 6.3% | 0.0% | pseudoXOF 3.65e+04, pseudohash 28 |
| `Galas-160S` | keygen | 0.0% | 0.1% | drng 6 |
| `Galas-160S` | sign | 23% | 0.0% | pseudoXOF 2.49e+05, pseudohash 1.85e+03 |
| `Galas-160S` | verify | 30% | 0.0% | pseudoXOF 2.49e+05, pseudohash 22 |
| `Galas-256F` | keygen | 0.0% | 0.0% | drng 6 |
| `Galas-256F` | sign | 3.1% | 0.0% | pseudoXOF 6.18e+04, pseudohash 297 |
| `Galas-256F` | verify | 4.7% | 0.0% | pseudoXOF 6.11e+04, pseudohash 44 |
| `Galas-256S` | keygen | 0.0% | 0.0% | drng 6 |
| `Galas-256S` | sign | 33% | 0.0% | pseudoXOF 1.12e+06, pseudohash 5.65e+03 |
| `Galas-256S` | verify | 46% | 0.0% | pseudoXOF 1.12e+06, pseudohash 30 |
| `Galas-384F` | keygen | 0.0% | 0.0% | drng 6 |
| `Galas-384F` | sign | 2.8% | 0.0% | pseudoXOF 2.3e+05, pseudohash 266 |
| `Galas-384F` | verify | 4.9% | 0.0% | pseudoXOF 2.29e+05, pseudohash 57 |
| `Galas-384S` | keygen | 0.0% | 0.0% | drng 6 |
| `Galas-384S` | sign | 23% | 0.0% | pseudoXOF 2.38e+06, pseudohash 3.04e+03 |
| `Galas-384S` | verify | 34% | 0.0% | pseudoXOF 2.37e+06, pseudohash 41 |
| `Galas-512F` | keygen | 0.0% | 0.0% | drng 6 |
| `Galas-512F` | sign | 2.0% | 0.0% | pseudoXOF 4.22e+05, pseudohash 178 |
| `Galas-512F` | verify | 3.5% | 0.0% | pseudoXOF 4.19e+05, pseudohash 73 |
| `Galas-512S` | keygen | 0.0% | 0.0% | drng 6 |
| `Galas-512S` | sign | 18% | 0.0% | pseudoXOF 4.64e+06, pseudohash 3.29e+03 |
| `Galas-512S` | verify | 28% | 0.0% | pseudoXOF 4.64e+06, pseudohash 51 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Galas-160F` | KAT log (sha256 `d390f96532059a82…`) | `kat/sign-12/Galas-160F.log` |
| `Galas-160F` | timing keygen | `records/sign-12/Galas-160F__keygen.json` |
| `Galas-160F` | timing sign | `records/sign-12/Galas-160F__sign.json` |
| `Galas-160F` | timing verify | `records/sign-12/Galas-160F__verify.json` |
| `Galas-160F` | hash profile keygen | `profile/sign-12/Galas-160F__keygen.json` |
| `Galas-160F` | hash profile sign | `profile/sign-12/Galas-160F__sign.json` |
| `Galas-160F` | hash profile verify | `profile/sign-12/Galas-160F__verify.json` |
| `Galas-160S` | KAT log (sha256 `422e3efbdef6caed…`) | `kat/sign-12/Galas-160S.log` |
| `Galas-160S` | timing keygen | `records/sign-12/Galas-160S__keygen.json` |
| `Galas-160S` | timing sign | `records/sign-12/Galas-160S__sign.json` |
| `Galas-160S` | timing verify | `records/sign-12/Galas-160S__verify.json` |
| `Galas-160S` | hash profile keygen | `profile/sign-12/Galas-160S__keygen.json` |
| `Galas-160S` | hash profile sign | `profile/sign-12/Galas-160S__sign.json` |
| `Galas-160S` | hash profile verify | `profile/sign-12/Galas-160S__verify.json` |
| `Galas-256F` | KAT log (sha256 `0f05989039c3406e…`) | `kat/sign-12/Galas-256F.log` |
| `Galas-256F` | timing keygen | `records/sign-12/Galas-256F__keygen.json` |
| `Galas-256F` | timing sign | `records/sign-12/Galas-256F__sign.json` |
| `Galas-256F` | timing verify | `records/sign-12/Galas-256F__verify.json` |
| `Galas-256F` | hash profile keygen | `profile/sign-12/Galas-256F__keygen.json` |
| `Galas-256F` | hash profile sign | `profile/sign-12/Galas-256F__sign.json` |
| `Galas-256F` | hash profile verify | `profile/sign-12/Galas-256F__verify.json` |
| `Galas-256S` | KAT log (sha256 `7c3471c83d5a206f…`) | `kat/sign-12/Galas-256S.log` |
| `Galas-256S` | timing keygen | `records/sign-12/Galas-256S__keygen.json` |
| `Galas-256S` | timing sign | `records/sign-12/Galas-256S__sign.json` |
| `Galas-256S` | timing verify | `records/sign-12/Galas-256S__verify.json` |
| `Galas-256S` | hash profile keygen | `profile/sign-12/Galas-256S__keygen.json` |
| `Galas-256S` | hash profile sign | `profile/sign-12/Galas-256S__sign.json` |
| `Galas-256S` | hash profile verify | `profile/sign-12/Galas-256S__verify.json` |
| `Galas-384F` | KAT log (sha256 `b71d473d8c7862ac…`) | `kat/sign-12/Galas-384F.log` |
| `Galas-384F` | timing keygen | `records/sign-12/Galas-384F__keygen.json` |
| `Galas-384F` | timing sign | `records/sign-12/Galas-384F__sign.json` |
| `Galas-384F` | timing verify | `records/sign-12/Galas-384F__verify.json` |
| `Galas-384F` | hash profile keygen | `profile/sign-12/Galas-384F__keygen.json` |
| `Galas-384F` | hash profile sign | `profile/sign-12/Galas-384F__sign.json` |
| `Galas-384F` | hash profile verify | `profile/sign-12/Galas-384F__verify.json` |
| `Galas-384S` | KAT log (sha256 `137d78ac3c2056c1…`) | `kat/sign-12/Galas-384S.log` |
| `Galas-384S` | timing keygen | `records/sign-12/Galas-384S__keygen.json` |
| `Galas-384S` | timing sign | `records/sign-12/Galas-384S__sign.json` |
| `Galas-384S` | timing verify | `records/sign-12/Galas-384S__verify.json` |
| `Galas-384S` | hash profile keygen | `profile/sign-12/Galas-384S__keygen.json` |
| `Galas-384S` | hash profile sign | `profile/sign-12/Galas-384S__sign.json` |
| `Galas-384S` | hash profile verify | `profile/sign-12/Galas-384S__verify.json` |
| `Galas-512F` | KAT log (sha256 `b6b80fc45a83d57d…`) | `kat/sign-12/Galas-512F.log` |
| `Galas-512F` | timing keygen | `records/sign-12/Galas-512F__keygen.json` |
| `Galas-512F` | timing sign | `records/sign-12/Galas-512F__sign.json` |
| `Galas-512F` | timing verify | `records/sign-12/Galas-512F__verify.json` |
| `Galas-512F` | hash profile keygen | `profile/sign-12/Galas-512F__keygen.json` |
| `Galas-512F` | hash profile sign | `profile/sign-12/Galas-512F__sign.json` |
| `Galas-512F` | hash profile verify | `profile/sign-12/Galas-512F__verify.json` |
| `Galas-512S` | KAT log (sha256 `d0cc58f59ba98ad8…`) | `kat/sign-12/Galas-512S.log` |
| `Galas-512S` | timing keygen | `records/sign-12/Galas-512S__keygen.json` |
| `Galas-512S` | timing sign | `records/sign-12/Galas-512S__sign.json` |
| `Galas-512S` | timing verify | `records/sign-12/Galas-512S__verify.json` |
| `Galas-512S` | hash profile keygen | `profile/sign-12/Galas-512S__keygen.json` |
| `Galas-512S` | hash profile sign | `profile/sign-12/Galas-512S__sign.json` |
| `Galas-512S` | hash profile verify | `profile/sign-12/Galas-512S__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

