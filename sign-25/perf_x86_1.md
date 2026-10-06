# sign-25 SQIsign2D2 — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-25` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561096361562112.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: SQIsign2D2
- Implementation versions measured: reference
- Parameter sets: `SQISign2Dsquare-Level1-eff_compressed`, `SQISign2Dsquare-Level1-eff_uncompressed`, `SQISign2Dsquare-Level1-sec_compressed`, `SQISign2Dsquare-Level1-sec_uncompressed`, `SQISign2Dsquare-Level2-eff_compressed`, `SQISign2Dsquare-Level2-eff_uncompressed`, `SQISign2Dsquare-Level2-sec_compressed`, `SQISign2Dsquare-Level2-sec_uncompressed`, `SQISign2Dsquare-Level3-eff_compressed`, `SQISign2Dsquare-Level3-eff_uncompressed`, `SQISign2Dsquare-Level3-sec_compressed`, `SQISign2Dsquare-Level3-sec_uncompressed`, `SQISign2Dsquare-Level5-eff_compressed`, `SQISign2Dsquare-Level5-eff_uncompressed`, `SQISign2Dsquare-Level5-sec_compressed`, `SQISign2Dsquare-Level5-sec_uncompressed`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-25/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `SQISign2Dsquare-Level1-eff_compressed` | guide | PASS |
| `SQISign2Dsquare-Level1-eff_uncompressed` | guide | PASS |
| `SQISign2Dsquare-Level1-sec_compressed` | guide | PASS |
| `SQISign2Dsquare-Level1-sec_uncompressed` | guide | PASS |
| `SQISign2Dsquare-Level2-eff_compressed` | guide | PASS |
| `SQISign2Dsquare-Level2-eff_uncompressed` | harness-default | CRYPTOFAIL [1] |
| `SQISign2Dsquare-Level2-sec_compressed` | guide | PASS |
| `SQISign2Dsquare-Level2-sec_uncompressed` | guide | PASS |
| `SQISign2Dsquare-Level3-eff_compressed` | guide | PASS |
| `SQISign2Dsquare-Level3-eff_uncompressed` | guide | PASS |
| `SQISign2Dsquare-Level3-sec_compressed` | guide | PASS |
| `SQISign2Dsquare-Level3-sec_uncompressed` | guide | PASS |
| `SQISign2Dsquare-Level5-eff_compressed` | guide | PASS |
| `SQISign2Dsquare-Level5-eff_uncompressed` | guide | PASS |
| `SQISign2Dsquare-Level5-sec_compressed` | guide | PASS |
| `SQISign2Dsquare-Level5-sec_uncompressed` | guide | PASS |

