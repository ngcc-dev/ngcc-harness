# kex-07 NEV-AKE — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kex-07` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560626045865984.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key exchange
- Algorithm: NEV-AKE
- Implementation versions measured: reference
- Parameter sets: `NEV_AKE_512_769`, `NEV_AKE_512_769_C`, `NEV_AKE_512_1409`, `NEV_AKE_1024_769`, `NEV_AKE_1024_769_C`, `NEV_AKE_1024_1409`, `NEV_AKE_2048_769`, `NEV_AKE_2048_769_C`, `NEV_AKE_2048_1409`

## 2. Assessment environment

| item | value |
|---|---|
| processor | Qualcomm Oryon (CPU 2, one core) |
| machine | ASUS Vivobook S 15 |
| clock | fixed 2.71 GHz (governor performance, minimum = maximum), boost off, SMT none |
| memory | 30562 MiB |
| OS / kernel | Ubuntu 26.04.1 LTS / 7.0.0-34-generic |
| compiler / build tool | gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0 / cmake version 4.2.3 |
| campaign start / end (UTC) | 2026-09-28T15:05:46 / 2026-09-29T02:47:54 |

## 3. Functional testing (KAT)

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kex-07/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `NEV_AKE_512_769` | guide | PASS |
| `NEV_AKE_512_769_C` | guide | PASS |
| `NEV_AKE_512_1409` | guide | PASS |
| `NEV_AKE_1024_769` | guide | PASS |
| `NEV_AKE_1024_769_C` | guide | PASS |
| `NEV_AKE_1024_1409` | guide | PASS |
| `NEV_AKE_2048_769` | guide | PASS |
| `NEV_AKE_2048_769_C` | guide | PASS |
| `NEV_AKE_2048_1409` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `NEV_AKE_512_769` | exchange | 668.8 k | 248 µs | 4.03e+03 | 248 µs | 12575 (5 × 2515) |
| `NEV_AKE_512_769` | init_a | 59.1 k | 21.9 µs | 4.57e+04 | 21.9 µs | 12575 (5 × 2515) |
| `NEV_AKE_512_769` | init_b | 59.1 k | 21.9 µs | 4.57e+04 | 21.9 µs | 12575 (5 × 2515) |
| `NEV_AKE_512_769` | pass1 | 99.4 k | 36.8 µs | 2.71e+04 | 36.7 µs | 12575 (5 × 2515) |
| `NEV_AKE_512_769` | pass2 | 261.8 k | 97.1 µs | 1.03e+04 | 96.8 µs | 12575 (5 × 2515) |
| `NEV_AKE_512_769` | derive_a | 190.6 k | 70.7 µs | 1.41e+04 | 70.7 µs | 12575 (5 × 2515) |
| `NEV_AKE_512_769` | derive_b | 183 | 23.1 ns | 4.33e+07 | 23.6 ns | 12575 (5 × 2515) |
| `NEV_AKE_512_769_C` | exchange | 629.5 k | 234 µs | 4.28e+03 | 233 µs | 13345 (5 × 2669) |
| `NEV_AKE_512_769_C` | init_a | 59.2 k | 21.9 µs | 4.56e+04 | 21.9 µs | 13345 (5 × 2669) |
| `NEV_AKE_512_769_C` | init_b | 58.9 k | 21.8 µs | 4.59e+04 | 21.8 µs | 13345 (5 × 2669) |
| `NEV_AKE_512_769_C` | pass1 | 92.7 k | 34.3 µs | 2.91e+04 | 34.3 µs | 13345 (5 × 2669) |
| `NEV_AKE_512_769_C` | pass2 | 241.5 k | 89.6 µs | 1.12e+04 | 89.4 µs | 13345 (5 × 2669) |
| `NEV_AKE_512_769_C` | derive_a | 178.8 k | 66.3 µs | 1.51e+04 | 66.1 µs | 13345 (5 × 2669) |
| `NEV_AKE_512_769_C` | derive_b | 182 | 23.8 ns | 4.2e+07 | 23.6 ns | 13345 (5 × 2669) |
| `NEV_AKE_512_1409` | exchange | 825.0 k | 306 µs | 3.27e+03 | 306 µs | 10240 (5 × 2048) |
| `NEV_AKE_512_1409` | init_a | 85.9 k | 31.8 µs | 3.14e+04 | 31.7 µs | 10240 (5 × 2048) |
| `NEV_AKE_512_1409` | init_b | 85.6 k | 31.7 µs | 3.15e+04 | 31.6 µs | 10240 (5 × 2048) |
| `NEV_AKE_512_1409` | pass1 | 138.8 k | 51.4 µs | 1.94e+04 | 51.4 µs | 10240 (5 × 2048) |
| `NEV_AKE_512_1409` | pass2 | 306.7 k | 114 µs | 8.79e+03 | 114 µs | 10240 (5 × 2048) |
| `NEV_AKE_512_1409` | derive_a | 207.9 k | 77.1 µs | 1.3e+04 | 77 µs | 10240 (5 × 2048) |
| `NEV_AKE_512_1409` | derive_b | 182 | 23.5 ns | 4.25e+07 | 23.9 ns | 10240 (5 × 2048) |
| `NEV_AKE_1024_769` | exchange | 1.35 M | 502 µs | 1.99e+03 | 501 µs | 6215 (5 × 1243) |
| `NEV_AKE_1024_769` | init_a | 126.0 k | 46.7 µs | 2.14e+04 | 46.7 µs | 6215 (5 × 1243) |
| `NEV_AKE_1024_769` | init_b | 125.8 k | 46.6 µs | 2.14e+04 | 46.7 µs | 6215 (5 × 1243) |
| `NEV_AKE_1024_769` | pass1 | 205.3 k | 76.1 µs | 1.31e+04 | 76 µs | 6215 (5 × 1243) |
| `NEV_AKE_1024_769` | pass2 | 513.5 k | 190 µs | 5.25e+03 | 191 µs | 6215 (5 × 1243) |
| `NEV_AKE_1024_769` | derive_a | 384.3 k | 143 µs | 7.01e+03 | 143 µs | 6215 (5 × 1243) |
| `NEV_AKE_1024_769` | derive_b | 187 | 24.7 ns | 4.05e+07 | 25.1 ns | 6215 (5 × 1243) |
| `NEV_AKE_1024_769_C` | exchange | 1.29 M | 478 µs | 2.09e+03 | 478 µs | 6500 (5 × 1300) |
| `NEV_AKE_1024_769_C` | init_a | 125.8 k | 46.6 µs | 2.14e+04 | 46.7 µs | 6500 (5 × 1300) |
| `NEV_AKE_1024_769_C` | init_b | 125.1 k | 46.4 µs | 2.16e+04 | 46.4 µs | 6500 (5 × 1300) |
| `NEV_AKE_1024_769_C` | pass1 | 194.2 k | 72 µs | 1.39e+04 | 72 µs | 6500 (5 × 1300) |
| `NEV_AKE_1024_769_C` | pass2 | 479.7 k | 178 µs | 5.62e+03 | 178 µs | 6500 (5 × 1300) |
| `NEV_AKE_1024_769_C` | derive_a | 363.1 k | 135 µs | 7.43e+03 | 135 µs | 6500 (5 × 1300) |
| `NEV_AKE_1024_769_C` | derive_b | 186 | 25.4 ns | 3.94e+07 | 25.3 ns | 6500 (5 × 1300) |
| `NEV_AKE_1024_1409` | exchange | 1.55 M | 574 µs | 1.74e+03 | 573 µs | 5465 (5 × 1093) |
| `NEV_AKE_1024_1409` | init_a | 165.1 k | 61.2 µs | 1.63e+04 | 61.2 µs | 5465 (5 × 1093) |
| `NEV_AKE_1024_1409` | init_b | 164.9 k | 61.1 µs | 1.64e+04 | 61.1 µs | 5465 (5 × 1093) |
| `NEV_AKE_1024_1409` | pass1 | 251.9 k | 93.4 µs | 1.07e+04 | 93.3 µs | 5465 (5 × 1093) |
| `NEV_AKE_1024_1409` | pass2 | 553.1 k | 205 µs | 4.87e+03 | 205 µs | 5465 (5 × 1093) |
| `NEV_AKE_1024_1409` | derive_a | 407.9 k | 151 µs | 6.61e+03 | 151 µs | 5465 (5 × 1093) |
| `NEV_AKE_1024_1409` | derive_b | 187 | 24.5 ns | 4.08e+07 | 24.8 ns | 5465 (5 × 1093) |
| `NEV_AKE_2048_769` | exchange | 4.18 M | 1.55 ms | 645 | 1.55 ms | 1995 (5 × 399) |
| `NEV_AKE_2048_769` | init_a | 384.7 k | 143 µs | 7.01e+03 | 143 µs | 1995 (5 × 399) |
| `NEV_AKE_2048_769` | init_b | 385.3 k | 143 µs | 7e+03 | 143 µs | 1995 (5 × 399) |
| `NEV_AKE_2048_769` | pass1 | 569.0 k | 211 µs | 4.74e+03 | 211 µs | 1995 (5 × 399) |
| `NEV_AKE_2048_769` | pass2 | 1.57 M | 584 µs | 1.71e+03 | 581 µs | 1995 (5 × 399) |
| `NEV_AKE_2048_769` | derive_a | 1.28 M | 476 µs | 2.1e+03 | 474 µs | 1995 (5 × 399) |
| `NEV_AKE_2048_769` | derive_b | 191 | 26.2 ns | 3.81e+07 | 24.5 ns | 1995 (5 × 399) |
| `NEV_AKE_2048_769_C` | exchange | 3.91 M | 1.45 ms | 689 | 1.45 ms | 2160 (5 × 432) |
| `NEV_AKE_2048_769_C` | init_a | 383.6 k | 142 µs | 7.03e+03 | 142 µs | 2160 (5 × 432) |
| `NEV_AKE_2048_769_C` | init_b | 384.5 k | 143 µs | 7.01e+03 | 142 µs | 2160 (5 × 432) |
| `NEV_AKE_2048_769_C` | pass1 | 534.3 k | 198 µs | 5.05e+03 | 198 µs | 2160 (5 × 432) |
| `NEV_AKE_2048_769_C` | pass2 | 1.44 M | 534 µs | 1.87e+03 | 531 µs | 2160 (5 × 432) |
| `NEV_AKE_2048_769_C` | derive_a | 1.18 M | 438 µs | 2.28e+03 | 437 µs | 2160 (5 × 432) |
| `NEV_AKE_2048_769_C` | derive_b | 216 | 33.5 ns | 2.98e+07 | 29.1 ns | 2160 (5 × 432) |
| `NEV_AKE_2048_1409` | exchange | 4.69 M | 1.74 ms | 574 | 1.74 ms | 1815 (5 × 363) |
| `NEV_AKE_2048_1409` | init_a | 414.4 k | 154 µs | 6.5e+03 | 153 µs | 1815 (5 × 363) |
| `NEV_AKE_2048_1409` | init_b | 412.7 k | 153 µs | 6.53e+03 | 153 µs | 1815 (5 × 363) |
| `NEV_AKE_2048_1409` | pass1 | 648.0 k | 240 µs | 4.16e+03 | 240 µs | 1815 (5 × 363) |
| `NEV_AKE_2048_1409` | pass2 | 1.80 M | 669 µs | 1.49e+03 | 667 µs | 1815 (5 × 363) |
| `NEV_AKE_2048_1409` | derive_a | 1.41 M | 523 µs | 1.91e+03 | 523 µs | 1815 (5 × 363) |
| `NEV_AKE_2048_1409` | derive_b | 189 | 25.3 ns | 3.96e+07 | 26.2 ns | 1815 (5 × 363) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `NEV_AKE_512_769` | exchange | 41852 | 1460 KiB | 1528 KiB |
| `NEV_AKE_512_769` | init_a | 41852 | 1460 KiB | 1528 KiB |
| `NEV_AKE_512_769` | init_b | 41852 | 1460 KiB | 1528 KiB |
| `NEV_AKE_512_769` | pass1 | 41852 | 1460 KiB | 1528 KiB |
| `NEV_AKE_512_769` | pass2 | 41852 | 1460 KiB | 1528 KiB |
| `NEV_AKE_512_769` | derive_a | 41852 | 1460 KiB | 1528 KiB |
| `NEV_AKE_512_769` | derive_b | 41852 | 1456 KiB | 1524 KiB |
| `NEV_AKE_512_769_C` | exchange | 41884 | 3496 KiB | 3560 KiB |
| `NEV_AKE_512_769_C` | init_a | 41884 | 1460 KiB | 1524 KiB |
| `NEV_AKE_512_769_C` | init_b | 41884 | 1460 KiB | 1524 KiB |
| `NEV_AKE_512_769_C` | pass1 | 41884 | 1460 KiB | 1524 KiB |
| `NEV_AKE_512_769_C` | pass2 | 41884 | 1456 KiB | 1520 KiB |
| `NEV_AKE_512_769_C` | derive_a | 41884 | 1456 KiB | 1520 KiB |
| `NEV_AKE_512_769_C` | derive_b | 41884 | 1460 KiB | 1524 KiB |
| `NEV_AKE_512_1409` | exchange | 44060 | 1468 KiB | 1536 KiB |
| `NEV_AKE_512_1409` | init_a | 44060 | 1468 KiB | 1536 KiB |
| `NEV_AKE_512_1409` | init_b | 44060 | 1468 KiB | 1536 KiB |
| `NEV_AKE_512_1409` | pass1 | 44060 | 1468 KiB | 1536 KiB |
| `NEV_AKE_512_1409` | pass2 | 44060 | 1468 KiB | 1536 KiB |
| `NEV_AKE_512_1409` | derive_a | 44060 | 1468 KiB | 1536 KiB |
| `NEV_AKE_512_1409` | derive_b | 44060 | 1468 KiB | 1536 KiB |
| `NEV_AKE_1024_769` | exchange | 44804 | 1500 KiB | 1568 KiB |
| `NEV_AKE_1024_769` | init_a | 44804 | 1504 KiB | 1572 KiB |
| `NEV_AKE_1024_769` | init_b | 44804 | 1500 KiB | 1568 KiB |
| `NEV_AKE_1024_769` | pass1 | 44804 | 1500 KiB | 1568 KiB |
| `NEV_AKE_1024_769` | pass2 | 44804 | 1500 KiB | 1568 KiB |
| `NEV_AKE_1024_769` | derive_a | 44804 | 3480 KiB | 3544 KiB |
| `NEV_AKE_1024_769` | derive_b | 44804 | 1504 KiB | 1572 KiB |
| `NEV_AKE_1024_769_C` | exchange | 44836 | 1496 KiB | 1564 KiB |
| `NEV_AKE_1024_769_C` | init_a | 44836 | 1492 KiB | 1560 KiB |
| `NEV_AKE_1024_769_C` | init_b | 44836 | 1496 KiB | 1564 KiB |
| `NEV_AKE_1024_769_C` | pass1 | 44836 | 3504 KiB | 3568 KiB |
| `NEV_AKE_1024_769_C` | pass2 | 44836 | 1496 KiB | 1564 KiB |
| `NEV_AKE_1024_769_C` | derive_a | 44836 | 1496 KiB | 1564 KiB |
| `NEV_AKE_1024_769_C` | derive_b | 44836 | 1492 KiB | 1560 KiB |
| `NEV_AKE_1024_1409` | exchange | 47804 | 1512 KiB | 1580 KiB |
| `NEV_AKE_1024_1409` | init_a | 47804 | 1512 KiB | 1580 KiB |
| `NEV_AKE_1024_1409` | init_b | 47804 | 1512 KiB | 1580 KiB |
| `NEV_AKE_1024_1409` | pass1 | 47804 | 1512 KiB | 1580 KiB |
| `NEV_AKE_1024_1409` | pass2 | 47804 | 1512 KiB | 1580 KiB |
| `NEV_AKE_1024_1409` | derive_a | 47804 | 1512 KiB | 1580 KiB |
| `NEV_AKE_1024_1409` | derive_b | 47804 | 1512 KiB | 1580 KiB |
| `NEV_AKE_2048_769` | exchange | 48728 | 1584 KiB | 1652 KiB |
| `NEV_AKE_2048_769` | init_a | 48728 | 1584 KiB | 1652 KiB |
| `NEV_AKE_2048_769` | init_b | 48728 | 3560 KiB | 3624 KiB |
| `NEV_AKE_2048_769` | pass1 | 48728 | 3600 KiB | 3664 KiB |
| `NEV_AKE_2048_769` | pass2 | 48728 | 1584 KiB | 1652 KiB |
| `NEV_AKE_2048_769` | derive_a | 48728 | 3512 KiB | 3576 KiB |
| `NEV_AKE_2048_769` | derive_b | 48728 | 1584 KiB | 1652 KiB |
| `NEV_AKE_2048_769_C` | exchange | 48760 | 1572 KiB | 1640 KiB |
| `NEV_AKE_2048_769_C` | init_a | 48760 | 1572 KiB | 1640 KiB |
| `NEV_AKE_2048_769_C` | init_b | 48760 | 1576 KiB | 1644 KiB |
| `NEV_AKE_2048_769_C` | pass1 | 48760 | 1576 KiB | 1644 KiB |
| `NEV_AKE_2048_769_C` | pass2 | 48760 | 1576 KiB | 1644 KiB |
| `NEV_AKE_2048_769_C` | derive_a | 48760 | 1572 KiB | 1640 KiB |
| `NEV_AKE_2048_769_C` | derive_b | 48760 | 1576 KiB | 1644 KiB |
| `NEV_AKE_2048_1409` | exchange | 52464 | 1600 KiB | 1668 KiB |
| `NEV_AKE_2048_1409` | init_a | 52464 | 1604 KiB | 1672 KiB |
| `NEV_AKE_2048_1409` | init_b | 52464 | 1604 KiB | 1672 KiB |
| `NEV_AKE_2048_1409` | pass1 | 52464 | 3616 KiB | 3680 KiB |
| `NEV_AKE_2048_1409` | pass2 | 52464 | 1604 KiB | 3716 KiB |
| `NEV_AKE_2048_1409` | derive_a | 52464 | 3600 KiB | 3664 KiB |
| `NEV_AKE_2048_1409` | derive_b | 52464 | 1600 KiB | 1668 KiB |

