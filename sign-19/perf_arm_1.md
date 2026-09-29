# sign-19 Phoenix — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-19` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561087113121792.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: Phoenix
- Implementation versions measured: reference
- Parameter sets: `Phoenix-SHAKE-128f`, `Phoenix-SHAKE-128s`, `Phoenix-SHAKE-192f`, `Phoenix-SHAKE-192s`, `Phoenix-SHAKE-256f`, `Phoenix-SHAKE-256s`, `Phoenix-SHAKE-384f`, `Phoenix-SHAKE-384s`, `Phoenix-SHAKE-512f`, `Phoenix-SHAKE-512s`, `Phoenix-SM3-128f`, `Phoenix-SM3-128s`, `Phoenix-SM3-192f`, `Phoenix-SM3-192s`, `Phoenix-SM3-256f`, `Phoenix-SM3-256s`, `Phoenix-SM3-384f`, `Phoenix-SM3-384s`, `Phoenix-SM3-512f`, `Phoenix-SM3-512s`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-19/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Phoenix-SHAKE-128f` | guide | PASS |
| `Phoenix-SHAKE-128s` | guide | PASS |
| `Phoenix-SHAKE-192f` | guide | PASS |
| `Phoenix-SHAKE-192s` | guide | PASS |
| `Phoenix-SHAKE-256f` | guide | PASS |
| `Phoenix-SHAKE-256s` | guide | PASS |
| `Phoenix-SHAKE-384f` | guide | PASS |
| `Phoenix-SHAKE-384s` | guide | PASS |
| `Phoenix-SHAKE-512f` | guide | PASS |
| `Phoenix-SHAKE-512s` | guide | PASS |
| `Phoenix-SM3-128f` | harness-default | MISMATCH [1] |
| `Phoenix-SM3-128s` | harness-default | MISMATCH [1] |
| `Phoenix-SM3-192f` | harness-default | MISMATCH [1] |
| `Phoenix-SM3-192s` | harness-default | MISMATCH [1] |
| `Phoenix-SM3-256f` | harness-default | MISMATCH [1] |
| `Phoenix-SM3-256s` | harness-default | MISMATCH [1] |
| `Phoenix-SM3-384f` | harness-default | MISMATCH [1] |
| `Phoenix-SM3-384s` | harness-default | MISMATCH [1] |
| `Phoenix-SM3-512f` | harness-default | MISMATCH [1] |
| `Phoenix-SM3-512s` | harness-default | MISMATCH [1] |

[1] The submitted SM3 KAT signatures are rejected by the submitted code's own sig_verify at -O0/-O2/-O3, so the vectors were produced by different code; the SM3 path also copies 48/64 bytes out of a 32-byte hash output in the 384/512 parameter sets (out-of-bounds read). The SHAKE parameter sets pass. These instances are timed anyway; their output is not validated.

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Phoenix-SHAKE-128f` | keygen | 9.66 M | 3.58 ms | 279 | 3.58 ms | 885 (5 × 177) |
| `Phoenix-SHAKE-128f` | sign | 184.27 M | 68.4 ms | 14.6 | 68.1 ms | 100 (5 × 20) |
| `Phoenix-SHAKE-128f` | verify | 5.11 M | 1.9 ms | 527 | 1.9 ms | 1670 (5 × 334) |
| `Phoenix-SHAKE-128s` | keygen | 311.31 M | 115 ms | 8.66 | 115 ms | 100 (5 × 20) |
| `Phoenix-SHAKE-128s` | sign | 3.24 G | 1.2 s | 0.831 | 1.2 s | 100 (5 × 20) |
| `Phoenix-SHAKE-128s` | verify | 12.27 M | 4.55 ms | 220 | 4.56 ms | 695 (5 × 139) |
| `Phoenix-SHAKE-192f` | keygen | 14.64 M | 5.43 ms | 184 | 5.43 ms | 580 (5 × 116) |
| `Phoenix-SHAKE-192f` | sign | 289.94 M | 108 ms | 9.3 | 108 ms | 100 (5 × 20) |
| `Phoenix-SHAKE-192f` | verify | 7.80 M | 2.89 ms | 346 | 2.89 ms | 1090 (5 × 218) |
| `Phoenix-SHAKE-192s` | keygen | 545.11 M | 202 ms | 4.95 | 202 ms | 100 (5 × 20) |
| `Phoenix-SHAKE-192s` | sign | 5.59 G | 2.08 s | 0.482 | 2.08 s | 100 (5 × 20) |
| `Phoenix-SHAKE-192s` | verify | 19.52 M | 7.24 ms | 138 | 7.25 ms | 435 (5 × 87) |
| `Phoenix-SHAKE-256f` | keygen | 30.28 M | 11.2 ms | 89 | 11.2 ms | 285 (5 × 57) |
| `Phoenix-SHAKE-256f` | sign | 584.56 M | 217 ms | 4.61 | 217 ms | 100 (5 × 20) |
| `Phoenix-SHAKE-256f` | verify | 15.57 M | 5.78 ms | 173 | 5.78 ms | 550 (5 × 110) |
| `Phoenix-SHAKE-256s` | keygen | 402.70 M | 149 ms | 6.69 | 148 ms | 100 (5 × 20) |
| `Phoenix-SHAKE-256s` | sign | 5.23 G | 1.94 s | 0.515 | 1.94 s | 100 (5 × 20) |
| `Phoenix-SHAKE-256s` | verify | 34.93 M | 13 ms | 77.2 | 12.9 ms | 250 (5 × 50) |
| `Phoenix-SHAKE-384f` | keygen | 70.15 M | 26 ms | 38.4 | 26 ms | 125 (5 × 25) |
| `Phoenix-SHAKE-384f` | sign | 2.09 G | 775 ms | 1.29 | 775 ms | 100 (5 × 20) |
| `Phoenix-SHAKE-384f` | verify | 38.37 M | 14.2 ms | 70.3 | 14.2 ms | 225 (5 × 45) |
| `Phoenix-SHAKE-384s` | keygen | 689.50 M | 256 ms | 3.91 | 256 ms | 100 (5 × 20) |
| `Phoenix-SHAKE-384s` | sign | 9.47 G | 3.51 s | 0.285 | 3.51 s | 100 (5 × 20) |
| `Phoenix-SHAKE-384s` | verify | 60.50 M | 22.4 ms | 44.6 | 22.5 ms | 145 (5 × 29) |
| `Phoenix-SHAKE-512f` | keygen | 218.32 M | 81 ms | 12.3 | 81 ms | 100 (5 × 20) |
| `Phoenix-SHAKE-512f` | sign | 5.07 G | 1.88 s | 0.532 | 1.87 s | 100 (5 × 20) |
| `Phoenix-SHAKE-512f` | verify | 117.11 M | 43.4 ms | 23 | 43.4 ms | 100 (5 × 20) |
| `Phoenix-SHAKE-512s` | keygen | 2.31 G | 857 ms | 1.17 | 843 ms | 100 (5 × 20) |
| `Phoenix-SHAKE-512s` | sign | 21.13 G | 7.84 s | 0.128 | 7.79 s | 45 (5 × 9) |
| `Phoenix-SHAKE-512s` | verify | 37.44 M | 13.9 ms | 72 | 13.8 ms | 230 (5 × 46) |
| `Phoenix-SM3-128f` | keygen | 9.87 M | 3.66 ms | 273 | 3.66 ms | 860 (5 × 172) |
| `Phoenix-SM3-128f` | sign | 200.57 M | 74.4 ms | 13.4 | 74.4 ms | 100 (5 × 20) |
| `Phoenix-SM3-128f` | verify | 5.54 M | 2.05 ms | 487 | 2.05 ms | 1535 (5 × 307) |
| `Phoenix-SM3-128s` | keygen | 316.92 M | 118 ms | 8.5 | 117 ms | 100 (5 × 20) |
| `Phoenix-SM3-128s` | sign | 3.58 G | 1.33 s | 0.752 | 1.32 s | 100 (5 × 20) |
| `Phoenix-SM3-128s` | verify | 12.97 M | 4.81 ms | 208 | 4.72 ms | 670 (5 × 134) |
| `Phoenix-SM3-192f` | keygen | 26.66 M | 9.89 ms | 101 | 9.89 ms | 320 (5 × 64) |
| `Phoenix-SM3-192f` | sign | 525.98 M | 195 ms | 5.13 | 195 ms | 100 (5 × 20) |
| `Phoenix-SM3-192f` | verify | 14.07 M | 5.22 ms | 192 | 5.22 ms | 605 (5 × 121) |
| `Phoenix-SM3-192s` | keygen | 999.40 M | 371 ms | 2.7 | 369 ms | 100 (5 × 20) |
| `Phoenix-SM3-192s` | sign | 10.42 G | 3.86 s | 0.259 | 3.84 s | 90 (5 × 18) |
| `Phoenix-SM3-192s` | verify | 35.63 M | 13.2 ms | 75.7 | 13.2 ms | 240 (5 × 48) |
| `Phoenix-SM3-256f` | keygen | 53.48 M | 19.8 ms | 50.4 | 19.8 ms | 160 (5 × 32) |
| `Phoenix-SM3-256f` | sign | 1.03 G | 383 ms | 2.61 | 379 ms | 100 (5 × 20) |
| `Phoenix-SM3-256f` | verify | 27.29 M | 10.1 ms | 98.8 | 10.1 ms | 315 (5 × 63) |
| `Phoenix-SM3-256s` | keygen | 707.90 M | 263 ms | 3.81 | 262 ms | 100 (5 × 20) |
| `Phoenix-SM3-256s` | sign | 9.55 G | 3.54 s | 0.282 | 3.54 s | 100 (5 × 20) |
| `Phoenix-SM3-256s` | verify | 61.25 M | 22.7 ms | 44 | 22.7 ms | 140 (5 × 28) |
| `Phoenix-SM3-384f` | keygen | 113.88 M | 42.2 ms | 23.7 | 41.9 ms | 100 (5 × 20) |
| `Phoenix-SM3-384f` | sign | 3.29 G | 1.22 s | 0.819 | 1.2 s | 100 (5 × 20) |
| `Phoenix-SM3-384f` | verify | 61.33 M | 22.7 ms | 44 | 22.7 ms | 140 (5 × 28) |
| `Phoenix-SM3-384s` | keygen | 1.11 G | 413 ms | 2.42 | 412 ms | 100 (5 × 20) |
| `Phoenix-SM3-384s` | sign | 15.18 G | 5.63 s | 0.178 | 5.63 s | 65 (5 × 13) |
| `Phoenix-SM3-384s` | verify | 98.14 M | 36.4 ms | 27.5 | 36 ms | 100 (5 × 20) |
| `Phoenix-SM3-512f` | keygen | 225.35 M | 83.6 ms | 12 | 83.6 ms | 100 (5 × 20) |
| `Phoenix-SM3-512f` | sign | 5.54 G | 2.05 s | 0.487 | 2.05 s | 100 (5 × 20) |
| `Phoenix-SM3-512f` | verify | 122.27 M | 45.4 ms | 22 | 45 ms | 100 (5 × 20) |
| `Phoenix-SM3-512s` | keygen | 2.35 G | 870 ms | 1.15 | 870 ms | 100 (5 × 20) |
| `Phoenix-SM3-512s` | sign | 24.49 G | 9.09 s | 0.11 | 9.08 s | 40 (5 × 8) |
| `Phoenix-SM3-512s` | verify | 39.41 M | 14.6 ms | 68.4 | 14.5 ms | 220 (5 × 44) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Phoenix-SHAKE-128f` | keygen | 33000 | 1440 KiB | 1504 KiB |
| `Phoenix-SHAKE-128f` | sign | 33000 | 3472 KiB | 3540 KiB |
| `Phoenix-SHAKE-128f` | verify | 33000 | 1440 KiB | 1504 KiB |
| `Phoenix-SHAKE-128s` | keygen | 37128 | 1436 KiB | 1500 KiB |
| `Phoenix-SHAKE-128s` | sign | 37128 | 3464 KiB | 3528 KiB |
| `Phoenix-SHAKE-128s` | verify | 37128 | 1436 KiB | 1500 KiB |
| `Phoenix-SHAKE-192f` | keygen | 33584 | 1456 KiB | 1524 KiB |
| `Phoenix-SHAKE-192f` | sign | 33584 | 1456 KiB | 1520 KiB |
| `Phoenix-SHAKE-192f` | verify | 33584 | 1456 KiB | 1524 KiB |
| `Phoenix-SHAKE-192s` | keygen | 33512 | 1440 KiB | 1504 KiB |
| `Phoenix-SHAKE-192s` | sign | 33512 | 1440 KiB | 1508 KiB |
| `Phoenix-SHAKE-192s` | verify | 33512 | 1440 KiB | 1508 KiB |
| `Phoenix-SHAKE-256f` | keygen | 33616 | 1468 KiB | 1536 KiB |
| `Phoenix-SHAKE-256f` | sign | 33616 | 1472 KiB | 1536 KiB |
| `Phoenix-SHAKE-256f` | verify | 33616 | 1472 KiB | 1544 KiB |
| `Phoenix-SHAKE-256s` | keygen | 33472 | 1444 KiB | 1512 KiB |
| `Phoenix-SHAKE-256s` | sign | 33472 | 1448 KiB | 1516 KiB |
| `Phoenix-SHAKE-256s` | verify | 33472 | 1448 KiB | 1516 KiB |
| `Phoenix-SHAKE-384f` | keygen | 33872 | 1512 KiB | 1584 KiB |
| `Phoenix-SHAKE-384f` | sign | 33872 | 1516 KiB | 1580 KiB |
| `Phoenix-SHAKE-384f` | verify | 33872 | 1520 KiB | 1588 KiB |
| `Phoenix-SHAKE-384s` | keygen | 33896 | 1480 KiB | 1548 KiB |
| `Phoenix-SHAKE-384s` | sign | 33896 | 1484 KiB | 1548 KiB |
| `Phoenix-SHAKE-384s` | verify | 33896 | 1484 KiB | 1556 KiB |
| `Phoenix-SHAKE-512f` | keygen | 34144 | 3584 KiB | 3660 KiB |
| `Phoenix-SHAKE-512f` | sign | 34144 | 1572 KiB | 1636 KiB |
| `Phoenix-SHAKE-512f` | verify | 34144 | 3544 KiB | 3620 KiB |
| `Phoenix-SHAKE-512s` | keygen | 34024 | 1516 KiB | 1596 KiB |
| `Phoenix-SHAKE-512s` | sign | 34024 | 1532 KiB | 1596 KiB |
| `Phoenix-SHAKE-512s` | verify | 34024 | 1532 KiB | 1608 KiB |
| `Phoenix-SM3-128f` | keygen | 33128 | 1436 KiB | 1504 KiB |
| `Phoenix-SM3-128f` | sign | 33128 | 1440 KiB | 1504 KiB |
| `Phoenix-SM3-128f` | verify | 33128 | 1440 KiB | 1508 KiB |
| `Phoenix-SM3-128s` | keygen | 33056 | 1432 KiB | 1496 KiB |
| `Phoenix-SM3-128s` | sign | 33056 | 1432 KiB | 1496 KiB |
| `Phoenix-SM3-128s` | verify | 33056 | 1432 KiB | 1496 KiB |
| `Phoenix-SM3-192f` | keygen | 33744 | 1456 KiB | 1524 KiB |
| `Phoenix-SM3-192f` | sign | 33744 | 1460 KiB | 1524 KiB |
| `Phoenix-SM3-192f` | verify | 33744 | 1460 KiB | 1524 KiB |
| `Phoenix-SM3-192s` | keygen | 33680 | 3456 KiB | 3520 KiB |
| `Phoenix-SM3-192s` | sign | 33680 | 1440 KiB | 1504 KiB |
| `Phoenix-SM3-192s` | verify | 33680 | 1440 KiB | 1508 KiB |
| `Phoenix-SM3-256f` | keygen | 33704 | 3468 KiB | 3536 KiB |
| `Phoenix-SM3-256f` | sign | 33704 | 1472 KiB | 1540 KiB |
| `Phoenix-SM3-256f` | verify | 33704 | 3448 KiB | 3520 KiB |
| `Phoenix-SM3-256s` | keygen | 33536 | 1448 KiB | 1512 KiB |
| `Phoenix-SM3-256s` | sign | 33536 | 3476 KiB | 3540 KiB |
| `Phoenix-SM3-256s` | verify | 33536 | 1452 KiB | 1520 KiB |
| `Phoenix-SM3-384f` | keygen | 34116 | 1512 KiB | 1588 KiB |
| `Phoenix-SM3-384f` | sign | 34116 | 1524 KiB | 1588 KiB |
| `Phoenix-SM3-384f` | verify | 34116 | 1524 KiB | 1592 KiB |
| `Phoenix-SM3-384s` | keygen | 34116 | 1480 KiB | 1552 KiB |
| `Phoenix-SM3-384s` | sign | 34116 | 1488 KiB | 1552 KiB |
| `Phoenix-SM3-384s` | verify | 34116 | 3456 KiB | 3524 KiB |
| `Phoenix-SM3-512f` | keygen | 34132 | 1560 KiB | 1640 KiB |
| `Phoenix-SM3-512f` | sign | 34132 | 1572 KiB | 1640 KiB |
| `Phoenix-SM3-512f` | verify | 34132 | 1580 KiB | 1652 KiB |
| `Phoenix-SM3-512s` | keygen | 34100 | 3516 KiB | 3600 KiB |
| `Phoenix-SM3-512s` | sign | 34100 | 1536 KiB | 1604 KiB |
| `Phoenix-SM3-512s` | verify | 34100 | 1544 KiB | 1612 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | signature |
|---|---|---|---|
| `Phoenix-SHAKE-128f` | 32 | 64 | 13670 |
| `Phoenix-SHAKE-128s` | 32 | 64 | 6258 |
| `Phoenix-SHAKE-192f` | 48 | 96 | 30766 |
| `Phoenix-SHAKE-192s` | 48 | 96 | 13332 |
| `Phoenix-SHAKE-256f` | 64 | 128 | 44906 |
| `Phoenix-SHAKE-256s` | 64 | 128 | 24618 |
| `Phoenix-SHAKE-384f` | 96 | 192 | 88442 |
| `Phoenix-SHAKE-384s` | 96 | 192 | 54726 |
| `Phoenix-SHAKE-512f` | 128 | 256 | 138454 |
| `Phoenix-SHAKE-512s` | 128 | 256 | 98476 |
| `Phoenix-SM3-128f` | 32 | 64 | 13670 |
| `Phoenix-SM3-128s` | 32 | 64 | 6258 |
| `Phoenix-SM3-192f` | 48 | 96 | 30766 |
| `Phoenix-SM3-192s` | 48 | 96 | 13332 |
| `Phoenix-SM3-256f` | 64 | 128 | 44906 |
| `Phoenix-SM3-256s` | 64 | 128 | 24618 |
| `Phoenix-SM3-384f` | 96 | 192 | 88442 |
| `Phoenix-SM3-384s` | 96 | 192 | 54726 |
| `Phoenix-SM3-512f` | 128 | 256 | 138454 |
| `Phoenix-SM3-512s` | 128 | 256 | 98476 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **instance-dependent** — SM3 sets mixed (own SM3 for all tree hashing/PRF, pseudoXOF for digest/indices); SHAKE sets bypass with a SHAKE-based drng.c

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Phoenix-SHAKE-128f` | keygen | 0.0% | 0.1% | drng 1 |
| `Phoenix-SHAKE-128f` | sign | 0.0% | 0.0% | drng 1 |
| `Phoenix-SHAKE-128f` | verify | 0.0% | 0.0% | – |
| `Phoenix-SHAKE-128s` | keygen | 0.0% | 0.0% | drng 1 |
| `Phoenix-SHAKE-128s` | sign | 0.0% | 0.0% | drng 1 |
| `Phoenix-SHAKE-128s` | verify | 0.0% | 0.0% | – |
| `Phoenix-SHAKE-192f` | keygen | 0.0% | 0.1% | drng 1 |
| `Phoenix-SHAKE-192f` | sign | 0.0% | 0.0% | drng 1 |
| `Phoenix-SHAKE-192f` | verify | 0.0% | 0.0% | – |
| `Phoenix-SHAKE-192s` | keygen | 0.0% | 0.0% | drng 1 |
| `Phoenix-SHAKE-192s` | sign | 0.0% | 0.0% | drng 1 |
| `Phoenix-SHAKE-192s` | verify | 0.0% | 0.0% | – |
| `Phoenix-SHAKE-256f` | keygen | 0.0% | 0.0% | drng 1 |
| `Phoenix-SHAKE-256f` | sign | 0.0% | 0.0% | drng 1 |
| `Phoenix-SHAKE-256f` | verify | 0.0% | 0.0% | – |
| `Phoenix-SHAKE-256s` | keygen | 0.0% | 0.0% | drng 1 |
| `Phoenix-SHAKE-256s` | sign | 0.0% | 0.0% | drng 1 |
| `Phoenix-SHAKE-256s` | verify | 0.0% | 0.0% | – |
| `Phoenix-SHAKE-384f` | keygen | 0.0% | 0.0% | drng 1 |
| `Phoenix-SHAKE-384f` | sign | 0.0% | 0.0% | drng 1 |
| `Phoenix-SHAKE-384f` | verify | 0.0% | 0.0% | – |
| `Phoenix-SHAKE-384s` | keygen | 0.0% | 0.0% | drng 1 |
| `Phoenix-SHAKE-384s` | sign | 0.0% | 0.0% | drng 1 |
| `Phoenix-SHAKE-384s` | verify | 0.0% | 0.0% | – |
| `Phoenix-SHAKE-512f` | keygen | 0.0% | 0.0% | drng 1 |
| `Phoenix-SHAKE-512f` | sign | 0.0% | 0.0% | drng 1 |
| `Phoenix-SHAKE-512f` | verify | 0.0% | 0.0% | – |
| `Phoenix-SHAKE-512s` | keygen | 0.0% | 0.0% | drng 1 |
| `Phoenix-SHAKE-512s` | sign | 0.0% | 0.0% | drng 1 |
| `Phoenix-SHAKE-512s` | verify | 0.0% | 0.0% | – |
| `Phoenix-SM3-128f` | keygen | 0.0% | 0.1% | drng 1 |
| `Phoenix-SM3-128f` | sign | 4.2% | 0.0% | drng 1, pseudoXOF 632 |
| `Phoenix-SM3-128f` | verify | 1.3% | 0.0% | pseudoXOF 20 |
| `Phoenix-SM3-128s` | keygen | 0.0% | 0.0% | drng 1 |
| `Phoenix-SM3-128s` | sign | 5.3% | 0.0% | drng 1, pseudoXOF 1.3e+04 |
| `Phoenix-SM3-128s` | verify | 0.5% | 0.0% | pseudoXOF 13 |
| `Phoenix-SM3-192f` | keygen | 0.0% | 0.0% | drng 1 |
| `Phoenix-SM3-192f` | sign | 0.3% | 0.0% | drng 1, pseudoXOF 146 |
| `Phoenix-SM3-192f` | verify | 0.7% | 0.0% | pseudoXOF 20 |
| `Phoenix-SM3-192s` | keygen | 0.0% | 0.0% | drng 1 |
| `Phoenix-SM3-192s` | sign | 0.6% | 0.0% | drng 1, pseudoXOF 4.62e+03 |
| `Phoenix-SM3-192s` | verify | 0.2% | 0.0% | pseudoXOF 12 |
| `Phoenix-SM3-256f` | keygen | 0.0% | 0.0% | drng 1 |
| `Phoenix-SM3-256f` | sign | 1.6% | 0.0% | drng 1, pseudoXOF 670 |
| `Phoenix-SM3-256f` | verify | 0.5% | 0.0% | pseudoXOF 19 |
| `Phoenix-SM3-256s` | keygen | 0.0% | 0.0% | drng 1 |
| `Phoenix-SM3-256s` | sign | 12% | 0.0% | drng 1, pseudoXOF 4.97e+04 |
| `Phoenix-SM3-256s` | verify | 0.2% | 0.0% | pseudoXOF 14 |
| `Phoenix-SM3-384f` | keygen | 0.0% | 0.0% | drng 1 |
| `Phoenix-SM3-384f` | sign | 0.0% | 0.0% | drng 1, pseudoXOF 19 |
| `Phoenix-SM3-384f` | verify | 0.3% | 0.0% | pseudoXOF 20 |
| `Phoenix-SM3-384s` | keygen | 0.0% | 0.0% | drng 1 |
| `Phoenix-SM3-384s` | sign | 1.5% | 0.0% | drng 1, pseudoXOF 7.83e+03 |
| `Phoenix-SM3-384s` | verify | 0.2% | 0.0% | pseudoXOF 14 |
| `Phoenix-SM3-512f` | keygen | 0.0% | 0.0% | drng 1 |
| `Phoenix-SM3-512f` | sign | 4.9% | 0.0% | drng 1, pseudoXOF 6.51e+03 |
| `Phoenix-SM3-512f` | verify | 0.2% | 0.0% | pseudoXOF 20 |
| `Phoenix-SM3-512s` | keygen | 0.0% | 0.0% | drng 1 |
| `Phoenix-SM3-512s` | sign | 8.3% | 0.0% | drng 1, pseudoXOF 4.78e+04 |
| `Phoenix-SM3-512s` | verify | 0.5% | 0.0% | pseudoXOF 11 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Phoenix-SHAKE-128f` | KAT log (sha256 `6f0a351321cdd0f2…`) | `kat/sign-19/Phoenix-SHAKE-128f.log` |
| `Phoenix-SHAKE-128f` | timing keygen | `records/sign-19/Phoenix-SHAKE-128f__keygen.json` |
| `Phoenix-SHAKE-128f` | timing sign | `records/sign-19/Phoenix-SHAKE-128f__sign.json` |
| `Phoenix-SHAKE-128f` | timing verify | `records/sign-19/Phoenix-SHAKE-128f__verify.json` |
| `Phoenix-SHAKE-128f` | hash profile keygen | `profile/sign-19/Phoenix-SHAKE-128f__keygen.json` |
| `Phoenix-SHAKE-128f` | hash profile sign | `profile/sign-19/Phoenix-SHAKE-128f__sign.json` |
| `Phoenix-SHAKE-128f` | hash profile verify | `profile/sign-19/Phoenix-SHAKE-128f__verify.json` |
| `Phoenix-SHAKE-128s` | KAT log (sha256 `876cc489dedf674f…`) | `kat/sign-19/Phoenix-SHAKE-128s.log` |
| `Phoenix-SHAKE-128s` | timing keygen | `records/sign-19/Phoenix-SHAKE-128s__keygen.json` |
| `Phoenix-SHAKE-128s` | timing sign | `records/sign-19/Phoenix-SHAKE-128s__sign.json` |
| `Phoenix-SHAKE-128s` | timing verify | `records/sign-19/Phoenix-SHAKE-128s__verify.json` |
| `Phoenix-SHAKE-128s` | hash profile keygen | `profile/sign-19/Phoenix-SHAKE-128s__keygen.json` |
| `Phoenix-SHAKE-128s` | hash profile sign | `profile/sign-19/Phoenix-SHAKE-128s__sign.json` |
| `Phoenix-SHAKE-128s` | hash profile verify | `profile/sign-19/Phoenix-SHAKE-128s__verify.json` |
| `Phoenix-SHAKE-192f` | KAT log (sha256 `892831ab5c0196ca…`) | `kat/sign-19/Phoenix-SHAKE-192f.log` |
| `Phoenix-SHAKE-192f` | timing keygen | `records/sign-19/Phoenix-SHAKE-192f__keygen.json` |
| `Phoenix-SHAKE-192f` | timing sign | `records/sign-19/Phoenix-SHAKE-192f__sign.json` |
| `Phoenix-SHAKE-192f` | timing verify | `records/sign-19/Phoenix-SHAKE-192f__verify.json` |
| `Phoenix-SHAKE-192f` | hash profile keygen | `profile/sign-19/Phoenix-SHAKE-192f__keygen.json` |
| `Phoenix-SHAKE-192f` | hash profile sign | `profile/sign-19/Phoenix-SHAKE-192f__sign.json` |
| `Phoenix-SHAKE-192f` | hash profile verify | `profile/sign-19/Phoenix-SHAKE-192f__verify.json` |
| `Phoenix-SHAKE-192s` | KAT log (sha256 `ee8befb75da7df8f…`) | `kat/sign-19/Phoenix-SHAKE-192s.log` |
| `Phoenix-SHAKE-192s` | timing keygen | `records/sign-19/Phoenix-SHAKE-192s__keygen.json` |
| `Phoenix-SHAKE-192s` | timing sign | `records/sign-19/Phoenix-SHAKE-192s__sign.json` |
| `Phoenix-SHAKE-192s` | timing verify | `records/sign-19/Phoenix-SHAKE-192s__verify.json` |
| `Phoenix-SHAKE-192s` | hash profile keygen | `profile/sign-19/Phoenix-SHAKE-192s__keygen.json` |
| `Phoenix-SHAKE-192s` | hash profile sign | `profile/sign-19/Phoenix-SHAKE-192s__sign.json` |
| `Phoenix-SHAKE-192s` | hash profile verify | `profile/sign-19/Phoenix-SHAKE-192s__verify.json` |
| `Phoenix-SHAKE-256f` | KAT log (sha256 `5de05d3ecc832bd6…`) | `kat/sign-19/Phoenix-SHAKE-256f.log` |
| `Phoenix-SHAKE-256f` | timing keygen | `records/sign-19/Phoenix-SHAKE-256f__keygen.json` |
| `Phoenix-SHAKE-256f` | timing sign | `records/sign-19/Phoenix-SHAKE-256f__sign.json` |
| `Phoenix-SHAKE-256f` | timing verify | `records/sign-19/Phoenix-SHAKE-256f__verify.json` |
| `Phoenix-SHAKE-256f` | hash profile keygen | `profile/sign-19/Phoenix-SHAKE-256f__keygen.json` |
| `Phoenix-SHAKE-256f` | hash profile sign | `profile/sign-19/Phoenix-SHAKE-256f__sign.json` |
| `Phoenix-SHAKE-256f` | hash profile verify | `profile/sign-19/Phoenix-SHAKE-256f__verify.json` |
| `Phoenix-SHAKE-256s` | KAT log (sha256 `c7f22fa5a3a7533d…`) | `kat/sign-19/Phoenix-SHAKE-256s.log` |
| `Phoenix-SHAKE-256s` | timing keygen | `records/sign-19/Phoenix-SHAKE-256s__keygen.json` |
| `Phoenix-SHAKE-256s` | timing sign | `records/sign-19/Phoenix-SHAKE-256s__sign.json` |
| `Phoenix-SHAKE-256s` | timing verify | `records/sign-19/Phoenix-SHAKE-256s__verify.json` |
| `Phoenix-SHAKE-256s` | hash profile keygen | `profile/sign-19/Phoenix-SHAKE-256s__keygen.json` |
| `Phoenix-SHAKE-256s` | hash profile sign | `profile/sign-19/Phoenix-SHAKE-256s__sign.json` |
| `Phoenix-SHAKE-256s` | hash profile verify | `profile/sign-19/Phoenix-SHAKE-256s__verify.json` |
| `Phoenix-SHAKE-384f` | KAT log (sha256 `1ffa929a8064d65c…`) | `kat/sign-19/Phoenix-SHAKE-384f.log` |
| `Phoenix-SHAKE-384f` | timing keygen | `records/sign-19/Phoenix-SHAKE-384f__keygen.json` |
| `Phoenix-SHAKE-384f` | timing sign | `records/sign-19/Phoenix-SHAKE-384f__sign.json` |
| `Phoenix-SHAKE-384f` | timing verify | `records/sign-19/Phoenix-SHAKE-384f__verify.json` |
| `Phoenix-SHAKE-384f` | hash profile keygen | `profile/sign-19/Phoenix-SHAKE-384f__keygen.json` |
| `Phoenix-SHAKE-384f` | hash profile sign | `profile/sign-19/Phoenix-SHAKE-384f__sign.json` |
| `Phoenix-SHAKE-384f` | hash profile verify | `profile/sign-19/Phoenix-SHAKE-384f__verify.json` |
| `Phoenix-SHAKE-384s` | KAT log (sha256 `3e7b1da4a4cfe205…`) | `kat/sign-19/Phoenix-SHAKE-384s.log` |
| `Phoenix-SHAKE-384s` | timing keygen | `records/sign-19/Phoenix-SHAKE-384s__keygen.json` |
| `Phoenix-SHAKE-384s` | timing sign | `records/sign-19/Phoenix-SHAKE-384s__sign.json` |
| `Phoenix-SHAKE-384s` | timing verify | `records/sign-19/Phoenix-SHAKE-384s__verify.json` |
| `Phoenix-SHAKE-384s` | hash profile keygen | `profile/sign-19/Phoenix-SHAKE-384s__keygen.json` |
| `Phoenix-SHAKE-384s` | hash profile sign | `profile/sign-19/Phoenix-SHAKE-384s__sign.json` |
| `Phoenix-SHAKE-384s` | hash profile verify | `profile/sign-19/Phoenix-SHAKE-384s__verify.json` |
| `Phoenix-SHAKE-512f` | KAT log (sha256 `ac36f5d11b73fc40…`) | `kat/sign-19/Phoenix-SHAKE-512f.log` |
| `Phoenix-SHAKE-512f` | timing keygen | `records/sign-19/Phoenix-SHAKE-512f__keygen.json` |
| `Phoenix-SHAKE-512f` | timing sign | `records/sign-19/Phoenix-SHAKE-512f__sign.json` |
| `Phoenix-SHAKE-512f` | timing verify | `records/sign-19/Phoenix-SHAKE-512f__verify.json` |
| `Phoenix-SHAKE-512f` | hash profile keygen | `profile/sign-19/Phoenix-SHAKE-512f__keygen.json` |
| `Phoenix-SHAKE-512f` | hash profile sign | `profile/sign-19/Phoenix-SHAKE-512f__sign.json` |
| `Phoenix-SHAKE-512f` | hash profile verify | `profile/sign-19/Phoenix-SHAKE-512f__verify.json` |
| `Phoenix-SHAKE-512s` | KAT log (sha256 `e5d33df2de6e0622…`) | `kat/sign-19/Phoenix-SHAKE-512s.log` |
| `Phoenix-SHAKE-512s` | timing keygen | `records/sign-19/Phoenix-SHAKE-512s__keygen.json` |
| `Phoenix-SHAKE-512s` | timing sign | `records/sign-19/Phoenix-SHAKE-512s__sign.json` |
| `Phoenix-SHAKE-512s` | timing verify | `records/sign-19/Phoenix-SHAKE-512s__verify.json` |
| `Phoenix-SHAKE-512s` | hash profile keygen | `profile/sign-19/Phoenix-SHAKE-512s__keygen.json` |
| `Phoenix-SHAKE-512s` | hash profile sign | `profile/sign-19/Phoenix-SHAKE-512s__sign.json` |
| `Phoenix-SHAKE-512s` | hash profile verify | `profile/sign-19/Phoenix-SHAKE-512s__verify.json` |
| `Phoenix-SM3-128f` | KAT log (sha256 `211d8fba90bb5f0c…`) | `kat/sign-19/Phoenix-SM3-128f.log` |
| `Phoenix-SM3-128f` | timing keygen | `records/sign-19/Phoenix-SM3-128f__keygen.json` |
| `Phoenix-SM3-128f` | timing sign | `records/sign-19/Phoenix-SM3-128f__sign.json` |
| `Phoenix-SM3-128f` | timing verify | `records/sign-19/Phoenix-SM3-128f__verify.json` |
| `Phoenix-SM3-128f` | hash profile keygen | `profile/sign-19/Phoenix-SM3-128f__keygen.json` |
| `Phoenix-SM3-128f` | hash profile sign | `profile/sign-19/Phoenix-SM3-128f__sign.json` |
| `Phoenix-SM3-128f` | hash profile verify | `profile/sign-19/Phoenix-SM3-128f__verify.json` |
| `Phoenix-SM3-128s` | KAT log (sha256 `0412843401341c36…`) | `kat/sign-19/Phoenix-SM3-128s.log` |
| `Phoenix-SM3-128s` | timing keygen | `records/sign-19/Phoenix-SM3-128s__keygen.json` |
| `Phoenix-SM3-128s` | timing sign | `records/sign-19/Phoenix-SM3-128s__sign.json` |
| `Phoenix-SM3-128s` | timing verify | `records/sign-19/Phoenix-SM3-128s__verify.json` |
| `Phoenix-SM3-128s` | hash profile keygen | `profile/sign-19/Phoenix-SM3-128s__keygen.json` |
| `Phoenix-SM3-128s` | hash profile sign | `profile/sign-19/Phoenix-SM3-128s__sign.json` |
| `Phoenix-SM3-128s` | hash profile verify | `profile/sign-19/Phoenix-SM3-128s__verify.json` |
| `Phoenix-SM3-192f` | KAT log (sha256 `53b10b21f7ebc6f6…`) | `kat/sign-19/Phoenix-SM3-192f.log` |
| `Phoenix-SM3-192f` | timing keygen | `records/sign-19/Phoenix-SM3-192f__keygen.json` |
| `Phoenix-SM3-192f` | timing sign | `records/sign-19/Phoenix-SM3-192f__sign.json` |
| `Phoenix-SM3-192f` | timing verify | `records/sign-19/Phoenix-SM3-192f__verify.json` |
| `Phoenix-SM3-192f` | hash profile keygen | `profile/sign-19/Phoenix-SM3-192f__keygen.json` |
| `Phoenix-SM3-192f` | hash profile sign | `profile/sign-19/Phoenix-SM3-192f__sign.json` |
| `Phoenix-SM3-192f` | hash profile verify | `profile/sign-19/Phoenix-SM3-192f__verify.json` |
| `Phoenix-SM3-192s` | KAT log (sha256 `fde50ceeae650343…`) | `kat/sign-19/Phoenix-SM3-192s.log` |
| `Phoenix-SM3-192s` | timing keygen | `records/sign-19/Phoenix-SM3-192s__keygen.json` |
| `Phoenix-SM3-192s` | timing sign | `records/sign-19/Phoenix-SM3-192s__sign.json` |
| `Phoenix-SM3-192s` | timing verify | `records/sign-19/Phoenix-SM3-192s__verify.json` |
| `Phoenix-SM3-192s` | hash profile keygen | `profile/sign-19/Phoenix-SM3-192s__keygen.json` |
| `Phoenix-SM3-192s` | hash profile sign | `profile/sign-19/Phoenix-SM3-192s__sign.json` |
| `Phoenix-SM3-192s` | hash profile verify | `profile/sign-19/Phoenix-SM3-192s__verify.json` |
| `Phoenix-SM3-256f` | KAT log (sha256 `9104a8928a4a0464…`) | `kat/sign-19/Phoenix-SM3-256f.log` |
| `Phoenix-SM3-256f` | timing keygen | `records/sign-19/Phoenix-SM3-256f__keygen.json` |
| `Phoenix-SM3-256f` | timing sign | `records/sign-19/Phoenix-SM3-256f__sign.json` |
| `Phoenix-SM3-256f` | timing verify | `records/sign-19/Phoenix-SM3-256f__verify.json` |
| `Phoenix-SM3-256f` | hash profile keygen | `profile/sign-19/Phoenix-SM3-256f__keygen.json` |
| `Phoenix-SM3-256f` | hash profile sign | `profile/sign-19/Phoenix-SM3-256f__sign.json` |
| `Phoenix-SM3-256f` | hash profile verify | `profile/sign-19/Phoenix-SM3-256f__verify.json` |
| `Phoenix-SM3-256s` | KAT log (sha256 `53625297f3173252…`) | `kat/sign-19/Phoenix-SM3-256s.log` |
| `Phoenix-SM3-256s` | timing keygen | `records/sign-19/Phoenix-SM3-256s__keygen.json` |
| `Phoenix-SM3-256s` | timing sign | `records/sign-19/Phoenix-SM3-256s__sign.json` |
| `Phoenix-SM3-256s` | timing verify | `records/sign-19/Phoenix-SM3-256s__verify.json` |
| `Phoenix-SM3-256s` | hash profile keygen | `profile/sign-19/Phoenix-SM3-256s__keygen.json` |
| `Phoenix-SM3-256s` | hash profile sign | `profile/sign-19/Phoenix-SM3-256s__sign.json` |
| `Phoenix-SM3-256s` | hash profile verify | `profile/sign-19/Phoenix-SM3-256s__verify.json` |
| `Phoenix-SM3-384f` | KAT log (sha256 `fb913fa1c6f5efd9…`) | `kat/sign-19/Phoenix-SM3-384f.log` |
| `Phoenix-SM3-384f` | timing keygen | `records/sign-19/Phoenix-SM3-384f__keygen.json` |
| `Phoenix-SM3-384f` | timing sign | `records/sign-19/Phoenix-SM3-384f__sign.json` |
| `Phoenix-SM3-384f` | timing verify | `records/sign-19/Phoenix-SM3-384f__verify.json` |
| `Phoenix-SM3-384f` | hash profile keygen | `profile/sign-19/Phoenix-SM3-384f__keygen.json` |
| `Phoenix-SM3-384f` | hash profile sign | `profile/sign-19/Phoenix-SM3-384f__sign.json` |
| `Phoenix-SM3-384f` | hash profile verify | `profile/sign-19/Phoenix-SM3-384f__verify.json` |
| `Phoenix-SM3-384s` | KAT log (sha256 `e5ac969bc4562bbe…`) | `kat/sign-19/Phoenix-SM3-384s.log` |
| `Phoenix-SM3-384s` | timing keygen | `records/sign-19/Phoenix-SM3-384s__keygen.json` |
| `Phoenix-SM3-384s` | timing sign | `records/sign-19/Phoenix-SM3-384s__sign.json` |
| `Phoenix-SM3-384s` | timing verify | `records/sign-19/Phoenix-SM3-384s__verify.json` |
| `Phoenix-SM3-384s` | hash profile keygen | `profile/sign-19/Phoenix-SM3-384s__keygen.json` |
| `Phoenix-SM3-384s` | hash profile sign | `profile/sign-19/Phoenix-SM3-384s__sign.json` |
| `Phoenix-SM3-384s` | hash profile verify | `profile/sign-19/Phoenix-SM3-384s__verify.json` |
| `Phoenix-SM3-512f` | KAT log (sha256 `535501cec023a9d9…`) | `kat/sign-19/Phoenix-SM3-512f.log` |
| `Phoenix-SM3-512f` | timing keygen | `records/sign-19/Phoenix-SM3-512f__keygen.json` |
| `Phoenix-SM3-512f` | timing sign | `records/sign-19/Phoenix-SM3-512f__sign.json` |
| `Phoenix-SM3-512f` | timing verify | `records/sign-19/Phoenix-SM3-512f__verify.json` |
| `Phoenix-SM3-512f` | hash profile keygen | `profile/sign-19/Phoenix-SM3-512f__keygen.json` |
| `Phoenix-SM3-512f` | hash profile sign | `profile/sign-19/Phoenix-SM3-512f__sign.json` |
| `Phoenix-SM3-512f` | hash profile verify | `profile/sign-19/Phoenix-SM3-512f__verify.json` |
| `Phoenix-SM3-512s` | KAT log (sha256 `8f6bea2945a6180f…`) | `kat/sign-19/Phoenix-SM3-512s.log` |
| `Phoenix-SM3-512s` | timing keygen | `records/sign-19/Phoenix-SM3-512s__keygen.json` |
| `Phoenix-SM3-512s` | timing sign | `records/sign-19/Phoenix-SM3-512s__sign.json` |
| `Phoenix-SM3-512s` | timing verify | `records/sign-19/Phoenix-SM3-512s__verify.json` |
| `Phoenix-SM3-512s` | hash profile keygen | `profile/sign-19/Phoenix-SM3-512s__keygen.json` |
| `Phoenix-SM3-512s` | hash profile sign | `profile/sign-19/Phoenix-SM3-512s__sign.json` |
| `Phoenix-SM3-512s` | hash profile verify | `profile/sign-19/Phoenix-SM3-512s__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

