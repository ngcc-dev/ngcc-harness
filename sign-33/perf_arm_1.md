# sign-33 VDOO: Vinegar-Diagonal-Oil-Oil — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-33` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561114325766144.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: VDOO: Vinegar-Diagonal-Oil-Oil
- Implementation versions measured: reference
- Parameter sets: `vdoo_128`, `vdoo_256`, `vdoo_512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-33/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `vdoo_128` | guide | PASS |
| `vdoo_256` | guide | PASS |
| `vdoo_512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `vdoo_128` | keygen | 962.71 M | 357 ms | 2.8 | 357 ms | 100 (5 × 20) |
| `vdoo_128` | sign | 180.66 M | 67 ms | 14.9 | 67 ms | 415 (5 × 83) |
| `vdoo_128` | verify | 1.80 M | 668 µs | 1.5e+03 | 668 µs | 4715 (5 × 943) |
| `vdoo_256` | keygen | 19.99 G | 7.42 s | 0.135 | 7.42 s | 50 (5 × 10) |
| `vdoo_256` | sign | 194.82 M | 72.3 ms | 13.8 | 72.3 ms | 100 (5 × 20) |
| `vdoo_256` | verify | 1.99 M | 738 µs | 1.35e+03 | 740 µs | 4175 (5 × 835) |
| `vdoo_512` | keygen | 128.85 G | 47.8 s | 0.0209 | 47.8 s | 5 (5 × 1) |
| `vdoo_512` | sign | 1.33 G | 492 ms | 2.03 | 490 ms | 100 (5 × 20) |
| `vdoo_512` | verify | 4.17 M | 1.55 ms | 646 | 1.54 ms | 2040 (5 × 408) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `vdoo_128` | keygen | 2018972 | 3448 KiB | 5496 KiB |
| `vdoo_128` | sign | 2018972 | 6120 KiB | 6184 KiB |
| `vdoo_128` | verify | 2018972 | 4076 KiB | 4140 KiB |
| `vdoo_256` | keygen | 25734420 | 1456 KiB | 35432 KiB |
| `vdoo_256` | sign | 25734420 | 35372 KiB | 35440 KiB |
| `vdoo_256` | verify | 25734420 | 35372 KiB | 35436 KiB |
| `vdoo_512` | keygen | 121254552 | 1456 KiB | 160500 KiB |
| `vdoo_512` | sign | 121254552 | 160436 KiB | 160504 KiB |
| `vdoo_512` | verify | 121254552 | 160440 KiB | 160504 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `vdoo_128` | 330855 | 342790 | 85 |
| `vdoo_256` | 4289250 | 4388307 | 316 |
| `vdoo_512` | 20229300 | 20474382 | 471 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `vdoo_128` | keygen | 0.0% | 12% | drng 2.39e+04 |
| `vdoo_128` | sign | 0.0% | 12% | drng 5.82e+03, pseudoXOF 2.48 |
| `vdoo_128` | verify | 0.3% | 0.0% | pseudoXOF 2 |
| `vdoo_256` | keygen | 0.0% | 3.0% | drng 9.94e+04 |
| `vdoo_256` | sign | 0.0% | 0.6% | drng 265, pseudoXOF 2 |
| `vdoo_256` | verify | 0.4% | 0.0% | pseudoXOF 2 |
| `vdoo_512` | keygen | 0.0% | 1.5% | drng 2.46e+05 |
| `vdoo_512` | sign | 0.0% | 0.2% | drng 434, pseudoXOF 2 |
| `vdoo_512` | verify | 0.3% | 0.0% | pseudoXOF 2 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `vdoo_128` | KAT log (sha256 `bb9199e815c8c0d2…`) | `kat/sign-33/vdoo_128.log` |
| `vdoo_128` | timing keygen | `records/sign-33/vdoo_128__keygen.json` |
| `vdoo_128` | timing sign | `records/sign-33/vdoo_128__sign.json` |
| `vdoo_128` | timing verify | `records/sign-33/vdoo_128__verify.json` |
| `vdoo_128` | hash profile keygen | `profile/sign-33/vdoo_128__keygen.json` |
| `vdoo_128` | hash profile sign | `profile/sign-33/vdoo_128__sign.json` |
| `vdoo_128` | hash profile verify | `profile/sign-33/vdoo_128__verify.json` |
| `vdoo_256` | KAT log (sha256 `a4aca7ccba685c77…`) | `kat/sign-33/vdoo_256.log` |
| `vdoo_256` | timing keygen | `records/sign-33/vdoo_256__keygen.json` |
| `vdoo_256` | timing sign | `records/sign-33/vdoo_256__sign.json` |
| `vdoo_256` | timing verify | `records/sign-33/vdoo_256__verify.json` |
| `vdoo_256` | hash profile keygen | `profile/sign-33/vdoo_256__keygen.json` |
| `vdoo_256` | hash profile sign | `profile/sign-33/vdoo_256__sign.json` |
| `vdoo_256` | hash profile verify | `profile/sign-33/vdoo_256__verify.json` |
| `vdoo_512` | KAT log (sha256 `87c98ccf7a324bd2…`) | `kat/sign-33/vdoo_512.log` |
| `vdoo_512` | timing keygen | `records/sign-33/vdoo_512__keygen.json` |
| `vdoo_512` | timing sign | `records/sign-33/vdoo_512__sign.json` |
| `vdoo_512` | timing verify | `records/sign-33/vdoo_512__verify.json` |
| `vdoo_512` | hash profile keygen | `profile/sign-33/vdoo_512__keygen.json` |
| `vdoo_512` | hash profile sign | `profile/sign-33/vdoo_512__sign.json` |
| `vdoo_512` | hash profile verify | `profile/sign-33/vdoo_512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

