# kem-17 Hybrid Equivalent Punctured and Quasi-Cyclic — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-17` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560853532332032.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: Hybrid Equivalent Punctured and Quasi-Cyclic
- Implementation versions measured: reference
- Parameter sets: `hep-qc-1`, `hep-qc-3`, `hep-qc-5`, `hep-qc-7`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-17/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `hep-qc-1` | guide | PASS |
| `hep-qc-3` | guide | PASS |
| `hep-qc-5` | guide | PASS |
| `hep-qc-7` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `hep-qc-1` | keygen | 213.62 M | 79.3 ms | 12.6 | 79.3 ms | 100 (5 × 20) |
| `hep-qc-1` | enc | 11.24 M | 4.19 ms | 239 | 4.17 ms | 600 (5 × 120) |
| `hep-qc-1` | dec | 227.93 M | 84.6 ms | 11.8 | 84.6 ms | 100 (5 × 20) |
| `hep-qc-3` | keygen | 454.79 M | 169 ms | 5.93 | 169 ms | 100 (5 × 20) |
| `hep-qc-3` | enc | 33.33 M | 12.4 ms | 80.8 | 12.4 ms | 205 (5 × 41) |
| `hep-qc-3` | dec | 443.20 M | 164 ms | 6.08 | 164 ms | 100 (5 × 20) |
| `hep-qc-5` | keygen | 775.55 M | 288 ms | 3.47 | 288 ms | 100 (5 × 20) |
| `hep-qc-5` | enc | 75.81 M | 28.2 ms | 35.5 | 28.2 ms | 100 (5 × 20) |
| `hep-qc-5` | dec | 751.25 M | 279 ms | 3.59 | 276 ms | 100 (5 × 20) |
| `hep-qc-7` | keygen | 3.57 G | 1.32 s | 0.755 | 1.32 s | 100 (5 × 20) |
| `hep-qc-7` | enc | 501.64 M | 186 ms | 5.37 | 186 ms | 100 (5 × 20) |
| `hep-qc-7` | dec | 3.04 G | 1.13 s | 0.887 | 1.13 s | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `hep-qc-1` | keygen | 325320 | 1992 KiB | 5012 KiB |
| `hep-qc-1` | enc | 325320 | 4692 KiB | 37524 KiB |
| `hep-qc-1` | dec | 325320 | 5164 KiB | 11372 KiB |
| `hep-qc-3` | keygen | 905272 | 3136 KiB | 8804 KiB |
| `hep-qc-3` | enc | 905272 | 7052 KiB | 43980 KiB |
| `hep-qc-3` | dec | 905272 | 8836 KiB | 27332 KiB |
| `hep-qc-5` | keygen | 1896344 | 1464 KiB | 16600 KiB |
| `hep-qc-5` | enc | 1896344 | 11052 KiB | 52548 KiB |
| `hep-qc-5` | dec | 1896344 | 14472 KiB | 52180 KiB |
| `hep-qc-7` | keygen | 12729648 | 3500 KiB | 91248 KiB |
| `hep-qc-7` | enc | 12729648 | 54456 KiB | 312844 KiB |
| `hep-qc-7` | dec | 12729648 | 79036 KiB | 338324 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `hep-qc-1` | 285889 | 285969 | 4433 | 32 |
| `hep-qc-3` | 866210 | 866298 | 8978 | 32 |
| `hep-qc-5` | 1852485 | 1852581 | 14421 | 32 |
| `hep-qc-7` | 12644449 | 12644577 | 49297 | 32 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — never calls the DRNG: zero-initialised PRNG (known kem-17-1/-3)

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `hep-qc-1` | keygen | 90% | 0.0% | pseudoXOF 3.67e+04, pseudohash 1 |
| `hep-qc-1` | enc | 55% | 0.0% | pseudoXOF 14, pseudohash 1, sm3hash 1 |
| `hep-qc-1` | dec | 90% | 0.0% | pseudoXOF 3.83e+04, pseudohash 1, sm3hash 4 |
| `hep-qc-3` | keygen | 85% | 0.0% | pseudoXOF 7.32e+04, pseudohash 1 |
| `hep-qc-3` | enc | 54% | 0.0% | pseudoXOF 14, pseudohash 1, sm3hash 1 |
| `hep-qc-3` | dec | 90% | 0.0% | pseudoXOF 7.3e+04, pseudohash 1, sm3hash 4 |
| `hep-qc-5` | keygen | 80% | 0.0% | pseudoXOF 1.17e+05, pseudohash 1 |
| `hep-qc-5` | enc | 50% | 0.0% | pseudoXOF 14, pseudohash 1, sm3hash 1 |
| `hep-qc-5` | dec | 87% | 0.0% | pseudoXOF 1.16e+05, pseudohash 1, sm3hash 4 |
| `hep-qc-7` | keygen | 55% | 0.0% | pseudoXOF 4e+05, pseudohash 1 |
| `hep-qc-7` | enc | 51% | 0.0% | pseudoXOF 14, pseudohash 1, sm3hash 1 |
| `hep-qc-7` | dec | 77% | 0.0% | pseudoXOF 3.97e+05, pseudohash 1, sm3hash 4 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `hep-qc-1` | KAT log (sha256 `6eea8465b3c7593f…`) | `kat/kem-17/hep-qc-1.log` |
| `hep-qc-1` | timing dec | `records/kem-17/hep-qc-1__dec.json` |
| `hep-qc-1` | timing enc | `records/kem-17/hep-qc-1__enc.json` |
| `hep-qc-1` | timing keygen | `records/kem-17/hep-qc-1__keygen.json` |
| `hep-qc-1` | hash profile dec | `profile/kem-17/hep-qc-1__dec.json` |
| `hep-qc-1` | hash profile enc | `profile/kem-17/hep-qc-1__enc.json` |
| `hep-qc-1` | hash profile keygen | `profile/kem-17/hep-qc-1__keygen.json` |
| `hep-qc-3` | KAT log (sha256 `a798e2f658d341fd…`) | `kat/kem-17/hep-qc-3.log` |
| `hep-qc-3` | timing dec | `records/kem-17/hep-qc-3__dec.json` |
| `hep-qc-3` | timing enc | `records/kem-17/hep-qc-3__enc.json` |
| `hep-qc-3` | timing keygen | `records/kem-17/hep-qc-3__keygen.json` |
| `hep-qc-3` | hash profile dec | `profile/kem-17/hep-qc-3__dec.json` |
| `hep-qc-3` | hash profile enc | `profile/kem-17/hep-qc-3__enc.json` |
| `hep-qc-3` | hash profile keygen | `profile/kem-17/hep-qc-3__keygen.json` |
| `hep-qc-5` | KAT log (sha256 `73aad6621f7563f6…`) | `kat/kem-17/hep-qc-5.log` |
| `hep-qc-5` | timing dec | `records/kem-17/hep-qc-5__dec.json` |
| `hep-qc-5` | timing enc | `records/kem-17/hep-qc-5__enc.json` |
| `hep-qc-5` | timing keygen | `records/kem-17/hep-qc-5__keygen.json` |
| `hep-qc-5` | hash profile dec | `profile/kem-17/hep-qc-5__dec.json` |
| `hep-qc-5` | hash profile enc | `profile/kem-17/hep-qc-5__enc.json` |
| `hep-qc-5` | hash profile keygen | `profile/kem-17/hep-qc-5__keygen.json` |
| `hep-qc-7` | KAT log (sha256 `f96dd5e83553d464…`) | `kat/kem-17/hep-qc-7.log` |
| `hep-qc-7` | timing dec | `records/kem-17/hep-qc-7__dec.json` |
| `hep-qc-7` | timing enc | `records/kem-17/hep-qc-7__enc.json` |
| `hep-qc-7` | timing keygen | `records/kem-17/hep-qc-7__keygen.json` |
| `hep-qc-7` | hash profile dec | `profile/kem-17/hep-qc-7__dec.json` |
| `hep-qc-7` | hash profile enc | `profile/kem-17/hep-qc-7__enc.json` |
| `hep-qc-7` | hash profile keygen | `profile/kem-17/hep-qc-7__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

