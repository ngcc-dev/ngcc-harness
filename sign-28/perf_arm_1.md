# sign-28 SYDO — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-28` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561105194766336.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: SYDO
- Implementation versions measured: reference
- Parameter sets: `sydo_160f`, `sydo_160s`, `sydo_256f`, `sydo_256s`, `sydo_512f`, `sydo_512s`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-28/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `sydo_160f` | guide | PASS |
| `sydo_160s` | guide | PASS |
| `sydo_256f` | guide | PASS |
| `sydo_256s` | guide | PASS |
| `sydo_512f` | guide | PASS |
| `sydo_512s` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `sydo_160f` | keygen | 784.4 k | 291 µs | 3.44e+03 | 291 µs | 10375 (5 × 2075) |
| `sydo_160f` | sign | 265.41 M | 98.5 ms | 10.2 | 98.4 ms | 100 (5 × 20) |
| `sydo_160f` | verify | 246.41 M | 91.5 ms | 10.9 | 91.4 ms | 100 (5 × 20) |
| `sydo_160s` | keygen | 782.9 k | 291 µs | 3.44e+03 | 291 µs | 10360 (5 × 2072) |
| `sydo_160s` | sign | 684.75 M | 254 ms | 3.94 | 253 ms | 100 (5 × 20) |
| `sydo_160s` | verify | 649.73 M | 241 ms | 4.15 | 241 ms | 100 (5 × 20) |
| `sydo_256f` | keygen | 1.37 M | 509 µs | 1.96e+03 | 509 µs | 6030 (5 × 1206) |
| `sydo_256f` | sign | 564.11 M | 209 ms | 4.78 | 209 ms | 100 (5 × 20) |
| `sydo_256f` | verify | 534.40 M | 198 ms | 5.04 | 199 ms | 100 (5 × 20) |
| `sydo_256s` | keygen | 1.37 M | 510 µs | 1.96e+03 | 509 µs | 6040 (5 × 1208) |
| `sydo_256s` | sign | 1.48 G | 549 ms | 1.82 | 548 ms | 100 (5 × 20) |
| `sydo_256s` | verify | 1.45 G | 537 ms | 1.86 | 537 ms | 100 (5 × 20) |
| `sydo_512f` | keygen | 665.9 k | 247 µs | 4.05e+03 | 247 µs | 11855 (5 × 2371) |
| `sydo_512f` | sign | 1.60 G | 594 ms | 1.68 | 593 ms | 100 (5 × 20) |
| `sydo_512f` | verify | 1.52 G | 564 ms | 1.77 | 564 ms | 100 (5 × 20) |
| `sydo_512s` | keygen | 669.9 k | 249 µs | 4.02e+03 | 249 µs | 12235 (5 × 2447) |
| `sydo_512s` | sign | 3.71 G | 1.38 s | 0.726 | 1.38 s | 100 (5 × 20) |
| `sydo_512s` | verify | 3.53 G | 1.31 s | 0.763 | 1.31 s | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `sydo_160f` | keygen | 255640 | 1520 KiB | 1588 KiB |
| `sydo_160f` | sign | 255640 | 1524 KiB | 5368 KiB |
| `sydo_160f` | verify | 255640 | 3068 KiB | 5476 KiB |
| `sydo_160s` | keygen | 255640 | 1520 KiB | 1588 KiB |
| `sydo_160s` | sign | 255640 | 1524 KiB | 7876 KiB |
| `sydo_160s` | verify | 255640 | 3648 KiB | 7320 KiB |
| `sydo_256f` | keygen | 255640 | 1532 KiB | 1600 KiB |
| `sydo_256f` | sign | 255640 | 1536 KiB | 8064 KiB |
| `sydo_256f` | verify | 255640 | 4588 KiB | 7028 KiB |
| `sydo_256s` | keygen | 255640 | 1528 KiB | 1596 KiB |
| `sydo_256s` | sign | 255640 | 1532 KiB | 13180 KiB |
| `sydo_256s` | verify | 255640 | 3924 KiB | 12508 KiB |
| `sydo_512f` | keygen | 255640 | 1580 KiB | 1644 KiB |
| `sydo_512f` | sign | 255640 | 1580 KiB | 19964 KiB |
| `sydo_512f` | verify | 255640 | 2660 KiB | 17952 KiB |
| `sydo_512s` | keygen | 255640 | 1568 KiB | 1636 KiB |
| `sydo_512s` | sign | 255640 | 1572 KiB | 43580 KiB |
| `sydo_512s` | verify | 255640 | 4636 KiB | 43032 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | signature |
|---|---|---|---|
| `sydo_160f` | 80 | 174 | 6724 |
| `sydo_160s` | 80 | 174 | 5428 |
| `sydo_256f` | 128 | 278 | 17604 |
| `sydo_256s` | 128 | 278 | 14444 |
| `sydo_512f` | 246 | 534 | 67716 |
| `sydo_512s` | 246 | 534 | 56672 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **mixed** — hashing via pseudoXOF; PRGs via own AES/Rijndael and a BLAKE2s-round cipher

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `sydo_160f` | keygen | 1.5% | 0.7% | drng 1, pseudoXOF 8 |
| `sydo_160f` | sign | 3.3% | 0.0% | drng 1, pseudoXOF 35.5 |
| `sydo_160f` | verify | 3.6% | 0.0% | pseudoXOF 25 |
| `sydo_160s` | keygen | 1.5% | 0.7% | drng 1, pseudoXOF 8 |
| `sydo_160s` | sign | 8.6% | 0.0% | drng 1, pseudoXOF 2.35e+03 |
| `sydo_160s` | verify | 7.1% | 0.0% | pseudoXOF 19 |
| `sydo_256f` | keygen | 1.3% | 0.4% | drng 1, pseudoXOF 12 |
| `sydo_256f` | sign | 4.1% | 0.0% | drng 1, pseudoXOF 50 |
| `sydo_256f` | verify | 4.3% | 0.0% | pseudoXOF 37 |
| `sydo_256s` | keygen | 1.3% | 0.4% | drng 1, pseudoXOF 12 |
| `sydo_256s` | sign | 8.4% | 0.0% | drng 1, pseudoXOF 145 |
| `sydo_256s` | verify | 8.5% | 0.0% | pseudoXOF 28 |
| `sydo_512f` | keygen | 10% | 1.3% | drng 1, pseudoXOF 23 |
| `sydo_512f` | sign | 11% | 0.0% | drng 1, pseudoXOF 97.3 |
| `sydo_512f` | verify | 12% | 0.0% | pseudoXOF 69 |
| `sydo_512s` | keygen | 10% | 1.3% | drng 1, pseudoXOF 23 |
| `sydo_512s` | sign | 28% | 0.0% | drng 1, pseudoXOF 2.98e+03 |
| `sydo_512s` | verify | 28% | 0.0% | pseudoXOF 51 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `sydo_160f` | KAT log (sha256 `416ceeead1cb94ff…`) | `kat/sign-28/sydo_160f.log` |
| `sydo_160f` | timing keygen | `records/sign-28/sydo_160f__keygen.json` |
| `sydo_160f` | timing sign | `records/sign-28/sydo_160f__sign.json` |
| `sydo_160f` | timing verify | `records/sign-28/sydo_160f__verify.json` |
| `sydo_160f` | hash profile keygen | `profile/sign-28/sydo_160f__keygen.json` |
| `sydo_160f` | hash profile sign | `profile/sign-28/sydo_160f__sign.json` |
| `sydo_160f` | hash profile verify | `profile/sign-28/sydo_160f__verify.json` |
| `sydo_160s` | KAT log (sha256 `5879a813ce53a187…`) | `kat/sign-28/sydo_160s.log` |
| `sydo_160s` | timing keygen | `records/sign-28/sydo_160s__keygen.json` |
| `sydo_160s` | timing sign | `records/sign-28/sydo_160s__sign.json` |
| `sydo_160s` | timing verify | `records/sign-28/sydo_160s__verify.json` |
| `sydo_160s` | hash profile keygen | `profile/sign-28/sydo_160s__keygen.json` |
| `sydo_160s` | hash profile sign | `profile/sign-28/sydo_160s__sign.json` |
| `sydo_160s` | hash profile verify | `profile/sign-28/sydo_160s__verify.json` |
| `sydo_256f` | KAT log (sha256 `bbbac8ac3fc50d70…`) | `kat/sign-28/sydo_256f.log` |
| `sydo_256f` | timing keygen | `records/sign-28/sydo_256f__keygen.json` |
| `sydo_256f` | timing sign | `records/sign-28/sydo_256f__sign.json` |
| `sydo_256f` | timing verify | `records/sign-28/sydo_256f__verify.json` |
| `sydo_256f` | hash profile keygen | `profile/sign-28/sydo_256f__keygen.json` |
| `sydo_256f` | hash profile sign | `profile/sign-28/sydo_256f__sign.json` |
| `sydo_256f` | hash profile verify | `profile/sign-28/sydo_256f__verify.json` |
| `sydo_256s` | KAT log (sha256 `45b9fe4e302b799c…`) | `kat/sign-28/sydo_256s.log` |
| `sydo_256s` | timing keygen | `records/sign-28/sydo_256s__keygen.json` |
| `sydo_256s` | timing sign | `records/sign-28/sydo_256s__sign.json` |
| `sydo_256s` | timing verify | `records/sign-28/sydo_256s__verify.json` |
| `sydo_256s` | hash profile keygen | `profile/sign-28/sydo_256s__keygen.json` |
| `sydo_256s` | hash profile sign | `profile/sign-28/sydo_256s__sign.json` |
| `sydo_256s` | hash profile verify | `profile/sign-28/sydo_256s__verify.json` |
| `sydo_512f` | KAT log (sha256 `814255a38b7e7498…`) | `kat/sign-28/sydo_512f.log` |
| `sydo_512f` | timing keygen | `records/sign-28/sydo_512f__keygen.json` |
| `sydo_512f` | timing sign | `records/sign-28/sydo_512f__sign.json` |
| `sydo_512f` | timing verify | `records/sign-28/sydo_512f__verify.json` |
| `sydo_512f` | hash profile keygen | `profile/sign-28/sydo_512f__keygen.json` |
| `sydo_512f` | hash profile sign | `profile/sign-28/sydo_512f__sign.json` |
| `sydo_512f` | hash profile verify | `profile/sign-28/sydo_512f__verify.json` |
| `sydo_512s` | KAT log (sha256 `ff3fed828a4fe83a…`) | `kat/sign-28/sydo_512s.log` |
| `sydo_512s` | timing keygen | `records/sign-28/sydo_512s__keygen.json` |
| `sydo_512s` | timing sign | `records/sign-28/sydo_512s__sign.json` |
| `sydo_512s` | timing verify | `records/sign-28/sydo_512s__verify.json` |
| `sydo_512s` | hash profile keygen | `profile/sign-28/sydo_512s__keygen.json` |
| `sydo_512s` | hash profile sign | `profile/sign-28/sydo_512s__sign.json` |
| `sydo_512s` | hash profile verify | `profile/sign-28/sydo_512s__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

