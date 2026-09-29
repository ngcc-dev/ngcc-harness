# sign-13 GreatWall Signature Algorithm — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-13` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561077852098560.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: GreatWall Signature Algorithm
- Implementation versions measured: reference
- Parameter sets: `GreatWall128f`, `GreatWall128s`, `GreatWall192f`, `GreatWall192s`, `GreatWall256f`, `GreatWall256s`, `GreatWall512f`, `GreatWall512s`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-13/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `GreatWall128f` | harness-default | PASS |
| `GreatWall128s` | harness-default | PASS |
| `GreatWall192f` | harness-default | PASS |
| `GreatWall192s` | harness-default | PASS |
| `GreatWall256f` | harness-default | PASS |
| `GreatWall256s` | harness-default | PASS |
| `GreatWall512f` | harness-default | PASS |
| `GreatWall512s` | harness-default | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `GreatWall128f` | keygen | 4.31 M | 1.6 ms | 626 | 1.6 ms | 5860 (5 × 1172) |
| `GreatWall128f` | sign | 87.08 M | 32.3 ms | 31 | 32.3 ms | 100 (5 × 20) |
| `GreatWall128f` | verify | 64.29 M | 23.8 ms | 41.9 | 23.8 ms | 135 (5 × 27) |
| `GreatWall128s` | keygen | 4.32 M | 1.6 ms | 623 | 1.6 ms | 5850 (5 × 1170) |
| `GreatWall128s` | sign | 406.20 M | 151 ms | 6.63 | 151 ms | 100 (5 × 20) |
| `GreatWall128s` | verify | 339.58 M | 126 ms | 7.94 | 126 ms | 100 (5 × 20) |
| `GreatWall192f` | keygen | 10.12 M | 3.75 ms | 267 | 3.75 ms | 690 (5 × 138) |
| `GreatWall192f` | sign | 273.62 M | 102 ms | 9.85 | 101 ms | 100 (5 × 20) |
| `GreatWall192f` | verify | 187.41 M | 69.5 ms | 14.4 | 69.5 ms | 100 (5 × 20) |
| `GreatWall192s` | keygen | 10.11 M | 3.75 ms | 267 | 3.75 ms | 690 (5 × 138) |
| `GreatWall192s` | sign | 1.15 G | 425 ms | 2.35 | 425 ms | 100 (5 × 20) |
| `GreatWall192s` | verify | 1.06 G | 393 ms | 2.55 | 393 ms | 100 (5 × 20) |
| `GreatWall256f` | keygen | 18.58 M | 6.89 ms | 145 | 6.91 ms | 420 (5 × 84) |
| `GreatWall256f` | sign | 644.86 M | 239 ms | 4.18 | 239 ms | 100 (5 × 20) |
| `GreatWall256f` | verify | 420.43 M | 156 ms | 6.41 | 156 ms | 100 (5 × 20) |
| `GreatWall256s` | keygen | 18.75 M | 6.96 ms | 144 | 6.97 ms | 415 (5 × 83) |
| `GreatWall256s` | sign | 2.09 G | 778 ms | 1.28 | 777 ms | 100 (5 × 20) |
| `GreatWall256s` | verify | 1.86 G | 691 ms | 1.45 | 691 ms | 100 (5 × 20) |
| `GreatWall512f` | keygen | 81.63 M | 30.3 ms | 33 | 30.3 ms | 270 (5 × 54) |
| `GreatWall512f` | sign | 6.88 G | 2.55 s | 0.391 | 2.55 s | 100 (5 × 20) |
| `GreatWall512f` | verify | 4.29 G | 1.59 s | 0.629 | 1.59 s | 100 (5 × 20) |
| `GreatWall512s` | keygen | 77.00 M | 28.6 ms | 35 | 28.7 ms | 265 (5 × 53) |
| `GreatWall512s` | sign | 19.50 G | 7.24 s | 0.138 | 7.24 s | 50 (5 × 10) |
| `GreatWall512s` | verify | 16.98 G | 6.3 s | 0.159 | 6.3 s | 55 (5 × 11) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `GreatWall128f` | keygen | 187488 | 1524 KiB | 1644 KiB |
| `GreatWall128f` | sign | 187488 | 1580 KiB | 3848 KiB |
| `GreatWall128f` | verify | 187488 | 3868 KiB | 3932 KiB |
| `GreatWall128s` | keygen | 186792 | 3560 KiB | 3680 KiB |
| `GreatWall128s` | sign | 186792 | 1580 KiB | 6568 KiB |
| `GreatWall128s` | verify | 186792 | 3420 KiB | 5592 KiB |
| `GreatWall192f` | keygen | 198576 | 1524 KiB | 1652 KiB |
| `GreatWall192f` | sign | 198576 | 1588 KiB | 4568 KiB |
| `GreatWall192f` | verify | 198576 | 4336 KiB | 4400 KiB |
| `GreatWall192s` | keygen | 198712 | 1528 KiB | 1656 KiB |
| `GreatWall192s` | sign | 198712 | 1592 KiB | 12792 KiB |
| `GreatWall192s` | verify | 198712 | 3804 KiB | 11500 KiB |
| `GreatWall256f` | keygen | 256888 | 1536 KiB | 1724 KiB |
| `GreatWall256f` | sign | 256888 | 1660 KiB | 5612 KiB |
| `GreatWall256f` | verify | 256888 | 3780 KiB | 5132 KiB |
| `GreatWall256s` | keygen | 257024 | 1532 KiB | 1720 KiB |
| `GreatWall256s` | sign | 257024 | 1656 KiB | 19024 KiB |
| `GreatWall256s` | verify | 257024 | 3872 KiB | 16840 KiB |
| `GreatWall512f` | keygen | 417016 | 1576 KiB | 1856 KiB |
| `GreatWall512f` | sign | 417016 | 3780 KiB | 12976 KiB |
| `GreatWall512f` | verify | 417016 | 6856 KiB | 11016 KiB |
| `GreatWall512s` | keygen | 418800 | 1572 KiB | 1852 KiB |
| `GreatWall512s` | sign | 418800 | 1788 KiB | 78584 KiB |
| `GreatWall512s` | verify | 418800 | 4724 KiB | 77400 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | signature |
|---|---|---|---|
| `GreatWall128f` | 36 | 36 | 3396 |
| `GreatWall128s` | 36 | 36 | 2758 |
| `GreatWall192f` | 50 | 50 | 8012 |
| `GreatWall192s` | 50 | 50 | 6804 |
| `GreatWall256f` | 66 | 66 | 14260 |
| `GreatWall256s` | 66 | 66 | 12236 |
| `GreatWall512f` | 132 | 132 | 57812 |
| `GreatWall512s` | 132 | 132 | 50012 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **bypass** — own XKCP Keccak + AES-CTR PRGs; no auxfunc.c shipped

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `GreatWall128f` | keygen | 0.0% | 0.5% | drng 4.12 |
| `GreatWall128f` | sign | 0.0% | 0.0% | drng 1 |
| `GreatWall128f` | verify | 0.0% | 0.0% | – |
| `GreatWall128s` | keygen | 0.0% | 0.5% | drng 4.12 |
| `GreatWall128s` | sign | 0.0% | 0.0% | drng 1 |
| `GreatWall128s` | verify | 0.0% | 0.0% | – |
| `GreatWall192f` | keygen | 0.0% | 0.2% | drng 3.76 |
| `GreatWall192f` | sign | 0.0% | 0.0% | drng 1 |
| `GreatWall192f` | verify | 0.0% | 0.0% | – |
| `GreatWall192s` | keygen | 0.0% | 0.2% | drng 3.76 |
| `GreatWall192s` | sign | 0.0% | 0.0% | drng 1 |
| `GreatWall192s` | verify | 0.0% | 0.0% | – |
| `GreatWall256f` | keygen | 0.0% | 0.2% | drng 4.18 |
| `GreatWall256f` | sign | 0.0% | 0.0% | drng 1 |
| `GreatWall256f` | verify | 0.0% | 0.0% | – |
| `GreatWall256s` | keygen | 0.0% | 0.2% | drng 4.17 |
| `GreatWall256s` | sign | 0.0% | 0.0% | drng 1 |
| `GreatWall256s` | verify | 0.0% | 0.0% | – |
| `GreatWall512f` | keygen | 0.0% | 0.1% | drng 4.38 |
| `GreatWall512f` | sign | 0.0% | 0.0% | drng 1 |
| `GreatWall512f` | verify | 0.0% | 0.0% | – |
| `GreatWall512s` | keygen | 0.0% | 0.1% | drng 4.41 |
| `GreatWall512s` | sign | 0.0% | 0.0% | drng 1 |
| `GreatWall512s` | verify | 0.0% | 0.0% | – |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `GreatWall128f` | KAT log (sha256 `439a03f71d89546b…`) | `kat/sign-13/GreatWall128f.log` |
| `GreatWall128f` | timing keygen | `records/sign-13/GreatWall128f__keygen.json` |
| `GreatWall128f` | timing sign | `records/sign-13/GreatWall128f__sign.json` |
| `GreatWall128f` | timing verify | `records/sign-13/GreatWall128f__verify.json` |
| `GreatWall128f` | hash profile keygen | `profile/sign-13/GreatWall128f__keygen.json` |
| `GreatWall128f` | hash profile sign | `profile/sign-13/GreatWall128f__sign.json` |
| `GreatWall128f` | hash profile verify | `profile/sign-13/GreatWall128f__verify.json` |
| `GreatWall128s` | KAT log (sha256 `03b5259a7b0ff286…`) | `kat/sign-13/GreatWall128s.log` |
| `GreatWall128s` | timing keygen | `records/sign-13/GreatWall128s__keygen.json` |
| `GreatWall128s` | timing sign | `records/sign-13/GreatWall128s__sign.json` |
| `GreatWall128s` | timing verify | `records/sign-13/GreatWall128s__verify.json` |
| `GreatWall128s` | hash profile keygen | `profile/sign-13/GreatWall128s__keygen.json` |
| `GreatWall128s` | hash profile sign | `profile/sign-13/GreatWall128s__sign.json` |
| `GreatWall128s` | hash profile verify | `profile/sign-13/GreatWall128s__verify.json` |
| `GreatWall192f` | KAT log (sha256 `5b0c68a835e4d5c0…`) | `kat/sign-13/GreatWall192f.log` |
| `GreatWall192f` | timing keygen | `records/sign-13/GreatWall192f__keygen.json` |
| `GreatWall192f` | timing sign | `records/sign-13/GreatWall192f__sign.json` |
| `GreatWall192f` | timing verify | `records/sign-13/GreatWall192f__verify.json` |
| `GreatWall192f` | hash profile keygen | `profile/sign-13/GreatWall192f__keygen.json` |
| `GreatWall192f` | hash profile sign | `profile/sign-13/GreatWall192f__sign.json` |
| `GreatWall192f` | hash profile verify | `profile/sign-13/GreatWall192f__verify.json` |
| `GreatWall192s` | KAT log (sha256 `9fe915305328b136…`) | `kat/sign-13/GreatWall192s.log` |
| `GreatWall192s` | timing keygen | `records/sign-13/GreatWall192s__keygen.json` |
| `GreatWall192s` | timing sign | `records/sign-13/GreatWall192s__sign.json` |
| `GreatWall192s` | timing verify | `records/sign-13/GreatWall192s__verify.json` |
| `GreatWall192s` | hash profile keygen | `profile/sign-13/GreatWall192s__keygen.json` |
| `GreatWall192s` | hash profile sign | `profile/sign-13/GreatWall192s__sign.json` |
| `GreatWall192s` | hash profile verify | `profile/sign-13/GreatWall192s__verify.json` |
| `GreatWall256f` | KAT log (sha256 `a0c1f6e245aed742…`) | `kat/sign-13/GreatWall256f.log` |
| `GreatWall256f` | timing keygen | `records/sign-13/GreatWall256f__keygen.json` |
| `GreatWall256f` | timing sign | `records/sign-13/GreatWall256f__sign.json` |
| `GreatWall256f` | timing verify | `records/sign-13/GreatWall256f__verify.json` |
| `GreatWall256f` | hash profile keygen | `profile/sign-13/GreatWall256f__keygen.json` |
| `GreatWall256f` | hash profile sign | `profile/sign-13/GreatWall256f__sign.json` |
| `GreatWall256f` | hash profile verify | `profile/sign-13/GreatWall256f__verify.json` |
| `GreatWall256s` | KAT log (sha256 `b804b3f6c3b3b6c8…`) | `kat/sign-13/GreatWall256s.log` |
| `GreatWall256s` | timing keygen | `records/sign-13/GreatWall256s__keygen.json` |
| `GreatWall256s` | timing sign | `records/sign-13/GreatWall256s__sign.json` |
| `GreatWall256s` | timing verify | `records/sign-13/GreatWall256s__verify.json` |
| `GreatWall256s` | hash profile keygen | `profile/sign-13/GreatWall256s__keygen.json` |
| `GreatWall256s` | hash profile sign | `profile/sign-13/GreatWall256s__sign.json` |
| `GreatWall256s` | hash profile verify | `profile/sign-13/GreatWall256s__verify.json` |
| `GreatWall512f` | KAT log (sha256 `d485ca82b619a3e0…`) | `kat/sign-13/GreatWall512f.log` |
| `GreatWall512f` | timing keygen | `records/sign-13/GreatWall512f__keygen.json` |
| `GreatWall512f` | timing sign | `records/sign-13/GreatWall512f__sign.json` |
| `GreatWall512f` | timing verify | `records/sign-13/GreatWall512f__verify.json` |
| `GreatWall512f` | hash profile keygen | `profile/sign-13/GreatWall512f__keygen.json` |
| `GreatWall512f` | hash profile sign | `profile/sign-13/GreatWall512f__sign.json` |
| `GreatWall512f` | hash profile verify | `profile/sign-13/GreatWall512f__verify.json` |
| `GreatWall512s` | KAT log (sha256 `b67b0331a7feca61…`) | `kat/sign-13/GreatWall512s.log` |
| `GreatWall512s` | timing keygen | `records/sign-13/GreatWall512s__keygen.json` |
| `GreatWall512s` | timing sign | `records/sign-13/GreatWall512s__sign.json` |
| `GreatWall512s` | timing verify | `records/sign-13/GreatWall512s__verify.json` |
| `GreatWall512s` | hash profile keygen | `profile/sign-13/GreatWall512s__keygen.json` |
| `GreatWall512s` | hash profile sign | `profile/sign-13/GreatWall512s__sign.json` |
| `GreatWall512s` | hash profile verify | `profile/sign-13/GreatWall512s__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

