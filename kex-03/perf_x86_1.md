# kex-03 CreTAKE — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kex-03` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560625517383680.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key exchange
- Algorithm: CreTAKE
- Implementation versions measured: reference
- Parameter sets: `CreTAKE-K2K-PLAC128`, `CreTAKE-K2K-PLAC256`, `CreTAKE-K2K-PLAC512`, `CreTAKE-K2K-PLAC512Star`, `CreTAKE-K2K-ZEN128`, `CreTAKE-K2K-ZEN256`, `CreTAKE-K2K-ZEN512`, `CreTAKE-K2S-PLAC128-BiT128`, `CreTAKE-K2S-PLAC256-BiT256`, `CreTAKE-K2S-PLAC512-BiT512`, `CreTAKE-K2S-ZEN128-BiT128`, `CreTAKE-K2S-ZEN256-BiT256`, `CreTAKE-K2S-ZEN512-BiT512`, `CreTAKE-S2K-BiT128-PLAC128`, `CreTAKE-S2K-BiT128-ZEN128`, `CreTAKE-S2K-BiT256-PLAC256`, `CreTAKE-S2K-BiT256-ZEN256`, `CreTAKE-S2K-BiT512-PLAC512`, `CreTAKE-S2K-BiT512-ZEN512`, `CreTAKE-S2S-BiT128-ePLAC128`, `CreTAKE-S2S-BiT128-eZEN128`, `CreTAKE-S2S-BiT256-ePLAC256`, `CreTAKE-S2S-BiT256-eZEN256`, `CreTAKE-S2S-BiT512-ePLAC512`, `CreTAKE-S2S-BiT512-eZEN512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kex-03/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `CreTAKE-K2K-PLAC128` | guide | PASS |
| `CreTAKE-K2K-PLAC256` | guide | PASS |
| `CreTAKE-K2K-PLAC512` | guide | PASS |
| `CreTAKE-K2K-PLAC512Star` | guide | PASS |
| `CreTAKE-K2K-ZEN128` | guide | PASS |
| `CreTAKE-K2K-ZEN256` | guide | PASS |
| `CreTAKE-K2K-ZEN512` | guide | PASS |
| `CreTAKE-K2S-PLAC128-BiT128` | guide | PASS |
| `CreTAKE-K2S-PLAC256-BiT256` | guide | PASS |
| `CreTAKE-K2S-PLAC512-BiT512` | guide | PASS |
| `CreTAKE-K2S-ZEN128-BiT128` | guide | PASS |
| `CreTAKE-K2S-ZEN256-BiT256` | guide | PASS |
| `CreTAKE-K2S-ZEN512-BiT512` | guide | PASS |
| `CreTAKE-S2K-BiT128-PLAC128` | guide | PASS |
| `CreTAKE-S2K-BiT128-ZEN128` | guide | PASS |
| `CreTAKE-S2K-BiT256-PLAC256` | guide | PASS |
| `CreTAKE-S2K-BiT256-ZEN256` | guide | PASS |
| `CreTAKE-S2K-BiT512-PLAC512` | guide | PASS |
| `CreTAKE-S2K-BiT512-ZEN512` | guide | PASS |
| `CreTAKE-S2S-BiT128-ePLAC128` | guide | PASS |
| `CreTAKE-S2S-BiT128-eZEN128` | guide | PASS |
| `CreTAKE-S2S-BiT256-ePLAC256` | guide | PASS |
| `CreTAKE-S2S-BiT256-eZEN256` | guide | PASS |
| `CreTAKE-S2S-BiT512-ePLAC512` | guide | PASS |
| `CreTAKE-S2S-BiT512-eZEN512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `CreTAKE-K2K-PLAC128` | exchange | 1.99 M | 951 µs | 1.05e+03 | 951 µs | 5225 (5 × 1045) |
| `CreTAKE-K2K-PLAC128` | init_a | 160.9 k | 76.8 µs | 1.3e+04 | 76.8 µs | 5225 (5 × 1045) |
| `CreTAKE-K2K-PLAC128` | init_b | 161.0 k | 79.8 µs | 1.25e+04 | 76.8 µs | 5225 (5 × 1045) |
| `CreTAKE-K2K-PLAC128` | pass1 | 384.6 k | 184 µs | 5.45e+03 | 184 µs | 5225 (5 × 1045) |
| `CreTAKE-K2K-PLAC128` | pass2 | 734.6 k | 351 µs | 2.85e+03 | 351 µs | 5225 (5 × 1045) |
| `CreTAKE-K2K-PLAC128` | derive_a | 552.2 k | 264 µs | 3.79e+03 | 264 µs | 5225 (5 × 1045) |
| `CreTAKE-K2K-PLAC128` | derive_b | 358 | 70.6 ns | 1.42e+07 | 69.3 ns | 5225 (5 × 1045) |
| `CreTAKE-K2K-PLAC256` | exchange | 4.05 M | 1.94 ms | 517 | 1.93 ms | 2580 (5 × 516) |
| `CreTAKE-K2K-PLAC256` | init_a | 287.9 k | 137 µs | 7.28e+03 | 137 µs | 2580 (5 × 516) |
| `CreTAKE-K2K-PLAC256` | init_b | 288.3 k | 140 µs | 7.15e+03 | 138 µs | 2580 (5 × 516) |
| `CreTAKE-K2K-PLAC256` | pass1 | 708.5 k | 338 µs | 2.96e+03 | 338 µs | 2580 (5 × 516) |
| `CreTAKE-K2K-PLAC256` | pass2 | 1.56 M | 771 µs | 1.3e+03 | 747 µs | 2580 (5 × 516) |
| `CreTAKE-K2K-PLAC256` | derive_a | 1.22 M | 581 µs | 1.72e+03 | 580 µs | 2580 (5 × 516) |
| `CreTAKE-K2K-PLAC256` | derive_b | 415 | 93.6 ns | 1.07e+07 | 89.8 ns | 2580 (5 × 516) |
| `CreTAKE-K2K-PLAC512` | exchange | 14.18 M | 6.77 ms | 148 | 6.77 ms | 745 (5 × 149) |
| `CreTAKE-K2K-PLAC512` | init_a | 979.2 k | 481 µs | 2.08e+03 | 468 µs | 745 (5 × 149) |
| `CreTAKE-K2K-PLAC512` | init_b | 977.5 k | 467 µs | 2.14e+03 | 467 µs | 745 (5 × 149) |
| `CreTAKE-K2K-PLAC512` | pass1 | 2.33 M | 1.12 ms | 897 | 1.12 ms | 745 (5 × 149) |
| `CreTAKE-K2K-PLAC512` | pass2 | 5.10 M | 2.43 ms | 411 | 2.43 ms | 745 (5 × 149) |
| `CreTAKE-K2K-PLAC512` | derive_a | 4.79 M | 2.29 ms | 437 | 2.29 ms | 745 (5 × 149) |
| `CreTAKE-K2K-PLAC512` | derive_b | 479 | 122 ns | 8.16e+06 | 123 ns | 745 (5 × 149) |
| `CreTAKE-K2K-PLAC512Star` | exchange | 15.43 M | 7.37 ms | 136 | 7.37 ms | 680 (5 × 136) |
| `CreTAKE-K2K-PLAC512Star` | init_a | 1.07 M | 510 µs | 1.96e+03 | 510 µs | 680 (5 × 136) |
| `CreTAKE-K2K-PLAC512Star` | init_b | 1.07 M | 510 µs | 1.96e+03 | 510 µs | 680 (5 × 136) |
| `CreTAKE-K2K-PLAC512Star` | pass1 | 2.53 M | 1.21 ms | 827 | 1.21 ms | 680 (5 × 136) |
| `CreTAKE-K2K-PLAC512Star` | pass2 | 5.55 M | 2.65 ms | 377 | 2.65 ms | 680 (5 × 136) |
| `CreTAKE-K2K-PLAC512Star` | derive_a | 5.21 M | 2.49 ms | 402 | 2.49 ms | 680 (5 × 136) |
| `CreTAKE-K2K-PLAC512Star` | derive_b | 493 | 130 ns | 7.71e+06 | 131 ns | 680 (5 × 136) |
| `CreTAKE-K2K-ZEN128` | exchange | 1.91 M | 910 µs | 1.1e+03 | 910 µs | 5175 (5 × 1035) |
| `CreTAKE-K2K-ZEN128` | init_a | 225.2 k | 107 µs | 9.3e+03 | 107 µs | 5175 (5 × 1035) |
| `CreTAKE-K2K-ZEN128` | init_b | 224.0 k | 107 µs | 9.36e+03 | 107 µs | 5175 (5 × 1035) |
| `CreTAKE-K2K-ZEN128` | pass1 | 364.5 k | 174 µs | 5.75e+03 | 174 µs | 5175 (5 × 1035) |
| `CreTAKE-K2K-ZEN128` | pass2 | 543.5 k | 260 µs | 3.85e+03 | 260 µs | 5175 (5 × 1035) |
| `CreTAKE-K2K-ZEN128` | derive_a | 553.9 k | 264 µs | 3.78e+03 | 265 µs | 5175 (5 × 1035) |
| `CreTAKE-K2K-ZEN128` | derive_b | 361 | 72.4 ns | 1.38e+07 | 70.3 ns | 5175 (5 × 1035) |
| `CreTAKE-K2K-ZEN256` | exchange | 3.55 M | 1.7 ms | 589 | 1.7 ms | 2865 (5 × 573) |
| `CreTAKE-K2K-ZEN256` | init_a | 400.3 k | 194 µs | 5.15e+03 | 191 µs | 2865 (5 × 573) |
| `CreTAKE-K2K-ZEN256` | init_b | 395.2 k | 189 µs | 5.3e+03 | 189 µs | 2865 (5 × 573) |
| `CreTAKE-K2K-ZEN256` | pass1 | 586.2 k | 280 µs | 3.57e+03 | 280 µs | 2865 (5 × 573) |
| `CreTAKE-K2K-ZEN256` | pass2 | 1.01 M | 482 µs | 2.08e+03 | 482 µs | 2865 (5 × 573) |
| `CreTAKE-K2K-ZEN256` | derive_a | 1.17 M | 558 µs | 1.79e+03 | 558 µs | 2865 (5 × 573) |
| `CreTAKE-K2K-ZEN256` | derive_b | 411 | 93.3 ns | 1.07e+07 | 91.4 ns | 2865 (5 × 573) |
| `CreTAKE-K2K-ZEN512` | exchange | 9.55 M | 4.56 ms | 219 | 4.56 ms | 1140 (5 × 228) |
| `CreTAKE-K2K-ZEN512` | init_a | 987.6 k | 478 µs | 2.09e+03 | 472 µs | 1140 (5 × 228) |
| `CreTAKE-K2K-ZEN512` | init_b | 969.6 k | 463 µs | 2.16e+03 | 463 µs | 1140 (5 × 228) |
| `CreTAKE-K2K-ZEN512` | pass1 | 1.41 M | 672 µs | 1.49e+03 | 672 µs | 1140 (5 × 228) |
| `CreTAKE-K2K-ZEN512` | pass2 | 2.93 M | 1.44 ms | 695 | 1.4 ms | 1140 (5 × 228) |
| `CreTAKE-K2K-ZEN512` | derive_a | 3.27 M | 1.56 ms | 640 | 1.56 ms | 1140 (5 × 228) |
| `CreTAKE-K2K-ZEN512` | derive_b | 486 | 126 ns | 7.96e+06 | 125 ns | 1140 (5 × 228) |
| `CreTAKE-K2S-PLAC128-BiT128` | exchange | 4.40 M | 2.1 ms | 476 | 2.1 ms | 1895 (5 × 379) |
| `CreTAKE-K2S-PLAC128-BiT128` | init_a | 160.9 k | 76.7 µs | 1.3e+04 | 76.7 µs | 1895 (5 × 379) |
| `CreTAKE-K2S-PLAC128-BiT128` | init_b | 463.2 k | 221 µs | 4.52e+03 | 221 µs | 1895 (5 × 379) |
| `CreTAKE-K2S-PLAC128-BiT128` | pass1 | 156.4 k | 74.6 µs | 1.34e+04 | 74.6 µs | 1895 (5 × 379) |
| `CreTAKE-K2S-PLAC128-BiT128` | pass2 | 2.76 M | 1.32 ms | 759 | 1.32 ms | 1895 (5 × 379) |
| `CreTAKE-K2S-PLAC128-BiT128` | derive_a | 860.4 k | 411 µs | 2.43e+03 | 411 µs | 1895 (5 × 379) |
| `CreTAKE-K2S-PLAC128-BiT128` | derive_b | 381 | 78.4 ns | 1.28e+07 | 76.2 ns | 1895 (5 × 379) |
| `CreTAKE-K2S-PLAC256-BiT256` | exchange | 8.04 M | 3.84 ms | 260 | 3.84 ms | 1110 (5 × 222) |
| `CreTAKE-K2S-PLAC256-BiT256` | init_a | 287.7 k | 139 µs | 7.2e+03 | 137 µs | 1110 (5 × 222) |
| `CreTAKE-K2S-PLAC256-BiT256` | init_b | 1.25 M | 598 µs | 1.67e+03 | 598 µs | 1110 (5 × 222) |
| `CreTAKE-K2S-PLAC256-BiT256` | pass1 | 283.4 k | 135 µs | 7.39e+03 | 135 µs | 1110 (5 × 222) |
| `CreTAKE-K2S-PLAC256-BiT256` | pass2 | 3.86 M | 1.84 ms | 543 | 1.84 ms | 1110 (5 × 222) |
| `CreTAKE-K2S-PLAC256-BiT256` | derive_a | 2.36 M | 1.13 ms | 888 | 1.13 ms | 1110 (5 × 222) |
| `CreTAKE-K2S-PLAC256-BiT256` | derive_b | 462 | 110 ns | 9.1e+06 | 105 ns | 1110 (5 × 222) |
| `CreTAKE-K2S-PLAC512-BiT512` | exchange | 26.97 M | 12.9 ms | 77.6 | 12.9 ms | 380 (5 × 76) |
| `CreTAKE-K2S-PLAC512-BiT512` | init_a | 970.6 k | 463 µs | 2.16e+03 | 463 µs | 380 (5 × 76) |
| `CreTAKE-K2S-PLAC512-BiT512` | init_b | 3.59 M | 1.72 ms | 583 | 1.72 ms | 380 (5 × 76) |
| `CreTAKE-K2S-PLAC512-BiT512` | pass1 | 965.7 k | 461 µs | 2.17e+03 | 461 µs | 380 (5 × 76) |
| `CreTAKE-K2S-PLAC512-BiT512` | pass2 | 14.47 M | 6.91 ms | 145 | 6.91 ms | 380 (5 × 76) |
| `CreTAKE-K2S-PLAC512-BiT512` | derive_a | 6.95 M | 3.32 ms | 301 | 3.32 ms | 380 (5 × 76) |
| `CreTAKE-K2S-PLAC512-BiT512` | derive_b | 780 | 191 ns | 5.24e+06 | 191 ns | 380 (5 × 76) |
| `CreTAKE-K2S-ZEN128-BiT128` | exchange | 4.26 M | 2.04 ms | 491 | 2.03 ms | 2480 (5 × 496) |
| `CreTAKE-K2S-ZEN128-BiT128` | init_a | 226.0 k | 108 µs | 9.27e+03 | 108 µs | 2480 (5 × 496) |
| `CreTAKE-K2S-ZEN128-BiT128` | init_b | 462.1 k | 221 µs | 4.53e+03 | 221 µs | 2480 (5 × 496) |
| `CreTAKE-K2S-ZEN128-BiT128` | pass1 | 207.0 k | 98.7 µs | 1.01e+04 | 98.7 µs | 2480 (5 × 496) |
| `CreTAKE-K2S-ZEN128-BiT128` | pass2 | 2.55 M | 1.22 ms | 820 | 1.22 ms | 2480 (5 × 496) |
| `CreTAKE-K2S-ZEN128-BiT128` | derive_a | 811.9 k | 388 µs | 2.58e+03 | 388 µs | 2480 (5 × 496) |
| `CreTAKE-K2S-ZEN128-BiT128` | derive_b | 383 | 79.5 ns | 1.26e+07 | 79.2 ns | 2480 (5 × 496) |
| `CreTAKE-K2S-ZEN256-BiT256` | exchange | 7.88 M | 3.77 ms | 266 | 3.76 ms | 1465 (5 × 293) |
| `CreTAKE-K2S-ZEN256-BiT256` | init_a | 396.2 k | 193 µs | 5.18e+03 | 189 µs | 1465 (5 × 293) |
| `CreTAKE-K2S-ZEN256-BiT256` | init_b | 1.25 M | 598 µs | 1.67e+03 | 598 µs | 1465 (5 × 293) |
| `CreTAKE-K2S-ZEN256-BiT256` | pass1 | 362.2 k | 173 µs | 5.78e+03 | 173 µs | 1465 (5 × 293) |
| `CreTAKE-K2S-ZEN256-BiT256` | pass2 | 3.65 M | 1.74 ms | 574 | 1.74 ms | 1465 (5 × 293) |
| `CreTAKE-K2S-ZEN256-BiT256` | derive_a | 2.22 M | 1.06 ms | 942 | 1.06 ms | 1465 (5 × 293) |
| `CreTAKE-K2S-ZEN256-BiT256` | derive_b | 442 | 105 ns | 9.49e+06 | 106 ns | 1465 (5 × 293) |
| `CreTAKE-K2S-ZEN512-BiT512` | exchange | 24.31 M | 11.6 ms | 86.1 | 11.6 ms | 450 (5 × 90) |
| `CreTAKE-K2S-ZEN512-BiT512` | init_a | 985.9 k | 471 µs | 2.12e+03 | 471 µs | 450 (5 × 90) |
| `CreTAKE-K2S-ZEN512-BiT512` | init_b | 3.62 M | 1.73 ms | 578 | 1.73 ms | 450 (5 × 90) |
| `CreTAKE-K2S-ZEN512-BiT512` | pass1 | 872.8 k | 417 µs | 2.4e+03 | 417 µs | 450 (5 × 90) |
| `CreTAKE-K2S-ZEN512-BiT512` | pass2 | 12.62 M | 6.03 ms | 166 | 6.03 ms | 450 (5 × 90) |
| `CreTAKE-K2S-ZEN512-BiT512` | derive_a | 6.21 M | 2.97 ms | 337 | 2.96 ms | 450 (5 × 90) |
| `CreTAKE-K2S-ZEN512-BiT512` | derive_b | 500 | 132 ns | 7.56e+06 | 134 ns | 450 (5 × 90) |
| `CreTAKE-S2K-BiT128-PLAC128` | exchange | 4.45 M | 2.16 ms | 462 | 2.13 ms | 3295 (5 × 659) |
| `CreTAKE-S2K-BiT128-PLAC128` | init_a | 462.5 k | 221 µs | 4.53e+03 | 221 µs | 3295 (5 × 659) |
| `CreTAKE-S2K-BiT128-PLAC128` | init_b | 161.2 k | 76.9 µs | 1.3e+04 | 76.9 µs | 3295 (5 × 659) |
| `CreTAKE-S2K-BiT128-PLAC128` | pass1 | 2.56 M | 1.22 ms | 818 | 1.22 ms | 3295 (5 × 659) |
| `CreTAKE-S2K-BiT128-PLAC128` | pass2 | 985.4 k | 476 µs | 2.1e+03 | 471 µs | 3295 (5 × 659) |
| `CreTAKE-S2K-BiT128-PLAC128` | derive_a | 282.5 k | 140 µs | 7.17e+03 | 135 µs | 3295 (5 × 659) |
| `CreTAKE-S2K-BiT128-PLAC128` | derive_b | 377 | 76.3 ns | 1.31e+07 | 75.5 ns | 3295 (5 × 659) |
| `CreTAKE-S2K-BiT128-ZEN128` | exchange | 4.49 M | 2.15 ms | 466 | 2.15 ms | 2160 (5 × 432) |
| `CreTAKE-S2K-BiT128-ZEN128` | init_a | 461.9 k | 225 µs | 4.45e+03 | 221 µs | 2160 (5 × 432) |
| `CreTAKE-S2K-BiT128-ZEN128` | init_b | 225.8 k | 108 µs | 9.28e+03 | 108 µs | 2160 (5 × 432) |
| `CreTAKE-S2K-BiT128-ZEN128` | pass1 | 2.59 M | 1.24 ms | 807 | 1.24 ms | 2160 (5 × 432) |
| `CreTAKE-S2K-BiT128-ZEN128` | pass2 | 861.4 k | 425 µs | 2.35e+03 | 411 µs | 2160 (5 × 432) |
| `CreTAKE-S2K-BiT128-ZEN128` | derive_a | 349.2 k | 167 µs | 6e+03 | 167 µs | 2160 (5 × 432) |
| `CreTAKE-S2K-BiT128-ZEN128` | derive_b | 376 | 75.6 ns | 1.32e+07 | 75.1 ns | 2160 (5 × 432) |
| `CreTAKE-S2K-BiT256-PLAC256` | exchange | 8.34 M | 3.99 ms | 251 | 3.99 ms | 1355 (5 × 271) |
| `CreTAKE-S2K-BiT256-PLAC256` | init_a | 1.24 M | 594 µs | 1.68e+03 | 594 µs | 1355 (5 × 271) |
| `CreTAKE-S2K-BiT256-PLAC256` | init_b | 286.5 k | 137 µs | 7.31e+03 | 137 µs | 1355 (5 × 271) |
| `CreTAKE-S2K-BiT256-PLAC256` | pass1 | 3.59 M | 1.71 ms | 584 | 1.71 ms | 1355 (5 × 271) |
| `CreTAKE-S2K-BiT256-PLAC256` | pass2 | 2.54 M | 1.24 ms | 804 | 1.22 ms | 1355 (5 × 271) |
| `CreTAKE-S2K-BiT256-PLAC256` | derive_a | 689.4 k | 332 µs | 3.01e+03 | 329 µs | 1355 (5 × 271) |
| `CreTAKE-S2K-BiT256-PLAC256` | derive_b | 435 | 104 ns | 9.58e+06 | 101 ns | 1355 (5 × 271) |
| `CreTAKE-S2K-BiT256-ZEN256` | exchange | 8.13 M | 3.88 ms | 258 | 3.88 ms | 1150 (5 × 230) |
| `CreTAKE-S2K-BiT256-ZEN256` | init_a | 1.25 M | 598 µs | 1.67e+03 | 598 µs | 1150 (5 × 230) |
| `CreTAKE-S2K-BiT256-ZEN256` | init_b | 391.9 k | 187 µs | 5.35e+03 | 187 µs | 1150 (5 × 230) |
| `CreTAKE-S2K-BiT256-ZEN256` | pass1 | 3.48 M | 1.66 ms | 602 | 1.66 ms | 1150 (5 × 230) |
| `CreTAKE-S2K-BiT256-ZEN256` | pass2 | 2.20 M | 1.05 ms | 954 | 1.05 ms | 1150 (5 × 230) |
| `CreTAKE-S2K-BiT256-ZEN256` | derive_a | 815.2 k | 389 µs | 2.57e+03 | 389 µs | 1150 (5 × 230) |
| `CreTAKE-S2K-BiT256-ZEN256` | derive_b | 443 | 104 ns | 9.61e+06 | 104 ns | 1150 (5 × 230) |
| `CreTAKE-S2K-BiT512-PLAC512` | exchange | 29.73 M | 14.2 ms | 70.4 | 14.2 ms | 430 (5 × 86) |
| `CreTAKE-S2K-BiT512-PLAC512` | init_a | 3.59 M | 1.74 ms | 573 | 1.72 ms | 430 (5 × 86) |
| `CreTAKE-S2K-BiT512-PLAC512` | init_b | 983.9 k | 470 µs | 2.13e+03 | 470 µs | 430 (5 × 86) |
| `CreTAKE-S2K-BiT512-PLAC512` | pass1 | 14.91 M | 7.12 ms | 140 | 7.12 ms | 430 (5 × 86) |
| `CreTAKE-S2K-BiT512-PLAC512` | pass2 | 7.79 M | 3.8 ms | 263 | 3.72 ms | 430 (5 × 86) |
| `CreTAKE-S2K-BiT512-PLAC512` | derive_a | 2.45 M | 1.17 ms | 854 | 1.17 ms | 430 (5 × 86) |
| `CreTAKE-S2K-BiT512-PLAC512` | derive_b | 493 | 128 ns | 7.84e+06 | 127 ns | 430 (5 × 86) |
| `CreTAKE-S2K-BiT512-ZEN512` | exchange | 25.76 M | 12.3 ms | 81.2 | 12.3 ms | 485 (5 × 97) |
| `CreTAKE-S2K-BiT512-ZEN512` | init_a | 3.59 M | 1.72 ms | 583 | 1.72 ms | 485 (5 × 97) |
| `CreTAKE-S2K-BiT512-ZEN512` | init_b | 972.9 k | 465 µs | 2.15e+03 | 465 µs | 485 (5 × 97) |
| `CreTAKE-S2K-BiT512-ZEN512` | pass1 | 12.62 M | 6.18 ms | 162 | 6.03 ms | 485 (5 × 97) |
| `CreTAKE-S2K-BiT512-ZEN512` | pass2 | 6.16 M | 2.94 ms | 340 | 2.94 ms | 485 (5 × 97) |
| `CreTAKE-S2K-BiT512-ZEN512` | derive_a | 2.42 M | 1.16 ms | 864 | 1.16 ms | 485 (5 × 97) |
| `CreTAKE-S2K-BiT512-ZEN512` | derive_b | 502 | 131 ns | 7.63e+06 | 129 ns | 485 (5 × 97) |
| `CreTAKE-S2S-BiT128-ePLAC128` | exchange | 6.97 M | 3.33 ms | 300 | 3.33 ms | 1675 (5 × 335) |
| `CreTAKE-S2S-BiT128-ePLAC128` | init_a | 463.8 k | 221 µs | 4.52e+03 | 221 µs | 1675 (5 × 335) |
| `CreTAKE-S2S-BiT128-ePLAC128` | init_b | 461.7 k | 221 µs | 4.53e+03 | 220 µs | 1675 (5 × 335) |
| `CreTAKE-S2S-BiT128-ePLAC128` | pass1 | 2.40 M | 1.15 ms | 871 | 1.15 ms | 1675 (5 × 335) |
| `CreTAKE-S2S-BiT128-ePLAC128` | pass2 | 2.88 M | 1.38 ms | 726 | 1.38 ms | 1675 (5 × 335) |
| `CreTAKE-S2S-BiT128-ePLAC128` | derive_a | 760.0 k | 363 µs | 2.75e+03 | 363 µs | 1675 (5 × 335) |
| `CreTAKE-S2S-BiT128-ePLAC128` | derive_b | 382 | 77.8 ns | 1.29e+07 | 78.9 ns | 1675 (5 × 335) |
| `CreTAKE-S2S-BiT128-eZEN128` | exchange | 7.02 M | 3.36 ms | 298 | 3.36 ms | 1610 (5 × 322) |
| `CreTAKE-S2S-BiT128-eZEN128` | init_a | 462.3 k | 221 µs | 4.53e+03 | 221 µs | 1610 (5 × 322) |
| `CreTAKE-S2S-BiT128-eZEN128` | init_b | 460.9 k | 220 µs | 4.54e+03 | 220 µs | 1610 (5 × 322) |
| `CreTAKE-S2S-BiT128-eZEN128` | pass1 | 2.57 M | 1.23 ms | 813 | 1.23 ms | 1610 (5 × 322) |
| `CreTAKE-S2S-BiT128-eZEN128` | pass2 | 2.70 M | 1.29 ms | 774 | 1.29 ms | 1610 (5 × 322) |
| `CreTAKE-S2S-BiT128-eZEN128` | derive_a | 825.5 k | 403 µs | 2.48e+03 | 394 µs | 1610 (5 × 322) |
| `CreTAKE-S2S-BiT128-eZEN128` | derive_b | 385 | 79.9 ns | 1.25e+07 | 80.1 ns | 1610 (5 × 322) |
| `CreTAKE-S2S-BiT256-ePLAC256` | exchange | 12.50 M | 5.97 ms | 167 | 5.97 ms | 795 (5 × 159) |
| `CreTAKE-S2S-BiT256-ePLAC256` | init_a | 1.24 M | 593 µs | 1.68e+03 | 593 µs | 795 (5 × 159) |
| `CreTAKE-S2S-BiT256-ePLAC256` | init_b | 1.24 M | 593 µs | 1.69e+03 | 593 µs | 795 (5 × 159) |
| `CreTAKE-S2S-BiT256-ePLAC256` | pass1 | 3.08 M | 1.54 ms | 649 | 1.47 ms | 795 (5 × 159) |
| `CreTAKE-S2S-BiT256-ePLAC256` | pass2 | 4.84 M | 2.31 ms | 433 | 2.31 ms | 795 (5 × 159) |
| `CreTAKE-S2S-BiT256-ePLAC256` | derive_a | 2.10 M | 1 ms | 998 | 1 ms | 795 (5 × 159) |
| `CreTAKE-S2S-BiT256-ePLAC256` | derive_b | 460 | 105 ns | 9.5e+06 | 105 ns | 795 (5 × 159) |
| `CreTAKE-S2S-BiT256-eZEN256` | exchange | 12.64 M | 6.04 ms | 166 | 6.04 ms | 775 (5 × 155) |
| `CreTAKE-S2S-BiT256-eZEN256` | init_a | 1.25 M | 597 µs | 1.67e+03 | 597 µs | 775 (5 × 155) |
| `CreTAKE-S2S-BiT256-eZEN256` | init_b | 1.25 M | 596 µs | 1.68e+03 | 596 µs | 775 (5 × 155) |
| `CreTAKE-S2S-BiT256-eZEN256` | pass1 | 3.22 M | 1.54 ms | 649 | 1.54 ms | 775 (5 × 155) |
| `CreTAKE-S2S-BiT256-eZEN256` | pass2 | 4.69 M | 2.24 ms | 447 | 2.24 ms | 775 (5 × 155) |
| `CreTAKE-S2S-BiT256-eZEN256` | derive_a | 2.23 M | 1.07 ms | 939 | 1.07 ms | 775 (5 × 155) |
| `CreTAKE-S2S-BiT256-eZEN256` | derive_b | 445 | 106 ns | 9.46e+06 | 106 ns | 775 (5 × 155) |
| `CreTAKE-S2S-BiT512-ePLAC512` | exchange | 41.88 M | 20 ms | 50 | 20 ms | 245 (5 × 49) |
| `CreTAKE-S2S-BiT512-ePLAC512` | init_a | 3.59 M | 1.72 ms | 582 | 1.72 ms | 245 (5 × 49) |
| `CreTAKE-S2S-BiT512-ePLAC512` | init_b | 3.59 M | 1.72 ms | 583 | 1.72 ms | 245 (5 × 49) |
| `CreTAKE-S2S-BiT512-ePLAC512` | pass1 | 12.08 M | 5.77 ms | 173 | 5.77 ms | 245 (5 × 49) |
| `CreTAKE-S2S-BiT512-ePLAC512` | pass2 | 16.34 M | 7.8 ms | 128 | 7.8 ms | 245 (5 × 49) |
| `CreTAKE-S2S-BiT512-ePLAC512` | derive_a | 6.27 M | 2.99 ms | 334 | 2.99 ms | 245 (5 × 49) |
| `CreTAKE-S2S-BiT512-ePLAC512` | derive_b | 743 | 182 ns | 5.5e+06 | 183 ns | 245 (5 × 49) |
| `CreTAKE-S2S-BiT512-eZEN512` | exchange | 43.97 M | 21 ms | 47.6 | 21 ms | 230 (5 × 46) |
| `CreTAKE-S2S-BiT512-eZEN512` | init_a | 3.60 M | 1.72 ms | 582 | 1.72 ms | 230 (5 × 46) |
| `CreTAKE-S2S-BiT512-eZEN512` | init_b | 3.60 M | 1.72 ms | 582 | 1.72 ms | 230 (5 × 46) |
| `CreTAKE-S2S-BiT512-eZEN512` | pass1 | 12.07 M | 5.76 ms | 174 | 5.76 ms | 230 (5 × 46) |
| `CreTAKE-S2S-BiT512-eZEN512` | pass2 | 18.48 M | 8.82 ms | 113 | 8.82 ms | 230 (5 × 46) |
| `CreTAKE-S2S-BiT512-eZEN512` | derive_a | 6.24 M | 2.98 ms | 336 | 2.98 ms | 230 (5 × 46) |
| `CreTAKE-S2S-BiT512-eZEN512` | derive_b | 740 | 183 ns | 5.48e+06 | 186 ns | 230 (5 × 46) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `CreTAKE-K2K-PLAC128` | exchange | 65877 | 1700 KiB | 1832 KiB |
| `CreTAKE-K2K-PLAC128` | init_a | 65877 | 1784 KiB | 1852 KiB |
| `CreTAKE-K2K-PLAC128` | init_b | 65877 | 1800 KiB | 1892 KiB |
| `CreTAKE-K2K-PLAC128` | pass1 | 65877 | 1780 KiB | 1848 KiB |
| `CreTAKE-K2K-PLAC128` | pass2 | 65877 | 1792 KiB | 1860 KiB |
| `CreTAKE-K2K-PLAC128` | derive_a | 65877 | 1768 KiB | 1872 KiB |
| `CreTAKE-K2K-PLAC128` | derive_b | 65877 | 1796 KiB | 1864 KiB |
| `CreTAKE-K2K-PLAC256` | exchange | 74333 | 1864 KiB | 1932 KiB |
| `CreTAKE-K2K-PLAC256` | init_a | 74333 | 1868 KiB | 1968 KiB |
| `CreTAKE-K2K-PLAC256` | init_b | 74333 | 1832 KiB | 1964 KiB |
| `CreTAKE-K2K-PLAC256` | pass1 | 74333 | 1856 KiB | 1968 KiB |
| `CreTAKE-K2K-PLAC256` | pass2 | 74333 | 1876 KiB | 1948 KiB |
| `CreTAKE-K2K-PLAC256` | derive_a | 74333 | 1828 KiB | 1960 KiB |
| `CreTAKE-K2K-PLAC256` | derive_b | 74333 | 1816 KiB | 1948 KiB |
| `CreTAKE-K2K-PLAC512` | exchange | 91701 | 2008 KiB | 2076 KiB |
| `CreTAKE-K2K-PLAC512` | init_a | 91701 | 2040 KiB | 2108 KiB |
| `CreTAKE-K2K-PLAC512` | init_b | 91701 | 2012 KiB | 2112 KiB |
| `CreTAKE-K2K-PLAC512` | pass1 | 91701 | 2016 KiB | 2124 KiB |
| `CreTAKE-K2K-PLAC512` | pass2 | 91701 | 2024 KiB | 2124 KiB |
| `CreTAKE-K2K-PLAC512` | derive_a | 91701 | 2016 KiB | 2124 KiB |
| `CreTAKE-K2K-PLAC512` | derive_b | 91701 | 2040 KiB | 2132 KiB |
| `CreTAKE-K2K-PLAC512Star` | exchange | 97369 | 2060 KiB | 2176 KiB |
| `CreTAKE-K2K-PLAC512Star` | init_a | 97369 | 2084 KiB | 2172 KiB |
| `CreTAKE-K2K-PLAC512Star` | init_b | 97369 | 2056 KiB | 2168 KiB |
| `CreTAKE-K2K-PLAC512Star` | pass1 | 97369 | 2060 KiB | 2128 KiB |
| `CreTAKE-K2K-PLAC512Star` | pass2 | 97369 | 2080 KiB | 2148 KiB |
| `CreTAKE-K2K-PLAC512Star` | derive_a | 97369 | 2068 KiB | 2180 KiB |
| `CreTAKE-K2K-PLAC512Star` | derive_b | 97369 | 2060 KiB | 2128 KiB |
| `CreTAKE-K2K-ZEN128` | exchange | 42261 | 1756 KiB | 1860 KiB |
| `CreTAKE-K2K-ZEN128` | init_a | 42261 | 1772 KiB | 1840 KiB |
| `CreTAKE-K2K-ZEN128` | init_b | 42261 | 1764 KiB | 1844 KiB |
| `CreTAKE-K2K-ZEN128` | pass1 | 42261 | 1760 KiB | 1868 KiB |
| `CreTAKE-K2K-ZEN128` | pass2 | 42261 | 1760 KiB | 1860 KiB |
| `CreTAKE-K2K-ZEN128` | derive_a | 42261 | 1780 KiB | 1868 KiB |
| `CreTAKE-K2K-ZEN128` | derive_b | 42261 | 1772 KiB | 1840 KiB |
| `CreTAKE-K2K-ZEN256` | exchange | 48821 | 1732 KiB | 1868 KiB |
| `CreTAKE-K2K-ZEN256` | init_a | 48821 | 1816 KiB | 1920 KiB |
| `CreTAKE-K2K-ZEN256` | init_b | 48821 | 1808 KiB | 1880 KiB |
| `CreTAKE-K2K-ZEN256` | pass1 | 48821 | 1808 KiB | 1912 KiB |
| `CreTAKE-K2K-ZEN256` | pass2 | 48821 | 1816 KiB | 1916 KiB |
| `CreTAKE-K2K-ZEN256` | derive_a | 48821 | 1808 KiB | 1920 KiB |
| `CreTAKE-K2K-ZEN256` | derive_b | 48821 | 1824 KiB | 1920 KiB |
| `CreTAKE-K2K-ZEN512` | exchange | 59549 | 1932 KiB | 2000 KiB |
| `CreTAKE-K2K-ZEN512` | init_a | 59549 | 1956 KiB | 2024 KiB |
| `CreTAKE-K2K-ZEN512` | init_b | 59549 | 1952 KiB | 2020 KiB |
| `CreTAKE-K2K-ZEN512` | pass1 | 59549 | 1904 KiB | 2036 KiB |
| `CreTAKE-K2K-ZEN512` | pass2 | 59549 | 1948 KiB | 2028 KiB |
| `CreTAKE-K2K-ZEN512` | derive_a | 59549 | 1940 KiB | 2008 KiB |
| `CreTAKE-K2K-ZEN512` | derive_b | 59549 | 1940 KiB | 2044 KiB |
| `CreTAKE-K2S-PLAC128-BiT128` | exchange | 102373 | 1860 KiB | 1928 KiB |
| `CreTAKE-K2S-PLAC128-BiT128` | init_a | 102373 | 1868 KiB | 1936 KiB |
| `CreTAKE-K2S-PLAC128-BiT128` | init_b | 102373 | 1864 KiB | 1972 KiB |
| `CreTAKE-K2S-PLAC128-BiT128` | pass1 | 102373 | 1864 KiB | 1968 KiB |
| `CreTAKE-K2S-PLAC128-BiT128` | pass2 | 102373 | 1888 KiB | 1956 KiB |
| `CreTAKE-K2S-PLAC128-BiT128` | derive_a | 102373 | 1860 KiB | 1968 KiB |
| `CreTAKE-K2S-PLAC128-BiT128` | derive_b | 102373 | 1872 KiB | 1956 KiB |
| `CreTAKE-K2S-PLAC256-BiT256` | exchange | 110285 | 2036 KiB | 2164 KiB |
| `CreTAKE-K2S-PLAC256-BiT256` | init_a | 110285 | 2064 KiB | 2148 KiB |
| `CreTAKE-K2S-PLAC256-BiT256` | init_b | 110285 | 2056 KiB | 2168 KiB |
| `CreTAKE-K2S-PLAC256-BiT256` | pass1 | 110285 | 2076 KiB | 2144 KiB |
| `CreTAKE-K2S-PLAC256-BiT256` | pass2 | 110285 | 2056 KiB | 2164 KiB |
| `CreTAKE-K2S-PLAC256-BiT256` | derive_a | 110285 | 2076 KiB | 2168 KiB |
| `CreTAKE-K2S-PLAC256-BiT256` | derive_b | 110285 | 2052 KiB | 2168 KiB |
| `CreTAKE-K2S-PLAC512-BiT512` | exchange | 131149 | 2372 KiB | 2444 KiB |
| `CreTAKE-K2S-PLAC512-BiT512` | init_a | 131149 | 2380 KiB | 2452 KiB |
| `CreTAKE-K2S-PLAC512-BiT512` | init_b | 131149 | 2380 KiB | 2472 KiB |
| `CreTAKE-K2S-PLAC512-BiT512` | pass1 | 131149 | 2380 KiB | 2476 KiB |
| `CreTAKE-K2S-PLAC512-BiT512` | pass2 | 131149 | 2352 KiB | 2476 KiB |
| `CreTAKE-K2S-PLAC512-BiT512` | derive_a | 131149 | 2364 KiB | 2436 KiB |
| `CreTAKE-K2S-PLAC512-BiT512` | derive_b | 131149 | 2360 KiB | 2432 KiB |
| `CreTAKE-K2S-ZEN128-BiT128` | exchange | 78709 | 1856 KiB | 1928 KiB |
| `CreTAKE-K2S-ZEN128-BiT128` | init_a | 78709 | 1856 KiB | 1924 KiB |
| `CreTAKE-K2S-ZEN128-BiT128` | init_b | 78709 | 1848 KiB | 1940 KiB |
| `CreTAKE-K2S-ZEN128-BiT128` | pass1 | 78709 | 1852 KiB | 1920 KiB |
| `CreTAKE-K2S-ZEN128-BiT128` | pass2 | 78709 | 1828 KiB | 1940 KiB |
| `CreTAKE-K2S-ZEN128-BiT128` | derive_a | 78709 | 1840 KiB | 1948 KiB |
| `CreTAKE-K2S-ZEN128-BiT128` | derive_b | 78709 | 1840 KiB | 1932 KiB |
| `CreTAKE-K2S-ZEN256-BiT256` | exchange | 84677 | 2040 KiB | 2132 KiB |
| `CreTAKE-K2S-ZEN256-BiT256` | init_a | 84677 | 2020 KiB | 2136 KiB |
| `CreTAKE-K2S-ZEN256-BiT256` | init_b | 84677 | 2012 KiB | 2080 KiB |
| `CreTAKE-K2S-ZEN256-BiT256` | pass1 | 84677 | 2028 KiB | 2112 KiB |
| `CreTAKE-K2S-ZEN256-BiT256` | pass2 | 84677 | 2020 KiB | 2136 KiB |
| `CreTAKE-K2S-ZEN256-BiT256` | derive_a | 84677 | 2020 KiB | 2132 KiB |
| `CreTAKE-K2S-ZEN256-BiT256` | derive_b | 84677 | 2012 KiB | 2080 KiB |
| `CreTAKE-K2S-ZEN512-BiT512` | exchange | 102069 | 2344 KiB | 2424 KiB |
| `CreTAKE-K2S-ZEN512-BiT512` | init_a | 102069 | 2328 KiB | 2432 KiB |
| `CreTAKE-K2S-ZEN512-BiT512` | init_b | 102069 | 2328 KiB | 2396 KiB |
| `CreTAKE-K2S-ZEN512-BiT512` | pass1 | 102069 | 2312 KiB | 2380 KiB |
| `CreTAKE-K2S-ZEN512-BiT512` | pass2 | 102069 | 2332 KiB | 2420 KiB |
| `CreTAKE-K2S-ZEN512-BiT512` | derive_a | 102069 | 2328 KiB | 2436 KiB |
| `CreTAKE-K2S-ZEN512-BiT512` | derive_b | 102069 | 2328 KiB | 2396 KiB |
| `CreTAKE-S2K-BiT128-PLAC128` | exchange | 102325 | 1792 KiB | 1924 KiB |
| `CreTAKE-S2K-BiT128-PLAC128` | init_a | 102325 | 1848 KiB | 1972 KiB |
| `CreTAKE-S2K-BiT128-PLAC128` | init_b | 102325 | 1868 KiB | 1936 KiB |
| `CreTAKE-S2K-BiT128-PLAC128` | pass1 | 102325 | 1852 KiB | 1964 KiB |
| `CreTAKE-S2K-BiT128-PLAC128` | pass2 | 102325 | 1848 KiB | 1972 KiB |
| `CreTAKE-S2K-BiT128-PLAC128` | derive_a | 102325 | 1880 KiB | 1948 KiB |
| `CreTAKE-S2K-BiT128-PLAC128` | derive_b | 102325 | 1864 KiB | 1964 KiB |
| `CreTAKE-S2K-BiT128-ZEN128` | exchange | 78669 | 1840 KiB | 1904 KiB |
| `CreTAKE-S2K-BiT128-ZEN128` | init_a | 78669 | 1848 KiB | 1928 KiB |
| `CreTAKE-S2K-BiT128-ZEN128` | init_b | 78669 | 1848 KiB | 1944 KiB |
| `CreTAKE-S2K-BiT128-ZEN128` | pass1 | 78669 | 1840 KiB | 1904 KiB |
| `CreTAKE-S2K-BiT128-ZEN128` | pass2 | 78669 | 1844 KiB | 1908 KiB |
| `CreTAKE-S2K-BiT128-ZEN128` | derive_a | 78669 | 1840 KiB | 1904 KiB |
| `CreTAKE-S2K-BiT128-ZEN128` | derive_b | 78669 | 1824 KiB | 1940 KiB |
| `CreTAKE-S2K-BiT256-PLAC256` | exchange | 110293 | 2072 KiB | 2140 KiB |
| `CreTAKE-S2K-BiT256-PLAC256` | init_a | 110293 | 2072 KiB | 2140 KiB |
| `CreTAKE-S2K-BiT256-PLAC256` | init_b | 110293 | 2024 KiB | 2156 KiB |
| `CreTAKE-S2K-BiT256-PLAC256` | pass1 | 110293 | 2048 KiB | 2116 KiB |
| `CreTAKE-S2K-BiT256-PLAC256` | pass2 | 110293 | 2064 KiB | 2156 KiB |
| `CreTAKE-S2K-BiT256-PLAC256` | derive_a | 110293 | 2012 KiB | 2144 KiB |
| `CreTAKE-S2K-BiT256-PLAC256` | derive_b | 110293 | 2060 KiB | 2156 KiB |
| `CreTAKE-S2K-BiT256-ZEN256` | exchange | 84637 | 2032 KiB | 2124 KiB |
| `CreTAKE-S2K-BiT256-ZEN256` | init_a | 84637 | 2016 KiB | 2128 KiB |
| `CreTAKE-S2K-BiT256-ZEN256` | init_b | 84637 | 2016 KiB | 2084 KiB |
| `CreTAKE-S2K-BiT256-ZEN256` | pass1 | 84637 | 2040 KiB | 2124 KiB |
| `CreTAKE-S2K-BiT256-ZEN256` | pass2 | 84637 | 2020 KiB | 2132 KiB |
| `CreTAKE-S2K-BiT256-ZEN256` | derive_a | 84637 | 2032 KiB | 2100 KiB |
| `CreTAKE-S2K-BiT256-ZEN256` | derive_b | 84637 | 2040 KiB | 2108 KiB |
| `CreTAKE-S2K-BiT512-PLAC512` | exchange | 131101 | 2360 KiB | 2448 KiB |
| `CreTAKE-S2K-BiT512-PLAC512` | init_a | 131101 | 2348 KiB | 2452 KiB |
| `CreTAKE-S2K-BiT512-PLAC512` | init_b | 131101 | 2324 KiB | 2452 KiB |
| `CreTAKE-S2K-BiT512-PLAC512` | pass1 | 131101 | 2352 KiB | 2444 KiB |
| `CreTAKE-S2K-BiT512-PLAC512` | pass2 | 131101 | 2344 KiB | 2452 KiB |
| `CreTAKE-S2K-BiT512-PLAC512` | derive_a | 131101 | 2332 KiB | 2432 KiB |
| `CreTAKE-S2K-BiT512-PLAC512` | derive_b | 131101 | 2364 KiB | 2456 KiB |
| `CreTAKE-S2K-BiT512-ZEN512` | exchange | 102029 | 2336 KiB | 2428 KiB |
| `CreTAKE-S2K-BiT512-ZEN512` | init_a | 102029 | 2332 KiB | 2400 KiB |
| `CreTAKE-S2K-BiT512-ZEN512` | init_b | 102029 | 2324 KiB | 2404 KiB |
| `CreTAKE-S2K-BiT512-ZEN512` | pass1 | 102029 | 2316 KiB | 2384 KiB |
| `CreTAKE-S2K-BiT512-ZEN512` | pass2 | 102029 | 2332 KiB | 2400 KiB |
| `CreTAKE-S2K-BiT512-ZEN512` | derive_a | 102029 | 2324 KiB | 2392 KiB |
| `CreTAKE-S2K-BiT512-ZEN512` | derive_b | 102029 | 2332 KiB | 2400 KiB |
| `CreTAKE-S2S-BiT128-ePLAC128` | exchange | 102229 | 1852 KiB | 1980 KiB |
| `CreTAKE-S2S-BiT128-ePLAC128` | init_a | 102229 | 1868 KiB | 1936 KiB |
| `CreTAKE-S2S-BiT128-ePLAC128` | init_b | 102229 | 1896 KiB | 1964 KiB |
| `CreTAKE-S2S-BiT128-ePLAC128` | pass1 | 102229 | 1892 KiB | 1960 KiB |
| `CreTAKE-S2S-BiT128-ePLAC128` | pass2 | 102229 | 1868 KiB | 1976 KiB |
| `CreTAKE-S2S-BiT128-ePLAC128` | derive_a | 102229 | 1868 KiB | 1976 KiB |
| `CreTAKE-S2S-BiT128-ePLAC128` | derive_b | 102229 | 1888 KiB | 1988 KiB |
| `CreTAKE-S2S-BiT128-eZEN128` | exchange | 78645 | 1868 KiB | 1936 KiB |
| `CreTAKE-S2S-BiT128-eZEN128` | init_a | 78645 | 1856 KiB | 1924 KiB |
| `CreTAKE-S2S-BiT128-eZEN128` | init_b | 78645 | 1856 KiB | 1960 KiB |
| `CreTAKE-S2S-BiT128-eZEN128` | pass1 | 78645 | 1868 KiB | 1952 KiB |
| `CreTAKE-S2S-BiT128-eZEN128` | pass2 | 78645 | 1868 KiB | 1952 KiB |
| `CreTAKE-S2S-BiT128-eZEN128` | derive_a | 78645 | 1852 KiB | 1956 KiB |
| `CreTAKE-S2S-BiT128-eZEN128` | derive_b | 78645 | 1860 KiB | 1940 KiB |
| `CreTAKE-S2S-BiT256-ePLAC256` | exchange | 110245 | 2104 KiB | 2196 KiB |
| `CreTAKE-S2S-BiT256-ePLAC256` | init_a | 110245 | 2080 KiB | 2148 KiB |
| `CreTAKE-S2S-BiT256-ePLAC256` | init_b | 110245 | 2084 KiB | 2152 KiB |
| `CreTAKE-S2S-BiT256-ePLAC256` | pass1 | 110245 | 2080 KiB | 2188 KiB |
| `CreTAKE-S2S-BiT256-ePLAC256` | pass2 | 110245 | 2092 KiB | 2192 KiB |
| `CreTAKE-S2S-BiT256-ePLAC256` | derive_a | 110245 | 2072 KiB | 2188 KiB |
| `CreTAKE-S2S-BiT256-ePLAC256` | derive_b | 110245 | 2088 KiB | 2156 KiB |
| `CreTAKE-S2S-BiT256-eZEN256` | exchange | 84597 | 2072 KiB | 2164 KiB |
| `CreTAKE-S2S-BiT256-eZEN256` | init_a | 84597 | 2024 KiB | 2160 KiB |
| `CreTAKE-S2S-BiT256-eZEN256` | init_b | 84597 | 2076 KiB | 2168 KiB |
| `CreTAKE-S2S-BiT256-eZEN256` | pass1 | 84597 | 2060 KiB | 2132 KiB |
| `CreTAKE-S2S-BiT256-eZEN256` | pass2 | 84597 | 2048 KiB | 2164 KiB |
| `CreTAKE-S2S-BiT256-eZEN256` | derive_a | 84597 | 2052 KiB | 2124 KiB |
| `CreTAKE-S2S-BiT256-eZEN256` | derive_b | 84597 | 2068 KiB | 2164 KiB |
| `CreTAKE-S2S-BiT512-ePLAC512` | exchange | 131045 | 2412 KiB | 2516 KiB |
| `CreTAKE-S2S-BiT512-ePLAC512` | init_a | 131045 | 2404 KiB | 2472 KiB |
| `CreTAKE-S2S-BiT512-ePLAC512` | init_b | 131045 | 2412 KiB | 2512 KiB |
| `CreTAKE-S2S-BiT512-ePLAC512` | pass1 | 131045 | 2420 KiB | 2488 KiB |
| `CreTAKE-S2S-BiT512-ePLAC512` | pass2 | 131045 | 2424 KiB | 2512 KiB |
| `CreTAKE-S2S-BiT512-ePLAC512` | derive_a | 131045 | 2404 KiB | 2472 KiB |
| `CreTAKE-S2S-BiT512-ePLAC512` | derive_b | 131045 | 2396 KiB | 2464 KiB |
| `CreTAKE-S2S-BiT512-eZEN512` | exchange | 102021 | 2364 KiB | 2492 KiB |
| `CreTAKE-S2S-BiT512-eZEN512` | init_a | 102021 | 2304 KiB | 2440 KiB |
| `CreTAKE-S2S-BiT512-eZEN512` | init_b | 102021 | 2396 KiB | 2468 KiB |
| `CreTAKE-S2S-BiT512-eZEN512` | pass1 | 102021 | 2376 KiB | 2448 KiB |
| `CreTAKE-S2S-BiT512-eZEN512` | pass2 | 102021 | 2384 KiB | 2472 KiB |
| `CreTAKE-S2S-BiT512-eZEN512` | derive_a | 102021 | 2392 KiB | 2472 KiB |
| `CreTAKE-S2S-BiT512-eZEN512` | derive_b | 102021 | 2384 KiB | 2456 KiB |

