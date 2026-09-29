# sign-01 Aigis-Sig+ — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-01` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101567126852161536.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: Aigis-Sig+
- Implementation versions measured: reference
- Parameter sets: `Aigis-sig1`, `Aigis-sig2`, `Aigis-sig3`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-01/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Aigis-sig1` | guide | PASS |
| `Aigis-sig2` | guide | PASS |
| `Aigis-sig3` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Aigis-sig1` | keygen | 564.0 k | 209 µs | 4.78e+03 | 208 µs | 13890 (5 × 2778) |
| `Aigis-sig1` | sign | 9.75 M | 3.62 ms | 277 | 3.61 ms | 845 (5 × 169) |
| `Aigis-sig1` | verify | 494.1 k | 183 µs | 5.45e+03 | 183 µs | 17070 (5 × 3414) |
| `Aigis-sig2` | keygen | 1.78 M | 659 µs | 1.52e+03 | 654 µs | 4630 (5 × 926) |
| `Aigis-sig2` | sign | 7.50 M | 2.78 ms | 359 | 2.79 ms | 1120 (5 × 224) |
| `Aigis-sig2` | verify | 1.68 M | 623 µs | 1.61e+03 | 618 µs | 5085 (5 × 1017) |
| `Aigis-sig3` | keygen | 5.90 M | 2.19 ms | 457 | 2.19 ms | 1395 (5 × 279) |
| `Aigis-sig3` | sign | 8.31 M | 3.08 ms | 324 | 3.07 ms | 1005 (5 × 201) |
| `Aigis-sig3` | verify | 5.80 M | 2.15 ms | 465 | 2.15 ms | 1450 (5 × 290) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Aigis-sig1` | keygen | 43000 | 1440 KiB | 1536 KiB |
| `Aigis-sig1` | sign | 43000 | 1472 KiB | 1576 KiB |
| `Aigis-sig1` | verify | 43000 | 1512 KiB | 1576 KiB |
| `Aigis-sig2` | keygen | 44344 | 1444 KiB | 1592 KiB |
| `Aigis-sig2` | sign | 44344 | 1520 KiB | 1656 KiB |
| `Aigis-sig2` | verify | 44344 | 1588 KiB | 1656 KiB |
| `Aigis-sig3` | keygen | 43872 | 1456 KiB | 1724 KiB |
| `Aigis-sig3` | sign | 43872 | 1660 KiB | 1848 KiB |
| `Aigis-sig3` | verify | 43872 | 1780 KiB | 1848 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | signature |
|---|---|---|---|
| `Aigis-sig1` | 928 | 2800 | 2015 |
| `Aigis-sig2` | 1824 | 4976 | 4533 |
| `Aigis-sig3` | 4672 | 8800 | 9134 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — own fips202.c compiled but unreachable from the API

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Aigis-sig1` | keygen | 7.6% | 66% | drng 25, pseudoXOF 2 |
| `Aigis-sig1` | sign | 65% | 6.3% | drng 57, pseudoXOF 75, sm3hash 37 |
| `Aigis-sig1` | verify | 11% | 63% | drng 21, pseudoXOF 2, sm3hash 1 |
| `Aigis-sig2` | keygen | 4.4% | 74% | drng 88, pseudoXOF 2 |
| `Aigis-sig2` | sign | 57% | 17% | drng 90, pseudoXOF 45, sm3hash 11 |
| `Aigis-sig2` | verify | 5.8% | 73% | drng 80, pseudoXOF 2, sm3hash 1 |
| `Aigis-sig3` | keygen | 5.0% | 75% | drng 293, pseudoXOF 2 |
| `Aigis-sig3` | sign | 29% | 51% | drng 278, pseudoXOF 15, pseudohash 2 |
| `Aigis-sig3` | verify | 6.0% | 73% | drng 277, pseudoXOF 2, pseudohash 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Aigis-sig1` | KAT log (sha256 `78472f66205b0187…`) | `kat/sign-01/Aigis-sig1.log` |
| `Aigis-sig1` | timing keygen | `records/sign-01/Aigis-sig1__keygen.json` |
| `Aigis-sig1` | timing sign | `records/sign-01/Aigis-sig1__sign.json` |
| `Aigis-sig1` | timing verify | `records/sign-01/Aigis-sig1__verify.json` |
| `Aigis-sig1` | hash profile keygen | `profile/sign-01/Aigis-sig1__keygen.json` |
| `Aigis-sig1` | hash profile sign | `profile/sign-01/Aigis-sig1__sign.json` |
| `Aigis-sig1` | hash profile verify | `profile/sign-01/Aigis-sig1__verify.json` |
| `Aigis-sig2` | KAT log (sha256 `f20e1494288c8aa3…`) | `kat/sign-01/Aigis-sig2.log` |
| `Aigis-sig2` | timing keygen | `records/sign-01/Aigis-sig2__keygen.json` |
| `Aigis-sig2` | timing sign | `records/sign-01/Aigis-sig2__sign.json` |
| `Aigis-sig2` | timing verify | `records/sign-01/Aigis-sig2__verify.json` |
| `Aigis-sig2` | hash profile keygen | `profile/sign-01/Aigis-sig2__keygen.json` |
| `Aigis-sig2` | hash profile sign | `profile/sign-01/Aigis-sig2__sign.json` |
| `Aigis-sig2` | hash profile verify | `profile/sign-01/Aigis-sig2__verify.json` |
| `Aigis-sig3` | KAT log (sha256 `e519b919e383ab6a…`) | `kat/sign-01/Aigis-sig3.log` |
| `Aigis-sig3` | timing keygen | `records/sign-01/Aigis-sig3__keygen.json` |
| `Aigis-sig3` | timing sign | `records/sign-01/Aigis-sig3__sign.json` |
| `Aigis-sig3` | timing verify | `records/sign-01/Aigis-sig3__verify.json` |
| `Aigis-sig3` | hash profile keygen | `profile/sign-01/Aigis-sig3__keygen.json` |
| `Aigis-sig3` | hash profile sign | `profile/sign-01/Aigis-sig3__sign.json` |
| `Aigis-sig3` | hash profile verify | `profile/sign-01/Aigis-sig3__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