[1] CRYPTOFAIL: sig_verify accepts a modified message because the verifier's verdict is decided by stale stack contents (confirmed finding sign-25-1). Key generation and signing are timed normally; the verification time is that of the flawed verifier. These instances are timed anyway; their output is not validated.

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `SQISign2Dsquare-Level1-eff_compressed` | keygen | 48.79 M | 23.4 ms | 42.8 | 23.4 ms | 225 (5 × 45) |
| `SQISign2Dsquare-Level1-eff_compressed` | sign | 162.12 M | 77.8 ms | 12.8 | 77.9 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level1-eff_compressed` | verify | 34.65 M | 16.7 ms | 59.9 | 16.7 ms | 300 (5 × 60) |
| `SQISign2Dsquare-Level1-eff_uncompressed` | keygen | 25.31 M | 12.2 ms | 82.2 | 12.2 ms | 440 (5 × 88) |
| `SQISign2Dsquare-Level1-eff_uncompressed` | sign | 139.10 M | 66.7 ms | 15 | 66.7 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level1-eff_uncompressed` | verify | 28.48 M | 13.7 ms | 72.9 | 13.7 ms | 360 (5 × 72) |
| `SQISign2Dsquare-Level1-sec_compressed` | keygen | 67.27 M | 32.2 ms | 31 | 32.2 ms | 155 (5 × 31) |
| `SQISign2Dsquare-Level1-sec_compressed` | sign | 223.10 M | 107 ms | 9.34 | 107 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level1-sec_compressed` | verify | 46.29 M | 22.3 ms | 44.9 | 22.3 ms | 220 (5 × 44) |
| `SQISign2Dsquare-Level1-sec_uncompressed` | keygen | 38.91 M | 18.7 ms | 53.5 | 18.7 ms | 280 (5 × 56) |
| `SQISign2Dsquare-Level1-sec_uncompressed` | sign | 217.06 M | 104 ms | 9.61 | 104 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level1-sec_uncompressed` | verify | 45.58 M | 21.9 ms | 45.6 | 21.9 ms | 225 (5 × 45) |
| `SQISign2Dsquare-Level2-eff_compressed` | keygen | 53.82 M | 25.9 ms | 38.6 | 25.9 ms | 200 (5 × 40) |
| `SQISign2Dsquare-Level2-eff_compressed` | sign | 177.99 M | 85.8 ms | 11.7 | 85.8 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level2-eff_compressed` | verify | 41.99 M | 20.3 ms | 49.3 | 20.3 ms | 250 (5 × 50) |
| `SQISign2Dsquare-Level2-eff_uncompressed` | keygen | 28.88 M | 13.9 ms | 71.7 | 13.9 ms | 380 (5 × 76) |
| `SQISign2Dsquare-Level2-eff_uncompressed` | sign | 168.46 M | 81 ms | 12.3 | 81 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level2-eff_uncompressed` | verify | 38.76 M | 18.7 ms | 53.4 | 18.7 ms | 270 (5 × 54) |
| `SQISign2Dsquare-Level2-sec_compressed` | keygen | 122.46 M | 58.6 ms | 17.1 | 58.6 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level2-sec_compressed` | sign | 394.31 M | 190 ms | 5.25 | 189 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level2-sec_compressed` | verify | 98.08 M | 47.1 ms | 21.2 | 47.1 ms | 110 (5 × 22) |
| `SQISign2Dsquare-Level2-sec_uncompressed` | keygen | 60.07 M | 28.8 ms | 34.7 | 28.8 ms | 175 (5 × 35) |
| `SQISign2Dsquare-Level2-sec_uncompressed` | sign | 330.40 M | 158 ms | 6.32 | 158 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level2-sec_uncompressed` | verify | 71.94 M | 34.6 ms | 28.9 | 34.6 ms | 145 (5 × 29) |
| `SQISign2Dsquare-Level3-eff_compressed` | keygen | 263.66 M | 126 ms | 7.92 | 126 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level3-eff_compressed` | sign | 899.01 M | 432 ms | 2.31 | 431 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level3-eff_compressed` | verify | 202.48 M | 97.2 ms | 10.3 | 97.2 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level3-eff_uncompressed` | keygen | 141.90 M | 68 ms | 14.7 | 68 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level3-eff_uncompressed` | sign | 844.61 M | 406 ms | 2.46 | 405 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level3-eff_uncompressed` | verify | 189.07 M | 90.8 ms | 11 | 90.8 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level3-sec_compressed` | keygen | 371.43 M | 178 ms | 5.63 | 178 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level3-sec_compressed` | sign | 1.19 G | 570 ms | 1.75 | 570 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level3-sec_compressed` | verify | 264.35 M | 127 ms | 7.88 | 127 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level3-sec_uncompressed` | keygen | 198.03 M | 94.9 ms | 10.5 | 94.9 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level3-sec_uncompressed` | sign | 1.13 G | 543 ms | 1.84 | 542 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level3-sec_uncompressed` | verify | 246.60 M | 118 ms | 8.45 | 118 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level5-eff_compressed` | keygen | 1.92 G | 923 ms | 1.08 | 922 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level5-eff_compressed` | sign | 6.25 G | 2.99 s | 0.335 | 2.99 s | 100 (5 × 20) |
| `SQISign2Dsquare-Level5-eff_compressed` | verify | 1.37 G | 658 ms | 1.52 | 656 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level5-eff_uncompressed` | keygen | 1.01 G | 481 ms | 2.08 | 481 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level5-eff_uncompressed` | sign | 5.67 G | 2.73 s | 0.367 | 2.73 s | 100 (5 × 20) |
| `SQISign2Dsquare-Level5-eff_uncompressed` | verify | 1.20 G | 575 ms | 1.74 | 574 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level5-sec_compressed` | keygen | 2.50 G | 1.2 s | 0.832 | 1.2 s | 100 (5 × 20) |
| `SQISign2Dsquare-Level5-sec_compressed` | sign | 7.95 G | 3.82 s | 0.262 | 3.82 s | 100 (5 × 20) |
| `SQISign2Dsquare-Level5-sec_compressed` | verify | 1.77 G | 852 ms | 1.17 | 849 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level5-sec_uncompressed` | keygen | 1.32 G | 635 ms | 1.57 | 633 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level5-sec_uncompressed` | sign | 7.23 G | 3.46 s | 0.289 | 3.45 s | 100 (5 × 20) |
| `SQISign2Dsquare-Level5-sec_uncompressed` | verify | 1.52 G | 731 ms | 1.37 | 730 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `SQISign2Dsquare-Level1-eff_compressed` | keygen | 264509 | 1948 KiB | 11636 KiB |
| `SQISign2Dsquare-Level1-eff_compressed` | sign | 264509 | 2664 KiB | 23760 KiB |
| `SQISign2Dsquare-Level1-eff_compressed` | verify | 264509 | 3712 KiB | 24180 KiB |
| `SQISign2Dsquare-Level1-eff_uncompressed` | keygen | 262429 | 1960 KiB | 18652 KiB |
| `SQISign2Dsquare-Level1-eff_uncompressed` | sign | 262429 | 2652 KiB | 17212 KiB |
| `SQISign2Dsquare-Level1-eff_uncompressed` | verify | 262429 | 3400 KiB | 27736 KiB |
| `SQISign2Dsquare-Level1-sec_compressed` | keygen | 264033 | 1952 KiB | 10956 KiB |
| `SQISign2Dsquare-Level1-sec_compressed` | sign | 264033 | 2708 KiB | 29872 KiB |
| `SQISign2Dsquare-Level1-sec_compressed` | verify | 264033 | 4088 KiB | 23760 KiB |
| `SQISign2Dsquare-Level1-sec_uncompressed` | keygen | 261633 | 1944 KiB | 16152 KiB |
| `SQISign2Dsquare-Level1-sec_uncompressed` | sign | 261633 | 2716 KiB | 21260 KiB |
| `SQISign2Dsquare-Level1-sec_uncompressed` | verify | 261633 | 3628 KiB | 23876 KiB |
| `SQISign2Dsquare-Level2-eff_compressed` | keygen | 263565 | 1964 KiB | 14560 KiB |
| `SQISign2Dsquare-Level2-eff_compressed` | sign | 263565 | 2776 KiB | 34216 KiB |
| `SQISign2Dsquare-Level2-eff_compressed` | verify | 263565 | 4340 KiB | 30248 KiB |
| `SQISign2Dsquare-Level2-eff_uncompressed` | keygen | – | 1984 KiB | 23336 KiB |
| `SQISign2Dsquare-Level2-eff_uncompressed` | sign | – | 2744 KiB | 24296 KiB |
| `SQISign2Dsquare-Level2-eff_uncompressed` | verify | – | 3884 KiB | 31728 KiB |
| `SQISign2Dsquare-Level2-sec_compressed` | keygen | 279385 | 1956 KiB | 9824 KiB |
| `SQISign2Dsquare-Level2-sec_compressed` | sign | 279385 | 2840 KiB | 41740 KiB |
| `SQISign2Dsquare-Level2-sec_compressed` | verify | 279385 | 4752 KiB | 19928 KiB |
| `SQISign2Dsquare-Level2-sec_uncompressed` | keygen | 277073 | 1976 KiB | 14872 KiB |
| `SQISign2Dsquare-Level2-sec_uncompressed` | sign | 277073 | 2824 KiB | 29372 KiB |
| `SQISign2Dsquare-Level2-sec_uncompressed` | verify | 277073 | 4116 KiB | 23508 KiB |
| `SQISign2Dsquare-Level3-eff_compressed` | keygen | 307153 | 1944 KiB | 16444 KiB |
| `SQISign2Dsquare-Level3-eff_compressed` | sign | 307153 | 3200 KiB | 81828 KiB |
| `SQISign2Dsquare-Level3-eff_compressed` | verify | 307153 | 7048 KiB | 32860 KiB |
| `SQISign2Dsquare-Level3-eff_uncompressed` | keygen | 304817 | 2000 KiB | 15864 KiB |
| `SQISign2Dsquare-Level3-eff_uncompressed` | sign | 304817 | 3156 KiB | 56500 KiB |
| `SQISign2Dsquare-Level3-eff_uncompressed` | verify | 304817 | 5816 KiB | 31620 KiB |
| `SQISign2Dsquare-Level3-sec_compressed` | keygen | 312957 | 1988 KiB | 18448 KiB |
| `SQISign2Dsquare-Level3-sec_compressed` | sign | 312957 | 3348 KiB | 91596 KiB |
| `SQISign2Dsquare-Level3-sec_compressed` | verify | 312957 | 7600 KiB | 36684 KiB |
| `SQISign2Dsquare-Level3-sec_uncompressed` | keygen | 310429 | 1980 KiB | 17836 KiB |
| `SQISign2Dsquare-Level3-sec_uncompressed` | sign | 310429 | 3304 KiB | 63068 KiB |
| `SQISign2Dsquare-Level3-sec_uncompressed` | verify | 310429 | 6216 KiB | 35260 KiB |
| `SQISign2Dsquare-Level5-eff_compressed` | keygen | 438793 | 2012 KiB | 54296 KiB |
| `SQISign2Dsquare-Level5-eff_compressed` | sign | 438793 | 6452 KiB | 333488 KiB |
| `SQISign2Dsquare-Level5-eff_compressed` | verify | 438793 | 19576 KiB | 118292 KiB |
| `SQISign2Dsquare-Level5-eff_uncompressed` | keygen | 436457 | 2008 KiB | 53064 KiB |
| `SQISign2Dsquare-Level5-eff_uncompressed` | sign | 436457 | 5144 KiB | 206240 KiB |
| `SQISign2Dsquare-Level5-eff_uncompressed` | verify | 436457 | 14836 KiB | 113516 KiB |
| `SQISign2Dsquare-Level5-sec_compressed` | keygen | 459293 | 2004 KiB | 60204 KiB |
| `SQISign2Dsquare-Level5-sec_compressed` | sign | 459293 | 5468 KiB | 336032 KiB |
| `SQISign2Dsquare-Level5-sec_compressed` | verify | 459293 | 21388 KiB | 130620 KiB |
| `SQISign2Dsquare-Level5-sec_uncompressed` | keygen | 455365 | 2000 KiB | 58872 KiB |
| `SQISign2Dsquare-Level5-sec_uncompressed` | sign | 455365 | 5412 KiB | 227888 KiB |
| `SQISign2Dsquare-Level5-sec_uncompressed` | verify | 455365 | 16128 KiB | 125328 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `SQISign2Dsquare-Level1-eff_compressed` | 65 | 240 | 168 |
| `SQISign2Dsquare-Level1-eff_uncompressed` | 64 | 488 | 200 |
| `SQISign2Dsquare-Level1-sec_compressed` | 69 | 257 | 178 |
| `SQISign2Dsquare-Level1-sec_uncompressed` | 68 | 521 | 212 |
| `SQISign2Dsquare-Level2-eff_compressed` | 81 | 298 | 208 |
| `SQISign2Dsquare-Level2-eff_uncompressed` | 80 | 610 | 248 |
| `SQISign2Dsquare-Level2-sec_compressed` | 85 | 315 | 218 |
| `SQISign2Dsquare-Level2-sec_uncompressed` | 84 | 643 | 260 |
| `SQISign2Dsquare-Level3-eff_compressed` | 129 | 468 | 326 |
| `SQISign2Dsquare-Level3-eff_uncompressed` | 128 | 976 | 392 |
| `SQISign2Dsquare-Level3-sec_compressed` | 133 | 489 | 338 |
| `SQISign2Dsquare-Level3-sec_uncompressed` | 132 | 1009 | 404 |
| `SQISign2Dsquare-Level5-eff_compressed` | 257 | 936 | 648 |
| `SQISign2Dsquare-Level5-eff_uncompressed` | 256 | 1952 | 776 |
| `SQISign2Dsquare-Level5-sec_compressed` | 269 | 982 | 678 |
| `SQISign2Dsquare-Level5-sec_uncompressed` | 268 | 2046 | 812 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `SQISign2Dsquare-Level1-eff_compressed` | keygen | 0.0% | 2.6% | drng 277 |
| `SQISign2Dsquare-Level1-eff_compressed` | sign | 0.5% | 1.1% | drng 397, pseudohash 56.6 |
| `SQISign2Dsquare-Level1-eff_compressed` | verify | 1.2% | 0.0% | pseudohash 32 |
| `SQISign2Dsquare-Level1-eff_uncompressed` | keygen | 0.0% | 4.6% | drng 260 |
| `SQISign2Dsquare-Level1-eff_uncompressed` | sign | 0.4% | 1.6% | drng 474, pseudohash 40.5 |
| `SQISign2Dsquare-Level1-eff_uncompressed` | verify | 1.4% | 0.0% | pseudohash 32 |
| `SQISign2Dsquare-Level1-sec_compressed` | keygen | 0.0% | 2.0% | drng 293 |
| `SQISign2Dsquare-Level1-sec_compressed` | sign | 0.0% | 1.2% | drng 588, pseudohash 2 |
| `SQISign2Dsquare-Level1-sec_compressed` | verify | 0.0% | 0.0% | pseudohash 1 |
| `SQISign2Dsquare-Level1-sec_uncompressed` | keygen | 0.0% | 3.1% | drng 266 |
| `SQISign2Dsquare-Level1-sec_uncompressed` | sign | 0.0% | 0.7% | drng 355, pseudohash 2.2 |
| `SQISign2Dsquare-Level1-sec_uncompressed` | verify | 0.0% | 0.0% | pseudohash 1 |
| `SQISign2Dsquare-Level2-eff_compressed` | keygen | 0.0% | 2.5% | drng 305 |
| `SQISign2Dsquare-Level2-eff_compressed` | sign | 2.6% | 0.9% | drng 362, pseudohash 363 |
| `SQISign2Dsquare-Level2-eff_compressed` | verify | 15% | 0.0% | pseudohash 512 |
| `SQISign2Dsquare-Level2-eff_uncompressed` | keygen | 0.0% | 4.5% | drng 286 |
| `SQISign2Dsquare-Level2-eff_uncompressed` | sign | 2.4% | 1.1% | drng 411, pseudohash 315 |
| `SQISign2Dsquare-Level2-eff_uncompressed` | verify | 17% | 0.0% | pseudohash 512 |
| `SQISign2Dsquare-Level2-sec_compressed` | keygen | 0.0% | 1.5% | drng 406 |
| `SQISign2Dsquare-Level2-sec_compressed` | sign | 0.0% | 0.3% | drng 277, pseudohash 2 |
| `SQISign2Dsquare-Level2-sec_compressed` | verify | 0.0% | 0.0% | pseudohash 1 |
| `SQISign2Dsquare-Level2-sec_uncompressed` | keygen | 0.0% | 2.7% | drng 357 |
| `SQISign2Dsquare-Level2-sec_uncompressed` | sign | 0.0% | 0.3% | drng 240, pseudohash 1.71 |
| `SQISign2Dsquare-Level2-sec_uncompressed` | verify | 0.0% | 0.0% | pseudohash 1 |
| `SQISign2Dsquare-Level3-eff_compressed` | keygen | 0.0% | 0.8% | drng 474 |
| `SQISign2Dsquare-Level3-eff_compressed` | sign | 1.0% | 0.3% | drng 504, pseudohash 683 |
| `SQISign2Dsquare-Level3-eff_compressed` | verify | 6.4% | 0.0% | pseudohash 1.02e+03 |
| `SQISign2Dsquare-Level3-eff_uncompressed` | keygen | 0.0% | 1.6% | drng 498 |
| `SQISign2Dsquare-Level3-eff_uncompressed` | sign | 1.0% | 0.3% | drng 512, pseudohash 683 |
| `SQISign2Dsquare-Level3-eff_uncompressed` | verify | 6.9% | 0.0% | pseudohash 1.02e+03 |
| `SQISign2Dsquare-Level3-sec_compressed` | keygen | 0.0% | 0.6% | drng 514 |
| `SQISign2Dsquare-Level3-sec_compressed` | sign | 0.0% | 0.1% | drng 193, pseudohash 1.33 |
| `SQISign2Dsquare-Level3-sec_compressed` | verify | 0.0% | 0.0% | pseudohash 1 |
| `SQISign2Dsquare-Level3-sec_uncompressed` | keygen | 0.0% | 1.2% | drng 517 |
| `SQISign2Dsquare-Level3-sec_uncompressed` | sign | 0.0% | 0.2% | drng 353, pseudohash 1.33 |
| `SQISign2Dsquare-Level3-sec_uncompressed` | verify | 0.0% | 0.0% | pseudohash 1 |
| `SQISign2Dsquare-Level5-eff_compressed` | keygen | 0.0% | 0.3% | drng 1.32e+03 |
| `SQISign2Dsquare-Level5-eff_compressed` | sign | 0.1% | 0.1% | drng 1.25e+03, pseudohash 171 |
| `SQISign2Dsquare-Level5-eff_compressed` | verify | 0.7% | 0.0% | pseudohash 256 |
| `SQISign2Dsquare-Level5-eff_uncompressed` | keygen | 0.0% | 0.6% | drng 1.32e+03 |
| `SQISign2Dsquare-Level5-eff_uncompressed` | sign | 0.1% | 0.2% | drng 2.01e+03, pseudohash 171 |
| `SQISign2Dsquare-Level5-eff_uncompressed` | verify | 0.8% | 0.0% | pseudohash 256 |
| `SQISign2Dsquare-Level5-sec_compressed` | keygen | 0.0% | 0.5% | drng 2.29e+03 |
| `SQISign2Dsquare-Level5-sec_compressed` | sign | 0.0% | 0.1% | drng 1.25e+03, pseudohash 1 |
| `SQISign2Dsquare-Level5-sec_compressed` | verify | 0.0% | 0.0% | pseudohash 1 |
| `SQISign2Dsquare-Level5-sec_uncompressed` | keygen | 0.0% | 0.8% | drng 2.29e+03 |
| `SQISign2Dsquare-Level5-sec_uncompressed` | sign | 0.0% | 0.1% | drng 1.27e+03, pseudohash 1 |
| `SQISign2Dsquare-Level5-sec_uncompressed` | verify | 0.0% | 0.0% | pseudohash 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `SQISign2Dsquare-Level1-eff_compressed` | KAT log (sha256 `b6d062497ef81cc3…`) | `kat/sign-25/SQISign2Dsquare-Level1-eff_compressed.log` |
| `SQISign2Dsquare-Level1-eff_compressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level1-eff_compressed__keygen.json` |
| `SQISign2Dsquare-Level1-eff_compressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level1-eff_compressed__sign.json` |
| `SQISign2Dsquare-Level1-eff_compressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level1-eff_compressed__verify.json` |
| `SQISign2Dsquare-Level1-eff_compressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level1-eff_compressed__keygen.json` |
| `SQISign2Dsquare-Level1-eff_compressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level1-eff_compressed__sign.json` |
| `SQISign2Dsquare-Level1-eff_compressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level1-eff_compressed__verify.json` |
| `SQISign2Dsquare-Level1-eff_uncompressed` | KAT log (sha256 `e3a51a843bd63987…`) | `kat/sign-25/SQISign2Dsquare-Level1-eff_uncompressed.log` |
| `SQISign2Dsquare-Level1-eff_uncompressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level1-eff_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level1-eff_uncompressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level1-eff_uncompressed__sign.json` |
| `SQISign2Dsquare-Level1-eff_uncompressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level1-eff_uncompressed__verify.json` |
| `SQISign2Dsquare-Level1-eff_uncompressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level1-eff_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level1-eff_uncompressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level1-eff_uncompressed__sign.json` |
| `SQISign2Dsquare-Level1-eff_uncompressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level1-eff_uncompressed__verify.json` |
| `SQISign2Dsquare-Level1-sec_compressed` | KAT log (sha256 `baaefc82aa955650…`) | `kat/sign-25/SQISign2Dsquare-Level1-sec_compressed.log` |
| `SQISign2Dsquare-Level1-sec_compressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level1-sec_compressed__keygen.json` |
| `SQISign2Dsquare-Level1-sec_compressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level1-sec_compressed__sign.json` |
| `SQISign2Dsquare-Level1-sec_compressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level1-sec_compressed__verify.json` |
| `SQISign2Dsquare-Level1-sec_compressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level1-sec_compressed__keygen.json` |
| `SQISign2Dsquare-Level1-sec_compressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level1-sec_compressed__sign.json` |
| `SQISign2Dsquare-Level1-sec_compressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level1-sec_compressed__verify.json` |
| `SQISign2Dsquare-Level1-sec_uncompressed` | KAT log (sha256 `caff1a94bfd3b6bc…`) | `kat/sign-25/SQISign2Dsquare-Level1-sec_uncompressed.log` |
| `SQISign2Dsquare-Level1-sec_uncompressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level1-sec_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level1-sec_uncompressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level1-sec_uncompressed__sign.json` |
| `SQISign2Dsquare-Level1-sec_uncompressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level1-sec_uncompressed__verify.json` |
| `SQISign2Dsquare-Level1-sec_uncompressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level1-sec_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level1-sec_uncompressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level1-sec_uncompressed__sign.json` |
| `SQISign2Dsquare-Level1-sec_uncompressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level1-sec_uncompressed__verify.json` |
| `SQISign2Dsquare-Level2-eff_compressed` | KAT log (sha256 `0cb7c3cb53c75efb…`) | `kat/sign-25/SQISign2Dsquare-Level2-eff_compressed.log` |
| `SQISign2Dsquare-Level2-eff_compressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level2-eff_compressed__keygen.json` |
| `SQISign2Dsquare-Level2-eff_compressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level2-eff_compressed__sign.json` |
| `SQISign2Dsquare-Level2-eff_compressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level2-eff_compressed__verify.json` |
| `SQISign2Dsquare-Level2-eff_compressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level2-eff_compressed__keygen.json` |
| `SQISign2Dsquare-Level2-eff_compressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level2-eff_compressed__sign.json` |
| `SQISign2Dsquare-Level2-eff_compressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level2-eff_compressed__verify.json` |
| `SQISign2Dsquare-Level2-eff_uncompressed` | KAT log (sha256 `1d4adeb5167123af…`) | `kat/sign-25/SQISign2Dsquare-Level2-eff_uncompressed.log` |
| `SQISign2Dsquare-Level2-eff_uncompressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level2-eff_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level2-eff_uncompressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level2-eff_uncompressed__sign.json` |
| `SQISign2Dsquare-Level2-eff_uncompressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level2-eff_uncompressed__verify.json` |
| `SQISign2Dsquare-Level2-eff_uncompressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level2-eff_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level2-eff_uncompressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level2-eff_uncompressed__sign.json` |
| `SQISign2Dsquare-Level2-eff_uncompressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level2-eff_uncompressed__verify.json` |
| `SQISign2Dsquare-Level2-sec_compressed` | KAT log (sha256 `218b8880f48076aa…`) | `kat/sign-25/SQISign2Dsquare-Level2-sec_compressed.log` |
| `SQISign2Dsquare-Level2-sec_compressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level2-sec_compressed__keygen.json` |
| `SQISign2Dsquare-Level2-sec_compressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level2-sec_compressed__sign.json` |
| `SQISign2Dsquare-Level2-sec_compressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level2-sec_compressed__verify.json` |
| `SQISign2Dsquare-Level2-sec_compressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level2-sec_compressed__keygen.json` |
| `SQISign2Dsquare-Level2-sec_compressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level2-sec_compressed__sign.json` |
| `SQISign2Dsquare-Level2-sec_compressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level2-sec_compressed__verify.json` |
| `SQISign2Dsquare-Level2-sec_uncompressed` | KAT log (sha256 `2e38aa4b95adbe20…`) | `kat/sign-25/SQISign2Dsquare-Level2-sec_uncompressed.log` |
| `SQISign2Dsquare-Level2-sec_uncompressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level2-sec_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level2-sec_uncompressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level2-sec_uncompressed__sign.json` |
| `SQISign2Dsquare-Level2-sec_uncompressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level2-sec_uncompressed__verify.json` |
| `SQISign2Dsquare-Level2-sec_uncompressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level2-sec_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level2-sec_uncompressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level2-sec_uncompressed__sign.json` |
| `SQISign2Dsquare-Level2-sec_uncompressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level2-sec_uncompressed__verify.json` |
| `SQISign2Dsquare-Level3-eff_compressed` | KAT log (sha256 `945f5ef8331c5135…`) | `kat/sign-25/SQISign2Dsquare-Level3-eff_compressed.log` |
| `SQISign2Dsquare-Level3-eff_compressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level3-eff_compressed__keygen.json` |
| `SQISign2Dsquare-Level3-eff_compressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level3-eff_compressed__sign.json` |
| `SQISign2Dsquare-Level3-eff_compressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level3-eff_compressed__verify.json` |
| `SQISign2Dsquare-Level3-eff_compressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level3-eff_compressed__keygen.json` |
| `SQISign2Dsquare-Level3-eff_compressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level3-eff_compressed__sign.json` |
| `SQISign2Dsquare-Level3-eff_compressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level3-eff_compressed__verify.json` |
| `SQISign2Dsquare-Level3-eff_uncompressed` | KAT log (sha256 `8f6e225aa5fa2ead…`) | `kat/sign-25/SQISign2Dsquare-Level3-eff_uncompressed.log` |
| `SQISign2Dsquare-Level3-eff_uncompressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level3-eff_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level3-eff_uncompressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level3-eff_uncompressed__sign.json` |
| `SQISign2Dsquare-Level3-eff_uncompressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level3-eff_uncompressed__verify.json` |
| `SQISign2Dsquare-Level3-eff_uncompressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level3-eff_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level3-eff_uncompressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level3-eff_uncompressed__sign.json` |
| `SQISign2Dsquare-Level3-eff_uncompressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level3-eff_uncompressed__verify.json` |
| `SQISign2Dsquare-Level3-sec_compressed` | KAT log (sha256 `f7802ec96b30b9c4…`) | `kat/sign-25/SQISign2Dsquare-Level3-sec_compressed.log` |
| `SQISign2Dsquare-Level3-sec_compressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level3-sec_compressed__keygen.json` |
| `SQISign2Dsquare-Level3-sec_compressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level3-sec_compressed__sign.json` |
| `SQISign2Dsquare-Level3-sec_compressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level3-sec_compressed__verify.json` |
| `SQISign2Dsquare-Level3-sec_compressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level3-sec_compressed__keygen.json` |
| `SQISign2Dsquare-Level3-sec_compressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level3-sec_compressed__sign.json` |
| `SQISign2Dsquare-Level3-sec_compressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level3-sec_compressed__verify.json` |
| `SQISign2Dsquare-Level3-sec_uncompressed` | KAT log (sha256 `6918298bac5ecb34…`) | `kat/sign-25/SQISign2Dsquare-Level3-sec_uncompressed.log` |
| `SQISign2Dsquare-Level3-sec_uncompressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level3-sec_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level3-sec_uncompressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level3-sec_uncompressed__sign.json` |
| `SQISign2Dsquare-Level3-sec_uncompressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level3-sec_uncompressed__verify.json` |
| `SQISign2Dsquare-Level3-sec_uncompressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level3-sec_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level3-sec_uncompressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level3-sec_uncompressed__sign.json` |
| `SQISign2Dsquare-Level3-sec_uncompressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level3-sec_uncompressed__verify.json` |
| `SQISign2Dsquare-Level5-eff_compressed` | KAT log (sha256 `6acf9e24036da424…`) | `kat/sign-25/SQISign2Dsquare-Level5-eff_compressed.log` |
| `SQISign2Dsquare-Level5-eff_compressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level5-eff_compressed__keygen.json` |
| `SQISign2Dsquare-Level5-eff_compressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level5-eff_compressed__sign.json` |
| `SQISign2Dsquare-Level5-eff_compressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level5-eff_compressed__verify.json` |
| `SQISign2Dsquare-Level5-eff_compressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level5-eff_compressed__keygen.json` |
| `SQISign2Dsquare-Level5-eff_compressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level5-eff_compressed__sign.json` |
| `SQISign2Dsquare-Level5-eff_compressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level5-eff_compressed__verify.json` |
| `SQISign2Dsquare-Level5-eff_uncompressed` | KAT log (sha256 `3fdf74a3b0f96f56…`) | `kat/sign-25/SQISign2Dsquare-Level5-eff_uncompressed.log` |
| `SQISign2Dsquare-Level5-eff_uncompressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level5-eff_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level5-eff_uncompressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level5-eff_uncompressed__sign.json` |
| `SQISign2Dsquare-Level5-eff_uncompressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level5-eff_uncompressed__verify.json` |
| `SQISign2Dsquare-Level5-eff_uncompressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level5-eff_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level5-eff_uncompressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level5-eff_uncompressed__sign.json` |
| `SQISign2Dsquare-Level5-eff_uncompressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level5-eff_uncompressed__verify.json` |
| `SQISign2Dsquare-Level5-sec_compressed` | KAT log (sha256 `00b8b602c6171ba5…`) | `kat/sign-25/SQISign2Dsquare-Level5-sec_compressed.log` |
| `SQISign2Dsquare-Level5-sec_compressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level5-sec_compressed__keygen.json` |
| `SQISign2Dsquare-Level5-sec_compressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level5-sec_compressed__sign.json` |
| `SQISign2Dsquare-Level5-sec_compressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level5-sec_compressed__verify.json` |
| `SQISign2Dsquare-Level5-sec_compressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level5-sec_compressed__keygen.json` |
| `SQISign2Dsquare-Level5-sec_compressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level5-sec_compressed__sign.json` |
| `SQISign2Dsquare-Level5-sec_compressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level5-sec_compressed__verify.json` |
| `SQISign2Dsquare-Level5-sec_uncompressed` | KAT log (sha256 `4872b0fff4a1c4bc…`) | `kat/sign-25/SQISign2Dsquare-Level5-sec_uncompressed.log` |
| `SQISign2Dsquare-Level5-sec_uncompressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level5-sec_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level5-sec_uncompressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level5-sec_uncompressed__sign.json` |
| `SQISign2Dsquare-Level5-sec_uncompressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level5-sec_uncompressed__verify.json` |
| `SQISign2Dsquare-Level5-sec_uncompressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level5-sec_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level5-sec_uncompressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level5-sec_uncompressed__sign.json` |
| `SQISign2Dsquare-Level5-sec_uncompressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level5-sec_uncompressed__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

