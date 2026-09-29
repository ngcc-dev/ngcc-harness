# kem-19 Lore — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-19` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560853788184576.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: Lore
- Implementation versions measured: reference
- Parameter sets: `Lore-SHAKE-L1`, `Lore-SHAKE-L2`, `Lore-SHAKE-L3`, `Lore-SHAKE-L4`, `Lore-SM3-L1`, `Lore-SM3-L2`, `Lore-SM3-L3`, `Lore-SM3-L4`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-19/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Lore-SHAKE-L1` | guide | PASS |
| `Lore-SHAKE-L2` | guide | PASS |
| `Lore-SHAKE-L3` | guide | PASS |
| `Lore-SHAKE-L4` | guide | PASS |
| `Lore-SM3-L1` | guide | PASS |
| `Lore-SM3-L2` | guide | PASS |
| `Lore-SM3-L3` | guide | PASS |
| `Lore-SM3-L4` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Lore-SHAKE-L1` | keygen | 142.6 k | 52.9 µs | 1.89e+04 | 52.9 µs | 47015 (5 × 9403) |
| `Lore-SHAKE-L1` | enc | 270.9 k | 101 µs | 9.95e+03 | 100 µs | 30930 (5 × 6186) |
| `Lore-SHAKE-L1` | dec | 314.0 k | 116 µs | 8.58e+03 | 116 µs | 26160 (5 × 5232) |
| `Lore-SHAKE-L2` | keygen | 538.4 k | 200 µs | 5e+03 | 200 µs | 14235 (5 × 2847) |
| `Lore-SHAKE-L2` | enc | 786.7 k | 292 µs | 3.43e+03 | 292 µs | 10390 (5 × 2078) |
| `Lore-SHAKE-L2` | dec | 948.9 k | 352 µs | 2.84e+03 | 352 µs | 8820 (5 × 1764) |
| `Lore-SHAKE-L3` | keygen | 1.15 M | 429 µs | 2.33e+03 | 429 µs | 6970 (5 × 1394) |
| `Lore-SHAKE-L3` | enc | 1.50 M | 558 µs | 1.79e+03 | 558 µs | 5615 (5 × 1123) |
| `Lore-SHAKE-L3` | dec | 1.71 M | 635 µs | 1.58e+03 | 635 µs | 4825 (5 × 965) |
| `Lore-SHAKE-L4` | keygen | 2.01 M | 746 µs | 1.34e+03 | 747 µs | 3970 (5 × 794) |
| `Lore-SHAKE-L4` | enc | 2.63 M | 977 µs | 1.02e+03 | 977 µs | 3230 (5 × 646) |
| `Lore-SHAKE-L4` | dec | 3.08 M | 1.14 ms | 875 | 1.14 ms | 2720 (5 × 544) |
| `Lore-SM3-L1` | keygen | 218.6 k | 81.1 µs | 1.23e+04 | 80.4 µs | 34120 (5 × 6824) |
| `Lore-SM3-L1` | enc | 324.1 k | 120 µs | 8.31e+03 | 120 µs | 25740 (5 × 5148) |
| `Lore-SM3-L1` | dec | 364.8 k | 135 µs | 7.38e+03 | 135 µs | 21760 (5 × 4352) |
| `Lore-SM3-L2` | keygen | 882.2 k | 327 µs | 3.05e+03 | 325 µs | 9090 (5 × 1818) |
| `Lore-SM3-L2` | enc | 1.09 M | 406 µs | 2.47e+03 | 405 µs | 7540 (5 × 1508) |
| `Lore-SM3-L2` | dec | 1.25 M | 464 µs | 2.16e+03 | 464 µs | 6720 (5 × 1344) |
| `Lore-SM3-L3` | keygen | 1.94 M | 718 µs | 1.39e+03 | 718 µs | 4245 (5 × 849) |
| `Lore-SM3-L3` | enc | 2.23 M | 829 µs | 1.21e+03 | 829 µs | 3740 (5 × 748) |
| `Lore-SM3-L3` | dec | 2.43 M | 903 µs | 1.11e+03 | 903 µs | 3435 (5 × 687) |
| `Lore-SM3-L4` | keygen | 4.42 M | 1.64 ms | 610 | 1.63 ms | 1890 (5 × 378) |
| `Lore-SM3-L4` | enc | 4.95 M | 1.84 ms | 544 | 1.84 ms | 1705 (5 × 341) |
| `Lore-SM3-L4` | dec | 5.40 M | 2 ms | 499 | 2 ms | 1560 (5 × 312) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Lore-SHAKE-L1` | keygen | 44328 | 1436 KiB | 1508 KiB |
| `Lore-SHAKE-L1` | enc | 44328 | 1448 KiB | 1512 KiB |
| `Lore-SHAKE-L1` | dec | 44328 | 1452 KiB | 1516 KiB |
| `Lore-SHAKE-L2` | keygen | 50400 | 1444 KiB | 1536 KiB |
| `Lore-SHAKE-L2` | enc | 50400 | 1572 KiB | 1636 KiB |
| `Lore-SHAKE-L2` | dec | 50400 | 1572 KiB | 1636 KiB |
| `Lore-SHAKE-L3` | keygen | 47968 | 1448 KiB | 1560 KiB |
| `Lore-SHAKE-L3` | enc | 47968 | 1588 KiB | 1652 KiB |
| `Lore-SHAKE-L3` | dec | 47968 | 3592 KiB | 3656 KiB |
| `Lore-SHAKE-L4` | keygen | 48200 | 1456 KiB | 1592 KiB |
| `Lore-SHAKE-L4` | enc | 48200 | 1640 KiB | 1704 KiB |
| `Lore-SHAKE-L4` | dec | 48200 | 1640 KiB | 1704 KiB |
| `Lore-SM3-L1` | keygen | 41336 | 1432 KiB | 1512 KiB |
| `Lore-SM3-L1` | enc | 41336 | 1448 KiB | 1516 KiB |
| `Lore-SM3-L1` | dec | 41336 | 1448 KiB | 1516 KiB |
| `Lore-SM3-L2` | keygen | 43312 | 1440 KiB | 1544 KiB |
| `Lore-SM3-L2` | enc | 43312 | 1572 KiB | 1640 KiB |
| `Lore-SM3-L2` | dec | 43312 | 1572 KiB | 1640 KiB |
| `Lore-SM3-L3` | keygen | 44976 | 1444 KiB | 1564 KiB |
| `Lore-SM3-L3` | enc | 44976 | 1584 KiB | 1652 KiB |
| `Lore-SM3-L3` | dec | 44976 | 1584 KiB | 1652 KiB |
| `Lore-SM3-L4` | keygen | 45208 | 1452 KiB | 1596 KiB |
| `Lore-SM3-L4` | enc | 45208 | 1636 KiB | 1704 KiB |
| `Lore-SM3-L4` | dec | 45208 | 3616 KiB | 3680 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `Lore-SHAKE-L1` | 610 | 2108 | 706 | 32 |
| `Lore-SHAKE-L2` | 1186 | 4518 | 1282 | 32 |
| `Lore-SHAKE-L3` | 1954 | 7736 | 2114 | 32 |
| `Lore-SHAKE-L4` | 2914 | 11432 | 3170 | 32 |
| `Lore-SM3-L1` | 610 | 2108 | 706 | 32 |
| `Lore-SM3-L2` | 1186 | 4518 | 1282 | 32 |
| `Lore-SM3-L3` | 1954 | 7736 | 2114 | 32 |
| `Lore-SM3-L4` | 2914 | 11432 | 3170 | 32 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **instance-dependent** — Lore-SM3: ICCS-only; Lore-SHAKE: own FIPS 202 (bypass); AVX2/NEON SIMD SM3 in auxfunc

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Lore-SHAKE-L1` | keygen | 0.0% | 4.0% | drng 1 |
| `Lore-SHAKE-L1` | enc | 0.0% | 1.5% | drng 1 |
| `Lore-SHAKE-L1` | dec | 0.0% | 0.0% | – |
| `Lore-SHAKE-L2` | keygen | 0.0% | 1.1% | drng 1 |
| `Lore-SHAKE-L2` | enc | 0.0% | 0.5% | drng 1 |
| `Lore-SHAKE-L2` | dec | 0.0% | 0.0% | – |
| `Lore-SHAKE-L3` | keygen | 0.0% | 0.5% | drng 1 |
| `Lore-SHAKE-L3` | enc | 0.0% | 0.4% | drng 1 |
| `Lore-SHAKE-L3` | dec | 0.0% | 0.0% | – |
| `Lore-SHAKE-L4` | keygen | 0.0% | 0.3% | drng 1 |
| `Lore-SHAKE-L4` | enc | 0.0% | 0.2% | drng 1 |
| `Lore-SHAKE-L4` | dec | 0.0% | 0.0% | – |
| `Lore-SM3-L1` | keygen | 76% | 2.6% | drng 1, pseudoXOF 5, sm3hash 1 |
| `Lore-SM3-L1` | enc | 72% | 1.3% | drng 1, pseudoXOF 7, sm3hash 3 |
| `Lore-SM3-L1` | dec | 64% | 0.0% | pseudoXOF 8, sm3hash 3 |
| `Lore-SM3-L2` | keygen | 68% | 0.7% | drng 1, pseudoXOF 14, sm3hash 1 |
| `Lore-SM3-L2` | enc | 62% | 0.4% | drng 1, pseudoXOF 16, sm3hash 3 |
| `Lore-SM3-L2` | dec | 54% | 0.0% | pseudoXOF 17, sm3hash 3 |
| `Lore-SM3-L3` | keygen | 68% | 0.3% | drng 1, pseudoXOF 29, sm3hash 1 |
| `Lore-SM3-L3` | enc | 63% | 0.3% | drng 1, pseudoXOF 31, sm3hash 3 |
| `Lore-SM3-L3` | dec | 59% | 0.0% | pseudoXOF 32, sm3hash 3 |
| `Lore-SM3-L4` | keygen | 71% | 0.1% | drng 1, pseudoXOF 56, sm3hash 1 |
| `Lore-SM3-L4` | enc | 66% | 0.1% | drng 1, pseudoXOF 58, sm3hash 3 |
| `Lore-SM3-L4` | dec | 61% | 0.0% | pseudoXOF 59, sm3hash 3 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Lore-SHAKE-L1` | KAT log (sha256 `9464e1bbbec03c26…`) | `kat/kem-19/Lore-SHAKE-L1.log` |
| `Lore-SHAKE-L1` | timing dec | `records/kem-19/Lore-SHAKE-L1__dec.json` |
| `Lore-SHAKE-L1` | timing enc | `records/kem-19/Lore-SHAKE-L1__enc.json` |
| `Lore-SHAKE-L1` | timing keygen | `records/kem-19/Lore-SHAKE-L1__keygen.json` |
| `Lore-SHAKE-L1` | hash profile dec | `profile/kem-19/Lore-SHAKE-L1__dec.json` |
| `Lore-SHAKE-L1` | hash profile enc | `profile/kem-19/Lore-SHAKE-L1__enc.json` |
| `Lore-SHAKE-L1` | hash profile keygen | `profile/kem-19/Lore-SHAKE-L1__keygen.json` |
| `Lore-SHAKE-L2` | KAT log (sha256 `e00edc36d13909f2…`) | `kat/kem-19/Lore-SHAKE-L2.log` |
| `Lore-SHAKE-L2` | timing dec | `records/kem-19/Lore-SHAKE-L2__dec.json` |
| `Lore-SHAKE-L2` | timing enc | `records/kem-19/Lore-SHAKE-L2__enc.json` |
| `Lore-SHAKE-L2` | timing keygen | `records/kem-19/Lore-SHAKE-L2__keygen.json` |
| `Lore-SHAKE-L2` | hash profile dec | `profile/kem-19/Lore-SHAKE-L2__dec.json` |
| `Lore-SHAKE-L2` | hash profile enc | `profile/kem-19/Lore-SHAKE-L2__enc.json` |
| `Lore-SHAKE-L2` | hash profile keygen | `profile/kem-19/Lore-SHAKE-L2__keygen.json` |
| `Lore-SHAKE-L3` | KAT log (sha256 `784069ee7081801b…`) | `kat/kem-19/Lore-SHAKE-L3.log` |
| `Lore-SHAKE-L3` | timing dec | `records/kem-19/Lore-SHAKE-L3__dec.json` |
| `Lore-SHAKE-L3` | timing enc | `records/kem-19/Lore-SHAKE-L3__enc.json` |
| `Lore-SHAKE-L3` | timing keygen | `records/kem-19/Lore-SHAKE-L3__keygen.json` |
| `Lore-SHAKE-L3` | hash profile dec | `profile/kem-19/Lore-SHAKE-L3__dec.json` |
| `Lore-SHAKE-L3` | hash profile enc | `profile/kem-19/Lore-SHAKE-L3__enc.json` |
| `Lore-SHAKE-L3` | hash profile keygen | `profile/kem-19/Lore-SHAKE-L3__keygen.json` |
| `Lore-SHAKE-L4` | KAT log (sha256 `4b560c12ce2dd951…`) | `kat/kem-19/Lore-SHAKE-L4.log` |
| `Lore-SHAKE-L4` | timing dec | `records/kem-19/Lore-SHAKE-L4__dec.json` |
| `Lore-SHAKE-L4` | timing enc | `records/kem-19/Lore-SHAKE-L4__enc.json` |
| `Lore-SHAKE-L4` | timing keygen | `records/kem-19/Lore-SHAKE-L4__keygen.json` |
| `Lore-SHAKE-L4` | hash profile dec | `profile/kem-19/Lore-SHAKE-L4__dec.json` |
| `Lore-SHAKE-L4` | hash profile enc | `profile/kem-19/Lore-SHAKE-L4__enc.json` |
| `Lore-SHAKE-L4` | hash profile keygen | `profile/kem-19/Lore-SHAKE-L4__keygen.json` |
| `Lore-SM3-L1` | KAT log (sha256 `d0fdb6d7db835eac…`) | `kat/kem-19/Lore-SM3-L1.log` |
| `Lore-SM3-L1` | timing dec | `records/kem-19/Lore-SM3-L1__dec.json` |
| `Lore-SM3-L1` | timing enc | `records/kem-19/Lore-SM3-L1__enc.json` |
| `Lore-SM3-L1` | timing keygen | `records/kem-19/Lore-SM3-L1__keygen.json` |
| `Lore-SM3-L1` | hash profile dec | `profile/kem-19/Lore-SM3-L1__dec.json` |
| `Lore-SM3-L1` | hash profile enc | `profile/kem-19/Lore-SM3-L1__enc.json` |
| `Lore-SM3-L1` | hash profile keygen | `profile/kem-19/Lore-SM3-L1__keygen.json` |
| `Lore-SM3-L2` | KAT log (sha256 `7794488b882234a5…`) | `kat/kem-19/Lore-SM3-L2.log` |
| `Lore-SM3-L2` | timing dec | `records/kem-19/Lore-SM3-L2__dec.json` |
| `Lore-SM3-L2` | timing enc | `records/kem-19/Lore-SM3-L2__enc.json` |
| `Lore-SM3-L2` | timing keygen | `records/kem-19/Lore-SM3-L2__keygen.json` |
| `Lore-SM3-L2` | hash profile dec | `profile/kem-19/Lore-SM3-L2__dec.json` |
| `Lore-SM3-L2` | hash profile enc | `profile/kem-19/Lore-SM3-L2__enc.json` |
| `Lore-SM3-L2` | hash profile keygen | `profile/kem-19/Lore-SM3-L2__keygen.json` |
| `Lore-SM3-L3` | KAT log (sha256 `10a78fd610b333a3…`) | `kat/kem-19/Lore-SM3-L3.log` |
| `Lore-SM3-L3` | timing dec | `records/kem-19/Lore-SM3-L3__dec.json` |
| `Lore-SM3-L3` | timing enc | `records/kem-19/Lore-SM3-L3__enc.json` |
| `Lore-SM3-L3` | timing keygen | `records/kem-19/Lore-SM3-L3__keygen.json` |
| `Lore-SM3-L3` | hash profile dec | `profile/kem-19/Lore-SM3-L3__dec.json` |
| `Lore-SM3-L3` | hash profile enc | `profile/kem-19/Lore-SM3-L3__enc.json` |
| `Lore-SM3-L3` | hash profile keygen | `profile/kem-19/Lore-SM3-L3__keygen.json` |
| `Lore-SM3-L4` | KAT log (sha256 `749d3bc6f506bf54…`) | `kat/kem-19/Lore-SM3-L4.log` |
| `Lore-SM3-L4` | timing dec | `records/kem-19/Lore-SM3-L4__dec.json` |
| `Lore-SM3-L4` | timing enc | `records/kem-19/Lore-SM3-L4__enc.json` |
| `Lore-SM3-L4` | timing keygen | `records/kem-19/Lore-SM3-L4__keygen.json` |
| `Lore-SM3-L4` | hash profile dec | `profile/kem-19/Lore-SM3-L4__dec.json` |
| `Lore-SM3-L4` | hash profile enc | `profile/kem-19/Lore-SM3-L4__enc.json` |
| `Lore-SM3-L4` | hash profile keygen | `profile/kem-19/Lore-SM3-L4__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

