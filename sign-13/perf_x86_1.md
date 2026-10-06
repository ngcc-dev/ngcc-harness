# sign-13 GreatWall Signature Algorithm — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-13` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561077852098560.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: GreatWall Signature Algorithm
- Implementation versions measured: reference
- Parameter sets: `GreatWall128f`, `GreatWall128s`, `GreatWall192f`, `GreatWall192s`, `GreatWall256f`, `GreatWall256s`, `GreatWall512f`, `GreatWall512s`

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
| `GreatWall128f` | keygen | 5.88 M | 2.81 ms | 356 | 2.81 ms | 5440 (5 × 1088) |
| `GreatWall128f` | sign | 137.40 M | 65.7 ms | 15.2 | 65.7 ms | 100 (5 × 20) |
| `GreatWall128f` | verify | 101.83 M | 48.6 ms | 20.6 | 48.6 ms | 105 (5 × 21) |
| `GreatWall128s` | keygen | 5.89 M | 2.84 ms | 352 | 2.82 ms | 5350 (5 × 1070) |
| `GreatWall128s` | sign | 632.05 M | 304 ms | 3.29 | 301 ms | 100 (5 × 20) |
| `GreatWall128s` | verify | 538.77 M | 257 ms | 3.88 | 257 ms | 100 (5 × 20) |
| `GreatWall192f` | keygen | 13.49 M | 6.45 ms | 155 | 6.44 ms | 635 (5 × 127) |
| `GreatWall192f` | sign | 450.19 M | 217 ms | 4.61 | 216 ms | 100 (5 × 20) |
| `GreatWall192f` | verify | 311.50 M | 149 ms | 6.71 | 149 ms | 100 (5 × 20) |
| `GreatWall192s` | keygen | 13.54 M | 6.47 ms | 155 | 6.47 ms | 630 (5 × 126) |
| `GreatWall192s` | sign | 1.95 G | 938 ms | 1.07 | 940 ms | 100 (5 × 20) |
| `GreatWall192s` | verify | 1.81 G | 867 ms | 1.15 | 864 ms | 100 (5 × 20) |
| `GreatWall256f` | keygen | 27.35 M | 13.1 ms | 76.5 | 13.1 ms | 360 (5 × 72) |
| `GreatWall256f` | sign | 1.04 G | 502 ms | 1.99 | 499 ms | 100 (5 × 20) |
| `GreatWall256f` | verify | 699.87 M | 336 ms | 2.98 | 334 ms | 100 (5 × 20) |
| `GreatWall256s` | keygen | 27.36 M | 13.1 ms | 76.5 | 13.1 ms | 360 (5 × 72) |
| `GreatWall256s` | sign | 3.65 G | 1.75 s | 0.57 | 1.75 s | 100 (5 × 20) |
| `GreatWall256s` | verify | 3.30 G | 1.58 s | 0.633 | 1.58 s | 100 (5 × 20) |
| `GreatWall512f` | keygen | 121.94 M | 58.5 ms | 17.1 | 58.5 ms | 210 (5 × 42) |
| `GreatWall512f` | sign | 9.92 G | 4.74 s | 0.211 | 4.74 s | 100 (5 × 20) |
| `GreatWall512f` | verify | 5.91 G | 2.83 s | 0.353 | 2.84 s | 100 (5 × 20) |
| `GreatWall512s` | keygen | 121.91 M | 58.5 ms | 17.1 | 58.5 ms | 210 (5 × 42) |
| `GreatWall512s` | sign | 24.65 G | 11.8 s | 0.0846 | 11.8 s | 75 (5 × 15) |
| `GreatWall512s` | verify | 20.69 G | 9.92 s | 0.101 | 9.92 s | 90 (5 × 18) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `GreatWall128f` | keygen | 232617 | 1788 KiB | 1940 KiB |
| `GreatWall128f` | sign | 232617 | 1856 KiB | 2368 KiB |
| `GreatWall128f` | verify | 232617 | 2208 KiB | 2360 KiB |
| `GreatWall128s` | keygen | 231553 | 1780 KiB | 1948 KiB |
| `GreatWall128s` | sign | 231553 | 1824 KiB | 4836 KiB |
| `GreatWall128s` | verify | 231553 | 1992 KiB | 4536 KiB |
| `GreatWall192f` | keygen | 236377 | 1804 KiB | 1932 KiB |
| `GreatWall192f` | sign | 236377 | 1856 KiB | 3044 KiB |
| `GreatWall192f` | verify | 236377 | 2216 KiB | 2680 KiB |
| `GreatWall192s` | keygen | 236433 | 1812 KiB | 1960 KiB |
| `GreatWall192s` | sign | 236433 | 1832 KiB | 11296 KiB |
| `GreatWall192s` | verify | 236433 | 2112 KiB | 10904 KiB |
| `GreatWall256f` | keygen | 318657 | 1804 KiB | 1968 KiB |
| `GreatWall256f` | sign | 318657 | 1904 KiB | 4196 KiB |
| `GreatWall256f` | verify | 318657 | 2316 KiB | 3524 KiB |
| `GreatWall256s` | keygen | 318977 | 1812 KiB | 2012 KiB |
| `GreatWall256s` | sign | 318977 | 1888 KiB | 17748 KiB |
| `GreatWall256s` | verify | 318977 | 2328 KiB | 17148 KiB |
| `GreatWall512f` | keygen | 447001 | 1860 KiB | 2212 KiB |
| `GreatWall512f` | sign | 447001 | 2100 KiB | 11372 KiB |
| `GreatWall512f` | verify | 447001 | 3208 KiB | 10156 KiB |
| `GreatWall512s` | keygen | 449169 | 1848 KiB | 2152 KiB |
| `GreatWall512s` | sign | 449169 | 2052 KiB | 76928 KiB |
| `GreatWall512s` | verify | 449169 | 3196 KiB | 75688 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

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
| `GreatWall128f` | keygen | 0.0% | 0.4% | drng 4.12 |
| `GreatWall128f` | sign | 0.0% | 0.0% | drng 1 |
| `GreatWall128f` | verify | 0.0% | 0.0% | – |
| `GreatWall128s` | keygen | 0.0% | 0.4% | drng 4.12 |
| `GreatWall128s` | sign | 0.0% | 0.0% | drng 1 |
| `GreatWall128s` | verify | 0.0% | 0.0% | – |
| `GreatWall192f` | keygen | 0.0% | 0.2% | drng 3.9 |
| `GreatWall192f` | sign | 0.0% | 0.0% | drng 1 |
| `GreatWall192f` | verify | 0.0% | 0.0% | – |
| `GreatWall192s` | keygen | 0.0% | 0.2% | drng 3.9 |
| `GreatWall192s` | sign | 0.0% | 0.0% | drng 1 |
| `GreatWall192s` | verify | 0.0% | 0.0% | – |
| `GreatWall256f` | keygen | 0.0% | 0.1% | drng 4.44 |
| `GreatWall256f` | sign | 0.0% | 0.0% | drng 1 |
| `GreatWall256f` | verify | 0.0% | 0.0% | – |
| `GreatWall256s` | keygen | 0.0% | 0.1% | drng 4.44 |
| `GreatWall256s` | sign | 0.0% | 0.0% | drng 1 |
| `GreatWall256s` | verify | 0.0% | 0.0% | – |
| `GreatWall512f` | keygen | 0.0% | 0.1% | drng 4.57 |
| `GreatWall512f` | sign | 0.0% | 0.0% | drng 1 |
| `GreatWall512f` | verify | 0.0% | 0.0% | – |
| `GreatWall512s` | keygen | 0.0% | 0.1% | drng 4.57 |
| `GreatWall512s` | sign | 0.0% | 0.0% | drng 1 |
| `GreatWall512s` | verify | 0.0% | 0.0% | – |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `GreatWall128f` | KAT log (sha256 `806e3897a6245839…`) | `kat/sign-13/GreatWall128f.log` |
| `GreatWall128f` | timing keygen | `records/sign-13/GreatWall128f__keygen.json` |
| `GreatWall128f` | timing sign | `records/sign-13/GreatWall128f__sign.json` |
| `GreatWall128f` | timing verify | `records/sign-13/GreatWall128f__verify.json` |
| `GreatWall128f` | hash profile keygen | `profile/sign-13/GreatWall128f__keygen.json` |
| `GreatWall128f` | hash profile sign | `profile/sign-13/GreatWall128f__sign.json` |
| `GreatWall128f` | hash profile verify | `profile/sign-13/GreatWall128f__verify.json` |
| `GreatWall128s` | KAT log (sha256 `e131e2d63901312e…`) | `kat/sign-13/GreatWall128s.log` |
| `GreatWall128s` | timing keygen | `records/sign-13/GreatWall128s__keygen.json` |
| `GreatWall128s` | timing sign | `records/sign-13/GreatWall128s__sign.json` |
| `GreatWall128s` | timing verify | `records/sign-13/GreatWall128s__verify.json` |
| `GreatWall128s` | hash profile keygen | `profile/sign-13/GreatWall128s__keygen.json` |
| `GreatWall128s` | hash profile sign | `profile/sign-13/GreatWall128s__sign.json` |
| `GreatWall128s` | hash profile verify | `profile/sign-13/GreatWall128s__verify.json` |
| `GreatWall192f` | KAT log (sha256 `9ebb4cbdfbb76453…`) | `kat/sign-13/GreatWall192f.log` |
| `GreatWall192f` | timing keygen | `records/sign-13/GreatWall192f__keygen.json` |
| `GreatWall192f` | timing sign | `records/sign-13/GreatWall192f__sign.json` |
| `GreatWall192f` | timing verify | `records/sign-13/GreatWall192f__verify.json` |
| `GreatWall192f` | hash profile keygen | `profile/sign-13/GreatWall192f__keygen.json` |
| `GreatWall192f` | hash profile sign | `profile/sign-13/GreatWall192f__sign.json` |
| `GreatWall192f` | hash profile verify | `profile/sign-13/GreatWall192f__verify.json` |
| `GreatWall192s` | KAT log (sha256 `95220c8477f94df0…`) | `kat/sign-13/GreatWall192s.log` |
| `GreatWall192s` | timing keygen | `records/sign-13/GreatWall192s__keygen.json` |
| `GreatWall192s` | timing sign | `records/sign-13/GreatWall192s__sign.json` |
| `GreatWall192s` | timing verify | `records/sign-13/GreatWall192s__verify.json` |
| `GreatWall192s` | hash profile keygen | `profile/sign-13/GreatWall192s__keygen.json` |
| `GreatWall192s` | hash profile sign | `profile/sign-13/GreatWall192s__sign.json` |
| `GreatWall192s` | hash profile verify | `profile/sign-13/GreatWall192s__verify.json` |
| `GreatWall256f` | KAT log (sha256 `43b21cace030fd54…`) | `kat/sign-13/GreatWall256f.log` |
| `GreatWall256f` | timing keygen | `records/sign-13/GreatWall256f__keygen.json` |
| `GreatWall256f` | timing sign | `records/sign-13/GreatWall256f__sign.json` |
| `GreatWall256f` | timing verify | `records/sign-13/GreatWall256f__verify.json` |
| `GreatWall256f` | hash profile keygen | `profile/sign-13/GreatWall256f__keygen.json` |
| `GreatWall256f` | hash profile sign | `profile/sign-13/GreatWall256f__sign.json` |
| `GreatWall256f` | hash profile verify | `profile/sign-13/GreatWall256f__verify.json` |
| `GreatWall256s` | KAT log (sha256 `3716fa72e725e5b8…`) | `kat/sign-13/GreatWall256s.log` |
| `GreatWall256s` | timing keygen | `records/sign-13/GreatWall256s__keygen.json` |
| `GreatWall256s` | timing sign | `records/sign-13/GreatWall256s__sign.json` |
| `GreatWall256s` | timing verify | `records/sign-13/GreatWall256s__verify.json` |
| `GreatWall256s` | hash profile keygen | `profile/sign-13/GreatWall256s__keygen.json` |
| `GreatWall256s` | hash profile sign | `profile/sign-13/GreatWall256s__sign.json` |
| `GreatWall256s` | hash profile verify | `profile/sign-13/GreatWall256s__verify.json` |
| `GreatWall512f` | KAT log (sha256 `91811d84adca092d…`) | `kat/sign-13/GreatWall512f.log` |
| `GreatWall512f` | timing keygen | `records/sign-13/GreatWall512f__keygen.json` |
| `GreatWall512f` | timing sign | `records/sign-13/GreatWall512f__sign.json` |
| `GreatWall512f` | timing verify | `records/sign-13/GreatWall512f__verify.json` |
| `GreatWall512f` | hash profile keygen | `profile/sign-13/GreatWall512f__keygen.json` |
| `GreatWall512f` | hash profile sign | `profile/sign-13/GreatWall512f__sign.json` |
| `GreatWall512f` | hash profile verify | `profile/sign-13/GreatWall512f__verify.json` |
| `GreatWall512s` | KAT log (sha256 `2d5b89d84136e860…`) | `kat/sign-13/GreatWall512s.log` |
| `GreatWall512s` | timing keygen | `records/sign-13/GreatWall512s__keygen.json` |
| `GreatWall512s` | timing sign | `records/sign-13/GreatWall512s__sign.json` |
| `GreatWall512s` | timing verify | `records/sign-13/GreatWall512s__verify.json` |
| `GreatWall512s` | hash profile keygen | `profile/sign-13/GreatWall512s__keygen.json` |
| `GreatWall512s` | hash profile sign | `profile/sign-13/GreatWall512s__sign.json` |
| `GreatWall512s` | hash profile verify | `profile/sign-13/GreatWall512s__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

