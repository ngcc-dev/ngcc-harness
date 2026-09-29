# sign-25 SQIsign2D2 — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-25` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561096361562112.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: SQIsign2D2
- Implementation versions measured: reference
- Parameter sets: `SQISign2Dsquare-Level1-eff_compressed`, `SQISign2Dsquare-Level1-eff_uncompressed`, `SQISign2Dsquare-Level1-sec_compressed`, `SQISign2Dsquare-Level1-sec_uncompressed`, `SQISign2Dsquare-Level2-eff_compressed`, `SQISign2Dsquare-Level2-eff_uncompressed`, `SQISign2Dsquare-Level2-sec_compressed`, `SQISign2Dsquare-Level2-sec_uncompressed`, `SQISign2Dsquare-Level3-eff_compressed`, `SQISign2Dsquare-Level3-eff_uncompressed`, `SQISign2Dsquare-Level3-sec_compressed`, `SQISign2Dsquare-Level3-sec_uncompressed`, `SQISign2Dsquare-Level5-eff_compressed`, `SQISign2Dsquare-Level5-eff_uncompressed`, `SQISign2Dsquare-Level5-sec_compressed`, `SQISign2Dsquare-Level5-sec_uncompressed`

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
| `SQISign2Dsquare-Level1-eff_compressed` | keygen | 46.24 M | 17.2 ms | 58.2 | 17.2 ms | 190 (5 × 38) |
| `SQISign2Dsquare-Level1-eff_compressed` | sign | 152.62 M | 56.7 ms | 17.6 | 56.6 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level1-eff_compressed` | verify | 32.94 M | 12.2 ms | 81.7 | 12.2 ms | 255 (5 × 51) |
| `SQISign2Dsquare-Level1-eff_uncompressed` | keygen | 25.48 M | 9.46 ms | 106 | 9.46 ms | 345 (5 × 69) |
| `SQISign2Dsquare-Level1-eff_uncompressed` | sign | 137.80 M | 51.2 ms | 19.5 | 51.2 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level1-eff_uncompressed` | verify | 29.13 M | 10.8 ms | 92.3 | 10.8 ms | 295 (5 × 59) |
| `SQISign2Dsquare-Level1-sec_compressed` | keygen | 66.74 M | 24.8 ms | 40.4 | 24.8 ms | 125 (5 × 25) |
| `SQISign2Dsquare-Level1-sec_compressed` | sign | 218.62 M | 81.2 ms | 12.3 | 81.1 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level1-sec_compressed` | verify | 46.54 M | 17.3 ms | 57.8 | 17.3 ms | 180 (5 × 36) |
| `SQISign2Dsquare-Level1-sec_uncompressed` | keygen | 35.06 M | 13 ms | 76.8 | 13 ms | 255 (5 × 51) |
| `SQISign2Dsquare-Level1-sec_uncompressed` | sign | 197.49 M | 73.3 ms | 13.6 | 72.9 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level1-sec_uncompressed` | verify | 41.53 M | 15.4 ms | 64.7 | 15.4 ms | 205 (5 × 41) |
| `SQISign2Dsquare-Level2-eff_compressed` | keygen | 43.05 M | 16 ms | 62.5 | 15.9 ms | 210 (5 × 42) |
| `SQISign2Dsquare-Level2-eff_compressed` | sign | 140.46 M | 52.3 ms | 19.1 | 52.2 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level2-eff_compressed` | verify | 33.54 M | 12.5 ms | 80.1 | 12.4 ms | 255 (5 × 51) |
| `SQISign2Dsquare-Level2-eff_uncompressed` | keygen | 23.15 M | 8.61 ms | 116 | 8.61 ms | 380 (5 × 76) |
| `SQISign2Dsquare-Level2-eff_uncompressed` | sign | 132.36 M | 49.2 ms | 20.3 | 49.1 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level2-eff_uncompressed` | verify | 30.45 M | 11.3 ms | 88.2 | 11.3 ms | 285 (5 × 57) |
| `SQISign2Dsquare-Level2-sec_compressed` | keygen | 104.76 M | 38.9 ms | 25.7 | 38.5 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level2-sec_compressed` | sign | 330.48 M | 123 ms | 8.14 | 123 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level2-sec_compressed` | verify | 81.97 M | 30.5 ms | 32.8 | 30.4 ms | 105 (5 × 21) |
| `SQISign2Dsquare-Level2-sec_uncompressed` | keygen | 55.03 M | 20.5 ms | 48.9 | 20.3 ms | 160 (5 × 32) |
| `SQISign2Dsquare-Level2-sec_uncompressed` | sign | 296.46 M | 110 ms | 9.08 | 110 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level2-sec_uncompressed` | verify | 64.82 M | 24.1 ms | 41.5 | 24.1 ms | 135 (5 × 27) |
| `SQISign2Dsquare-Level3-eff_compressed` | keygen | 226.27 M | 84 ms | 11.9 | 84.7 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level3-eff_compressed` | sign | 763.78 M | 284 ms | 3.52 | 281 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level3-eff_compressed` | verify | 174.05 M | 64.9 ms | 15.4 | 64.8 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level3-eff_uncompressed` | keygen | 116.22 M | 43.2 ms | 23.2 | 43.1 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level3-eff_uncompressed` | sign | 694.01 M | 258 ms | 3.88 | 257 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level3-eff_uncompressed` | verify | 156.24 M | 58.3 ms | 17.1 | 58.2 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level3-sec_compressed` | keygen | 286.29 M | 106 ms | 9.41 | 106 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level3-sec_compressed` | sign | 930.37 M | 346 ms | 2.89 | 344 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level3-sec_compressed` | verify | 202.27 M | 75.5 ms | 13.3 | 75.4 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level3-sec_uncompressed` | keygen | 152.02 M | 56.5 ms | 17.7 | 55.9 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level3-sec_uncompressed` | sign | 846.47 M | 314 ms | 3.18 | 314 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level3-sec_uncompressed` | verify | 182.01 M | 67.9 ms | 14.7 | 67.9 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level5-eff_compressed` | keygen | 1.70 G | 631 ms | 1.59 | 626 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level5-eff_compressed` | sign | 5.52 G | 2.05 s | 0.487 | 2.05 s | 100 (5 × 20) |
| `SQISign2Dsquare-Level5-eff_compressed` | verify | 1.20 G | 447 ms | 2.24 | 448 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level5-eff_uncompressed` | keygen | 886.76 M | 330 ms | 3.03 | 330 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level5-eff_uncompressed` | sign | 5.02 G | 1.86 s | 0.536 | 1.86 s | 100 (5 × 20) |
| `SQISign2Dsquare-Level5-eff_uncompressed` | verify | 1.05 G | 391 ms | 2.56 | 391 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level5-sec_compressed` | keygen | 2.04 G | 759 ms | 1.32 | 759 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level5-sec_compressed` | sign | 6.53 G | 2.43 s | 0.412 | 2.42 s | 100 (5 × 20) |
| `SQISign2Dsquare-Level5-sec_compressed` | verify | 1.44 G | 535 ms | 1.87 | 535 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level5-sec_uncompressed` | keygen | 1.10 G | 407 ms | 2.46 | 407 ms | 100 (5 × 20) |
| `SQISign2Dsquare-Level5-sec_uncompressed` | sign | 5.94 G | 2.2 s | 0.454 | 2.2 s | 100 (5 × 20) |
| `SQISign2Dsquare-Level5-sec_uncompressed` | verify | 1.24 G | 461 ms | 2.17 | 461 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `SQISign2Dsquare-Level1-eff_compressed` | keygen | 244300 | 1600 KiB | 13900 KiB |
| `SQISign2Dsquare-Level1-eff_compressed` | sign | 244300 | 2380 KiB | 36036 KiB |
| `SQISign2Dsquare-Level1-eff_compressed` | verify | 244300 | 5428 KiB | 30852 KiB |
| `SQISign2Dsquare-Level1-eff_uncompressed` | keygen | 242452 | 1600 KiB | 20756 KiB |
| `SQISign2Dsquare-Level1-eff_uncompressed` | sign | 242452 | 2364 KiB | 26072 KiB |
| `SQISign2Dsquare-Level1-eff_uncompressed` | verify | 242452 | 5104 KiB | 33888 KiB |
| `SQISign2Dsquare-Level1-sec_compressed` | keygen | 245652 | 1600 KiB | 12980 KiB |
| `SQISign2Dsquare-Level1-sec_compressed` | sign | 245652 | 4476 KiB | 43128 KiB |
| `SQISign2Dsquare-Level1-sec_compressed` | verify | 245652 | 5780 KiB | 29784 KiB |
| `SQISign2Dsquare-Level1-sec_uncompressed` | keygen | 243436 | 1600 KiB | 19612 KiB |
| `SQISign2Dsquare-Level1-sec_uncompressed` | sign | 243436 | 2416 KiB | 29472 KiB |
| `SQISign2Dsquare-Level1-sec_uncompressed` | verify | 243436 | 5348 KiB | 31724 KiB |
| `SQISign2Dsquare-Level2-eff_compressed` | keygen | 249780 | 1644 KiB | 20972 KiB |
| `SQISign2Dsquare-Level2-eff_compressed` | sign | 249780 | 4264 KiB | 49920 KiB |
| `SQISign2Dsquare-Level2-eff_compressed` | verify | 249780 | 6076 KiB | 44280 KiB |
| `SQISign2Dsquare-Level2-eff_uncompressed` | keygen | 244092 | 1604 KiB | 31588 KiB |
| `SQISign2Dsquare-Level2-eff_uncompressed` | sign | 244092 | 4252 KiB | 34780 KiB |
| `SQISign2Dsquare-Level2-eff_uncompressed` | verify | 244092 | 5616 KiB | 48436 KiB |
| `SQISign2Dsquare-Level2-sec_compressed` | keygen | 257000 | 1652 KiB | 14648 KiB |
| `SQISign2Dsquare-Level2-sec_compressed` | sign | 257000 | 2548 KiB | 60620 KiB |
| `SQISign2Dsquare-Level2-sec_compressed` | verify | 257000 | 7220 KiB | 28264 KiB |
| `SQISign2Dsquare-Level2-sec_uncompressed` | keygen | 254912 | 1652 KiB | 19744 KiB |
| `SQISign2Dsquare-Level2-sec_uncompressed` | sign | 254912 | 4324 KiB | 42828 KiB |
| `SQISign2Dsquare-Level2-sec_uncompressed` | verify | 254912 | 5840 KiB | 32900 KiB |
| `SQISign2Dsquare-Level3-eff_compressed` | keygen | 285600 | 1664 KiB | 23720 KiB |
| `SQISign2Dsquare-Level3-eff_compressed` | sign | 285600 | 4912 KiB | 98308 KiB |
| `SQISign2Dsquare-Level3-eff_compressed` | verify | 285600 | 9532 KiB | 35648 KiB |
| `SQISign2Dsquare-Level3-eff_uncompressed` | keygen | 287688 | 1600 KiB | 24852 KiB |
| `SQISign2Dsquare-Level3-eff_uncompressed` | sign | 287688 | 2860 KiB | 69340 KiB |
| `SQISign2Dsquare-Level3-eff_uncompressed` | verify | 287688 | 7544 KiB | 36908 KiB |
| `SQISign2Dsquare-Level3-sec_compressed` | keygen | 287400 | 1668 KiB | 26776 KiB |
| `SQISign2Dsquare-Level3-sec_compressed` | sign | 287400 | 5028 KiB | 103084 KiB |
| `SQISign2Dsquare-Level3-sec_compressed` | verify | 287400 | 8564 KiB | 39992 KiB |
| `SQISign2Dsquare-Level3-sec_uncompressed` | keygen | 285200 | 1668 KiB | 26232 KiB |
| `SQISign2Dsquare-Level3-sec_uncompressed` | sign | 285200 | 5000 KiB | 74896 KiB |
| `SQISign2Dsquare-Level3-sec_uncompressed` | verify | 285200 | 9144 KiB | 37340 KiB |
| `SQISign2Dsquare-Level5-eff_compressed` | keygen | 379276 | 1736 KiB | 66124 KiB |
| `SQISign2Dsquare-Level5-eff_compressed` | sign | 379276 | 4884 KiB | 318056 KiB |
| `SQISign2Dsquare-Level5-eff_compressed` | verify | 379276 | 21136 KiB | 121460 KiB |
| `SQISign2Dsquare-Level5-eff_uncompressed` | keygen | 377252 | 1736 KiB | 67652 KiB |
| `SQISign2Dsquare-Level5-eff_uncompressed` | sign | 377252 | 4832 KiB | 221644 KiB |
| `SQISign2Dsquare-Level5-eff_uncompressed` | verify | 377252 | 16432 KiB | 116748 KiB |
| `SQISign2Dsquare-Level5-sec_compressed` | keygen | 395048 | 1736 KiB | 79260 KiB |
| `SQISign2Dsquare-Level5-sec_compressed` | sign | 395048 | 7180 KiB | 357824 KiB |
| `SQISign2Dsquare-Level5-sec_compressed` | verify | 395048 | 24692 KiB | 135668 KiB |
| `SQISign2Dsquare-Level5-sec_uncompressed` | keygen | 392424 | 1736 KiB | 71344 KiB |
| `SQISign2Dsquare-Level5-sec_uncompressed` | sign | 392424 | 7124 KiB | 254732 KiB |
| `SQISign2Dsquare-Level5-sec_uncompressed` | verify | 392424 | 17584 KiB | 128528 KiB |

## 6. Transmission and storage overhead

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
| `SQISign2Dsquare-Level1-eff_compressed` | keygen | 0.0% | 2.3% | drng 256 |
| `SQISign2Dsquare-Level1-eff_compressed` | sign | 0.4% | 1.3% | drng 479, pseudohash 50.9 |
| `SQISign2Dsquare-Level1-eff_compressed` | verify | 1.2% | 0.0% | pseudohash 32 |
| `SQISign2Dsquare-Level1-eff_uncompressed` | keygen | 0.0% | 4.3% | drng 256 |
| `SQISign2Dsquare-Level1-eff_uncompressed` | sign | 0.4% | 1.5% | drng 496, pseudohash 42.2 |
| `SQISign2Dsquare-Level1-eff_uncompressed` | verify | 1.3% | 0.0% | pseudohash 32 |
| `SQISign2Dsquare-Level1-sec_compressed` | keygen | 0.0% | 1.7% | drng 260 |
| `SQISign2Dsquare-Level1-sec_compressed` | sign | 0.0% | 1.1% | drng 580, pseudohash 1.81 |
| `SQISign2Dsquare-Level1-sec_compressed` | verify | 0.0% | 0.0% | pseudohash 1 |
| `SQISign2Dsquare-Level1-sec_uncompressed` | keygen | 0.0% | 3.4% | drng 285 |
| `SQISign2Dsquare-Level1-sec_uncompressed` | sign | 0.0% | 0.8% | drng 389, pseudohash 2.28 |
| `SQISign2Dsquare-Level1-sec_uncompressed` | verify | 0.0% | 0.0% | pseudohash 1 |
| `SQISign2Dsquare-Level2-eff_compressed` | keygen | 0.0% | 2.9% | drng 290 |
| `SQISign2Dsquare-Level2-eff_compressed` | sign | 3.2% | 1.2% | drng 392, pseudohash 371 |
| `SQISign2Dsquare-Level2-eff_compressed` | verify | 18% | 0.0% | pseudohash 512 |
| `SQISign2Dsquare-Level2-eff_uncompressed` | keygen | 0.0% | 5.1% | drng 274 |
| `SQISign2Dsquare-Level2-eff_uncompressed` | sign | 3.1% | 1.3% | drng 417, pseudohash 333 |
| `SQISign2Dsquare-Level2-eff_uncompressed` | verify | 20% | 0.0% | pseudohash 512 |
| `SQISign2Dsquare-Level2-sec_compressed` | keygen | 0.0% | 1.5% | drng 365 |
| `SQISign2Dsquare-Level2-sec_compressed` | sign | 0.0% | 0.5% | drng 408, pseudohash 2.45 |
| `SQISign2Dsquare-Level2-sec_compressed` | verify | 0.0% | 0.0% | pseudohash 1 |
| `SQISign2Dsquare-Level2-sec_uncompressed` | keygen | 0.0% | 2.8% | drng 348 |
| `SQISign2Dsquare-Level2-sec_uncompressed` | sign | 0.0% | 0.5% | drng 373, pseudohash 2.17 |
| `SQISign2Dsquare-Level2-sec_uncompressed` | verify | 0.0% | 0.0% | pseudohash 1 |
| `SQISign2Dsquare-Level3-eff_compressed` | keygen | 0.0% | 1.0% | drng 512 |
| `SQISign2Dsquare-Level3-eff_compressed` | sign | 1.1% | 0.4% | drng 742, pseudohash 717 |
| `SQISign2Dsquare-Level3-eff_compressed` | verify | 7.1% | 0.0% | pseudohash 1.02e+03 |
| `SQISign2Dsquare-Level3-eff_uncompressed` | keygen | 0.0% | 2.0% | drng 543 |
| `SQISign2Dsquare-Level3-eff_uncompressed` | sign | 1.2% | 0.5% | drng 749, pseudohash 717 |
| `SQISign2Dsquare-Level3-eff_uncompressed` | verify | 7.9% | 0.0% | pseudohash 1.02e+03 |
| `SQISign2Dsquare-Level3-sec_compressed` | keygen | 0.0% | 0.8% | drng 567 |
| `SQISign2Dsquare-Level3-sec_compressed` | sign | 0.0% | 0.1% | drng 260, pseudohash 1.5 |
| `SQISign2Dsquare-Level3-sec_compressed` | verify | 0.0% | 0.0% | pseudohash 1 |
| `SQISign2Dsquare-Level3-sec_uncompressed` | keygen | 0.0% | 1.6% | drng 572 |
| `SQISign2Dsquare-Level3-sec_uncompressed` | sign | 0.0% | 0.2% | drng 386, pseudohash 1.6 |
| `SQISign2Dsquare-Level3-sec_uncompressed` | verify | 0.0% | 0.0% | pseudohash 1 |
| `SQISign2Dsquare-Level5-eff_compressed` | keygen | 0.0% | 0.3% | drng 1.32e+03 |
| `SQISign2Dsquare-Level5-eff_compressed` | sign | 0.1% | 0.1% | drng 1.25e+03, pseudohash 171 |
| `SQISign2Dsquare-Level5-eff_compressed` | verify | 0.8% | 0.0% | pseudohash 256 |
| `SQISign2Dsquare-Level5-eff_uncompressed` | keygen | 0.0% | 0.5% | drng 1.09e+03 |
| `SQISign2Dsquare-Level5-eff_uncompressed` | sign | 0.1% | 0.2% | drng 2.01e+03, pseudohash 171 |
| `SQISign2Dsquare-Level5-eff_uncompressed` | verify | 0.9% | 0.0% | pseudohash 256 |
| `SQISign2Dsquare-Level5-sec_compressed` | keygen | 0.0% | 0.5% | drng 2.29e+03 |
| `SQISign2Dsquare-Level5-sec_compressed` | sign | 0.0% | 0.1% | drng 1.25e+03, pseudohash 1 |
| `SQISign2Dsquare-Level5-sec_compressed` | verify | 0.0% | 0.0% | pseudohash 1 |
| `SQISign2Dsquare-Level5-sec_uncompressed` | keygen | 0.0% | 1.0% | drng 2.18e+03 |
| `SQISign2Dsquare-Level5-sec_uncompressed` | sign | 0.0% | 0.1% | drng 1.27e+03, pseudohash 1 |
| `SQISign2Dsquare-Level5-sec_uncompressed` | verify | 0.0% | 0.0% | pseudohash 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `SQISign2Dsquare-Level1-eff_compressed` | KAT log (sha256 `d0727a7c6c8acc8c…`) | `kat/sign-25/SQISign2Dsquare-Level1-eff_compressed.log` |
| `SQISign2Dsquare-Level1-eff_compressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level1-eff_compressed__keygen.json` |
| `SQISign2Dsquare-Level1-eff_compressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level1-eff_compressed__sign.json` |
| `SQISign2Dsquare-Level1-eff_compressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level1-eff_compressed__verify.json` |
| `SQISign2Dsquare-Level1-eff_compressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level1-eff_compressed__keygen.json` |
| `SQISign2Dsquare-Level1-eff_compressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level1-eff_compressed__sign.json` |
| `SQISign2Dsquare-Level1-eff_compressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level1-eff_compressed__verify.json` |
| `SQISign2Dsquare-Level1-eff_uncompressed` | KAT log (sha256 `0ed07e3b7ebbd08e…`) | `kat/sign-25/SQISign2Dsquare-Level1-eff_uncompressed.log` |
| `SQISign2Dsquare-Level1-eff_uncompressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level1-eff_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level1-eff_uncompressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level1-eff_uncompressed__sign.json` |
| `SQISign2Dsquare-Level1-eff_uncompressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level1-eff_uncompressed__verify.json` |
| `SQISign2Dsquare-Level1-eff_uncompressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level1-eff_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level1-eff_uncompressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level1-eff_uncompressed__sign.json` |
| `SQISign2Dsquare-Level1-eff_uncompressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level1-eff_uncompressed__verify.json` |
| `SQISign2Dsquare-Level1-sec_compressed` | KAT log (sha256 `42e996000006eb7d…`) | `kat/sign-25/SQISign2Dsquare-Level1-sec_compressed.log` |
| `SQISign2Dsquare-Level1-sec_compressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level1-sec_compressed__keygen.json` |
| `SQISign2Dsquare-Level1-sec_compressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level1-sec_compressed__sign.json` |
| `SQISign2Dsquare-Level1-sec_compressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level1-sec_compressed__verify.json` |
| `SQISign2Dsquare-Level1-sec_compressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level1-sec_compressed__keygen.json` |
| `SQISign2Dsquare-Level1-sec_compressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level1-sec_compressed__sign.json` |
| `SQISign2Dsquare-Level1-sec_compressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level1-sec_compressed__verify.json` |
| `SQISign2Dsquare-Level1-sec_uncompressed` | KAT log (sha256 `f3eeb0dfa059b3ef…`) | `kat/sign-25/SQISign2Dsquare-Level1-sec_uncompressed.log` |
| `SQISign2Dsquare-Level1-sec_uncompressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level1-sec_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level1-sec_uncompressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level1-sec_uncompressed__sign.json` |
| `SQISign2Dsquare-Level1-sec_uncompressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level1-sec_uncompressed__verify.json` |
| `SQISign2Dsquare-Level1-sec_uncompressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level1-sec_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level1-sec_uncompressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level1-sec_uncompressed__sign.json` |
| `SQISign2Dsquare-Level1-sec_uncompressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level1-sec_uncompressed__verify.json` |
| `SQISign2Dsquare-Level2-eff_compressed` | KAT log (sha256 `35a55a2943bd8ffe…`) | `kat/sign-25/SQISign2Dsquare-Level2-eff_compressed.log` |
| `SQISign2Dsquare-Level2-eff_compressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level2-eff_compressed__keygen.json` |
| `SQISign2Dsquare-Level2-eff_compressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level2-eff_compressed__sign.json` |
| `SQISign2Dsquare-Level2-eff_compressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level2-eff_compressed__verify.json` |
| `SQISign2Dsquare-Level2-eff_compressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level2-eff_compressed__keygen.json` |
| `SQISign2Dsquare-Level2-eff_compressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level2-eff_compressed__sign.json` |
| `SQISign2Dsquare-Level2-eff_compressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level2-eff_compressed__verify.json` |
| `SQISign2Dsquare-Level2-eff_uncompressed` | KAT log (sha256 `b9d3e5cebc4cca2b…`) | `kat/sign-25/SQISign2Dsquare-Level2-eff_uncompressed.log` |
| `SQISign2Dsquare-Level2-eff_uncompressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level2-eff_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level2-eff_uncompressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level2-eff_uncompressed__sign.json` |
| `SQISign2Dsquare-Level2-eff_uncompressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level2-eff_uncompressed__verify.json` |
| `SQISign2Dsquare-Level2-eff_uncompressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level2-eff_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level2-eff_uncompressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level2-eff_uncompressed__sign.json` |
| `SQISign2Dsquare-Level2-eff_uncompressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level2-eff_uncompressed__verify.json` |
| `SQISign2Dsquare-Level2-sec_compressed` | KAT log (sha256 `53672d9ecdbb8769…`) | `kat/sign-25/SQISign2Dsquare-Level2-sec_compressed.log` |
| `SQISign2Dsquare-Level2-sec_compressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level2-sec_compressed__keygen.json` |
| `SQISign2Dsquare-Level2-sec_compressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level2-sec_compressed__sign.json` |
| `SQISign2Dsquare-Level2-sec_compressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level2-sec_compressed__verify.json` |
| `SQISign2Dsquare-Level2-sec_compressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level2-sec_compressed__keygen.json` |
| `SQISign2Dsquare-Level2-sec_compressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level2-sec_compressed__sign.json` |
| `SQISign2Dsquare-Level2-sec_compressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level2-sec_compressed__verify.json` |
| `SQISign2Dsquare-Level2-sec_uncompressed` | KAT log (sha256 `090070761f5e825a…`) | `kat/sign-25/SQISign2Dsquare-Level2-sec_uncompressed.log` |
| `SQISign2Dsquare-Level2-sec_uncompressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level2-sec_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level2-sec_uncompressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level2-sec_uncompressed__sign.json` |
| `SQISign2Dsquare-Level2-sec_uncompressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level2-sec_uncompressed__verify.json` |
| `SQISign2Dsquare-Level2-sec_uncompressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level2-sec_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level2-sec_uncompressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level2-sec_uncompressed__sign.json` |
| `SQISign2Dsquare-Level2-sec_uncompressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level2-sec_uncompressed__verify.json` |
| `SQISign2Dsquare-Level3-eff_compressed` | KAT log (sha256 `971e729d9baf58f2…`) | `kat/sign-25/SQISign2Dsquare-Level3-eff_compressed.log` |
| `SQISign2Dsquare-Level3-eff_compressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level3-eff_compressed__keygen.json` |
| `SQISign2Dsquare-Level3-eff_compressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level3-eff_compressed__sign.json` |
| `SQISign2Dsquare-Level3-eff_compressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level3-eff_compressed__verify.json` |
| `SQISign2Dsquare-Level3-eff_compressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level3-eff_compressed__keygen.json` |
| `SQISign2Dsquare-Level3-eff_compressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level3-eff_compressed__sign.json` |
| `SQISign2Dsquare-Level3-eff_compressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level3-eff_compressed__verify.json` |
| `SQISign2Dsquare-Level3-eff_uncompressed` | KAT log (sha256 `68144b96157acabd…`) | `kat/sign-25/SQISign2Dsquare-Level3-eff_uncompressed.log` |
| `SQISign2Dsquare-Level3-eff_uncompressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level3-eff_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level3-eff_uncompressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level3-eff_uncompressed__sign.json` |
| `SQISign2Dsquare-Level3-eff_uncompressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level3-eff_uncompressed__verify.json` |
| `SQISign2Dsquare-Level3-eff_uncompressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level3-eff_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level3-eff_uncompressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level3-eff_uncompressed__sign.json` |
| `SQISign2Dsquare-Level3-eff_uncompressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level3-eff_uncompressed__verify.json` |
| `SQISign2Dsquare-Level3-sec_compressed` | KAT log (sha256 `17edb2c11994c4d9…`) | `kat/sign-25/SQISign2Dsquare-Level3-sec_compressed.log` |
| `SQISign2Dsquare-Level3-sec_compressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level3-sec_compressed__keygen.json` |
| `SQISign2Dsquare-Level3-sec_compressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level3-sec_compressed__sign.json` |
| `SQISign2Dsquare-Level3-sec_compressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level3-sec_compressed__verify.json` |
| `SQISign2Dsquare-Level3-sec_compressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level3-sec_compressed__keygen.json` |
| `SQISign2Dsquare-Level3-sec_compressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level3-sec_compressed__sign.json` |
| `SQISign2Dsquare-Level3-sec_compressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level3-sec_compressed__verify.json` |
| `SQISign2Dsquare-Level3-sec_uncompressed` | KAT log (sha256 `1233c671b3737ab3…`) | `kat/sign-25/SQISign2Dsquare-Level3-sec_uncompressed.log` |
| `SQISign2Dsquare-Level3-sec_uncompressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level3-sec_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level3-sec_uncompressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level3-sec_uncompressed__sign.json` |
| `SQISign2Dsquare-Level3-sec_uncompressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level3-sec_uncompressed__verify.json` |
| `SQISign2Dsquare-Level3-sec_uncompressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level3-sec_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level3-sec_uncompressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level3-sec_uncompressed__sign.json` |
| `SQISign2Dsquare-Level3-sec_uncompressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level3-sec_uncompressed__verify.json` |
| `SQISign2Dsquare-Level5-eff_compressed` | KAT log (sha256 `ae094d284f37edd1…`) | `kat/sign-25/SQISign2Dsquare-Level5-eff_compressed.log` |
| `SQISign2Dsquare-Level5-eff_compressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level5-eff_compressed__keygen.json` |
| `SQISign2Dsquare-Level5-eff_compressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level5-eff_compressed__sign.json` |
| `SQISign2Dsquare-Level5-eff_compressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level5-eff_compressed__verify.json` |
| `SQISign2Dsquare-Level5-eff_compressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level5-eff_compressed__keygen.json` |
| `SQISign2Dsquare-Level5-eff_compressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level5-eff_compressed__sign.json` |
| `SQISign2Dsquare-Level5-eff_compressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level5-eff_compressed__verify.json` |
| `SQISign2Dsquare-Level5-eff_uncompressed` | KAT log (sha256 `faeea9df987c95f7…`) | `kat/sign-25/SQISign2Dsquare-Level5-eff_uncompressed.log` |
| `SQISign2Dsquare-Level5-eff_uncompressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level5-eff_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level5-eff_uncompressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level5-eff_uncompressed__sign.json` |
| `SQISign2Dsquare-Level5-eff_uncompressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level5-eff_uncompressed__verify.json` |
| `SQISign2Dsquare-Level5-eff_uncompressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level5-eff_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level5-eff_uncompressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level5-eff_uncompressed__sign.json` |
| `SQISign2Dsquare-Level5-eff_uncompressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level5-eff_uncompressed__verify.json` |
| `SQISign2Dsquare-Level5-sec_compressed` | KAT log (sha256 `0166dfec32c1bf31…`) | `kat/sign-25/SQISign2Dsquare-Level5-sec_compressed.log` |
| `SQISign2Dsquare-Level5-sec_compressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level5-sec_compressed__keygen.json` |
| `SQISign2Dsquare-Level5-sec_compressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level5-sec_compressed__sign.json` |
| `SQISign2Dsquare-Level5-sec_compressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level5-sec_compressed__verify.json` |
| `SQISign2Dsquare-Level5-sec_compressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level5-sec_compressed__keygen.json` |
| `SQISign2Dsquare-Level5-sec_compressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level5-sec_compressed__sign.json` |
| `SQISign2Dsquare-Level5-sec_compressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level5-sec_compressed__verify.json` |
| `SQISign2Dsquare-Level5-sec_uncompressed` | KAT log (sha256 `738bd2ee8cf1c6df…`) | `kat/sign-25/SQISign2Dsquare-Level5-sec_uncompressed.log` |
| `SQISign2Dsquare-Level5-sec_uncompressed` | timing keygen | `records/sign-25/SQISign2Dsquare-Level5-sec_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level5-sec_uncompressed` | timing sign | `records/sign-25/SQISign2Dsquare-Level5-sec_uncompressed__sign.json` |
| `SQISign2Dsquare-Level5-sec_uncompressed` | timing verify | `records/sign-25/SQISign2Dsquare-Level5-sec_uncompressed__verify.json` |
| `SQISign2Dsquare-Level5-sec_uncompressed` | hash profile keygen | `profile/sign-25/SQISign2Dsquare-Level5-sec_uncompressed__keygen.json` |
| `SQISign2Dsquare-Level5-sec_uncompressed` | hash profile sign | `profile/sign-25/SQISign2Dsquare-Level5-sec_uncompressed__sign.json` |
| `SQISign2Dsquare-Level5-sec_uncompressed` | hash profile verify | `profile/sign-25/SQISign2Dsquare-Level5-sec_uncompressed__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