## 6. Transmission and storage overhead

Bandwidth counts all specified protocol messages and each required public key once. Public keys are transmitted bytes too. Certificates and transport framing are excluded. The published raw timing records are unchanged.

| instance | passes | messages (bytes; raw API) | protocol-message bytes | public key A / B | bandwidth (bytes) | long-term sk (API cap) | shared secret |
|---|---|---|---|---|---|---|---|
| `CreTAKE-K2K-PLAC128` | 2 | 1170 / 1280 | 2450 | 530 / 530 | 3510 | 1570 | 32 |
| `CreTAKE-K2K-PLAC256` | 2 | 2340 / 2560 | 4900 | 1060 / 1060 | 7020 | 3140 | 64 |
| `CreTAKE-K2K-PLAC512` | 2 | 4676 / 4608 | 9284 | 2116 / 2116 | 13516 | 6276 | 128 |
| `CreTAKE-K2K-PLAC512Star` | 2 | 5492 / 5428 | 10920 | 2522 / 2522 | 15964 | 6682 | 128 |
| `CreTAKE-K2K-ZEN128` | 2 | 1127 / 1024 | 2151 | 615 / 615 | 3381 | 1303 | 32 |
| `CreTAKE-K2K-ZEN256` | 2 | 2253 / 2048 | 4301 | 1229 / 1229 | 6759 | 2605 | 64 |
| `CreTAKE-K2K-ZEN512` | 2 | 4506 / 4096 | 8602 | 2458 / 2458 | 13518 | 5210 | 128 |
| `CreTAKE-K2S-PLAC128-BiT128` | 2 | 530 / 2784 | 3314 | 530 / 1048 | 4892 | 1864 | 32 |
| `CreTAKE-K2S-PLAC256-BiT256` | 2 | 1060 / 6016 | 7076 | 1060 / 2144 | 10280 | 4160 | 64 |
| `CreTAKE-K2S-PLAC512-BiT512` | 2 | 2116 / 11815 | 13931 | 2116 / 5056 | 21103 | 9024 | 128 |
| `CreTAKE-K2S-ZEN128-BiT128` | 2 | 615 / 2528 | 3143 | 615 / 1048 | 4806 | 1864 | 32 |
| `CreTAKE-K2S-ZEN256-BiT256` | 2 | 1229 / 5504 | 6733 | 1229 / 2144 | 10106 | 4160 | 64 |
| `CreTAKE-K2S-ZEN512-BiT512` | 2 | 2458 / 10791 | 13249 | 2458 / 5056 | 20763 | 9024 | 128 |
| `CreTAKE-S2K-BiT128-PLAC128` | 2 | 2674 / 640 | 3314 | 1048 / 530 | 4892 | 1864 | 32 |
| `CreTAKE-S2K-BiT128-ZEN128` | 2 | 2631 / 512 | 3143 | 1048 / 615 | 4806 | 1864 | 32 |
| `CreTAKE-S2K-BiT256-PLAC256` | 2 | 5796 / 1280 | 7076 | 2144 / 1060 | 10280 | 4160 | 64 |
| `CreTAKE-S2K-BiT256-ZEN256` | 2 | 5709 / 1024 | 6733 | 2144 / 1229 | 10106 | 4160 | 64 |
| `CreTAKE-S2K-BiT512-PLAC512` | 2 | 11371 / 2560 | 13931 | 5056 / 2116 | 21103 | 9024 | 128 |
| `CreTAKE-S2K-BiT512-ZEN512` | 2 | 11201 / 2048 | 13249 | 5056 / 2458 | 20763 | 9024 | 128 |
| `CreTAKE-S2S-BiT128-ePLAC128` | 2 | 2034 / 2144 | 4178 | 1048 / 1048 | 6274 | 1864 | 32 |
| `CreTAKE-S2S-BiT128-eZEN128` | 2 | 2119 / 2016 | 4135 | 1048 / 1048 | 6231 | 1864 | 32 |
| `CreTAKE-S2S-BiT256-ePLAC256` | 2 | 4516 / 4736 | 9252 | 2144 / 2144 | 13540 | 4160 | 64 |
| `CreTAKE-S2S-BiT256-eZEN256` | 2 | 4685 / 4480 | 9165 | 2144 / 2144 | 13453 | 4160 | 64 |
| `CreTAKE-S2S-BiT512-ePLAC512` | 2 | 8811 / 9255 | 18066 | 5056 / 5056 | 28178 | 9024 | 128 |
| `CreTAKE-S2S-BiT512-eZEN512` | 2 | 9153 / 8743 | 17896 | 5056 / 5056 | 28008 | 9024 | 128 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — PolarLAC/ZEN/BiT components; Keccak only if BIT_USE_SHAKE=1

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `CreTAKE-K2K-PLAC128` | exchange | 68% | 2.2% | drng 8, pseudoXOF 92.4, pseudohash 2 |
| `CreTAKE-K2K-PLAC256` | exchange | 66% | 1.1% | drng 8, pseudoXOF 92.4, pseudohash 2 |
| `CreTAKE-K2K-PLAC512` | exchange | 74% | 0.3% | drng 7, pseudoXOF 98.8, pseudohash 4 |
| `CreTAKE-K2K-PLAC512Star` | exchange | 70% | 0.3% | drng 7, pseudoXOF 139, pseudohash 4 |
| `CreTAKE-K2K-ZEN128` | exchange | 46% | 2.3% | drng 8, pseudoXOF 26.8, pseudohash 6, sm3hash 6 |
| `CreTAKE-K2K-ZEN256` | exchange | 46% | 1.2% | drng 8, pseudoXOF 27.1, pseudohash 6, sm3hash 6 |
| `CreTAKE-K2K-ZEN512` | exchange | 57% | 0.5% | drng 8, pseudoXOF 29.1, pseudohash 10 |
| `CreTAKE-K2S-PLAC128-BiT128` | exchange | 64% | 1.1% | drng 9, pseudoXOF 145, sm3hash 10.9 |
| `CreTAKE-K2S-PLAC256-BiT256` | exchange | 71% | 0.6% | drng 9, pseudoXOF 171, pseudohash 6.79 |
| `CreTAKE-K2S-PLAC512-BiT512` | exchange | 77% | 0.2% | drng 9, pseudoXOF 249, pseudohash 7.74 |
| `CreTAKE-K2S-ZEN128-BiT128` | exchange | 59% | 1.1% | drng 9, pseudoXOF 108, pseudohash 2, sm3hash 13.6 |
| `CreTAKE-K2S-ZEN256-BiT256` | exchange | 66% | 0.6% | drng 9, pseudoXOF 138, pseudohash 9.05, sm3hash 3 |
| `CreTAKE-K2S-ZEN512-BiT512` | exchange | 74% | 0.2% | drng 9, pseudoXOF 211, pseudohash 11.5 |
| `CreTAKE-S2K-BiT128-PLAC128` | exchange | 64% | 1.1% | drng 9, pseudoXOF 152, pseudohash 2, sm3hash 10.5 |
| `CreTAKE-S2K-BiT128-ZEN128` | exchange | 58% | 1.1% | drng 9, pseudoXOF 110, pseudohash 4, sm3hash 13.6 |
| `CreTAKE-S2K-BiT256-PLAC256` | exchange | 71% | 0.6% | drng 9, pseudoXOF 182, pseudohash 8.95 |
| `CreTAKE-S2K-BiT256-ZEN256` | exchange | 64% | 0.6% | drng 9, pseudoXOF 139, pseudohash 11, sm3hash 3 |
| `CreTAKE-S2K-BiT512-PLAC512` | exchange | 76% | 0.2% | drng 9, pseudoXOF 270, pseudohash 10.5 |
| `CreTAKE-S2K-BiT512-ZEN512` | exchange | 72% | 0.2% | drng 9, pseudoXOF 219, pseudohash 13.9 |
| `CreTAKE-S2S-BiT128-ePLAC128` | exchange | 63% | 0.7% | drng 10, pseudoXOF 215, pseudohash 2, sm3hash 21.2 |
| `CreTAKE-S2S-BiT128-eZEN128` | exchange | 61% | 0.7% | drng 10, pseudoXOF 196, pseudohash 2, sm3hash 21.2 |
| `CreTAKE-S2S-BiT256-ePLAC256` | exchange | 72% | 0.4% | drng 10, pseudoXOF 271, pseudohash 15.8 |
| `CreTAKE-S2S-BiT256-eZEN256` | exchange | 69% | 0.4% | drng 10, pseudoXOF 252, pseudohash 15.9 |
| `CreTAKE-S2S-BiT512-ePLAC512` | exchange | 78% | 0.1% | drng 10, pseudoXOF 420, pseudohash 17 |
| `CreTAKE-S2S-BiT512-eZEN512` | exchange | 75% | 0.1% | drng 10, pseudoXOF 424, pseudohash 18.6 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `CreTAKE-K2K-PLAC128` | KAT log (sha256 `9871fb036dc10fb6…`) | `kat/kex-03/CreTAKE-K2K-PLAC128.log` |
| `CreTAKE-K2K-PLAC128` | timing derive_a | `records/kex-03/CreTAKE-K2K-PLAC128__derive_a.json` |
| `CreTAKE-K2K-PLAC128` | timing derive_b | `records/kex-03/CreTAKE-K2K-PLAC128__derive_b.json` |
| `CreTAKE-K2K-PLAC128` | timing exchange | `records/kex-03/CreTAKE-K2K-PLAC128__exchange.json` |
| `CreTAKE-K2K-PLAC128` | timing init_a | `records/kex-03/CreTAKE-K2K-PLAC128__init_a.json` |
| `CreTAKE-K2K-PLAC128` | timing init_b | `records/kex-03/CreTAKE-K2K-PLAC128__init_b.json` |
| `CreTAKE-K2K-PLAC128` | timing pass1 | `records/kex-03/CreTAKE-K2K-PLAC128__pass1.json` |
| `CreTAKE-K2K-PLAC128` | timing pass2 | `records/kex-03/CreTAKE-K2K-PLAC128__pass2.json` |
| `CreTAKE-K2K-PLAC128` | hash profile exchange | `profile/kex-03/CreTAKE-K2K-PLAC128__exchange.json` |
| `CreTAKE-K2K-PLAC256` | KAT log (sha256 `f818c6377bd2361d…`) | `kat/kex-03/CreTAKE-K2K-PLAC256.log` |
| `CreTAKE-K2K-PLAC256` | timing derive_a | `records/kex-03/CreTAKE-K2K-PLAC256__derive_a.json` |
| `CreTAKE-K2K-PLAC256` | timing derive_b | `records/kex-03/CreTAKE-K2K-PLAC256__derive_b.json` |
| `CreTAKE-K2K-PLAC256` | timing exchange | `records/kex-03/CreTAKE-K2K-PLAC256__exchange.json` |
| `CreTAKE-K2K-PLAC256` | timing init_a | `records/kex-03/CreTAKE-K2K-PLAC256__init_a.json` |
| `CreTAKE-K2K-PLAC256` | timing init_b | `records/kex-03/CreTAKE-K2K-PLAC256__init_b.json` |
| `CreTAKE-K2K-PLAC256` | timing pass1 | `records/kex-03/CreTAKE-K2K-PLAC256__pass1.json` |
| `CreTAKE-K2K-PLAC256` | timing pass2 | `records/kex-03/CreTAKE-K2K-PLAC256__pass2.json` |
| `CreTAKE-K2K-PLAC256` | hash profile exchange | `profile/kex-03/CreTAKE-K2K-PLAC256__exchange.json` |
| `CreTAKE-K2K-PLAC512` | KAT log (sha256 `2a8113354b6107c8…`) | `kat/kex-03/CreTAKE-K2K-PLAC512.log` |
| `CreTAKE-K2K-PLAC512` | timing derive_a | `records/kex-03/CreTAKE-K2K-PLAC512__derive_a.json` |
| `CreTAKE-K2K-PLAC512` | timing derive_b | `records/kex-03/CreTAKE-K2K-PLAC512__derive_b.json` |
| `CreTAKE-K2K-PLAC512` | timing exchange | `records/kex-03/CreTAKE-K2K-PLAC512__exchange.json` |
| `CreTAKE-K2K-PLAC512` | timing init_a | `records/kex-03/CreTAKE-K2K-PLAC512__init_a.json` |
| `CreTAKE-K2K-PLAC512` | timing init_b | `records/kex-03/CreTAKE-K2K-PLAC512__init_b.json` |
| `CreTAKE-K2K-PLAC512` | timing pass1 | `records/kex-03/CreTAKE-K2K-PLAC512__pass1.json` |
| `CreTAKE-K2K-PLAC512` | timing pass2 | `records/kex-03/CreTAKE-K2K-PLAC512__pass2.json` |
| `CreTAKE-K2K-PLAC512` | hash profile exchange | `profile/kex-03/CreTAKE-K2K-PLAC512__exchange.json` |
| `CreTAKE-K2K-PLAC512Star` | KAT log (sha256 `b554ccb0f580e59a…`) | `kat/kex-03/CreTAKE-K2K-PLAC512Star.log` |
| `CreTAKE-K2K-PLAC512Star` | timing derive_a | `records/kex-03/CreTAKE-K2K-PLAC512Star__derive_a.json` |
| `CreTAKE-K2K-PLAC512Star` | timing derive_b | `records/kex-03/CreTAKE-K2K-PLAC512Star__derive_b.json` |
| `CreTAKE-K2K-PLAC512Star` | timing exchange | `records/kex-03/CreTAKE-K2K-PLAC512Star__exchange.json` |
| `CreTAKE-K2K-PLAC512Star` | timing init_a | `records/kex-03/CreTAKE-K2K-PLAC512Star__init_a.json` |
| `CreTAKE-K2K-PLAC512Star` | timing init_b | `records/kex-03/CreTAKE-K2K-PLAC512Star__init_b.json` |
| `CreTAKE-K2K-PLAC512Star` | timing pass1 | `records/kex-03/CreTAKE-K2K-PLAC512Star__pass1.json` |
| `CreTAKE-K2K-PLAC512Star` | timing pass2 | `records/kex-03/CreTAKE-K2K-PLAC512Star__pass2.json` |
| `CreTAKE-K2K-PLAC512Star` | hash profile exchange | `profile/kex-03/CreTAKE-K2K-PLAC512Star__exchange.json` |
| `CreTAKE-K2K-ZEN128` | KAT log (sha256 `e4080c094852c831…`) | `kat/kex-03/CreTAKE-K2K-ZEN128.log` |
| `CreTAKE-K2K-ZEN128` | timing derive_a | `records/kex-03/CreTAKE-K2K-ZEN128__derive_a.json` |
| `CreTAKE-K2K-ZEN128` | timing derive_b | `records/kex-03/CreTAKE-K2K-ZEN128__derive_b.json` |
| `CreTAKE-K2K-ZEN128` | timing exchange | `records/kex-03/CreTAKE-K2K-ZEN128__exchange.json` |
| `CreTAKE-K2K-ZEN128` | timing init_a | `records/kex-03/CreTAKE-K2K-ZEN128__init_a.json` |
| `CreTAKE-K2K-ZEN128` | timing init_b | `records/kex-03/CreTAKE-K2K-ZEN128__init_b.json` |
| `CreTAKE-K2K-ZEN128` | timing pass1 | `records/kex-03/CreTAKE-K2K-ZEN128__pass1.json` |
| `CreTAKE-K2K-ZEN128` | timing pass2 | `records/kex-03/CreTAKE-K2K-ZEN128__pass2.json` |
| `CreTAKE-K2K-ZEN128` | hash profile exchange | `profile/kex-03/CreTAKE-K2K-ZEN128__exchange.json` |
| `CreTAKE-K2K-ZEN256` | KAT log (sha256 `be1fc595949fb61b…`) | `kat/kex-03/CreTAKE-K2K-ZEN256.log` |
| `CreTAKE-K2K-ZEN256` | timing derive_a | `records/kex-03/CreTAKE-K2K-ZEN256__derive_a.json` |
| `CreTAKE-K2K-ZEN256` | timing derive_b | `records/kex-03/CreTAKE-K2K-ZEN256__derive_b.json` |
| `CreTAKE-K2K-ZEN256` | timing exchange | `records/kex-03/CreTAKE-K2K-ZEN256__exchange.json` |
| `CreTAKE-K2K-ZEN256` | timing init_a | `records/kex-03/CreTAKE-K2K-ZEN256__init_a.json` |
| `CreTAKE-K2K-ZEN256` | timing init_b | `records/kex-03/CreTAKE-K2K-ZEN256__init_b.json` |
| `CreTAKE-K2K-ZEN256` | timing pass1 | `records/kex-03/CreTAKE-K2K-ZEN256__pass1.json` |
| `CreTAKE-K2K-ZEN256` | timing pass2 | `records/kex-03/CreTAKE-K2K-ZEN256__pass2.json` |
| `CreTAKE-K2K-ZEN256` | hash profile exchange | `profile/kex-03/CreTAKE-K2K-ZEN256__exchange.json` |
| `CreTAKE-K2K-ZEN512` | KAT log (sha256 `edfb6b8a269d36bb…`) | `kat/kex-03/CreTAKE-K2K-ZEN512.log` |
| `CreTAKE-K2K-ZEN512` | timing derive_a | `records/kex-03/CreTAKE-K2K-ZEN512__derive_a.json` |
| `CreTAKE-K2K-ZEN512` | timing derive_b | `records/kex-03/CreTAKE-K2K-ZEN512__derive_b.json` |
| `CreTAKE-K2K-ZEN512` | timing exchange | `records/kex-03/CreTAKE-K2K-ZEN512__exchange.json` |
| `CreTAKE-K2K-ZEN512` | timing init_a | `records/kex-03/CreTAKE-K2K-ZEN512__init_a.json` |
| `CreTAKE-K2K-ZEN512` | timing init_b | `records/kex-03/CreTAKE-K2K-ZEN512__init_b.json` |
| `CreTAKE-K2K-ZEN512` | timing pass1 | `records/kex-03/CreTAKE-K2K-ZEN512__pass1.json` |
| `CreTAKE-K2K-ZEN512` | timing pass2 | `records/kex-03/CreTAKE-K2K-ZEN512__pass2.json` |
| `CreTAKE-K2K-ZEN512` | hash profile exchange | `profile/kex-03/CreTAKE-K2K-ZEN512__exchange.json` |
| `CreTAKE-K2S-PLAC128-BiT128` | KAT log (sha256 `056c2efb9bd9eebc…`) | `kat/kex-03/CreTAKE-K2S-PLAC128-BiT128.log` |
| `CreTAKE-K2S-PLAC128-BiT128` | timing derive_a | `records/kex-03/CreTAKE-K2S-PLAC128-BiT128__derive_a.json` |
| `CreTAKE-K2S-PLAC128-BiT128` | timing derive_b | `records/kex-03/CreTAKE-K2S-PLAC128-BiT128__derive_b.json` |
| `CreTAKE-K2S-PLAC128-BiT128` | timing exchange | `records/kex-03/CreTAKE-K2S-PLAC128-BiT128__exchange.json` |
| `CreTAKE-K2S-PLAC128-BiT128` | timing init_a | `records/kex-03/CreTAKE-K2S-PLAC128-BiT128__init_a.json` |
| `CreTAKE-K2S-PLAC128-BiT128` | timing init_b | `records/kex-03/CreTAKE-K2S-PLAC128-BiT128__init_b.json` |
| `CreTAKE-K2S-PLAC128-BiT128` | timing pass1 | `records/kex-03/CreTAKE-K2S-PLAC128-BiT128__pass1.json` |
| `CreTAKE-K2S-PLAC128-BiT128` | timing pass2 | `records/kex-03/CreTAKE-K2S-PLAC128-BiT128__pass2.json` |
| `CreTAKE-K2S-PLAC128-BiT128` | hash profile exchange | `profile/kex-03/CreTAKE-K2S-PLAC128-BiT128__exchange.json` |
| `CreTAKE-K2S-PLAC256-BiT256` | KAT log (sha256 `7beca5b6053ef8c0…`) | `kat/kex-03/CreTAKE-K2S-PLAC256-BiT256.log` |
| `CreTAKE-K2S-PLAC256-BiT256` | timing derive_a | `records/kex-03/CreTAKE-K2S-PLAC256-BiT256__derive_a.json` |
| `CreTAKE-K2S-PLAC256-BiT256` | timing derive_b | `records/kex-03/CreTAKE-K2S-PLAC256-BiT256__derive_b.json` |
| `CreTAKE-K2S-PLAC256-BiT256` | timing exchange | `records/kex-03/CreTAKE-K2S-PLAC256-BiT256__exchange.json` |
| `CreTAKE-K2S-PLAC256-BiT256` | timing init_a | `records/kex-03/CreTAKE-K2S-PLAC256-BiT256__init_a.json` |
| `CreTAKE-K2S-PLAC256-BiT256` | timing init_b | `records/kex-03/CreTAKE-K2S-PLAC256-BiT256__init_b.json` |
| `CreTAKE-K2S-PLAC256-BiT256` | timing pass1 | `records/kex-03/CreTAKE-K2S-PLAC256-BiT256__pass1.json` |
| `CreTAKE-K2S-PLAC256-BiT256` | timing pass2 | `records/kex-03/CreTAKE-K2S-PLAC256-BiT256__pass2.json` |
| `CreTAKE-K2S-PLAC256-BiT256` | hash profile exchange | `profile/kex-03/CreTAKE-K2S-PLAC256-BiT256__exchange.json` |
| `CreTAKE-K2S-PLAC512-BiT512` | KAT log (sha256 `aef32ca449f0ba1e…`) | `kat/kex-03/CreTAKE-K2S-PLAC512-BiT512.log` |
| `CreTAKE-K2S-PLAC512-BiT512` | timing derive_a | `records/kex-03/CreTAKE-K2S-PLAC512-BiT512__derive_a.json` |
| `CreTAKE-K2S-PLAC512-BiT512` | timing derive_b | `records/kex-03/CreTAKE-K2S-PLAC512-BiT512__derive_b.json` |
| `CreTAKE-K2S-PLAC512-BiT512` | timing exchange | `records/kex-03/CreTAKE-K2S-PLAC512-BiT512__exchange.json` |
| `CreTAKE-K2S-PLAC512-BiT512` | timing init_a | `records/kex-03/CreTAKE-K2S-PLAC512-BiT512__init_a.json` |
| `CreTAKE-K2S-PLAC512-BiT512` | timing init_b | `records/kex-03/CreTAKE-K2S-PLAC512-BiT512__init_b.json` |
| `CreTAKE-K2S-PLAC512-BiT512` | timing pass1 | `records/kex-03/CreTAKE-K2S-PLAC512-BiT512__pass1.json` |
| `CreTAKE-K2S-PLAC512-BiT512` | timing pass2 | `records/kex-03/CreTAKE-K2S-PLAC512-BiT512__pass2.json` |
| `CreTAKE-K2S-PLAC512-BiT512` | hash profile exchange | `profile/kex-03/CreTAKE-K2S-PLAC512-BiT512__exchange.json` |
| `CreTAKE-K2S-ZEN128-BiT128` | KAT log (sha256 `7121a4b6c5b60c15…`) | `kat/kex-03/CreTAKE-K2S-ZEN128-BiT128.log` |
| `CreTAKE-K2S-ZEN128-BiT128` | timing derive_a | `records/kex-03/CreTAKE-K2S-ZEN128-BiT128__derive_a.json` |
| `CreTAKE-K2S-ZEN128-BiT128` | timing derive_b | `records/kex-03/CreTAKE-K2S-ZEN128-BiT128__derive_b.json` |
| `CreTAKE-K2S-ZEN128-BiT128` | timing exchange | `records/kex-03/CreTAKE-K2S-ZEN128-BiT128__exchange.json` |
| `CreTAKE-K2S-ZEN128-BiT128` | timing init_a | `records/kex-03/CreTAKE-K2S-ZEN128-BiT128__init_a.json` |
| `CreTAKE-K2S-ZEN128-BiT128` | timing init_b | `records/kex-03/CreTAKE-K2S-ZEN128-BiT128__init_b.json` |
| `CreTAKE-K2S-ZEN128-BiT128` | timing pass1 | `records/kex-03/CreTAKE-K2S-ZEN128-BiT128__pass1.json` |
| `CreTAKE-K2S-ZEN128-BiT128` | timing pass2 | `records/kex-03/CreTAKE-K2S-ZEN128-BiT128__pass2.json` |
| `CreTAKE-K2S-ZEN128-BiT128` | hash profile exchange | `profile/kex-03/CreTAKE-K2S-ZEN128-BiT128__exchange.json` |
| `CreTAKE-K2S-ZEN256-BiT256` | KAT log (sha256 `9e10a43cfbcad3d1…`) | `kat/kex-03/CreTAKE-K2S-ZEN256-BiT256.log` |
| `CreTAKE-K2S-ZEN256-BiT256` | timing derive_a | `records/kex-03/CreTAKE-K2S-ZEN256-BiT256__derive_a.json` |
| `CreTAKE-K2S-ZEN256-BiT256` | timing derive_b | `records/kex-03/CreTAKE-K2S-ZEN256-BiT256__derive_b.json` |
| `CreTAKE-K2S-ZEN256-BiT256` | timing exchange | `records/kex-03/CreTAKE-K2S-ZEN256-BiT256__exchange.json` |
| `CreTAKE-K2S-ZEN256-BiT256` | timing init_a | `records/kex-03/CreTAKE-K2S-ZEN256-BiT256__init_a.json` |
| `CreTAKE-K2S-ZEN256-BiT256` | timing init_b | `records/kex-03/CreTAKE-K2S-ZEN256-BiT256__init_b.json` |
| `CreTAKE-K2S-ZEN256-BiT256` | timing pass1 | `records/kex-03/CreTAKE-K2S-ZEN256-BiT256__pass1.json` |
| `CreTAKE-K2S-ZEN256-BiT256` | timing pass2 | `records/kex-03/CreTAKE-K2S-ZEN256-BiT256__pass2.json` |
| `CreTAKE-K2S-ZEN256-BiT256` | hash profile exchange | `profile/kex-03/CreTAKE-K2S-ZEN256-BiT256__exchange.json` |
| `CreTAKE-K2S-ZEN512-BiT512` | KAT log (sha256 `5e3a24f548b6bde5…`) | `kat/kex-03/CreTAKE-K2S-ZEN512-BiT512.log` |
| `CreTAKE-K2S-ZEN512-BiT512` | timing derive_a | `records/kex-03/CreTAKE-K2S-ZEN512-BiT512__derive_a.json` |
| `CreTAKE-K2S-ZEN512-BiT512` | timing derive_b | `records/kex-03/CreTAKE-K2S-ZEN512-BiT512__derive_b.json` |
| `CreTAKE-K2S-ZEN512-BiT512` | timing exchange | `records/kex-03/CreTAKE-K2S-ZEN512-BiT512__exchange.json` |
| `CreTAKE-K2S-ZEN512-BiT512` | timing init_a | `records/kex-03/CreTAKE-K2S-ZEN512-BiT512__init_a.json` |
| `CreTAKE-K2S-ZEN512-BiT512` | timing init_b | `records/kex-03/CreTAKE-K2S-ZEN512-BiT512__init_b.json` |
| `CreTAKE-K2S-ZEN512-BiT512` | timing pass1 | `records/kex-03/CreTAKE-K2S-ZEN512-BiT512__pass1.json` |
| `CreTAKE-K2S-ZEN512-BiT512` | timing pass2 | `records/kex-03/CreTAKE-K2S-ZEN512-BiT512__pass2.json` |
| `CreTAKE-K2S-ZEN512-BiT512` | hash profile exchange | `profile/kex-03/CreTAKE-K2S-ZEN512-BiT512__exchange.json` |
| `CreTAKE-S2K-BiT128-PLAC128` | KAT log (sha256 `6bdc16d51823de9f…`) | `kat/kex-03/CreTAKE-S2K-BiT128-PLAC128.log` |
| `CreTAKE-S2K-BiT128-PLAC128` | timing derive_a | `records/kex-03/CreTAKE-S2K-BiT128-PLAC128__derive_a.json` |
| `CreTAKE-S2K-BiT128-PLAC128` | timing derive_b | `records/kex-03/CreTAKE-S2K-BiT128-PLAC128__derive_b.json` |
| `CreTAKE-S2K-BiT128-PLAC128` | timing exchange | `records/kex-03/CreTAKE-S2K-BiT128-PLAC128__exchange.json` |
| `CreTAKE-S2K-BiT128-PLAC128` | timing init_a | `records/kex-03/CreTAKE-S2K-BiT128-PLAC128__init_a.json` |
| `CreTAKE-S2K-BiT128-PLAC128` | timing init_b | `records/kex-03/CreTAKE-S2K-BiT128-PLAC128__init_b.json` |
| `CreTAKE-S2K-BiT128-PLAC128` | timing pass1 | `records/kex-03/CreTAKE-S2K-BiT128-PLAC128__pass1.json` |
| `CreTAKE-S2K-BiT128-PLAC128` | timing pass2 | `records/kex-03/CreTAKE-S2K-BiT128-PLAC128__pass2.json` |
| `CreTAKE-S2K-BiT128-PLAC128` | hash profile exchange | `profile/kex-03/CreTAKE-S2K-BiT128-PLAC128__exchange.json` |
| `CreTAKE-S2K-BiT128-ZEN128` | KAT log (sha256 `7646fd41bf238d95…`) | `kat/kex-03/CreTAKE-S2K-BiT128-ZEN128.log` |
| `CreTAKE-S2K-BiT128-ZEN128` | timing derive_a | `records/kex-03/CreTAKE-S2K-BiT128-ZEN128__derive_a.json` |
| `CreTAKE-S2K-BiT128-ZEN128` | timing derive_b | `records/kex-03/CreTAKE-S2K-BiT128-ZEN128__derive_b.json` |
| `CreTAKE-S2K-BiT128-ZEN128` | timing exchange | `records/kex-03/CreTAKE-S2K-BiT128-ZEN128__exchange.json` |
| `CreTAKE-S2K-BiT128-ZEN128` | timing init_a | `records/kex-03/CreTAKE-S2K-BiT128-ZEN128__init_a.json` |
| `CreTAKE-S2K-BiT128-ZEN128` | timing init_b | `records/kex-03/CreTAKE-S2K-BiT128-ZEN128__init_b.json` |
| `CreTAKE-S2K-BiT128-ZEN128` | timing pass1 | `records/kex-03/CreTAKE-S2K-BiT128-ZEN128__pass1.json` |
| `CreTAKE-S2K-BiT128-ZEN128` | timing pass2 | `records/kex-03/CreTAKE-S2K-BiT128-ZEN128__pass2.json` |
| `CreTAKE-S2K-BiT128-ZEN128` | hash profile exchange | `profile/kex-03/CreTAKE-S2K-BiT128-ZEN128__exchange.json` |
| `CreTAKE-S2K-BiT256-PLAC256` | KAT log (sha256 `7c6cbd50fa5da7e4…`) | `kat/kex-03/CreTAKE-S2K-BiT256-PLAC256.log` |
| `CreTAKE-S2K-BiT256-PLAC256` | timing derive_a | `records/kex-03/CreTAKE-S2K-BiT256-PLAC256__derive_a.json` |
| `CreTAKE-S2K-BiT256-PLAC256` | timing derive_b | `records/kex-03/CreTAKE-S2K-BiT256-PLAC256__derive_b.json` |
| `CreTAKE-S2K-BiT256-PLAC256` | timing exchange | `records/kex-03/CreTAKE-S2K-BiT256-PLAC256__exchange.json` |
| `CreTAKE-S2K-BiT256-PLAC256` | timing init_a | `records/kex-03/CreTAKE-S2K-BiT256-PLAC256__init_a.json` |
| `CreTAKE-S2K-BiT256-PLAC256` | timing init_b | `records/kex-03/CreTAKE-S2K-BiT256-PLAC256__init_b.json` |
| `CreTAKE-S2K-BiT256-PLAC256` | timing pass1 | `records/kex-03/CreTAKE-S2K-BiT256-PLAC256__pass1.json` |
| `CreTAKE-S2K-BiT256-PLAC256` | timing pass2 | `records/kex-03/CreTAKE-S2K-BiT256-PLAC256__pass2.json` |
| `CreTAKE-S2K-BiT256-PLAC256` | hash profile exchange | `profile/kex-03/CreTAKE-S2K-BiT256-PLAC256__exchange.json` |
| `CreTAKE-S2K-BiT256-ZEN256` | KAT log (sha256 `fa31925d0bf07291…`) | `kat/kex-03/CreTAKE-S2K-BiT256-ZEN256.log` |
| `CreTAKE-S2K-BiT256-ZEN256` | timing derive_a | `records/kex-03/CreTAKE-S2K-BiT256-ZEN256__derive_a.json` |
| `CreTAKE-S2K-BiT256-ZEN256` | timing derive_b | `records/kex-03/CreTAKE-S2K-BiT256-ZEN256__derive_b.json` |
| `CreTAKE-S2K-BiT256-ZEN256` | timing exchange | `records/kex-03/CreTAKE-S2K-BiT256-ZEN256__exchange.json` |
| `CreTAKE-S2K-BiT256-ZEN256` | timing init_a | `records/kex-03/CreTAKE-S2K-BiT256-ZEN256__init_a.json` |
| `CreTAKE-S2K-BiT256-ZEN256` | timing init_b | `records/kex-03/CreTAKE-S2K-BiT256-ZEN256__init_b.json` |
| `CreTAKE-S2K-BiT256-ZEN256` | timing pass1 | `records/kex-03/CreTAKE-S2K-BiT256-ZEN256__pass1.json` |
| `CreTAKE-S2K-BiT256-ZEN256` | timing pass2 | `records/kex-03/CreTAKE-S2K-BiT256-ZEN256__pass2.json` |
| `CreTAKE-S2K-BiT256-ZEN256` | hash profile exchange | `profile/kex-03/CreTAKE-S2K-BiT256-ZEN256__exchange.json` |
| `CreTAKE-S2K-BiT512-PLAC512` | KAT log (sha256 `1810811c6ab038e5…`) | `kat/kex-03/CreTAKE-S2K-BiT512-PLAC512.log` |
| `CreTAKE-S2K-BiT512-PLAC512` | timing derive_a | `records/kex-03/CreTAKE-S2K-BiT512-PLAC512__derive_a.json` |
| `CreTAKE-S2K-BiT512-PLAC512` | timing derive_b | `records/kex-03/CreTAKE-S2K-BiT512-PLAC512__derive_b.json` |
| `CreTAKE-S2K-BiT512-PLAC512` | timing exchange | `records/kex-03/CreTAKE-S2K-BiT512-PLAC512__exchange.json` |
| `CreTAKE-S2K-BiT512-PLAC512` | timing init_a | `records/kex-03/CreTAKE-S2K-BiT512-PLAC512__init_a.json` |
| `CreTAKE-S2K-BiT512-PLAC512` | timing init_b | `records/kex-03/CreTAKE-S2K-BiT512-PLAC512__init_b.json` |
| `CreTAKE-S2K-BiT512-PLAC512` | timing pass1 | `records/kex-03/CreTAKE-S2K-BiT512-PLAC512__pass1.json` |
| `CreTAKE-S2K-BiT512-PLAC512` | timing pass2 | `records/kex-03/CreTAKE-S2K-BiT512-PLAC512__pass2.json` |
| `CreTAKE-S2K-BiT512-PLAC512` | hash profile exchange | `profile/kex-03/CreTAKE-S2K-BiT512-PLAC512__exchange.json` |
| `CreTAKE-S2K-BiT512-ZEN512` | KAT log (sha256 `3644d30d906c65e7…`) | `kat/kex-03/CreTAKE-S2K-BiT512-ZEN512.log` |
| `CreTAKE-S2K-BiT512-ZEN512` | timing derive_a | `records/kex-03/CreTAKE-S2K-BiT512-ZEN512__derive_a.json` |
| `CreTAKE-S2K-BiT512-ZEN512` | timing derive_b | `records/kex-03/CreTAKE-S2K-BiT512-ZEN512__derive_b.json` |
| `CreTAKE-S2K-BiT512-ZEN512` | timing exchange | `records/kex-03/CreTAKE-S2K-BiT512-ZEN512__exchange.json` |
| `CreTAKE-S2K-BiT512-ZEN512` | timing init_a | `records/kex-03/CreTAKE-S2K-BiT512-ZEN512__init_a.json` |
| `CreTAKE-S2K-BiT512-ZEN512` | timing init_b | `records/kex-03/CreTAKE-S2K-BiT512-ZEN512__init_b.json` |
| `CreTAKE-S2K-BiT512-ZEN512` | timing pass1 | `records/kex-03/CreTAKE-S2K-BiT512-ZEN512__pass1.json` |
| `CreTAKE-S2K-BiT512-ZEN512` | timing pass2 | `records/kex-03/CreTAKE-S2K-BiT512-ZEN512__pass2.json` |
| `CreTAKE-S2K-BiT512-ZEN512` | hash profile exchange | `profile/kex-03/CreTAKE-S2K-BiT512-ZEN512__exchange.json` |
| `CreTAKE-S2S-BiT128-ePLAC128` | KAT log (sha256 `09d011b6f81cf2d7…`) | `kat/kex-03/CreTAKE-S2S-BiT128-ePLAC128.log` |
| `CreTAKE-S2S-BiT128-ePLAC128` | timing derive_a | `records/kex-03/CreTAKE-S2S-BiT128-ePLAC128__derive_a.json` |
| `CreTAKE-S2S-BiT128-ePLAC128` | timing derive_b | `records/kex-03/CreTAKE-S2S-BiT128-ePLAC128__derive_b.json` |
| `CreTAKE-S2S-BiT128-ePLAC128` | timing exchange | `records/kex-03/CreTAKE-S2S-BiT128-ePLAC128__exchange.json` |
| `CreTAKE-S2S-BiT128-ePLAC128` | timing init_a | `records/kex-03/CreTAKE-S2S-BiT128-ePLAC128__init_a.json` |
| `CreTAKE-S2S-BiT128-ePLAC128` | timing init_b | `records/kex-03/CreTAKE-S2S-BiT128-ePLAC128__init_b.json` |
| `CreTAKE-S2S-BiT128-ePLAC128` | timing pass1 | `records/kex-03/CreTAKE-S2S-BiT128-ePLAC128__pass1.json` |
| `CreTAKE-S2S-BiT128-ePLAC128` | timing pass2 | `records/kex-03/CreTAKE-S2S-BiT128-ePLAC128__pass2.json` |
| `CreTAKE-S2S-BiT128-ePLAC128` | hash profile exchange | `profile/kex-03/CreTAKE-S2S-BiT128-ePLAC128__exchange.json` |
| `CreTAKE-S2S-BiT128-eZEN128` | KAT log (sha256 `c949a8f234f56b8f…`) | `kat/kex-03/CreTAKE-S2S-BiT128-eZEN128.log` |
| `CreTAKE-S2S-BiT128-eZEN128` | timing derive_a | `records/kex-03/CreTAKE-S2S-BiT128-eZEN128__derive_a.json` |
| `CreTAKE-S2S-BiT128-eZEN128` | timing derive_b | `records/kex-03/CreTAKE-S2S-BiT128-eZEN128__derive_b.json` |
| `CreTAKE-S2S-BiT128-eZEN128` | timing exchange | `records/kex-03/CreTAKE-S2S-BiT128-eZEN128__exchange.json` |
| `CreTAKE-S2S-BiT128-eZEN128` | timing init_a | `records/kex-03/CreTAKE-S2S-BiT128-eZEN128__init_a.json` |
| `CreTAKE-S2S-BiT128-eZEN128` | timing init_b | `records/kex-03/CreTAKE-S2S-BiT128-eZEN128__init_b.json` |
| `CreTAKE-S2S-BiT128-eZEN128` | timing pass1 | `records/kex-03/CreTAKE-S2S-BiT128-eZEN128__pass1.json` |
| `CreTAKE-S2S-BiT128-eZEN128` | timing pass2 | `records/kex-03/CreTAKE-S2S-BiT128-eZEN128__pass2.json` |
| `CreTAKE-S2S-BiT128-eZEN128` | hash profile exchange | `profile/kex-03/CreTAKE-S2S-BiT128-eZEN128__exchange.json` |
| `CreTAKE-S2S-BiT256-ePLAC256` | KAT log (sha256 `ab294d20d6792749…`) | `kat/kex-03/CreTAKE-S2S-BiT256-ePLAC256.log` |
| `CreTAKE-S2S-BiT256-ePLAC256` | timing derive_a | `records/kex-03/CreTAKE-S2S-BiT256-ePLAC256__derive_a.json` |
| `CreTAKE-S2S-BiT256-ePLAC256` | timing derive_b | `records/kex-03/CreTAKE-S2S-BiT256-ePLAC256__derive_b.json` |
| `CreTAKE-S2S-BiT256-ePLAC256` | timing exchange | `records/kex-03/CreTAKE-S2S-BiT256-ePLAC256__exchange.json` |
| `CreTAKE-S2S-BiT256-ePLAC256` | timing init_a | `records/kex-03/CreTAKE-S2S-BiT256-ePLAC256__init_a.json` |
| `CreTAKE-S2S-BiT256-ePLAC256` | timing init_b | `records/kex-03/CreTAKE-S2S-BiT256-ePLAC256__init_b.json` |
| `CreTAKE-S2S-BiT256-ePLAC256` | timing pass1 | `records/kex-03/CreTAKE-S2S-BiT256-ePLAC256__pass1.json` |
| `CreTAKE-S2S-BiT256-ePLAC256` | timing pass2 | `records/kex-03/CreTAKE-S2S-BiT256-ePLAC256__pass2.json` |
| `CreTAKE-S2S-BiT256-ePLAC256` | hash profile exchange | `profile/kex-03/CreTAKE-S2S-BiT256-ePLAC256__exchange.json` |
| `CreTAKE-S2S-BiT256-eZEN256` | KAT log (sha256 `b1bd046a119f76b0…`) | `kat/kex-03/CreTAKE-S2S-BiT256-eZEN256.log` |
| `CreTAKE-S2S-BiT256-eZEN256` | timing derive_a | `records/kex-03/CreTAKE-S2S-BiT256-eZEN256__derive_a.json` |
| `CreTAKE-S2S-BiT256-eZEN256` | timing derive_b | `records/kex-03/CreTAKE-S2S-BiT256-eZEN256__derive_b.json` |
| `CreTAKE-S2S-BiT256-eZEN256` | timing exchange | `records/kex-03/CreTAKE-S2S-BiT256-eZEN256__exchange.json` |
| `CreTAKE-S2S-BiT256-eZEN256` | timing init_a | `records/kex-03/CreTAKE-S2S-BiT256-eZEN256__init_a.json` |
| `CreTAKE-S2S-BiT256-eZEN256` | timing init_b | `records/kex-03/CreTAKE-S2S-BiT256-eZEN256__init_b.json` |
| `CreTAKE-S2S-BiT256-eZEN256` | timing pass1 | `records/kex-03/CreTAKE-S2S-BiT256-eZEN256__pass1.json` |
| `CreTAKE-S2S-BiT256-eZEN256` | timing pass2 | `records/kex-03/CreTAKE-S2S-BiT256-eZEN256__pass2.json` |
| `CreTAKE-S2S-BiT256-eZEN256` | hash profile exchange | `profile/kex-03/CreTAKE-S2S-BiT256-eZEN256__exchange.json` |
| `CreTAKE-S2S-BiT512-ePLAC512` | KAT log (sha256 `e8f94edec6030f92…`) | `kat/kex-03/CreTAKE-S2S-BiT512-ePLAC512.log` |
| `CreTAKE-S2S-BiT512-ePLAC512` | timing derive_a | `records/kex-03/CreTAKE-S2S-BiT512-ePLAC512__derive_a.json` |
| `CreTAKE-S2S-BiT512-ePLAC512` | timing derive_b | `records/kex-03/CreTAKE-S2S-BiT512-ePLAC512__derive_b.json` |
| `CreTAKE-S2S-BiT512-ePLAC512` | timing exchange | `records/kex-03/CreTAKE-S2S-BiT512-ePLAC512__exchange.json` |
| `CreTAKE-S2S-BiT512-ePLAC512` | timing init_a | `records/kex-03/CreTAKE-S2S-BiT512-ePLAC512__init_a.json` |
| `CreTAKE-S2S-BiT512-ePLAC512` | timing init_b | `records/kex-03/CreTAKE-S2S-BiT512-ePLAC512__init_b.json` |
| `CreTAKE-S2S-BiT512-ePLAC512` | timing pass1 | `records/kex-03/CreTAKE-S2S-BiT512-ePLAC512__pass1.json` |
| `CreTAKE-S2S-BiT512-ePLAC512` | timing pass2 | `records/kex-03/CreTAKE-S2S-BiT512-ePLAC512__pass2.json` |
| `CreTAKE-S2S-BiT512-ePLAC512` | hash profile exchange | `profile/kex-03/CreTAKE-S2S-BiT512-ePLAC512__exchange.json` |
| `CreTAKE-S2S-BiT512-eZEN512` | KAT log (sha256 `d798898501fa4052…`) | `kat/kex-03/CreTAKE-S2S-BiT512-eZEN512.log` |
| `CreTAKE-S2S-BiT512-eZEN512` | timing derive_a | `records/kex-03/CreTAKE-S2S-BiT512-eZEN512__derive_a.json` |
| `CreTAKE-S2S-BiT512-eZEN512` | timing derive_b | `records/kex-03/CreTAKE-S2S-BiT512-eZEN512__derive_b.json` |
| `CreTAKE-S2S-BiT512-eZEN512` | timing exchange | `records/kex-03/CreTAKE-S2S-BiT512-eZEN512__exchange.json` |
| `CreTAKE-S2S-BiT512-eZEN512` | timing init_a | `records/kex-03/CreTAKE-S2S-BiT512-eZEN512__init_a.json` |
| `CreTAKE-S2S-BiT512-eZEN512` | timing init_b | `records/kex-03/CreTAKE-S2S-BiT512-eZEN512__init_b.json` |
| `CreTAKE-S2S-BiT512-eZEN512` | timing pass1 | `records/kex-03/CreTAKE-S2S-BiT512-eZEN512__pass1.json` |
| `CreTAKE-S2S-BiT512-eZEN512` | timing pass2 | `records/kex-03/CreTAKE-S2S-BiT512-eZEN512__pass2.json` |
| `CreTAKE-S2S-BiT512-eZEN512` | hash profile exchange | `profile/kex-03/CreTAKE-S2S-BiT512-eZEN512__exchange.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

