# sign-14 Lynxer — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-14` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561077990510592.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: Lynxer
- Implementation versions measured: reference
- Parameter sets: `Lynxer-160f`, `Lynxer-160s`, `Lynxer-256f`, `Lynxer-256s`, `Lynxer-384f`, `Lynxer-384s`, `Lynxer-512f`, `Lynxer-512s`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-14/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Lynxer-160f` | guide | PASS |
| `Lynxer-160s` | guide | PASS |
| `Lynxer-256f` | guide | PASS |
| `Lynxer-256s` | guide | PASS |
| `Lynxer-384f` | guide | PASS |
| `Lynxer-384s` | guide | PASS |
| `Lynxer-512f` | guide | PASS |
| `Lynxer-512s` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Lynxer-160f` | keygen | 22.72 M | 8.43 ms | 119 | 8.43 ms | 375 (5 × 75) |
| `Lynxer-160f` | sign | 403.20 M | 150 ms | 6.69 | 149 ms | 100 (5 × 20) |
| `Lynxer-160f` | verify | 352.92 M | 131 ms | 7.64 | 131 ms | 100 (5 × 20) |
| `Lynxer-160s` | keygen | 22.74 M | 8.43 ms | 119 | 8.43 ms | 370 (5 × 74) |
| `Lynxer-160s` | sign | 2.92 G | 1.08 s | 0.924 | 1.08 s | 100 (5 × 20) |
| `Lynxer-160s` | verify | 2.85 G | 1.06 s | 0.947 | 1.06 s | 100 (5 × 20) |
| `Lynxer-256f` | keygen | 167.67 M | 62.2 ms | 16.1 | 62 ms | 100 (5 × 20) |
| `Lynxer-256f` | sign | 1.29 G | 478 ms | 2.09 | 477 ms | 100 (5 × 20) |
| `Lynxer-256f` | verify | 941.25 M | 349 ms | 2.86 | 349 ms | 100 (5 × 20) |
| `Lynxer-256s` | keygen | 167.06 M | 62 ms | 16.1 | 62 ms | 100 (5 × 20) |
| `Lynxer-256s` | sign | 8.36 G | 3.1 s | 0.322 | 3.1 s | 100 (5 × 20) |
| `Lynxer-256s` | verify | 7.97 G | 2.96 s | 0.338 | 2.96 s | 100 (5 × 20) |
| `Lynxer-384f` | keygen | 695.57 M | 258 ms | 3.87 | 258 ms | 100 (5 × 20) |
| `Lynxer-384f` | sign | 4.48 G | 1.66 s | 0.602 | 1.66 s | 100 (5 × 20) |
| `Lynxer-384f` | verify | 3.04 G | 1.13 s | 0.886 | 1.13 s | 100 (5 × 20) |
| `Lynxer-384s` | keygen | 709.99 M | 263 ms | 3.8 | 258 ms | 100 (5 × 20) |
| `Lynxer-384s` | sign | 24.71 G | 9.17 s | 0.109 | 9.16 s | 40 (5 × 8) |
| `Lynxer-384s` | verify | 23.00 G | 8.53 s | 0.117 | 8.53 s | 40 (5 × 8) |
| `Lynxer-512f` | keygen | 2.01 G | 745 ms | 1.34 | 745 ms | 100 (5 × 20) |
| `Lynxer-512f` | sign | 6.34 G | 2.35 s | 0.425 | 2.35 s | 100 (5 × 20) |
| `Lynxer-512f` | verify | 2.32 G | 860 ms | 1.16 | 860 ms | 100 (5 × 20) |
| `Lynxer-512s` | keygen | 2.01 G | 745 ms | 1.34 | 745 ms | 100 (5 × 20) |
| `Lynxer-512s` | sign | 8.82 G | 3.27 s | 0.305 | 3.27 s | 100 (5 × 20) |
| `Lynxer-512s` | verify | 4.55 G | 1.69 s | 0.592 | 1.68 s | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Lynxer-160f` | keygen | 436796 | 1528 KiB | 1808 KiB |
| `Lynxer-160f` | sign | 436796 | 1740 KiB | 4276 KiB |
| `Lynxer-160f` | verify | 436796 | 4184 KiB | 4248 KiB |
| `Lynxer-160s` | keygen | 436796 | 1524 KiB | 1808 KiB |
| `Lynxer-160s` | sign | 436796 | 1740 KiB | 6916 KiB |
| `Lynxer-160s` | verify | 436796 | 3272 KiB | 6628 KiB |
| `Lynxer-256f` | keygen | 436796 | 3552 KiB | 3872 KiB |
| `Lynxer-256f` | sign | 436796 | 1852 KiB | 2920 KiB |
| `Lynxer-256f` | verify | 436796 | 4880 KiB | 4876 KiB |
| `Lynxer-256s` | keygen | 436796 | 1532 KiB | 1912 KiB |
| `Lynxer-256s` | sign | 436796 | 3876 KiB | 12780 KiB |
| `Lynxer-256s` | verify | 436796 | 5636 KiB | 12480 KiB |
| `Lynxer-384f` | keygen | 436796 | 1556 KiB | 2008 KiB |
| `Lynxer-384f` | sign | 436796 | 1944 KiB | 6120 KiB |
| `Lynxer-384f` | verify | 436796 | 4904 KiB | 5924 KiB |
| `Lynxer-384s` | keygen | 436796 | 1548 KiB | 2008 KiB |
| `Lynxer-384s` | sign | 436796 | 3860 KiB | 21632 KiB |
| `Lynxer-384s` | verify | 436796 | 4644 KiB | 21168 KiB |
| `Lynxer-512f` | keygen | 436796 | 1580 KiB | 2132 KiB |
| `Lynxer-512f` | sign | 436796 | 2064 KiB | 7560 KiB |
| `Lynxer-512f` | verify | 436796 | 6024 KiB | 7344 KiB |
| `Lynxer-512s` | keygen | 436796 | 1568 KiB | 3992 KiB |
| `Lynxer-512s` | sign | 436796 | 2052 KiB | 35184 KiB |
| `Lynxer-512s` | verify | 436796 | 5160 KiB | 34716 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `Lynxer-160f` | 40 | 40 | 5801 |
| `Lynxer-160s` | 40 | 40 | 4607 |
| `Lynxer-256f` | 64 | 64 | 15097 |
| `Lynxer-256s` | 64 | 64 | 12191 |
| `Lynxer-384f` | 96 | 96 | 34109 |
| `Lynxer-384s` | 96 | 96 | 27495 |
| `Lynxer-512f` | 128 | 128 | 61091 |
| `Lynxer-512s` | 128 | 128 | 48879 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **mixed** — with -DXOF_PSEUDO (as the KATs need): oracles via pseudoXOF, PRG via AES/Rijndael/SHACAL-2; source default is own Keccak (bypass)

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Lynxer-160f` | keygen | 95% | 0.0% | drng 2, pseudoXOF 62 |
| `Lynxer-160f` | sign | 18% | 0.0% | drng 1, pseudoXOF 436 |
| `Lynxer-160f` | verify | 7.8% | 0.0% | pseudoXOF 88 |
| `Lynxer-160s` | keygen | 95% | 0.0% | drng 2, pseudoXOF 62 |
| `Lynxer-160s` | sign | 4.2% | 0.0% | drng 1, pseudoXOF 3.64e+03 |
| `Lynxer-160s` | verify | 2.4% | 0.0% | pseudoXOF 81 |
| `Lynxer-256f` | keygen | 99% | 0.0% | drng 2, pseudoXOF 123 |
| `Lynxer-256f` | sign | 40% | 0.0% | drng 1, pseudoXOF 525 |
| `Lynxer-256f` | verify | 19% | 0.0% | pseudoXOF 163 |
| `Lynxer-256s` | keygen | 99% | 0.0% | drng 2, pseudoXOF 123 |
| `Lynxer-256s` | sign | 8.2% | 0.0% | drng 1, pseudoXOF 1.22e+04 |
| `Lynxer-256s` | verify | 3.7% | 0.0% | pseudoXOF 150 |
| `Lynxer-384f` | keygen | 99% | 0.0% | drng 2, pseudoXOF 255 |
| `Lynxer-384f` | sign | 48% | 0.0% | drng 1, pseudoXOF 971 |
| `Lynxer-384f` | verify | 24% | 0.0% | pseudoXOF 313 |
| `Lynxer-384s` | keygen | 99% | 0.0% | drng 2, pseudoXOF 255 |
| `Lynxer-384s` | sign | 11% | 0.0% | drng 1, pseudoXOF 2.73e+04 |
| `Lynxer-384s` | verify | 4.8% | 0.0% | pseudoXOF 294 |
| `Lynxer-512f` | keygen | 100% | 0.0% | drng 2, pseudoXOF 435 |
| `Lynxer-512f` | sign | 96% | 0.0% | drng 1, pseudoXOF 1.55e+03 |
| `Lynxer-512f` | verify | 91% | 0.0% | pseudoXOF 512 |
| `Lynxer-512s` | keygen | 100% | 0.0% | drng 2, pseudoXOF 435 |
| `Lynxer-512s` | sign | 82% | 0.0% | drng 1, pseudoXOF 1.21e+04 |
| `Lynxer-512s` | verify | 66% | 0.0% | pseudoXOF 486 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Lynxer-160f` | KAT log (sha256 `a84705093d5baee0…`) | `kat/sign-14/Lynxer-160f.log` |
| `Lynxer-160f` | timing keygen | `records/sign-14/Lynxer-160f__keygen.json` |
| `Lynxer-160f` | timing sign | `records/sign-14/Lynxer-160f__sign.json` |
| `Lynxer-160f` | timing verify | `records/sign-14/Lynxer-160f__verify.json` |
| `Lynxer-160f` | hash profile keygen | `profile/sign-14/Lynxer-160f__keygen.json` |
| `Lynxer-160f` | hash profile sign | `profile/sign-14/Lynxer-160f__sign.json` |
| `Lynxer-160f` | hash profile verify | `profile/sign-14/Lynxer-160f__verify.json` |
| `Lynxer-160s` | KAT log (sha256 `3f03764067260fb7…`) | `kat/sign-14/Lynxer-160s.log` |
| `Lynxer-160s` | timing keygen | `records/sign-14/Lynxer-160s__keygen.json` |
| `Lynxer-160s` | timing sign | `records/sign-14/Lynxer-160s__sign.json` |
| `Lynxer-160s` | timing verify | `records/sign-14/Lynxer-160s__verify.json` |
| `Lynxer-160s` | hash profile keygen | `profile/sign-14/Lynxer-160s__keygen.json` |
| `Lynxer-160s` | hash profile sign | `profile/sign-14/Lynxer-160s__sign.json` |
| `Lynxer-160s` | hash profile verify | `profile/sign-14/Lynxer-160s__verify.json` |
| `Lynxer-256f` | KAT log (sha256 `30d2e64fc5d04d20…`) | `kat/sign-14/Lynxer-256f.log` |
| `Lynxer-256f` | timing keygen | `records/sign-14/Lynxer-256f__keygen.json` |
| `Lynxer-256f` | timing sign | `records/sign-14/Lynxer-256f__sign.json` |
| `Lynxer-256f` | timing verify | `records/sign-14/Lynxer-256f__verify.json` |
| `Lynxer-256f` | hash profile keygen | `profile/sign-14/Lynxer-256f__keygen.json` |
| `Lynxer-256f` | hash profile sign | `profile/sign-14/Lynxer-256f__sign.json` |
| `Lynxer-256f` | hash profile verify | `profile/sign-14/Lynxer-256f__verify.json` |
| `Lynxer-256s` | KAT log (sha256 `b6838c63bd06e987…`) | `kat/sign-14/Lynxer-256s.log` |
| `Lynxer-256s` | timing keygen | `records/sign-14/Lynxer-256s__keygen.json` |
| `Lynxer-256s` | timing sign | `records/sign-14/Lynxer-256s__sign.json` |
| `Lynxer-256s` | timing verify | `records/sign-14/Lynxer-256s__verify.json` |
| `Lynxer-256s` | hash profile keygen | `profile/sign-14/Lynxer-256s__keygen.json` |
| `Lynxer-256s` | hash profile sign | `profile/sign-14/Lynxer-256s__sign.json` |
| `Lynxer-256s` | hash profile verify | `profile/sign-14/Lynxer-256s__verify.json` |
| `Lynxer-384f` | KAT log (sha256 `df109a8beb466ef2…`) | `kat/sign-14/Lynxer-384f.log` |
| `Lynxer-384f` | timing keygen | `records/sign-14/Lynxer-384f__keygen.json` |
| `Lynxer-384f` | timing sign | `records/sign-14/Lynxer-384f__sign.json` |
| `Lynxer-384f` | timing verify | `records/sign-14/Lynxer-384f__verify.json` |
| `Lynxer-384f` | hash profile keygen | `profile/sign-14/Lynxer-384f__keygen.json` |
| `Lynxer-384f` | hash profile sign | `profile/sign-14/Lynxer-384f__sign.json` |
| `Lynxer-384f` | hash profile verify | `profile/sign-14/Lynxer-384f__verify.json` |
| `Lynxer-384s` | KAT log (sha256 `dd0fe00cf6e819f9…`) | `kat/sign-14/Lynxer-384s.log` |
| `Lynxer-384s` | timing keygen | `records/sign-14/Lynxer-384s__keygen.json` |
| `Lynxer-384s` | timing sign | `records/sign-14/Lynxer-384s__sign.json` |
| `Lynxer-384s` | timing verify | `records/sign-14/Lynxer-384s__verify.json` |
| `Lynxer-384s` | hash profile keygen | `profile/sign-14/Lynxer-384s__keygen.json` |
| `Lynxer-384s` | hash profile sign | `profile/sign-14/Lynxer-384s__sign.json` |
| `Lynxer-384s` | hash profile verify | `profile/sign-14/Lynxer-384s__verify.json` |
| `Lynxer-512f` | KAT log (sha256 `c2cd6643c4f9d01d…`) | `kat/sign-14/Lynxer-512f.log` |
| `Lynxer-512f` | timing keygen | `records/sign-14/Lynxer-512f__keygen.json` |
| `Lynxer-512f` | timing sign | `records/sign-14/Lynxer-512f__sign.json` |
| `Lynxer-512f` | timing verify | `records/sign-14/Lynxer-512f__verify.json` |
| `Lynxer-512f` | hash profile keygen | `profile/sign-14/Lynxer-512f__keygen.json` |
| `Lynxer-512f` | hash profile sign | `profile/sign-14/Lynxer-512f__sign.json` |
| `Lynxer-512f` | hash profile verify | `profile/sign-14/Lynxer-512f__verify.json` |
| `Lynxer-512s` | KAT log (sha256 `8ef142b4f046ba42…`) | `kat/sign-14/Lynxer-512s.log` |
| `Lynxer-512s` | timing keygen | `records/sign-14/Lynxer-512s__keygen.json` |
| `Lynxer-512s` | timing sign | `records/sign-14/Lynxer-512s__sign.json` |
| `Lynxer-512s` | timing verify | `records/sign-14/Lynxer-512s__verify.json` |
| `Lynxer-512s` | hash profile keygen | `profile/sign-14/Lynxer-512s__keygen.json` |
| `Lynxer-512s` | hash profile sign | `profile/sign-14/Lynxer-512s__sign.json` |
| `Lynxer-512s` | hash profile verify | `profile/sign-14/Lynxer-512s__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

