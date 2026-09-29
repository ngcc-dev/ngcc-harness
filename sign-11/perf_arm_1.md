# sign-11 FlexTree — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-11` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561077587857408.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: FlexTree
- Implementation versions measured: reference
- Parameter sets: `Flextree-160f`, `Flextree-160s`, `Flextree-256f`, `Flextree-256s`, `Flextree-384f`, `Flextree-384s`, `Flextree-512f`, `Flextree-512s`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-11/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Flextree-160f` | guide | PASS |
| `Flextree-160s` | guide | PASS |
| `Flextree-256f` | guide | PASS |
| `Flextree-256s` | guide | PASS |
| `Flextree-384f` | guide | PASS |
| `Flextree-384s` | guide | PASS |
| `Flextree-512f` | guide | PASS |
| `Flextree-512s` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Flextree-160f` | keygen | 13.81 M | 5.12 ms | 195 | 5.12 ms | 615 (5 × 123) |
| `Flextree-160f` | sign | 532.03 M | 197 ms | 5.07 | 197 ms | 100 (5 × 20) |
| `Flextree-160f` | verify | 14.81 M | 5.5 ms | 182 | 5.45 ms | 580 (5 × 116) |
| `Flextree-160s` | keygen | 565.27 M | 210 ms | 4.77 | 210 ms | 100 (5 × 20) |
| `Flextree-160s` | sign | 7.88 G | 2.92 s | 0.342 | 2.92 s | 100 (5 × 20) |
| `Flextree-160s` | verify | 20.31 M | 7.53 ms | 133 | 7.53 ms | 420 (5 × 84) |
| `Flextree-256f` | keygen | 44.08 M | 16.3 ms | 61.2 | 16.3 ms | 185 (5 × 37) |
| `Flextree-256f` | sign | 918.11 M | 341 ms | 2.94 | 340 ms | 100 (5 × 20) |
| `Flextree-256f` | verify | 24.21 M | 8.98 ms | 111 | 8.9 ms | 350 (5 × 70) |
| `Flextree-256s` | keygen | 460.08 M | 171 ms | 5.86 | 171 ms | 100 (5 × 20) |
| `Flextree-256s` | sign | 8.79 G | 3.26 s | 0.307 | 3.24 s | 100 (5 × 20) |
| `Flextree-256s` | verify | 37.08 M | 13.8 ms | 72.7 | 13.8 ms | 230 (5 × 46) |
| `Flextree-384f` | keygen | 623.94 M | 231 ms | 4.32 | 229 ms | 100 (5 × 20) |
| `Flextree-384f` | sign | 11.73 G | 4.35 s | 0.23 | 4.34 s | 85 (5 × 17) |
| `Flextree-384f` | verify | 118.76 M | 44.1 ms | 22.7 | 44 ms | 100 (5 × 20) |
| `Flextree-384s` | keygen | 3.10 G | 1.15 s | 0.87 | 1.15 s | 100 (5 × 20) |
| `Flextree-384s` | sign | 42.86 G | 15.9 s | 0.0629 | 15.9 s | 20 (5 × 4) |
| `Flextree-384s` | verify | 51.41 M | 19.1 ms | 52.4 | 18.9 ms | 160 (5 × 32) |
| `Flextree-512f` | keygen | 1.67 G | 619 ms | 1.62 | 619 ms | 100 (5 × 20) |
| `Flextree-512f` | sign | 23.54 G | 8.73 s | 0.114 | 8.69 s | 40 (5 × 8) |
| `Flextree-512f` | verify | 152.42 M | 56.6 ms | 17.7 | 55.8 ms | 100 (5 × 20) |
| `Flextree-512s` | keygen | 6.65 G | 2.47 s | 0.406 | 2.46 s | 100 (5 × 20) |
| `Flextree-512s` | sign | 80.79 G | 30 s | 0.0334 | 29.7 s | 10 (5 × 2) |
| `Flextree-512s` | verify | 111.38 M | 41.3 ms | 24.2 | 41.3 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Flextree-160f` | keygen | 3180096 | 1476 KiB | 1572 KiB |
| `Flextree-160f` | sign | 3180096 | 1508 KiB | 4764 KiB |
| `Flextree-160f` | verify | 3180096 | 4696 KiB | 4764 KiB |
| `Flextree-160s` | keygen | 1821632 | 1468 KiB | 1584 KiB |
| `Flextree-160s` | sign | 1821632 | 1520 KiB | 6608 KiB |
| `Flextree-160s` | verify | 1821632 | 4836 KiB | 4900 KiB |
| `Flextree-256f` | keygen | 3120784 | 1500 KiB | 1612 KiB |
| `Flextree-256f` | sign | 3120784 | 1548 KiB | 6836 KiB |
| `Flextree-256f` | verify | 3120784 | 5544 KiB | 5612 KiB |
| `Flextree-256s` | keygen | 14745816 | 1484 KiB | 1616 KiB |
| `Flextree-256s` | sign | 14745816 | 1548 KiB | 19660 KiB |
| `Flextree-256s` | verify | 14745816 | 9536 KiB | 14480 KiB |
| `Flextree-384f` | keygen | 5291912 | 1528 KiB | 1652 KiB |
| `Flextree-384f` | sign | 5291912 | 1584 KiB | 15332 KiB |
| `Flextree-384f` | verify | 5291912 | 6648 KiB | 13252 KiB |
| `Flextree-384s` | keygen | 4420016 | 1516 KiB | 1624 KiB |
| `Flextree-384s` | sign | 4420016 | 3596 KiB | 31304 KiB |
| `Flextree-384s` | verify | 4420016 | 5780 KiB | 29132 KiB |
| `Flextree-512f` | keygen | 8126040 | 3512 KiB | 3656 KiB |
| `Flextree-512f` | sign | 8126040 | 1656 KiB | 32984 KiB |
| `Flextree-512f` | verify | 8126040 | 9468 KiB | 32080 KiB |
| `Flextree-512s` | keygen | 7443216 | 1552 KiB | 1676 KiB |
| `Flextree-512s` | sign | 7443216 | 1608 KiB | 62444 KiB |
| `Flextree-512s` | verify | 7443216 | 10692 KiB | 64224 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | signature |
|---|---|---|---|
| `Flextree-160f` | 40 | 80 | 18672 |
| `Flextree-160s` | 40 | 80 | 9580 |
| `Flextree-256f` | 64 | 128 | 46856 |
| `Flextree-256s` | 64 | 128 | 25420 |
| `Flextree-384f` | 96 | 192 | 73396 |
| `Flextree-384s` | 96 | 192 | 60180 |
| `Flextree-512f` | 128 | 256 | 117296 |
| `Flextree-512s` | 128 | 256 | 94948 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Flextree-160f` | keygen | 97% | 0.0% | drng 1, sm3hash 5.07e+03 |
| `Flextree-160f` | sign | 91% | 0.0% | drng 1, pseudoXOF 1.52e+03, sm3hash 1.84e+05 |
| `Flextree-160f` | verify | 97% | 0.0% | pseudoXOF 2, sm3hash 5.34e+03 |
| `Flextree-160s` | keygen | 97% | 0.0% | drng 1, sm3hash 2.09e+05 |
| `Flextree-160s` | sign | 97% | 0.0% | drng 1, pseudoXOF 2.33, sm3hash 2.94e+06 |
| `Flextree-160s` | verify | 97% | 0.0% | pseudoXOF 2, sm3hash 7.49e+03 |
| `Flextree-256f` | keygen | 97% | 0.0% | drng 1, sm3hash 1.63e+04 |
| `Flextree-256f` | sign | 97% | 0.0% | drng 1, pseudoXOF 15.5, sm3hash 3.31e+05 |
| `Flextree-256f` | verify | 97% | 0.0% | pseudoXOF 2, sm3hash 8.53e+03 |
| `Flextree-256s` | keygen | 97% | 0.0% | drng 1, sm3hash 1.72e+05 |
| `Flextree-256s` | sign | 97% | 0.0% | drng 1, pseudoXOF 2, sm3hash 3.21e+06 |
| `Flextree-256s` | verify | 97% | 0.0% | pseudoXOF 2, sm3hash 1.36e+04 |
| `Flextree-384f` | keygen | 99% | 0.0% | drng 1, pseudoXOF 7.74e+04 |
| `Flextree-384f` | sign | 99% | 0.0% | drng 1, pseudoXOF 1.47e+06 |
| `Flextree-384f` | verify | 99% | 0.0% | pseudoXOF 1.47e+04 |
| `Flextree-384s` | keygen | 99% | 0.0% | drng 1, pseudoXOF 3.86e+05 |
| `Flextree-384s` | sign | 99% | 0.0% | drng 1, pseudoXOF 5.34e+06 |
| `Flextree-384s` | verify | 98% | 0.0% | pseudoXOF 6.18e+03 |
| `Flextree-512f` | keygen | 99% | 0.0% | drng 1, pseudoXOF 2.08e+05 |
| `Flextree-512f` | sign | 99% | 0.0% | drng 1, pseudoXOF 2.85e+06 |
| `Flextree-512f` | verify | 98% | 0.0% | pseudoXOF 1.81e+04 |
| `Flextree-512s` | keygen | 99% | 0.0% | drng 1, pseudoXOF 8.28e+05 |
| `Flextree-512s` | sign | 99% | 0.0% | drng 1, pseudoXOF 9.81e+06 |
| `Flextree-512s` | verify | 97% | 0.0% | pseudoXOF 1.33e+04 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Flextree-160f` | KAT log (sha256 `b4cfaeeec6af47f8…`) | `kat/sign-11/Flextree-160f.log` |
| `Flextree-160f` | timing keygen | `records/sign-11/Flextree-160f__keygen.json` |
| `Flextree-160f` | timing sign | `records/sign-11/Flextree-160f__sign.json` |
| `Flextree-160f` | timing verify | `records/sign-11/Flextree-160f__verify.json` |
| `Flextree-160f` | hash profile keygen | `profile/sign-11/Flextree-160f__keygen.json` |
| `Flextree-160f` | hash profile sign | `profile/sign-11/Flextree-160f__sign.json` |
| `Flextree-160f` | hash profile verify | `profile/sign-11/Flextree-160f__verify.json` |
| `Flextree-160s` | KAT log (sha256 `3ed46dcc8f6c3c5a…`) | `kat/sign-11/Flextree-160s.log` |
| `Flextree-160s` | timing keygen | `records/sign-11/Flextree-160s__keygen.json` |
| `Flextree-160s` | timing sign | `records/sign-11/Flextree-160s__sign.json` |
| `Flextree-160s` | timing verify | `records/sign-11/Flextree-160s__verify.json` |
| `Flextree-160s` | hash profile keygen | `profile/sign-11/Flextree-160s__keygen.json` |
| `Flextree-160s` | hash profile sign | `profile/sign-11/Flextree-160s__sign.json` |
| `Flextree-160s` | hash profile verify | `profile/sign-11/Flextree-160s__verify.json` |
| `Flextree-256f` | KAT log (sha256 `bd6ed98d67e30c6a…`) | `kat/sign-11/Flextree-256f.log` |
| `Flextree-256f` | timing keygen | `records/sign-11/Flextree-256f__keygen.json` |
| `Flextree-256f` | timing sign | `records/sign-11/Flextree-256f__sign.json` |
| `Flextree-256f` | timing verify | `records/sign-11/Flextree-256f__verify.json` |
| `Flextree-256f` | hash profile keygen | `profile/sign-11/Flextree-256f__keygen.json` |
| `Flextree-256f` | hash profile sign | `profile/sign-11/Flextree-256f__sign.json` |
| `Flextree-256f` | hash profile verify | `profile/sign-11/Flextree-256f__verify.json` |
| `Flextree-256s` | KAT log (sha256 `1778240adcb6db56…`) | `kat/sign-11/Flextree-256s.log` |
| `Flextree-256s` | timing keygen | `records/sign-11/Flextree-256s__keygen.json` |
| `Flextree-256s` | timing sign | `records/sign-11/Flextree-256s__sign.json` |
| `Flextree-256s` | timing verify | `records/sign-11/Flextree-256s__verify.json` |
| `Flextree-256s` | hash profile keygen | `profile/sign-11/Flextree-256s__keygen.json` |
| `Flextree-256s` | hash profile sign | `profile/sign-11/Flextree-256s__sign.json` |
| `Flextree-256s` | hash profile verify | `profile/sign-11/Flextree-256s__verify.json` |
| `Flextree-384f` | KAT log (sha256 `9c994b6646786a07…`) | `kat/sign-11/Flextree-384f.log` |
| `Flextree-384f` | timing keygen | `records/sign-11/Flextree-384f__keygen.json` |
| `Flextree-384f` | timing sign | `records/sign-11/Flextree-384f__sign.json` |
| `Flextree-384f` | timing verify | `records/sign-11/Flextree-384f__verify.json` |
| `Flextree-384f` | hash profile keygen | `profile/sign-11/Flextree-384f__keygen.json` |
| `Flextree-384f` | hash profile sign | `profile/sign-11/Flextree-384f__sign.json` |
| `Flextree-384f` | hash profile verify | `profile/sign-11/Flextree-384f__verify.json` |
| `Flextree-384s` | KAT log (sha256 `7956cd9032b7ad59…`) | `kat/sign-11/Flextree-384s.log` |
| `Flextree-384s` | timing keygen | `records/sign-11/Flextree-384s__keygen.json` |
| `Flextree-384s` | timing sign | `records/sign-11/Flextree-384s__sign.json` |
| `Flextree-384s` | timing verify | `records/sign-11/Flextree-384s__verify.json` |
| `Flextree-384s` | hash profile keygen | `profile/sign-11/Flextree-384s__keygen.json` |
| `Flextree-384s` | hash profile sign | `profile/sign-11/Flextree-384s__sign.json` |
| `Flextree-384s` | hash profile verify | `profile/sign-11/Flextree-384s__verify.json` |
| `Flextree-512f` | KAT log (sha256 `47b8bb871fdefe6b…`) | `kat/sign-11/Flextree-512f.log` |
| `Flextree-512f` | timing keygen | `records/sign-11/Flextree-512f__keygen.json` |
| `Flextree-512f` | timing sign | `records/sign-11/Flextree-512f__sign.json` |
| `Flextree-512f` | timing verify | `records/sign-11/Flextree-512f__verify.json` |
| `Flextree-512f` | hash profile keygen | `profile/sign-11/Flextree-512f__keygen.json` |
| `Flextree-512f` | hash profile sign | `profile/sign-11/Flextree-512f__sign.json` |
| `Flextree-512f` | hash profile verify | `profile/sign-11/Flextree-512f__verify.json` |
| `Flextree-512s` | KAT log (sha256 `d0015a195318a7b7…`) | `kat/sign-11/Flextree-512s.log` |
| `Flextree-512s` | timing keygen | `records/sign-11/Flextree-512s__keygen.json` |
| `Flextree-512s` | timing sign | `records/sign-11/Flextree-512s__sign.json` |
| `Flextree-512s` | timing verify | `records/sign-11/Flextree-512s__verify.json` |
| `Flextree-512s` | hash profile keygen | `profile/sign-11/Flextree-512s__keygen.json` |
| `Flextree-512s` | hash profile sign | `profile/sign-11/Flextree-512s__sign.json` |
| `Flextree-512s` | hash profile verify | `profile/sign-11/Flextree-512s__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

