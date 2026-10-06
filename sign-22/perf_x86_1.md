# sign-22 Rhyme — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-22` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561095950520320.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: Rhyme
- Implementation versions measured: reference
- Parameter sets: `Rhyme-SHAKE-128`, `Rhyme-SHAKE-256`, `Rhyme-SHAKE-384`, `Rhyme-SHAKE-512`, `Rhyme-SM3-128`, `Rhyme-SM3-256`, `Rhyme-SM3-384`, `Rhyme-SM3-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-22/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Rhyme-SHAKE-128` | guide | PASS |
| `Rhyme-SHAKE-256` | guide | PASS |
| `Rhyme-SHAKE-384` | guide | PASS |
| `Rhyme-SHAKE-512` | guide | PASS |
| `Rhyme-SM3-128` | guide | PASS |
| `Rhyme-SM3-256` | guide | PASS |
| `Rhyme-SM3-384` | guide | PASS |
| `Rhyme-SM3-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Rhyme-SHAKE-128` | keygen | 813.50 M | 390 ms | 2.56 | 389 ms | 100 (5 × 20) |
| `Rhyme-SHAKE-128` | sign | 2.18 M | 1.04 ms | 961 | 1.04 ms | 4290 (5 × 858) |
| `Rhyme-SHAKE-128` | verify | 191.2 k | 91.3 µs | 1.1e+04 | 91.3 µs | 35535 (5 × 7107) |
| `Rhyme-SHAKE-256` | keygen | 1.86 G | 892 ms | 1.12 | 890 ms | 100 (5 × 20) |
| `Rhyme-SHAKE-256` | sign | 9.75 M | 4.66 ms | 215 | 4.66 ms | 1070 (5 × 214) |
| `Rhyme-SHAKE-256` | verify | 433.3 k | 207 µs | 4.83e+03 | 207 µs | 15310 (5 × 3062) |
| `Rhyme-SHAKE-384` | keygen | 7.40 G | 3.55 s | 0.281 | 3.55 s | 100 (5 × 20) |
| `Rhyme-SHAKE-384` | sign | 4.97 M | 2.37 ms | 421 | 2.37 ms | 2025 (5 × 405) |
| `Rhyme-SHAKE-384` | verify | 788.2 k | 376 µs | 2.66e+03 | 376 µs | 8815 (5 × 1763) |
| `Rhyme-SHAKE-512` | keygen | 10.49 G | 5.04 s | 0.199 | 5.04 s | 75 (5 × 15) |
| `Rhyme-SHAKE-512` | sign | 9.88 M | 4.72 ms | 212 | 4.72 ms | 1040 (5 × 208) |
| `Rhyme-SHAKE-512` | verify | 1.18 M | 565 µs | 1.77e+03 | 565 µs | 6885 (5 × 1377) |
| `Rhyme-SM3-128` | keygen | 867.63 M | 416 ms | 2.4 | 415 ms | 100 (5 × 20) |
| `Rhyme-SM3-128` | sign | 4.27 M | 2.05 ms | 487 | 2.05 ms | 2325 (5 × 465) |
| `Rhyme-SM3-128` | verify | 338.8 k | 163 µs | 6.14e+03 | 163 µs | 24045 (5 × 4809) |
| `Rhyme-SM3-256` | keygen | 2.32 G | 1.11 s | 0.898 | 1.12 s | 100 (5 × 20) |
| `Rhyme-SM3-256` | sign | 15.55 M | 7.48 ms | 134 | 7.48 ms | 660 (5 × 132) |
| `Rhyme-SM3-256` | verify | 870.9 k | 419 µs | 2.39e+03 | 419 µs | 8475 (5 × 1695) |
| `Rhyme-SM3-384` | keygen | 6.54 G | 3.14 s | 0.319 | 3.14 s | 100 (5 × 20) |
| `Rhyme-SM3-384` | sign | 23.32 M | 11.2 ms | 89.2 | 11.2 ms | 440 (5 × 88) |
| `Rhyme-SM3-384` | verify | 1.60 M | 770 µs | 1.3e+03 | 770 µs | 5270 (5 × 1054) |
| `Rhyme-SM3-512` | keygen | 6.87 G | 3.3 s | 0.303 | 3.3 s | 100 (5 × 20) |
| `Rhyme-SM3-512` | sign | 32.58 M | 15.7 ms | 63.8 | 15.7 ms | 320 (5 × 64) |
| `Rhyme-SM3-512` | verify | 2.22 M | 1.07 ms | 936 | 1.07 ms | 4100 (5 × 820) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Rhyme-SHAKE-128` | keygen | 166137 | 2132 KiB | 3160 KiB |
| `Rhyme-SHAKE-128` | sign | 166137 | 3008 KiB | 3152 KiB |
| `Rhyme-SHAKE-128` | verify | 166137 | 3072 KiB | 3136 KiB |
| `Rhyme-SHAKE-256` | keygen | 213849 | 2148 KiB | 3584 KiB |
| `Rhyme-SHAKE-256` | sign | 213849 | 3136 KiB | 3368 KiB |
| `Rhyme-SHAKE-256` | verify | 213849 | 3312 KiB | 3376 KiB |
| `Rhyme-SHAKE-384` | keygen | 305521 | 2172 KiB | 4564 KiB |
| `Rhyme-SHAKE-384` | sign | 305521 | 3664 KiB | 4016 KiB |
| `Rhyme-SHAKE-384` | verify | 305521 | 3960 KiB | 4032 KiB |
| `Rhyme-SHAKE-512` | keygen | 303657 | 2168 KiB | 4548 KiB |
| `Rhyme-SHAKE-512` | sign | 303657 | 3996 KiB | 4404 KiB |
| `Rhyme-SHAKE-512` | verify | 303657 | 4328 KiB | 4392 KiB |
| `Rhyme-SM3-128` | keygen | 161729 | 2100 KiB | 3812 KiB |
| `Rhyme-SM3-128` | sign | 161729 | 3028 KiB | 20364 KiB |
| `Rhyme-SM3-128` | verify | 161729 | 3036 KiB | 17196 KiB |
| `Rhyme-SM3-256` | keygen | 209089 | 2140 KiB | 5520 KiB |
| `Rhyme-SM3-256` | sign | 209089 | 3404 KiB | 21552 KiB |
| `Rhyme-SM3-256` | verify | 209089 | 3588 KiB | 16232 KiB |
| `Rhyme-SM3-384` | keygen | 300953 | 2152 KiB | 9288 KiB |
| `Rhyme-SM3-384` | sign | 300953 | 3812 KiB | 21336 KiB |
| `Rhyme-SM3-384` | verify | 300953 | 4076 KiB | 18568 KiB |
| `Rhyme-SM3-512` | keygen | 299057 | 2180 KiB | 9004 KiB |
| `Rhyme-SM3-512` | sign | 299057 | 3836 KiB | 22096 KiB |
| `Rhyme-SM3-512` | verify | 299057 | 4236 KiB | 16032 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `Rhyme-SHAKE-128` | 800 | 11072 | 1483 ≈ nominal |
| `Rhyme-SHAKE-256` | 1824 | 22336 | 3258 ≈ nominal |
| `Rhyme-SHAKE-384` | 2720 | 45760 | 4743 ≈ nominal |
| `Rhyme-SHAKE-512` | 3872 | 44864 | 7002 ≈ nominal |
| `Rhyme-SM3-128` | 800 | 11072 | 1483 ≈ nominal |
| `Rhyme-SM3-256` | 1824 | 22336 | 3258 ≈ nominal |
| `Rhyme-SM3-384` | 2720 | 45760 | 4743 ≈ nominal |
| `Rhyme-SM3-512` | 3872 | 44864 | 7002 ≈ nominal |

The submitted signature API reports a buffer capacity, not a fixed transmitted length; the nominal figure above is the specification's representative size, and actual signatures vary.

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **instance-dependent** — SM3 sets: counter XOF on sm3hash; SHAKE sets: own FIPS 202 (bypass)

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Rhyme-SHAKE-128` | keygen | 0.0% | 0.0% | drng 1 |
| `Rhyme-SHAKE-128` | sign | 0.0% | 0.0% | – |
| `Rhyme-SHAKE-128` | verify | 0.0% | 0.0% | – |
| `Rhyme-SHAKE-256` | keygen | 0.0% | 0.0% | drng 1 |
| `Rhyme-SHAKE-256` | sign | 0.0% | 0.0% | – |
| `Rhyme-SHAKE-256` | verify | 0.0% | 0.0% | – |
| `Rhyme-SHAKE-384` | keygen | 0.0% | 0.0% | drng 1 |
| `Rhyme-SHAKE-384` | sign | 0.0% | 0.0% | – |
| `Rhyme-SHAKE-384` | verify | 0.0% | 0.0% | – |
| `Rhyme-SHAKE-512` | keygen | 0.0% | 0.0% | drng 1 |
| `Rhyme-SHAKE-512` | sign | 0.0% | 0.0% | – |
| `Rhyme-SHAKE-512` | verify | 0.0% | 0.0% | – |
| `Rhyme-SM3-128` | keygen | 0.3% | 0.0% | drng 1, sm3hash 796 |
| `Rhyme-SM3-128` | sign | 75% | 0.0% | sm3hash 1.18e+03 |
| `Rhyme-SM3-128` | verify | 55% | 0.0% | sm3hash 92 |
| `Rhyme-SM3-256` | keygen | 0.4% | 0.0% | drng 1, sm3hash 1.77e+03 |
| `Rhyme-SM3-256` | sign | 77% | 0.0% | sm3hash 4.35e+03 |
| `Rhyme-SM3-256` | verify | 59% | 0.0% | sm3hash 241 |
| `Rhyme-SM3-384` | keygen | 0.2% | 0.0% | drng 1, sm3hash 1.38e+04 |
| `Rhyme-SM3-384` | sign | 74% | 0.0% | sm3hash 6.26e+03 |
| `Rhyme-SM3-384` | verify | 59% | 0.0% | sm3hash 446 |
| `Rhyme-SM3-512` | keygen | 0.3% | 0.0% | drng 1, sm3hash 6.59e+03 |
| `Rhyme-SM3-512` | sign | 76% | 0.0% | sm3hash 8.85e+03 |
| `Rhyme-SM3-512` | verify | 55% | 0.0% | sm3hash 474 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Rhyme-SHAKE-128` | KAT log (sha256 `3559fce51665e4e4…`) | `kat/sign-22/Rhyme-SHAKE-128.log` |
| `Rhyme-SHAKE-128` | timing keygen | `records/sign-22/Rhyme-SHAKE-128__keygen.json` |
| `Rhyme-SHAKE-128` | timing sign | `records/sign-22/Rhyme-SHAKE-128__sign.json` |
| `Rhyme-SHAKE-128` | timing verify | `records/sign-22/Rhyme-SHAKE-128__verify.json` |
| `Rhyme-SHAKE-128` | hash profile keygen | `profile/sign-22/Rhyme-SHAKE-128__keygen.json` |
| `Rhyme-SHAKE-128` | hash profile sign | `profile/sign-22/Rhyme-SHAKE-128__sign.json` |
| `Rhyme-SHAKE-128` | hash profile verify | `profile/sign-22/Rhyme-SHAKE-128__verify.json` |
| `Rhyme-SHAKE-256` | KAT log (sha256 `1b3f5916ff3c9d6e…`) | `kat/sign-22/Rhyme-SHAKE-256.log` |
| `Rhyme-SHAKE-256` | timing keygen | `records/sign-22/Rhyme-SHAKE-256__keygen.json` |
| `Rhyme-SHAKE-256` | timing sign | `records/sign-22/Rhyme-SHAKE-256__sign.json` |
| `Rhyme-SHAKE-256` | timing verify | `records/sign-22/Rhyme-SHAKE-256__verify.json` |
| `Rhyme-SHAKE-256` | hash profile keygen | `profile/sign-22/Rhyme-SHAKE-256__keygen.json` |
| `Rhyme-SHAKE-256` | hash profile sign | `profile/sign-22/Rhyme-SHAKE-256__sign.json` |
| `Rhyme-SHAKE-256` | hash profile verify | `profile/sign-22/Rhyme-SHAKE-256__verify.json` |
| `Rhyme-SHAKE-384` | KAT log (sha256 `2f14c775c51e2634…`) | `kat/sign-22/Rhyme-SHAKE-384.log` |
| `Rhyme-SHAKE-384` | timing keygen | `records/sign-22/Rhyme-SHAKE-384__keygen.json` |
| `Rhyme-SHAKE-384` | timing sign | `records/sign-22/Rhyme-SHAKE-384__sign.json` |
| `Rhyme-SHAKE-384` | timing verify | `records/sign-22/Rhyme-SHAKE-384__verify.json` |
| `Rhyme-SHAKE-384` | hash profile keygen | `profile/sign-22/Rhyme-SHAKE-384__keygen.json` |
| `Rhyme-SHAKE-384` | hash profile sign | `profile/sign-22/Rhyme-SHAKE-384__sign.json` |
| `Rhyme-SHAKE-384` | hash profile verify | `profile/sign-22/Rhyme-SHAKE-384__verify.json` |
| `Rhyme-SHAKE-512` | KAT log (sha256 `a8fb3bfafed8377f…`) | `kat/sign-22/Rhyme-SHAKE-512.log` |
| `Rhyme-SHAKE-512` | timing keygen | `records/sign-22/Rhyme-SHAKE-512__keygen.json` |
| `Rhyme-SHAKE-512` | timing sign | `records/sign-22/Rhyme-SHAKE-512__sign.json` |
| `Rhyme-SHAKE-512` | timing verify | `records/sign-22/Rhyme-SHAKE-512__verify.json` |
| `Rhyme-SHAKE-512` | hash profile keygen | `profile/sign-22/Rhyme-SHAKE-512__keygen.json` |
| `Rhyme-SHAKE-512` | hash profile sign | `profile/sign-22/Rhyme-SHAKE-512__sign.json` |
| `Rhyme-SHAKE-512` | hash profile verify | `profile/sign-22/Rhyme-SHAKE-512__verify.json` |
| `Rhyme-SM3-128` | KAT log (sha256 `790ffcbf37411ca1…`) | `kat/sign-22/Rhyme-SM3-128.log` |
| `Rhyme-SM3-128` | timing keygen | `records/sign-22/Rhyme-SM3-128__keygen.json` |
| `Rhyme-SM3-128` | timing sign | `records/sign-22/Rhyme-SM3-128__sign.json` |
| `Rhyme-SM3-128` | timing verify | `records/sign-22/Rhyme-SM3-128__verify.json` |
| `Rhyme-SM3-128` | hash profile keygen | `profile/sign-22/Rhyme-SM3-128__keygen.json` |
| `Rhyme-SM3-128` | hash profile sign | `profile/sign-22/Rhyme-SM3-128__sign.json` |
| `Rhyme-SM3-128` | hash profile verify | `profile/sign-22/Rhyme-SM3-128__verify.json` |
| `Rhyme-SM3-256` | KAT log (sha256 `ffdcf5cf85f00df6…`) | `kat/sign-22/Rhyme-SM3-256.log` |
| `Rhyme-SM3-256` | timing keygen | `records/sign-22/Rhyme-SM3-256__keygen.json` |
| `Rhyme-SM3-256` | timing sign | `records/sign-22/Rhyme-SM3-256__sign.json` |
| `Rhyme-SM3-256` | timing verify | `records/sign-22/Rhyme-SM3-256__verify.json` |
| `Rhyme-SM3-256` | hash profile keygen | `profile/sign-22/Rhyme-SM3-256__keygen.json` |
| `Rhyme-SM3-256` | hash profile sign | `profile/sign-22/Rhyme-SM3-256__sign.json` |
| `Rhyme-SM3-256` | hash profile verify | `profile/sign-22/Rhyme-SM3-256__verify.json` |
| `Rhyme-SM3-384` | KAT log (sha256 `6aedb8a92abbbf48…`) | `kat/sign-22/Rhyme-SM3-384.log` |
| `Rhyme-SM3-384` | timing keygen | `records/sign-22/Rhyme-SM3-384__keygen.json` |
| `Rhyme-SM3-384` | timing sign | `records/sign-22/Rhyme-SM3-384__sign.json` |
| `Rhyme-SM3-384` | timing verify | `records/sign-22/Rhyme-SM3-384__verify.json` |
| `Rhyme-SM3-384` | hash profile keygen | `profile/sign-22/Rhyme-SM3-384__keygen.json` |
| `Rhyme-SM3-384` | hash profile sign | `profile/sign-22/Rhyme-SM3-384__sign.json` |
| `Rhyme-SM3-384` | hash profile verify | `profile/sign-22/Rhyme-SM3-384__verify.json` |
| `Rhyme-SM3-512` | KAT log (sha256 `76fa3baf6bf63f87…`) | `kat/sign-22/Rhyme-SM3-512.log` |
| `Rhyme-SM3-512` | timing keygen | `records/sign-22/Rhyme-SM3-512__keygen.json` |
| `Rhyme-SM3-512` | timing sign | `records/sign-22/Rhyme-SM3-512__sign.json` |
| `Rhyme-SM3-512` | timing verify | `records/sign-22/Rhyme-SM3-512__verify.json` |
| `Rhyme-SM3-512` | hash profile keygen | `profile/sign-22/Rhyme-SM3-512__keygen.json` |
| `Rhyme-SM3-512` | hash profile sign | `profile/sign-22/Rhyme-SM3-512__sign.json` |
| `Rhyme-SM3-512` | hash profile verify | `profile/sign-22/Rhyme-SM3-512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

