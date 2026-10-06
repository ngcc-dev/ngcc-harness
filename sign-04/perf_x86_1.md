# sign-04 CEDRUSɑ — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-04` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561076644139008.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: CEDRUSɑ
- Implementation versions measured: reference
- Parameter sets: `CEDRUSALPHA-160f`, `CEDRUSALPHA-160s`, `CEDRUSALPHA-256f`, `CEDRUSALPHA-256s`, `CEDRUSALPHA-384f`, `CEDRUSALPHA-384s`, `CEDRUSALPHA-512f`, `CEDRUSALPHA-512s`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-04/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `CEDRUSALPHA-160f` | guide | PASS |
| `CEDRUSALPHA-160s` | guide | PASS |
| `CEDRUSALPHA-256f` | guide | PASS |
| `CEDRUSALPHA-256s` | guide | PASS |
| `CEDRUSALPHA-384f` | guide | PASS |
| `CEDRUSALPHA-384s` | guide | PASS |
| `CEDRUSALPHA-512f` | guide | PASS |
| `CEDRUSALPHA-512s` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `CEDRUSALPHA-160f` | keygen | 16.05 M | 7.67 ms | 130 | 7.67 ms | 625 (5 × 125) |
| `CEDRUSALPHA-160f` | sign | 570.59 M | 274 ms | 3.65 | 273 ms | 100 (5 × 20) |
| `CEDRUSALPHA-160f` | verify | 17.73 M | 8.47 ms | 118 | 8.47 ms | 590 (5 × 118) |
| `CEDRUSALPHA-160s` | keygen | 523.56 M | 251 ms | 3.98 | 250 ms | 100 (5 × 20) |
| `CEDRUSALPHA-160s` | sign | 7.78 G | 3.71 s | 0.269 | 3.71 s | 100 (5 × 20) |
| `CEDRUSALPHA-160s` | verify | 19.14 M | 9.14 ms | 109 | 9.14 ms | 540 (5 × 108) |
| `CEDRUSALPHA-256f` | keygen | 52.40 M | 25 ms | 40 | 25 ms | 175 (5 × 35) |
| `CEDRUSALPHA-256f` | sign | 1.35 G | 646 ms | 1.55 | 646 ms | 100 (5 × 20) |
| `CEDRUSALPHA-256f` | verify | 24.20 M | 11.6 ms | 86.5 | 11.6 ms | 435 (5 × 87) |
| `CEDRUSALPHA-256s` | keygen | 793.87 M | 379 ms | 2.64 | 379 ms | 100 (5 × 20) |
| `CEDRUSALPHA-256s` | sign | 12.06 G | 5.79 s | 0.173 | 5.79 s | 100 (5 × 20) |
| `CEDRUSALPHA-256s` | verify | 29.24 M | 14 ms | 71.6 | 14 ms | 360 (5 × 72) |
| `CEDRUSALPHA-384f` | keygen | 637.31 M | 307 ms | 3.26 | 305 ms | 100 (5 × 20) |
| `CEDRUSALPHA-384f` | sign | 12.46 G | 5.97 s | 0.168 | 5.97 s | 100 (5 × 20) |
| `CEDRUSALPHA-384f` | verify | 123.74 M | 59.1 ms | 16.9 | 59 ms | 100 (5 × 20) |
| `CEDRUSALPHA-384s` | keygen | 4.10 G | 1.96 s | 0.51 | 1.96 s | 100 (5 × 20) |
| `CEDRUSALPHA-384s` | sign | 44.24 G | 21.2 s | 0.0471 | 21.2 s | 40 (5 × 8) |
| `CEDRUSALPHA-384s` | verify | 68.37 M | 32.7 ms | 30.6 | 32.6 ms | 155 (5 × 31) |
| `CEDRUSALPHA-512f` | keygen | 1.60 G | 766 ms | 1.31 | 764 ms | 100 (5 × 20) |
| `CEDRUSALPHA-512f` | sign | 21.00 G | 10 s | 0.0997 | 10 s | 85 (5 × 17) |
| `CEDRUSALPHA-512f` | verify | 145.58 M | 69.5 ms | 14.4 | 69.5 ms | 100 (5 × 20) |
| `CEDRUSALPHA-512s` | keygen | 7.68 G | 3.67 s | 0.273 | 3.67 s | 100 (5 × 20) |
| `CEDRUSALPHA-512s` | sign | 88.57 G | 42.3 s | 0.0236 | 42.3 s | 20 (5 × 4) |
| `CEDRUSALPHA-512s` | verify | 128.14 M | 61.2 ms | 16.3 | 61.2 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `CEDRUSALPHA-160f` | keygen | 250461 | 1728 KiB | 2028 KiB |
| `CEDRUSALPHA-160f` | sign | 250461 | 1940 KiB | 2016 KiB |
| `CEDRUSALPHA-160f` | verify | 250461 | 1932 KiB | 1996 KiB |
| `CEDRUSALPHA-160s` | keygen | 379333 | 1696 KiB | 2092 KiB |
| `CEDRUSALPHA-160s` | sign | 379333 | 2044 KiB | 2144 KiB |
| `CEDRUSALPHA-160s` | verify | 379333 | 2052 KiB | 2120 KiB |
| `CEDRUSALPHA-256f` | keygen | 1126817 | 1756 KiB | 2884 KiB |
| `CEDRUSALPHA-256f` | sign | 1126817 | 2812 KiB | 2876 KiB |
| `CEDRUSALPHA-256f` | verify | 1126817 | 2800 KiB | 2896 KiB |
| `CEDRUSALPHA-256s` | keygen | 1691081 | 1712 KiB | 3412 KiB |
| `CEDRUSALPHA-256s` | sign | 1691081 | 3284 KiB | 3416 KiB |
| `CEDRUSALPHA-256s` | verify | 1691081 | 3332 KiB | 3420 KiB |
| `CEDRUSALPHA-384f` | keygen | 4504613 | 1788 KiB | 6188 KiB |
| `CEDRUSALPHA-384f` | sign | 4504613 | 6088 KiB | 6200 KiB |
| `CEDRUSALPHA-384f` | verify | 4504613 | 6092 KiB | 6160 KiB |
| `CEDRUSALPHA-384s` | keygen | 3942709 | 1768 KiB | 5632 KiB |
| `CEDRUSALPHA-384s` | sign | 3942709 | 5552 KiB | 5640 KiB |
| `CEDRUSALPHA-384s` | verify | 3942709 | 5564 KiB | 5656 KiB |
| `CEDRUSALPHA-512f` | keygen | 10104321 | 1684 KiB | 11580 KiB |
| `CEDRUSALPHA-512f` | sign | 10104321 | 11480 KiB | 11704 KiB |
| `CEDRUSALPHA-512f` | verify | 10104321 | 11552 KiB | 11684 KiB |
| `CEDRUSALPHA-512s` | keygen | 11401761 | 1812 KiB | 12900 KiB |
| `CEDRUSALPHA-512s` | sign | 11401761 | 12828 KiB | 12896 KiB |
| `CEDRUSALPHA-512s` | verify | 11401761 | 12812 KiB | 12908 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `CEDRUSALPHA-160f` | 40 | 80 | 19420 |
| `CEDRUSALPHA-160s` | 40 | 80 | 10300 |
| `CEDRUSALPHA-256f` | 64 | 128 | 43296 |
| `CEDRUSALPHA-256s` | 64 | 128 | 25568 |
| `CEDRUSALPHA-384f` | 96 | 192 | 76176 |
| `CEDRUSALPHA-384s` | 96 | 192 | 60672 |
| `CEDRUSALPHA-512f` | 128 | 256 | 127488 |
| `CEDRUSALPHA-512s` | 128 | 256 | 98048 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `CEDRUSALPHA-160f` | keygen | 98% | 0.0% | drng 1, sm3hash 5.74e+03 |
| `CEDRUSALPHA-160f` | sign | 98% | 0.0% | drng 1, pseudoXOF 1, sm3hash 2.05e+05 |
| `CEDRUSALPHA-160f` | verify | 98% | 0.0% | pseudoXOF 1, sm3hash 6.09e+03 |
| `CEDRUSALPHA-160s` | keygen | 98% | 0.0% | drng 1, sm3hash 1.85e+05 |
| `CEDRUSALPHA-160s` | sign | 98% | 0.0% | drng 1, pseudoXOF 1, sm3hash 2.75e+06 |
| `CEDRUSALPHA-160s` | verify | 98% | 0.0% | pseudoXOF 1, sm3hash 6.65e+03 |
| `CEDRUSALPHA-256f` | keygen | 98% | 0.0% | drng 1, sm3hash 1.82e+04 |
| `CEDRUSALPHA-256f` | sign | 98% | 0.0% | drng 1, pseudoXOF 1, sm3hash 4.64e+05 |
| `CEDRUSALPHA-256f` | verify | 97% | 0.0% | pseudoXOF 1, sm3hash 8.01e+03 |
| `CEDRUSALPHA-256s` | keygen | 98% | 0.0% | drng 1, sm3hash 2.78e+05 |
| `CEDRUSALPHA-256s` | sign | 98% | 0.0% | drng 1, pseudoXOF 1, sm3hash 4.17e+06 |
| `CEDRUSALPHA-256s` | verify | 98% | 0.0% | pseudoXOF 1, sm3hash 9.95e+03 |
| `CEDRUSALPHA-384f` | keygen | 99% | 0.0% | drng 1, pseudoXOF 7.63e+04 |
| `CEDRUSALPHA-384f` | sign | 99% | 0.0% | drng 1, pseudoXOF 1.51e+06 |
| `CEDRUSALPHA-384f` | verify | 99% | 0.0% | pseudoXOF 1.45e+04 |
| `CEDRUSALPHA-384s` | keygen | 99% | 0.0% | drng 1, pseudoXOF 4.92e+05 |
| `CEDRUSALPHA-384s` | sign | 99% | 0.0% | drng 1, pseudoXOF 5.36e+06 |
| `CEDRUSALPHA-384s` | verify | 99% | 0.0% | pseudoXOF 7.96e+03 |
| `CEDRUSALPHA-512f` | keygen | 99% | 0.0% | drng 1, pseudoXOF 1.9e+05 |
| `CEDRUSALPHA-512f` | sign | 99% | 0.0% | drng 1, pseudoXOF 2.5e+06 |
| `CEDRUSALPHA-512f` | verify | 99% | 0.0% | pseudoXOF 1.66e+04 |
| `CEDRUSALPHA-512s` | keygen | 99% | 0.0% | drng 1, pseudoXOF 9.18e+05 |
| `CEDRUSALPHA-512s` | sign | 99% | 0.0% | drng 1, pseudoXOF 1.06e+07 |
| `CEDRUSALPHA-512s` | verify | 99% | 0.0% | pseudoXOF 1.47e+04 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `CEDRUSALPHA-160f` | KAT log (sha256 `58bead2fc71ac9c6…`) | `kat/sign-04/CEDRUSALPHA-160f.log` |
| `CEDRUSALPHA-160f` | timing keygen | `records/sign-04/CEDRUSALPHA-160f__keygen.json` |
| `CEDRUSALPHA-160f` | timing sign | `records/sign-04/CEDRUSALPHA-160f__sign.json` |
| `CEDRUSALPHA-160f` | timing verify | `records/sign-04/CEDRUSALPHA-160f__verify.json` |
| `CEDRUSALPHA-160f` | hash profile keygen | `profile/sign-04/CEDRUSALPHA-160f__keygen.json` |
| `CEDRUSALPHA-160f` | hash profile sign | `profile/sign-04/CEDRUSALPHA-160f__sign.json` |
| `CEDRUSALPHA-160f` | hash profile verify | `profile/sign-04/CEDRUSALPHA-160f__verify.json` |
| `CEDRUSALPHA-160s` | KAT log (sha256 `d0e5dc37f3043047…`) | `kat/sign-04/CEDRUSALPHA-160s.log` |
| `CEDRUSALPHA-160s` | timing keygen | `records/sign-04/CEDRUSALPHA-160s__keygen.json` |
| `CEDRUSALPHA-160s` | timing sign | `records/sign-04/CEDRUSALPHA-160s__sign.json` |
| `CEDRUSALPHA-160s` | timing verify | `records/sign-04/CEDRUSALPHA-160s__verify.json` |
| `CEDRUSALPHA-160s` | hash profile keygen | `profile/sign-04/CEDRUSALPHA-160s__keygen.json` |
| `CEDRUSALPHA-160s` | hash profile sign | `profile/sign-04/CEDRUSALPHA-160s__sign.json` |
| `CEDRUSALPHA-160s` | hash profile verify | `profile/sign-04/CEDRUSALPHA-160s__verify.json` |
| `CEDRUSALPHA-256f` | KAT log (sha256 `e245e4d5e4fae4b5…`) | `kat/sign-04/CEDRUSALPHA-256f.log` |
| `CEDRUSALPHA-256f` | timing keygen | `records/sign-04/CEDRUSALPHA-256f__keygen.json` |
| `CEDRUSALPHA-256f` | timing sign | `records/sign-04/CEDRUSALPHA-256f__sign.json` |
| `CEDRUSALPHA-256f` | timing verify | `records/sign-04/CEDRUSALPHA-256f__verify.json` |
| `CEDRUSALPHA-256f` | hash profile keygen | `profile/sign-04/CEDRUSALPHA-256f__keygen.json` |
| `CEDRUSALPHA-256f` | hash profile sign | `profile/sign-04/CEDRUSALPHA-256f__sign.json` |
| `CEDRUSALPHA-256f` | hash profile verify | `profile/sign-04/CEDRUSALPHA-256f__verify.json` |
| `CEDRUSALPHA-256s` | KAT log (sha256 `f37910464d8fbb8c…`) | `kat/sign-04/CEDRUSALPHA-256s.log` |
| `CEDRUSALPHA-256s` | timing keygen | `records/sign-04/CEDRUSALPHA-256s__keygen.json` |
| `CEDRUSALPHA-256s` | timing sign | `records/sign-04/CEDRUSALPHA-256s__sign.json` |
| `CEDRUSALPHA-256s` | timing verify | `records/sign-04/CEDRUSALPHA-256s__verify.json` |
| `CEDRUSALPHA-256s` | hash profile keygen | `profile/sign-04/CEDRUSALPHA-256s__keygen.json` |
| `CEDRUSALPHA-256s` | hash profile sign | `profile/sign-04/CEDRUSALPHA-256s__sign.json` |
| `CEDRUSALPHA-256s` | hash profile verify | `profile/sign-04/CEDRUSALPHA-256s__verify.json` |
| `CEDRUSALPHA-384f` | KAT log (sha256 `e2dd7362970666e4…`) | `kat/sign-04/CEDRUSALPHA-384f.log` |
| `CEDRUSALPHA-384f` | timing keygen | `records/sign-04/CEDRUSALPHA-384f__keygen.json` |
| `CEDRUSALPHA-384f` | timing sign | `records/sign-04/CEDRUSALPHA-384f__sign.json` |
| `CEDRUSALPHA-384f` | timing verify | `records/sign-04/CEDRUSALPHA-384f__verify.json` |
| `CEDRUSALPHA-384f` | hash profile keygen | `profile/sign-04/CEDRUSALPHA-384f__keygen.json` |
| `CEDRUSALPHA-384f` | hash profile sign | `profile/sign-04/CEDRUSALPHA-384f__sign.json` |
| `CEDRUSALPHA-384f` | hash profile verify | `profile/sign-04/CEDRUSALPHA-384f__verify.json` |
| `CEDRUSALPHA-384s` | KAT log (sha256 `158bde17ef118a20…`) | `kat/sign-04/CEDRUSALPHA-384s.log` |
| `CEDRUSALPHA-384s` | timing keygen | `records/sign-04/CEDRUSALPHA-384s__keygen.json` |
| `CEDRUSALPHA-384s` | timing sign | `records/sign-04/CEDRUSALPHA-384s__sign.json` |
| `CEDRUSALPHA-384s` | timing verify | `records/sign-04/CEDRUSALPHA-384s__verify.json` |
| `CEDRUSALPHA-384s` | hash profile keygen | `profile/sign-04/CEDRUSALPHA-384s__keygen.json` |
| `CEDRUSALPHA-384s` | hash profile sign | `profile/sign-04/CEDRUSALPHA-384s__sign.json` |
| `CEDRUSALPHA-384s` | hash profile verify | `profile/sign-04/CEDRUSALPHA-384s__verify.json` |
| `CEDRUSALPHA-512f` | KAT log (sha256 `829fcb77d8d4027a…`) | `kat/sign-04/CEDRUSALPHA-512f.log` |
| `CEDRUSALPHA-512f` | timing keygen | `records/sign-04/CEDRUSALPHA-512f__keygen.json` |
| `CEDRUSALPHA-512f` | timing sign | `records/sign-04/CEDRUSALPHA-512f__sign.json` |
| `CEDRUSALPHA-512f` | timing verify | `records/sign-04/CEDRUSALPHA-512f__verify.json` |
| `CEDRUSALPHA-512f` | hash profile keygen | `profile/sign-04/CEDRUSALPHA-512f__keygen.json` |
| `CEDRUSALPHA-512f` | hash profile sign | `profile/sign-04/CEDRUSALPHA-512f__sign.json` |
| `CEDRUSALPHA-512f` | hash profile verify | `profile/sign-04/CEDRUSALPHA-512f__verify.json` |
| `CEDRUSALPHA-512s` | KAT log (sha256 `29624e9e32b36380…`) | `kat/sign-04/CEDRUSALPHA-512s.log` |
| `CEDRUSALPHA-512s` | timing keygen | `records/sign-04/CEDRUSALPHA-512s__keygen.json` |
| `CEDRUSALPHA-512s` | timing sign | `records/sign-04/CEDRUSALPHA-512s__sign.json` |
| `CEDRUSALPHA-512s` | timing verify | `records/sign-04/CEDRUSALPHA-512s__verify.json` |
| `CEDRUSALPHA-512s` | hash profile keygen | `profile/sign-04/CEDRUSALPHA-512s__keygen.json` |
| `CEDRUSALPHA-512s` | hash profile sign | `profile/sign-04/CEDRUSALPHA-512s__sign.json` |
| `CEDRUSALPHA-512s` | hash profile verify | `profile/sign-04/CEDRUSALPHA-512s__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