## 6. Transmission and storage overhead

| instance | passes | messages (bytes) | total | long-term pk / sk | shared secret |
|---|---|---|---|---|---|
| `NEV_AKE_512_769` | 2 | 1230 / 1230 | 2460 | 615 / 1246 | 16 |
| `NEV_AKE_512_769_C` | 2 | 1127 / 1127 | 2254 | 615 / 1246 | 16 |
| `NEV_AKE_512_1409` | 2 | 1344 / 1344 | 2688 | 672 / 1360 | 16 |
| `NEV_AKE_1024_769` | 2 | 2458 / 2458 | 4916 | 1229 / 2490 | 32 |
| `NEV_AKE_1024_769_C` | 2 | 2253 / 2253 | 4506 | 1229 / 2490 | 32 |
| `NEV_AKE_1024_1409` | 2 | 2688 / 2688 | 5376 | 1344 / 2720 | 32 |
| `NEV_AKE_2048_769` | 2 | 4916 / 4916 | 9832 | 2458 / 4980 | 64 |
| `NEV_AKE_2048_769_C` | 2 | 4506 / 4506 | 9012 | 2458 / 4980 | 64 |
| `NEV_AKE_2048_1409` | 2 | 5376 / 5376 | 10752 | 2688 / 5440 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — own fips202.c compiled but unreachable

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `NEV_AKE_512_769` | exchange | 56% | 5.1% | drng 8, pseudoXOF 22, sm3hash 25 |
| `NEV_AKE_512_769_C` | exchange | 53% | 5.4% | drng 8, pseudoXOF 22, sm3hash 5 |
| `NEV_AKE_512_1409` | exchange | 64% | 4.1% | drng 8, pseudoXOF 27 |
| `NEV_AKE_1024_769` | exchange | 53% | 2.5% | drng 8, pseudoXOF 19, sm3hash 43 |
| `NEV_AKE_1024_769_C` | exchange | 49% | 2.6% | drng 8, pseudoXOF 19, sm3hash 11 |
| `NEV_AKE_1024_1409` | exchange | 56% | 2.2% | drng 8, pseudoXOF 24, sm3hash 3 |
| `NEV_AKE_2048_769` | exchange | 63% | 1.1% | drng 8, pseudoXOF 19, pseudohash 3, sm3hash 70 |
| `NEV_AKE_2048_769_C` | exchange | 60% | 1.2% | drng 8, pseudoXOF 19, pseudohash 3, sm3hash 14 |
| `NEV_AKE_2048_1409` | exchange | 61% | 1.0% | drng 8, pseudoXOF 13, pseudohash 3, sm3hash 154 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `NEV_AKE_512_769` | KAT log (sha256 `97715c9472d4d783…`) | `kat/kex-07/NEV_AKE_512_769.log` |
| `NEV_AKE_512_769` | timing derive_a | `records/kex-07/NEV_AKE_512_769__derive_a.json` |
| `NEV_AKE_512_769` | timing derive_b | `records/kex-07/NEV_AKE_512_769__derive_b.json` |
| `NEV_AKE_512_769` | timing exchange | `records/kex-07/NEV_AKE_512_769__exchange.json` |
| `NEV_AKE_512_769` | timing init_a | `records/kex-07/NEV_AKE_512_769__init_a.json` |
| `NEV_AKE_512_769` | timing init_b | `records/kex-07/NEV_AKE_512_769__init_b.json` |
| `NEV_AKE_512_769` | timing pass1 | `records/kex-07/NEV_AKE_512_769__pass1.json` |
| `NEV_AKE_512_769` | timing pass2 | `records/kex-07/NEV_AKE_512_769__pass2.json` |
| `NEV_AKE_512_769` | hash profile exchange | `profile/kex-07/NEV_AKE_512_769__exchange.json` |
| `NEV_AKE_512_769_C` | KAT log (sha256 `075b11832b1587d9…`) | `kat/kex-07/NEV_AKE_512_769_C.log` |
| `NEV_AKE_512_769_C` | timing derive_a | `records/kex-07/NEV_AKE_512_769_C__derive_a.json` |
| `NEV_AKE_512_769_C` | timing derive_b | `records/kex-07/NEV_AKE_512_769_C__derive_b.json` |
| `NEV_AKE_512_769_C` | timing exchange | `records/kex-07/NEV_AKE_512_769_C__exchange.json` |
| `NEV_AKE_512_769_C` | timing init_a | `records/kex-07/NEV_AKE_512_769_C__init_a.json` |
| `NEV_AKE_512_769_C` | timing init_b | `records/kex-07/NEV_AKE_512_769_C__init_b.json` |
| `NEV_AKE_512_769_C` | timing pass1 | `records/kex-07/NEV_AKE_512_769_C__pass1.json` |
| `NEV_AKE_512_769_C` | timing pass2 | `records/kex-07/NEV_AKE_512_769_C__pass2.json` |
| `NEV_AKE_512_769_C` | hash profile exchange | `profile/kex-07/NEV_AKE_512_769_C__exchange.json` |
| `NEV_AKE_512_1409` | KAT log (sha256 `3ca9300cfc71f629…`) | `kat/kex-07/NEV_AKE_512_1409.log` |
| `NEV_AKE_512_1409` | timing derive_a | `records/kex-07/NEV_AKE_512_1409__derive_a.json` |
| `NEV_AKE_512_1409` | timing derive_b | `records/kex-07/NEV_AKE_512_1409__derive_b.json` |
| `NEV_AKE_512_1409` | timing exchange | `records/kex-07/NEV_AKE_512_1409__exchange.json` |
| `NEV_AKE_512_1409` | timing init_a | `records/kex-07/NEV_AKE_512_1409__init_a.json` |
| `NEV_AKE_512_1409` | timing init_b | `records/kex-07/NEV_AKE_512_1409__init_b.json` |
| `NEV_AKE_512_1409` | timing pass1 | `records/kex-07/NEV_AKE_512_1409__pass1.json` |
| `NEV_AKE_512_1409` | timing pass2 | `records/kex-07/NEV_AKE_512_1409__pass2.json` |
| `NEV_AKE_512_1409` | hash profile exchange | `profile/kex-07/NEV_AKE_512_1409__exchange.json` |
| `NEV_AKE_1024_769` | KAT log (sha256 `6c9bbcc8d674cd96…`) | `kat/kex-07/NEV_AKE_1024_769.log` |
| `NEV_AKE_1024_769` | timing derive_a | `records/kex-07/NEV_AKE_1024_769__derive_a.json` |
| `NEV_AKE_1024_769` | timing derive_b | `records/kex-07/NEV_AKE_1024_769__derive_b.json` |
| `NEV_AKE_1024_769` | timing exchange | `records/kex-07/NEV_AKE_1024_769__exchange.json` |
| `NEV_AKE_1024_769` | timing init_a | `records/kex-07/NEV_AKE_1024_769__init_a.json` |
| `NEV_AKE_1024_769` | timing init_b | `records/kex-07/NEV_AKE_1024_769__init_b.json` |
| `NEV_AKE_1024_769` | timing pass1 | `records/kex-07/NEV_AKE_1024_769__pass1.json` |
| `NEV_AKE_1024_769` | timing pass2 | `records/kex-07/NEV_AKE_1024_769__pass2.json` |
| `NEV_AKE_1024_769` | hash profile exchange | `profile/kex-07/NEV_AKE_1024_769__exchange.json` |
| `NEV_AKE_1024_769_C` | KAT log (sha256 `55f96cd88a1f9934…`) | `kat/kex-07/NEV_AKE_1024_769_C.log` |
| `NEV_AKE_1024_769_C` | timing derive_a | `records/kex-07/NEV_AKE_1024_769_C__derive_a.json` |
| `NEV_AKE_1024_769_C` | timing derive_b | `records/kex-07/NEV_AKE_1024_769_C__derive_b.json` |
| `NEV_AKE_1024_769_C` | timing exchange | `records/kex-07/NEV_AKE_1024_769_C__exchange.json` |
| `NEV_AKE_1024_769_C` | timing init_a | `records/kex-07/NEV_AKE_1024_769_C__init_a.json` |
| `NEV_AKE_1024_769_C` | timing init_b | `records/kex-07/NEV_AKE_1024_769_C__init_b.json` |
| `NEV_AKE_1024_769_C` | timing pass1 | `records/kex-07/NEV_AKE_1024_769_C__pass1.json` |
| `NEV_AKE_1024_769_C` | timing pass2 | `records/kex-07/NEV_AKE_1024_769_C__pass2.json` |
| `NEV_AKE_1024_769_C` | hash profile exchange | `profile/kex-07/NEV_AKE_1024_769_C__exchange.json` |
| `NEV_AKE_1024_1409` | KAT log (sha256 `bad981cfb7db65a6…`) | `kat/kex-07/NEV_AKE_1024_1409.log` |
| `NEV_AKE_1024_1409` | timing derive_a | `records/kex-07/NEV_AKE_1024_1409__derive_a.json` |
| `NEV_AKE_1024_1409` | timing derive_b | `records/kex-07/NEV_AKE_1024_1409__derive_b.json` |
| `NEV_AKE_1024_1409` | timing exchange | `records/kex-07/NEV_AKE_1024_1409__exchange.json` |
| `NEV_AKE_1024_1409` | timing init_a | `records/kex-07/NEV_AKE_1024_1409__init_a.json` |
| `NEV_AKE_1024_1409` | timing init_b | `records/kex-07/NEV_AKE_1024_1409__init_b.json` |
| `NEV_AKE_1024_1409` | timing pass1 | `records/kex-07/NEV_AKE_1024_1409__pass1.json` |
| `NEV_AKE_1024_1409` | timing pass2 | `records/kex-07/NEV_AKE_1024_1409__pass2.json` |
| `NEV_AKE_1024_1409` | hash profile exchange | `profile/kex-07/NEV_AKE_1024_1409__exchange.json` |
| `NEV_AKE_2048_769` | KAT log (sha256 `4eb3f32bf690260e…`) | `kat/kex-07/NEV_AKE_2048_769.log` |
| `NEV_AKE_2048_769` | timing derive_a | `records/kex-07/NEV_AKE_2048_769__derive_a.json` |
| `NEV_AKE_2048_769` | timing derive_b | `records/kex-07/NEV_AKE_2048_769__derive_b.json` |
| `NEV_AKE_2048_769` | timing exchange | `records/kex-07/NEV_AKE_2048_769__exchange.json` |
| `NEV_AKE_2048_769` | timing init_a | `records/kex-07/NEV_AKE_2048_769__init_a.json` |
| `NEV_AKE_2048_769` | timing init_b | `records/kex-07/NEV_AKE_2048_769__init_b.json` |
| `NEV_AKE_2048_769` | timing pass1 | `records/kex-07/NEV_AKE_2048_769__pass1.json` |
| `NEV_AKE_2048_769` | timing pass2 | `records/kex-07/NEV_AKE_2048_769__pass2.json` |
| `NEV_AKE_2048_769` | hash profile exchange | `profile/kex-07/NEV_AKE_2048_769__exchange.json` |
| `NEV_AKE_2048_769_C` | KAT log (sha256 `c69ef84f6adb240f…`) | `kat/kex-07/NEV_AKE_2048_769_C.log` |
| `NEV_AKE_2048_769_C` | timing derive_a | `records/kex-07/NEV_AKE_2048_769_C__derive_a.json` |
| `NEV_AKE_2048_769_C` | timing derive_b | `records/kex-07/NEV_AKE_2048_769_C__derive_b.json` |
| `NEV_AKE_2048_769_C` | timing exchange | `records/kex-07/NEV_AKE_2048_769_C__exchange.json` |
| `NEV_AKE_2048_769_C` | timing init_a | `records/kex-07/NEV_AKE_2048_769_C__init_a.json` |
| `NEV_AKE_2048_769_C` | timing init_b | `records/kex-07/NEV_AKE_2048_769_C__init_b.json` |
| `NEV_AKE_2048_769_C` | timing pass1 | `records/kex-07/NEV_AKE_2048_769_C__pass1.json` |
| `NEV_AKE_2048_769_C` | timing pass2 | `records/kex-07/NEV_AKE_2048_769_C__pass2.json` |
| `NEV_AKE_2048_769_C` | hash profile exchange | `profile/kex-07/NEV_AKE_2048_769_C__exchange.json` |
| `NEV_AKE_2048_1409` | KAT log (sha256 `ba806724bcbe4b85…`) | `kat/kex-07/NEV_AKE_2048_1409.log` |
| `NEV_AKE_2048_1409` | timing derive_a | `records/kex-07/NEV_AKE_2048_1409__derive_a.json` |
| `NEV_AKE_2048_1409` | timing derive_b | `records/kex-07/NEV_AKE_2048_1409__derive_b.json` |
| `NEV_AKE_2048_1409` | timing exchange | `records/kex-07/NEV_AKE_2048_1409__exchange.json` |
| `NEV_AKE_2048_1409` | timing init_a | `records/kex-07/NEV_AKE_2048_1409__init_a.json` |
| `NEV_AKE_2048_1409` | timing init_b | `records/kex-07/NEV_AKE_2048_1409__init_b.json` |
| `NEV_AKE_2048_1409` | timing pass1 | `records/kex-07/NEV_AKE_2048_1409__pass1.json` |
| `NEV_AKE_2048_1409` | timing pass2 | `records/kex-07/NEV_AKE_2048_1409__pass2.json` |
| `NEV_AKE_2048_1409` | hash profile exchange | `profile/kex-07/NEV_AKE_2048_1409__exchange.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

