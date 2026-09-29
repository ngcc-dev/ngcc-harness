# kex-03 CreTAKE — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kex-03` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560625517383680.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key exchange
- Algorithm: CreTAKE
- Implementation versions measured: reference
- Parameter sets: `CreTAKE-K2K-PLAC128`, `CreTAKE-K2K-PLAC256`, `CreTAKE-K2K-PLAC512`, `CreTAKE-K2K-PLAC512Star`, `CreTAKE-K2K-ZEN128`, `CreTAKE-K2K-ZEN256`, `CreTAKE-K2K-ZEN512`, `CreTAKE-K2S-PLAC128-BiT128`, `CreTAKE-K2S-PLAC256-BiT256`, `CreTAKE-K2S-PLAC512-BiT512`, `CreTAKE-K2S-ZEN128-BiT128`, `CreTAKE-K2S-ZEN256-BiT256`, `CreTAKE-K2S-ZEN512-BiT512`, `CreTAKE-S2K-BiT128-PLAC128`, `CreTAKE-S2K-BiT128-ZEN128`, `CreTAKE-S2K-BiT256-PLAC256`, `CreTAKE-S2K-BiT256-ZEN256`, `CreTAKE-S2K-BiT512-PLAC512`, `CreTAKE-S2K-BiT512-ZEN512`, `CreTAKE-S2S-BiT128-ePLAC128`, `CreTAKE-S2S-BiT128-eZEN128`, `CreTAKE-S2S-BiT256-ePLAC256`, `CreTAKE-S2S-BiT256-eZEN256`, `CreTAKE-S2S-BiT512-ePLAC512`, `CreTAKE-S2S-BiT512-eZEN512`

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
| `CreTAKE-K2K-PLAC128` | exchange | 1.69 M | 626 µs | 1.6e+03 | 626 µs | 5055 (5 × 1011) |
| `CreTAKE-K2K-PLAC128` | init_a | 135.1 k | 50.1 µs | 2e+04 | 50.1 µs | 5055 (5 × 1011) |
| `CreTAKE-K2K-PLAC128` | init_b | 136.0 k | 50.4 µs | 1.98e+04 | 50.2 µs | 5055 (5 × 1011) |
| `CreTAKE-K2K-PLAC128` | pass1 | 325.4 k | 121 µs | 8.29e+03 | 121 µs | 5055 (5 × 1011) |
| `CreTAKE-K2K-PLAC128` | pass2 | 621.9 k | 231 µs | 4.33e+03 | 230 µs | 5055 (5 × 1011) |
| `CreTAKE-K2K-PLAC128` | derive_a | 470.0 k | 174 µs | 5.74e+03 | 174 µs | 5055 (5 × 1011) |
| `CreTAKE-K2K-PLAC128` | derive_b | 217 | 32.2 ns | 3.11e+07 | 32.2 ns | 5055 (5 × 1011) |
| `CreTAKE-K2K-PLAC256` | exchange | 3.39 M | 1.26 ms | 795 | 1.26 ms | 2390 (5 × 478) |
| `CreTAKE-K2K-PLAC256` | init_a | 234.2 k | 86.9 µs | 1.15e+04 | 86.8 µs | 2390 (5 × 478) |
| `CreTAKE-K2K-PLAC256` | init_b | 235.7 k | 87.4 µs | 1.14e+04 | 86.9 µs | 2390 (5 × 478) |
| `CreTAKE-K2K-PLAC256` | pass1 | 582.3 k | 216 µs | 4.63e+03 | 216 µs | 2390 (5 × 478) |
| `CreTAKE-K2K-PLAC256` | pass2 | 1.31 M | 487 µs | 2.05e+03 | 485 µs | 2390 (5 × 478) |
| `CreTAKE-K2K-PLAC256` | derive_a | 1.03 M | 382 µs | 2.62e+03 | 381 µs | 2390 (5 × 478) |
| `CreTAKE-K2K-PLAC256` | derive_b | 282 | 49.5 ns | 2.02e+07 | 47.5 ns | 2390 (5 × 478) |
| `CreTAKE-K2K-PLAC512` | exchange | 12.12 M | 4.5 ms | 222 | 4.49 ms | 700 (5 × 140) |
| `CreTAKE-K2K-PLAC512` | init_a | 835.6 k | 310 µs | 3.22e+03 | 308 µs | 700 (5 × 140) |
| `CreTAKE-K2K-PLAC512` | init_b | 830.9 k | 308 µs | 3.24e+03 | 308 µs | 700 (5 × 140) |
| `CreTAKE-K2K-PLAC512` | pass1 | 1.98 M | 735 µs | 1.36e+03 | 734 µs | 700 (5 × 140) |
| `CreTAKE-K2K-PLAC512` | pass2 | 4.38 M | 1.63 ms | 615 | 1.62 ms | 700 (5 × 140) |
| `CreTAKE-K2K-PLAC512` | derive_a | 4.10 M | 1.52 ms | 657 | 1.52 ms | 700 (5 × 140) |
| `CreTAKE-K2K-PLAC512` | derive_b | 333 | 75.7 ns | 1.32e+07 | 77.7 ns | 700 (5 × 140) |
| `CreTAKE-K2K-PLAC512Star` | exchange | 12.13 M | 4.5 ms | 222 | 4.49 ms | 705 (5 × 141) |
| `CreTAKE-K2K-PLAC512Star` | init_a | 810.5 k | 301 µs | 3.33e+03 | 301 µs | 705 (5 × 141) |
| `CreTAKE-K2K-PLAC512Star` | init_b | 809.3 k | 300 µs | 3.33e+03 | 300 µs | 705 (5 × 141) |
| `CreTAKE-K2K-PLAC512Star` | pass1 | 1.95 M | 722 µs | 1.38e+03 | 721 µs | 705 (5 × 141) |
| `CreTAKE-K2K-PLAC512Star` | pass2 | 4.44 M | 1.65 ms | 607 | 1.65 ms | 705 (5 × 141) |
| `CreTAKE-K2K-PLAC512Star` | derive_a | 4.13 M | 1.53 ms | 653 | 1.53 ms | 705 (5 × 141) |
| `CreTAKE-K2K-PLAC512Star` | derive_b | 330 | 82.7 ns | 1.21e+07 | 76.8 ns | 705 (5 × 141) |
| `CreTAKE-K2K-ZEN128` | exchange | 1.43 M | 530 µs | 1.89e+03 | 530 µs | 6080 (5 × 1216) |
| `CreTAKE-K2K-ZEN128` | init_a | 153.5 k | 56.9 µs | 1.76e+04 | 56.8 µs | 6080 (5 × 1216) |
| `CreTAKE-K2K-ZEN128` | init_b | 153.5 k | 56.9 µs | 1.76e+04 | 56.9 µs | 6080 (5 × 1216) |
| `CreTAKE-K2K-ZEN128` | pass1 | 271.8 k | 101 µs | 9.92e+03 | 101 µs | 6080 (5 × 1216) |
| `CreTAKE-K2K-ZEN128` | pass2 | 447.5 k | 166 µs | 6.03e+03 | 166 µs | 6080 (5 × 1216) |
| `CreTAKE-K2K-ZEN128` | derive_a | 401.7 k | 149 µs | 6.71e+03 | 149 µs | 6080 (5 × 1216) |
| `CreTAKE-K2K-ZEN128` | derive_b | 215 | 33.4 ns | 2.99e+07 | 33 ns | 6080 (5 × 1216) |
| `CreTAKE-K2K-ZEN256` | exchange | 2.54 M | 943 µs | 1.06e+03 | 942 µs | 3240 (5 × 648) |
| `CreTAKE-K2K-ZEN256` | init_a | 268.2 k | 99.5 µs | 1.01e+04 | 99.5 µs | 3240 (5 × 648) |
| `CreTAKE-K2K-ZEN256` | init_b | 264.7 k | 98.2 µs | 1.02e+04 | 98.1 µs | 3240 (5 × 648) |
| `CreTAKE-K2K-ZEN256` | pass1 | 407.6 k | 151 µs | 6.61e+03 | 151 µs | 3240 (5 × 648) |
| `CreTAKE-K2K-ZEN256` | pass2 | 775.8 k | 288 µs | 3.47e+03 | 288 µs | 3240 (5 × 648) |
| `CreTAKE-K2K-ZEN256` | derive_a | 827.8 k | 307 µs | 3.26e+03 | 307 µs | 3240 (5 × 648) |
| `CreTAKE-K2K-ZEN256` | derive_b | 345 | 65.5 ns | 1.53e+07 | 68.6 ns | 3240 (5 × 648) |
| `CreTAKE-K2K-ZEN512` | exchange | 7.65 M | 2.84 ms | 352 | 2.84 ms | 1165 (5 × 233) |
| `CreTAKE-K2K-ZEN512` | init_a | 747.1 k | 277 µs | 3.61e+03 | 276 µs | 1165 (5 × 233) |
| `CreTAKE-K2K-ZEN512` | init_b | 729.9 k | 271 µs | 3.69e+03 | 271 µs | 1165 (5 × 233) |
| `CreTAKE-K2K-ZEN512` | pass1 | 1.09 M | 404 µs | 2.47e+03 | 404 µs | 1165 (5 × 233) |
| `CreTAKE-K2K-ZEN512` | pass2 | 2.48 M | 921 µs | 1.09e+03 | 921 µs | 1165 (5 × 233) |
| `CreTAKE-K2K-ZEN512` | derive_a | 2.59 M | 962 µs | 1.04e+03 | 961 µs | 1165 (5 × 233) |
| `CreTAKE-K2K-ZEN512` | derive_b | 402 | 95.6 ns | 1.05e+07 | 88.5 ns | 1165 (5 × 233) |
| `CreTAKE-K2S-PLAC128-BiT128` | exchange | 3.62 M | 1.34 ms | 746 | 1.34 ms | 1950 (5 × 390) |
| `CreTAKE-K2S-PLAC128-BiT128` | init_a | 135.5 k | 50.3 µs | 1.99e+04 | 50.1 µs | 1950 (5 × 390) |
| `CreTAKE-K2S-PLAC128-BiT128` | init_b | 409.3 k | 152 µs | 6.59e+03 | 152 µs | 1950 (5 × 390) |
| `CreTAKE-K2S-PLAC128-BiT128` | pass1 | 131.7 k | 48.8 µs | 2.05e+04 | 48.7 µs | 1950 (5 × 390) |
| `CreTAKE-K2S-PLAC128-BiT128` | pass2 | 2.19 M | 811 µs | 1.23e+03 | 809 µs | 1950 (5 × 390) |
| `CreTAKE-K2S-PLAC128-BiT128` | derive_a | 752.4 k | 279 µs | 3.58e+03 | 278 µs | 1950 (5 × 390) |
| `CreTAKE-K2S-PLAC128-BiT128` | derive_b | 211 | 33 ns | 3.03e+07 | 34.2 ns | 1950 (5 × 390) |
| `CreTAKE-K2S-PLAC256-BiT256` | exchange | 7.19 M | 2.67 ms | 375 | 2.66 ms | 1015 (5 × 203) |
| `CreTAKE-K2S-PLAC256-BiT256` | init_a | 234.8 k | 87.1 µs | 1.15e+04 | 87 µs | 1015 (5 × 203) |
| `CreTAKE-K2S-PLAC256-BiT256` | init_b | 1.12 M | 416 µs | 2.41e+03 | 416 µs | 1015 (5 × 203) |
| `CreTAKE-K2S-PLAC256-BiT256` | pass1 | 230.7 k | 85.6 µs | 1.17e+04 | 85.6 µs | 1015 (5 × 203) |
| `CreTAKE-K2S-PLAC256-BiT256` | pass2 | 3.52 M | 1.31 ms | 766 | 1.31 ms | 1015 (5 × 203) |
| `CreTAKE-K2S-PLAC256-BiT256` | derive_a | 2.07 M | 769 µs | 1.3e+03 | 769 µs | 1015 (5 × 203) |
| `CreTAKE-K2S-PLAC256-BiT256` | derive_b | 238 | 44.2 ns | 2.26e+07 | 43.1 ns | 1015 (5 × 203) |
| `CreTAKE-K2S-PLAC512-BiT512` | exchange | 25.06 M | 9.3 ms | 108 | 9.29 ms | 335 (5 × 67) |
| `CreTAKE-K2S-PLAC512-BiT512` | init_a | 824.4 k | 306 µs | 3.27e+03 | 305 µs | 335 (5 × 67) |
| `CreTAKE-K2S-PLAC512-BiT512` | init_b | 3.33 M | 1.24 ms | 809 | 1.24 ms | 335 (5 × 67) |
| `CreTAKE-K2S-PLAC512-BiT512` | pass1 | 825.0 k | 306 µs | 3.27e+03 | 303 µs | 335 (5 × 67) |
| `CreTAKE-K2S-PLAC512-BiT512` | pass2 | 13.85 M | 5.14 ms | 195 | 5.12 ms | 335 (5 × 67) |
| `CreTAKE-K2S-PLAC512-BiT512` | derive_a | 6.24 M | 2.31 ms | 432 | 2.31 ms | 335 (5 × 67) |
| `CreTAKE-K2S-PLAC512-BiT512` | derive_b | 361 | 86 ns | 1.16e+07 | 84.7 ns | 335 (5 × 67) |
| `CreTAKE-K2S-ZEN128-BiT128` | exchange | 3.40 M | 1.26 ms | 792 | 1.26 ms | 2440 (5 × 488) |
| `CreTAKE-K2S-ZEN128-BiT128` | init_a | 153.4 k | 56.9 µs | 1.76e+04 | 56.9 µs | 2440 (5 × 488) |
| `CreTAKE-K2S-ZEN128-BiT128` | init_b | 408.2 k | 151 µs | 6.6e+03 | 151 µs | 2440 (5 × 488) |
| `CreTAKE-K2S-ZEN128-BiT128` | pass1 | 136.1 k | 50.5 µs | 1.98e+04 | 50.5 µs | 2440 (5 × 488) |
| `CreTAKE-K2S-ZEN128-BiT128` | pass2 | 2.03 M | 752 µs | 1.33e+03 | 752 µs | 2440 (5 × 488) |
| `CreTAKE-K2S-ZEN128-BiT128` | derive_a | 679.8 k | 252 µs | 3.96e+03 | 251 µs | 2440 (5 × 488) |
| `CreTAKE-K2S-ZEN128-BiT128` | derive_b | 215 | 34.1 ns | 2.93e+07 | 34.1 ns | 2440 (5 × 488) |
| `CreTAKE-K2S-ZEN256-BiT256` | exchange | 6.80 M | 2.52 ms | 397 | 2.52 ms | 1380 (5 × 276) |
| `CreTAKE-K2S-ZEN256-BiT256` | init_a | 267.2 k | 99.1 µs | 1.01e+04 | 98.5 µs | 1380 (5 × 276) |
| `CreTAKE-K2S-ZEN256-BiT256` | init_b | 1.12 M | 416 µs | 2.41e+03 | 415 µs | 1380 (5 × 276) |
| `CreTAKE-K2S-ZEN256-BiT256` | pass1 | 234.6 k | 86.9 µs | 1.15e+04 | 86.5 µs | 1380 (5 × 276) |
| `CreTAKE-K2S-ZEN256-BiT256` | pass2 | 3.29 M | 1.22 ms | 818 | 1.22 ms | 1380 (5 × 276) |
| `CreTAKE-K2S-ZEN256-BiT256` | derive_a | 1.86 M | 690 µs | 1.45e+03 | 690 µs | 1380 (5 × 276) |
| `CreTAKE-K2S-ZEN256-BiT256` | derive_b | 246 | 44.8 ns | 2.23e+07 | 46.4 ns | 1380 (5 × 276) |
| `CreTAKE-K2S-ZEN512-BiT512` | exchange | 22.34 M | 8.29 ms | 121 | 8.29 ms | 400 (5 × 80) |
| `CreTAKE-K2S-ZEN512-BiT512` | init_a | 751.3 k | 279 µs | 3.59e+03 | 278 µs | 400 (5 × 80) |
| `CreTAKE-K2S-ZEN512-BiT512` | init_b | 3.33 M | 1.24 ms | 809 | 1.24 ms | 400 (5 × 80) |
| `CreTAKE-K2S-ZEN512-BiT512` | pass1 | 640.6 k | 237 µs | 4.21e+03 | 237 µs | 400 (5 × 80) |
| `CreTAKE-K2S-ZEN512-BiT512` | pass2 | 12.05 M | 4.47 ms | 224 | 4.47 ms | 400 (5 × 80) |
| `CreTAKE-K2S-ZEN512-BiT512` | derive_a | 5.48 M | 2.03 ms | 492 | 2.02 ms | 400 (5 × 80) |
| `CreTAKE-K2S-ZEN512-BiT512` | derive_b | 381 | 91 ns | 1.1e+07 | 91.2 ns | 400 (5 × 80) |
| `CreTAKE-S2K-BiT128-PLAC128` | exchange | 3.66 M | 1.36 ms | 736 | 1.36 ms | 3175 (5 × 635) |
| `CreTAKE-S2K-BiT128-PLAC128` | init_a | 408.5 k | 152 µs | 6.6e+03 | 151 µs | 3175 (5 × 635) |
| `CreTAKE-S2K-BiT128-PLAC128` | init_b | 135.4 k | 50.2 µs | 1.99e+04 | 50.1 µs | 3175 (5 × 635) |
| `CreTAKE-S2K-BiT128-PLAC128` | pass1 | 2.03 M | 752 µs | 1.33e+03 | 748 µs | 3175 (5 × 635) |
| `CreTAKE-S2K-BiT128-PLAC128` | pass2 | 859.3 k | 319 µs | 3.14e+03 | 317 µs | 3175 (5 × 635) |
| `CreTAKE-S2K-BiT128-PLAC128` | derive_a | 241.0 k | 89.4 µs | 1.12e+04 | 89.3 µs | 3175 (5 × 635) |
| `CreTAKE-S2K-BiT128-PLAC128` | derive_b | 206 | 30.6 ns | 3.26e+07 | 31.2 ns | 3175 (5 × 635) |
| `CreTAKE-S2K-BiT128-ZEN128` | exchange | 3.56 M | 1.32 ms | 758 | 1.32 ms | 2110 (5 × 422) |
| `CreTAKE-S2K-BiT128-ZEN128` | init_a | 407.9 k | 151 µs | 6.61e+03 | 151 µs | 2110 (5 × 422) |
| `CreTAKE-S2K-BiT128-ZEN128` | init_b | 154.5 k | 57.3 µs | 1.75e+04 | 57.3 µs | 2110 (5 × 422) |
| `CreTAKE-S2K-BiT128-ZEN128` | pass1 | 2.01 M | 747 µs | 1.34e+03 | 746 µs | 2110 (5 × 422) |
| `CreTAKE-S2K-BiT128-ZEN128` | pass2 | 734.7 k | 273 µs | 3.67e+03 | 273 µs | 2110 (5 × 422) |
| `CreTAKE-S2K-BiT128-ZEN128` | derive_a | 245.0 k | 90.9 µs | 1.1e+04 | 90.8 µs | 2110 (5 × 422) |
| `CreTAKE-S2K-BiT128-ZEN128` | derive_b | 345 | 42.1 ns | 2.37e+07 | 41.5 ns | 2110 (5 × 422) |
| `CreTAKE-S2K-BiT256-PLAC256` | exchange | 7.41 M | 2.75 ms | 364 | 2.75 ms | 1235 (5 × 247) |
| `CreTAKE-S2K-BiT256-PLAC256` | init_a | 1.12 M | 416 µs | 2.4e+03 | 416 µs | 1235 (5 × 247) |
| `CreTAKE-S2K-BiT256-PLAC256` | init_b | 234.3 k | 86.8 µs | 1.15e+04 | 86.8 µs | 1235 (5 × 247) |
| `CreTAKE-S2K-BiT256-PLAC256` | pass1 | 3.25 M | 1.21 ms | 829 | 1.2 ms | 1235 (5 × 247) |
| `CreTAKE-S2K-BiT256-PLAC256` | pass2 | 2.22 M | 825 µs | 1.21e+03 | 825 µs | 1235 (5 × 247) |
| `CreTAKE-S2K-BiT256-PLAC256` | derive_a | 593.2 k | 220 µs | 4.55e+03 | 220 µs | 1235 (5 × 247) |
| `CreTAKE-S2K-BiT256-PLAC256` | derive_b | 257 | 49.6 ns | 2.02e+07 | 49.8 ns | 1235 (5 × 247) |
| `CreTAKE-S2K-BiT256-ZEN256` | exchange | 6.95 M | 2.58 ms | 388 | 2.58 ms | 1075 (5 × 215) |
| `CreTAKE-S2K-BiT256-ZEN256` | init_a | 1.12 M | 417 µs | 2.4e+03 | 416 µs | 1075 (5 × 215) |
| `CreTAKE-S2K-BiT256-ZEN256` | init_b | 262.2 k | 97.3 µs | 1.03e+04 | 97.4 µs | 1075 (5 × 215) |
| `CreTAKE-S2K-BiT256-ZEN256` | pass1 | 3.09 M | 1.15 ms | 872 | 1.15 ms | 1075 (5 × 215) |
| `CreTAKE-S2K-BiT256-ZEN256` | pass2 | 1.86 M | 691 µs | 1.45e+03 | 691 µs | 1075 (5 × 215) |
| `CreTAKE-S2K-BiT256-ZEN256` | derive_a | 594.1 k | 220 µs | 4.54e+03 | 220 µs | 1075 (5 × 215) |
| `CreTAKE-S2K-BiT256-ZEN256` | derive_b | 247 | 46.5 ns | 2.15e+07 | 45.3 ns | 1075 (5 × 215) |
| `CreTAKE-S2K-BiT512-PLAC512` | exchange | 27.37 M | 10.2 ms | 98.5 | 10.2 ms | 385 (5 × 77) |
| `CreTAKE-S2K-BiT512-PLAC512` | init_a | 3.33 M | 1.24 ms | 808 | 1.24 ms | 385 (5 × 77) |
| `CreTAKE-S2K-BiT512-PLAC512` | init_b | 828.9 k | 308 µs | 3.25e+03 | 307 µs | 385 (5 × 77) |
| `CreTAKE-S2K-BiT512-PLAC512` | pass1 | 14.07 M | 5.22 ms | 192 | 5.21 ms | 385 (5 × 77) |
| `CreTAKE-S2K-BiT512-PLAC512` | pass2 | 6.95 M | 2.58 ms | 387 | 2.57 ms | 385 (5 × 77) |
| `CreTAKE-S2K-BiT512-PLAC512` | derive_a | 2.16 M | 803 µs | 1.24e+03 | 803 µs | 385 (5 × 77) |
| `CreTAKE-S2K-BiT512-PLAC512` | derive_b | 370 | 91.4 ns | 1.09e+07 | 88.6 ns | 385 (5 × 77) |
| `CreTAKE-S2K-BiT512-ZEN512` | exchange | 23.45 M | 8.7 ms | 115 | 8.7 ms | 435 (5 × 87) |
| `CreTAKE-S2K-BiT512-ZEN512` | init_a | 3.36 M | 1.25 ms | 802 | 1.24 ms | 435 (5 × 87) |
| `CreTAKE-S2K-BiT512-ZEN512` | init_b | 737.1 k | 274 µs | 3.65e+03 | 273 µs | 435 (5 × 87) |
| `CreTAKE-S2K-BiT512-ZEN512` | pass1 | 11.95 M | 4.43 ms | 226 | 4.43 ms | 435 (5 × 87) |
| `CreTAKE-S2K-BiT512-ZEN512` | pass2 | 5.47 M | 2.03 ms | 492 | 2.03 ms | 435 (5 × 87) |
| `CreTAKE-S2K-BiT512-ZEN512` | derive_a | 1.97 M | 729 µs | 1.37e+03 | 728 µs | 435 (5 × 87) |
| `CreTAKE-S2K-BiT512-ZEN512` | derive_b | 367 | 90.7 ns | 1.1e+07 | 89.2 ns | 435 (5 × 87) |
| `CreTAKE-S2S-BiT128-ePLAC128` | exchange | 5.73 M | 2.13 ms | 470 | 2.13 ms | 1640 (5 × 328) |
| `CreTAKE-S2S-BiT128-ePLAC128` | init_a | 408.2 k | 151 µs | 6.6e+03 | 151 µs | 1640 (5 × 328) |
| `CreTAKE-S2S-BiT128-ePLAC128` | init_b | 408.8 k | 152 µs | 6.6e+03 | 151 µs | 1640 (5 × 328) |
| `CreTAKE-S2S-BiT128-ePLAC128` | pass1 | 1.90 M | 706 µs | 1.42e+03 | 705 µs | 1640 (5 × 328) |
| `CreTAKE-S2S-BiT128-ePLAC128` | pass2 | 2.34 M | 869 µs | 1.15e+03 | 866 µs | 1640 (5 × 328) |
| `CreTAKE-S2S-BiT128-ePLAC128` | derive_a | 663.3 k | 246 µs | 4.06e+03 | 246 µs | 1640 (5 × 328) |
| `CreTAKE-S2S-BiT128-ePLAC128` | derive_b | 360 | 46.9 ns | 2.13e+07 | 45.2 ns | 1640 (5 × 328) |
| `CreTAKE-S2S-BiT128-eZEN128` | exchange | 5.67 M | 2.1 ms | 475 | 2.1 ms | 1615 (5 × 323) |
| `CreTAKE-S2S-BiT128-eZEN128` | init_a | 413.6 k | 153 µs | 6.52e+03 | 152 µs | 1615 (5 × 323) |
| `CreTAKE-S2S-BiT128-eZEN128` | init_b | 410.6 k | 152 µs | 6.57e+03 | 151 µs | 1615 (5 × 323) |
| `CreTAKE-S2S-BiT128-eZEN128` | pass1 | 1.99 M | 739 µs | 1.35e+03 | 739 µs | 1615 (5 × 323) |
| `CreTAKE-S2S-BiT128-eZEN128` | pass2 | 2.19 M | 813 µs | 1.23e+03 | 812 µs | 1615 (5 × 323) |
| `CreTAKE-S2S-BiT128-eZEN128` | derive_a | 667.9 k | 248 µs | 4.04e+03 | 248 µs | 1615 (5 × 323) |
| `CreTAKE-S2S-BiT128-eZEN128` | derive_b | 255 | 51.1 ns | 1.96e+07 | 49.3 ns | 1615 (5 × 323) |
| `CreTAKE-S2S-BiT256-ePLAC256` | exchange | 11.34 M | 4.21 ms | 238 | 4.2 ms | 715 (5 × 143) |
| `CreTAKE-S2S-BiT256-ePLAC256` | init_a | 1.12 M | 416 µs | 2.4e+03 | 416 µs | 715 (5 × 143) |
| `CreTAKE-S2S-BiT256-ePLAC256` | init_b | 1.12 M | 416 µs | 2.4e+03 | 416 µs | 715 (5 × 143) |
| `CreTAKE-S2S-BiT256-ePLAC256` | pass1 | 2.83 M | 1.05 ms | 953 | 1.05 ms | 715 (5 × 143) |
| `CreTAKE-S2S-BiT256-ePLAC256` | pass2 | 4.41 M | 1.64 ms | 611 | 1.63 ms | 715 (5 × 143) |
| `CreTAKE-S2S-BiT256-ePLAC256` | derive_a | 1.86 M | 689 µs | 1.45e+03 | 688 µs | 715 (5 × 143) |
| `CreTAKE-S2S-BiT256-ePLAC256` | derive_b | 271 | 55.9 ns | 1.79e+07 | 57.2 ns | 715 (5 × 143) |
| `CreTAKE-S2S-BiT256-eZEN256` | exchange | 11.27 M | 4.18 ms | 239 | 4.18 ms | 705 (5 × 141) |
| `CreTAKE-S2S-BiT256-eZEN256` | init_a | 1.13 M | 418 µs | 2.39e+03 | 416 µs | 705 (5 × 141) |
| `CreTAKE-S2S-BiT256-eZEN256` | init_b | 1.13 M | 417 µs | 2.4e+03 | 415 µs | 705 (5 × 141) |
| `CreTAKE-S2S-BiT256-eZEN256` | pass1 | 2.89 M | 1.07 ms | 931 | 1.07 ms | 705 (5 × 141) |
| `CreTAKE-S2S-BiT256-eZEN256` | pass2 | 4.28 M | 1.59 ms | 630 | 1.59 ms | 705 (5 × 141) |
| `CreTAKE-S2S-BiT256-eZEN256` | derive_a | 1.87 M | 693 µs | 1.44e+03 | 689 µs | 705 (5 × 141) |
| `CreTAKE-S2S-BiT256-eZEN256` | derive_b | 323 | 60.3 ns | 1.66e+07 | 59.8 ns | 705 (5 × 141) |
| `CreTAKE-S2S-BiT512-ePLAC512` | exchange | 38.72 M | 14.4 ms | 69.6 | 14.4 ms | 210 (5 × 42) |
| `CreTAKE-S2S-BiT512-ePLAC512` | init_a | 3.34 M | 1.24 ms | 806 | 1.24 ms | 210 (5 × 42) |
| `CreTAKE-S2S-BiT512-ePLAC512` | init_b | 3.34 M | 1.24 ms | 808 | 1.24 ms | 210 (5 × 42) |
| `CreTAKE-S2S-BiT512-ePLAC512` | pass1 | 10.98 M | 4.07 ms | 245 | 4.06 ms | 210 (5 × 42) |
| `CreTAKE-S2S-BiT512-ePLAC512` | pass2 | 15.42 M | 5.72 ms | 175 | 5.71 ms | 210 (5 × 42) |
| `CreTAKE-S2S-BiT512-ePLAC512` | derive_a | 5.68 M | 2.11 ms | 475 | 2.11 ms | 210 (5 × 42) |
| `CreTAKE-S2S-BiT512-ePLAC512` | derive_b | 378 | 88.8 ns | 1.13e+07 | 88 ns | 210 (5 × 42) |
| `CreTAKE-S2S-BiT512-eZEN512` | exchange | 41.77 M | 15.5 ms | 64.5 | 15.5 ms | 200 (5 × 40) |
| `CreTAKE-S2S-BiT512-eZEN512` | init_a | 3.33 M | 1.24 ms | 809 | 1.24 ms | 200 (5 × 40) |
| `CreTAKE-S2S-BiT512-eZEN512` | init_b | 3.36 M | 1.25 ms | 803 | 1.24 ms | 200 (5 × 40) |
| `CreTAKE-S2S-BiT512-eZEN512` | pass1 | 11.79 M | 4.37 ms | 229 | 4.36 ms | 200 (5 × 40) |
| `CreTAKE-S2S-BiT512-eZEN512` | pass2 | 17.88 M | 6.63 ms | 151 | 6.63 ms | 200 (5 × 40) |
| `CreTAKE-S2S-BiT512-eZEN512` | derive_a | 5.52 M | 2.05 ms | 488 | 2.03 ms | 200 (5 × 40) |
| `CreTAKE-S2S-BiT512-eZEN512` | derive_b | 432 | 73.2 ns | 1.37e+07 | 70.3 ns | 200 (5 × 40) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `CreTAKE-K2K-PLAC128` | exchange | 54740 | 3528 KiB | 3592 KiB |
| `CreTAKE-K2K-PLAC128` | init_a | 54740 | 3540 KiB | 3604 KiB |
| `CreTAKE-K2K-PLAC128` | init_b | 54740 | 1504 KiB | 1568 KiB |
| `CreTAKE-K2K-PLAC128` | pass1 | 54740 | 1504 KiB | 1568 KiB |
| `CreTAKE-K2K-PLAC128` | pass2 | 54740 | 3512 KiB | 3576 KiB |
| `CreTAKE-K2K-PLAC128` | derive_a | 54740 | 1504 KiB | 1568 KiB |
| `CreTAKE-K2K-PLAC128` | derive_b | 54740 | 1508 KiB | 1572 KiB |
| `CreTAKE-K2K-PLAC256` | exchange | 62596 | 1576 KiB | 1648 KiB |
| `CreTAKE-K2K-PLAC256` | init_a | 62596 | 1576 KiB | 1648 KiB |
| `CreTAKE-K2K-PLAC256` | init_b | 62596 | 1576 KiB | 1648 KiB |
| `CreTAKE-K2K-PLAC256` | pass1 | 62596 | 1576 KiB | 1648 KiB |
| `CreTAKE-K2K-PLAC256` | pass2 | 62596 | 1572 KiB | 1644 KiB |
| `CreTAKE-K2K-PLAC256` | derive_a | 62596 | 1572 KiB | 1644 KiB |
| `CreTAKE-K2K-PLAC256` | derive_b | 62596 | 3544 KiB | 3608 KiB |
| `CreTAKE-K2K-PLAC512` | exchange | 76532 | 1740 KiB | 3848 KiB |
| `CreTAKE-K2K-PLAC512` | init_a | 76532 | 1736 KiB | 1804 KiB |
| `CreTAKE-K2K-PLAC512` | init_b | 76532 | 1736 KiB | 1804 KiB |
| `CreTAKE-K2K-PLAC512` | pass1 | 76532 | 3784 KiB | 3848 KiB |
| `CreTAKE-K2K-PLAC512` | pass2 | 76532 | 3756 KiB | 3820 KiB |
| `CreTAKE-K2K-PLAC512` | derive_a | 76532 | 1740 KiB | 1808 KiB |
| `CreTAKE-K2K-PLAC512` | derive_b | 76532 | 1740 KiB | 1808 KiB |
| `CreTAKE-K2K-PLAC512Star` | exchange | 81300 | 3664 KiB | 3728 KiB |
| `CreTAKE-K2K-PLAC512Star` | init_a | 81300 | 3712 KiB | 3776 KiB |
| `CreTAKE-K2K-PLAC512Star` | init_b | 81300 | 3804 KiB | 3868 KiB |
| `CreTAKE-K2K-PLAC512Star` | pass1 | 81300 | 3676 KiB | 3740 KiB |
| `CreTAKE-K2K-PLAC512Star` | pass2 | 81300 | 3604 KiB | 3668 KiB |
| `CreTAKE-K2K-PLAC512Star` | derive_a | 81300 | 1784 KiB | 1852 KiB |
| `CreTAKE-K2K-PLAC512Star` | derive_b | 81300 | 1784 KiB | 1852 KiB |
| `CreTAKE-K2K-ZEN128` | exchange | 39204 | 1484 KiB | 1552 KiB |
| `CreTAKE-K2K-ZEN128` | init_a | 39204 | 1488 KiB | 1556 KiB |
| `CreTAKE-K2K-ZEN128` | init_b | 39204 | 3504 KiB | 3568 KiB |
| `CreTAKE-K2K-ZEN128` | pass1 | 39204 | 1484 KiB | 1552 KiB |
| `CreTAKE-K2K-ZEN128` | pass2 | 39204 | 1484 KiB | 1552 KiB |
| `CreTAKE-K2K-ZEN128` | derive_a | 39204 | 1484 KiB | 1552 KiB |
| `CreTAKE-K2K-ZEN128` | derive_b | 39204 | 1488 KiB | 1556 KiB |
| `CreTAKE-K2K-ZEN256` | exchange | 44672 | 1536 KiB | 1608 KiB |
| `CreTAKE-K2K-ZEN256` | init_a | 44672 | 3568 KiB | 3632 KiB |
| `CreTAKE-K2K-ZEN256` | init_b | 44672 | 1536 KiB | 1608 KiB |
| `CreTAKE-K2K-ZEN256` | pass1 | 44672 | 3560 KiB | 3624 KiB |
| `CreTAKE-K2K-ZEN256` | pass2 | 44672 | 1536 KiB | 1608 KiB |
| `CreTAKE-K2K-ZEN256` | derive_a | 44672 | 3492 KiB | 3556 KiB |
| `CreTAKE-K2K-ZEN256` | derive_b | 44672 | 1536 KiB | 1608 KiB |
| `CreTAKE-K2K-ZEN512` | exchange | 51456 | 1660 KiB | 1728 KiB |
| `CreTAKE-K2K-ZEN512` | init_a | 51456 | 1660 KiB | 1728 KiB |
| `CreTAKE-K2K-ZEN512` | init_b | 51456 | 1660 KiB | 1728 KiB |
| `CreTAKE-K2K-ZEN512` | pass1 | 51456 | 1660 KiB | 1728 KiB |
| `CreTAKE-K2K-ZEN512` | pass2 | 51456 | 1660 KiB | 1728 KiB |
| `CreTAKE-K2K-ZEN512` | derive_a | 51456 | 3604 KiB | 3668 KiB |
| `CreTAKE-K2K-ZEN512` | derive_b | 51456 | 3608 KiB | 3672 KiB |
| `CreTAKE-K2S-PLAC128-BiT128` | exchange | 87036 | 1580 KiB | 1648 KiB |
| `CreTAKE-K2S-PLAC128-BiT128` | init_a | 87036 | 1580 KiB | 1648 KiB |
| `CreTAKE-K2S-PLAC128-BiT128` | init_b | 87036 | 3604 KiB | 3668 KiB |
| `CreTAKE-K2S-PLAC128-BiT128` | pass1 | 87036 | 1576 KiB | 1644 KiB |
| `CreTAKE-K2S-PLAC128-BiT128` | pass2 | 87036 | 1580 KiB | 1648 KiB |
| `CreTAKE-K2S-PLAC128-BiT128` | derive_a | 87036 | 3568 KiB | 3632 KiB |
| `CreTAKE-K2S-PLAC128-BiT128` | derive_b | 87036 | 1576 KiB | 1644 KiB |
| `CreTAKE-K2S-PLAC256-BiT256` | exchange | 95812 | 1768 KiB | 1836 KiB |
| `CreTAKE-K2S-PLAC256-BiT256` | init_a | 95812 | 1764 KiB | 1832 KiB |
| `CreTAKE-K2S-PLAC256-BiT256` | init_b | 95812 | 1768 KiB | 1836 KiB |
| `CreTAKE-K2S-PLAC256-BiT256` | pass1 | 95812 | 3680 KiB | 3744 KiB |
| `CreTAKE-K2S-PLAC256-BiT256` | pass2 | 95812 | 3776 KiB | 3840 KiB |
| `CreTAKE-K2S-PLAC256-BiT256` | derive_a | 95812 | 1768 KiB | 1836 KiB |
| `CreTAKE-K2S-PLAC256-BiT256` | derive_b | 95812 | 3720 KiB | 3784 KiB |
| `CreTAKE-K2S-PLAC512-BiT512` | exchange | 113604 | 2080 KiB | 2152 KiB |
| `CreTAKE-K2S-PLAC512-BiT512` | init_a | 113604 | 3852 KiB | 3916 KiB |
| `CreTAKE-K2S-PLAC512-BiT512` | init_b | 113604 | 2076 KiB | 2148 KiB |
| `CreTAKE-K2S-PLAC512-BiT512` | pass1 | 113604 | 3972 KiB | 4036 KiB |
| `CreTAKE-K2S-PLAC512-BiT512` | pass2 | 113604 | 2076 KiB | 2148 KiB |
| `CreTAKE-K2S-PLAC512-BiT512` | derive_a | 113604 | 2080 KiB | 2152 KiB |
| `CreTAKE-K2S-PLAC512-BiT512` | derive_b | 113604 | 2076 KiB | 2148 KiB |
| `CreTAKE-K2S-ZEN128-BiT128` | exchange | 71308 | 1556 KiB | 1624 KiB |
| `CreTAKE-K2S-ZEN128-BiT128` | init_a | 71308 | 1556 KiB | 1624 KiB |
| `CreTAKE-K2S-ZEN128-BiT128` | init_b | 71308 | 1556 KiB | 1624 KiB |
| `CreTAKE-K2S-ZEN128-BiT128` | pass1 | 71308 | 3548 KiB | 3612 KiB |
| `CreTAKE-K2S-ZEN128-BiT128` | pass2 | 71308 | 3528 KiB | 3592 KiB |
| `CreTAKE-K2S-ZEN128-BiT128` | derive_a | 71308 | 1552 KiB | 1620 KiB |
| `CreTAKE-K2S-ZEN128-BiT128` | derive_b | 71308 | 1552 KiB | 1620 KiB |
| `CreTAKE-K2S-ZEN256-BiT256` | exchange | 81828 | 1748 KiB | 1816 KiB |
| `CreTAKE-K2S-ZEN256-BiT256` | init_a | 81828 | 1748 KiB | 1816 KiB |
| `CreTAKE-K2S-ZEN256-BiT256` | init_b | 81828 | 1748 KiB | 1816 KiB |
| `CreTAKE-K2S-ZEN256-BiT256` | pass1 | 81828 | 1748 KiB | 1816 KiB |
| `CreTAKE-K2S-ZEN256-BiT256` | pass2 | 81828 | 1748 KiB | 1816 KiB |
| `CreTAKE-K2S-ZEN256-BiT256` | derive_a | 81828 | 1748 KiB | 1816 KiB |
| `CreTAKE-K2S-ZEN256-BiT256` | derive_b | 81828 | 1748 KiB | 1816 KiB |
| `CreTAKE-K2S-ZEN512-BiT512` | exchange | 90772 | 3792 KiB | 3856 KiB |
| `CreTAKE-K2S-ZEN512-BiT512` | init_a | 90772 | 2040 KiB | 2112 KiB |
| `CreTAKE-K2S-ZEN512-BiT512` | init_b | 90772 | 3848 KiB | 3912 KiB |
| `CreTAKE-K2S-ZEN512-BiT512` | pass1 | 90772 | 4052 KiB | 4116 KiB |
| `CreTAKE-K2S-ZEN512-BiT512` | pass2 | 90772 | 2044 KiB | 2116 KiB |
| `CreTAKE-K2S-ZEN512-BiT512` | derive_a | 90772 | 2044 KiB | 2116 KiB |
| `CreTAKE-K2S-ZEN512-BiT512` | derive_b | 90772 | 3988 KiB | 4052 KiB |
| `CreTAKE-S2K-BiT128-PLAC128` | exchange | 87044 | 1580 KiB | 1644 KiB |
| `CreTAKE-S2K-BiT128-PLAC128` | init_a | 87044 | 1580 KiB | 1644 KiB |
| `CreTAKE-S2K-BiT128-PLAC128` | init_b | 87044 | 1580 KiB | 1644 KiB |
| `CreTAKE-S2K-BiT128-PLAC128` | pass1 | 87044 | 1576 KiB | 1640 KiB |
| `CreTAKE-S2K-BiT128-PLAC128` | pass2 | 87044 | 1580 KiB | 1644 KiB |
| `CreTAKE-S2K-BiT128-PLAC128` | derive_a | 87044 | 1580 KiB | 1644 KiB |
| `CreTAKE-S2K-BiT128-PLAC128` | derive_b | 87044 | 1576 KiB | 1640 KiB |
| `CreTAKE-S2K-BiT128-ZEN128` | exchange | 75484 | 1556 KiB | 1624 KiB |
| `CreTAKE-S2K-BiT128-ZEN128` | init_a | 75484 | 1552 KiB | 1620 KiB |
| `CreTAKE-S2K-BiT128-ZEN128` | init_b | 75484 | 1556 KiB | 1624 KiB |
| `CreTAKE-S2K-BiT128-ZEN128` | pass1 | 75484 | 1556 KiB | 1624 KiB |
| `CreTAKE-S2K-BiT128-ZEN128` | pass2 | 75484 | 1552 KiB | 1620 KiB |
| `CreTAKE-S2K-BiT128-ZEN128` | derive_a | 75484 | 1556 KiB | 1624 KiB |
| `CreTAKE-S2K-BiT128-ZEN128` | derive_b | 75484 | 1556 KiB | 1624 KiB |
| `CreTAKE-S2K-BiT256-PLAC256` | exchange | 95876 | 1768 KiB | 1836 KiB |
| `CreTAKE-S2K-BiT256-PLAC256` | init_a | 95876 | 1768 KiB | 1836 KiB |
| `CreTAKE-S2K-BiT256-PLAC256` | init_b | 95876 | 1768 KiB | 1836 KiB |
| `CreTAKE-S2K-BiT256-PLAC256` | pass1 | 95876 | 3732 KiB | 3796 KiB |
| `CreTAKE-S2K-BiT256-PLAC256` | pass2 | 95876 | 1768 KiB | 1836 KiB |
| `CreTAKE-S2K-BiT256-PLAC256` | derive_a | 95876 | 3748 KiB | 3812 KiB |
| `CreTAKE-S2K-BiT256-PLAC256` | derive_b | 95876 | 1768 KiB | 1836 KiB |
| `CreTAKE-S2K-BiT256-ZEN256` | exchange | 77868 | 1740 KiB | 1808 KiB |
| `CreTAKE-S2K-BiT256-ZEN256` | init_a | 77868 | 3780 KiB | 3844 KiB |
| `CreTAKE-S2K-BiT256-ZEN256` | init_b | 77868 | 1740 KiB | 1808 KiB |
| `CreTAKE-S2K-BiT256-ZEN256` | pass1 | 77868 | 1740 KiB | 1808 KiB |
| `CreTAKE-S2K-BiT256-ZEN256` | pass2 | 77868 | 3712 KiB | 3776 KiB |
| `CreTAKE-S2K-BiT256-ZEN256` | derive_a | 77868 | 1744 KiB | 1812 KiB |
| `CreTAKE-S2K-BiT256-ZEN256` | derive_b | 77868 | 1740 KiB | 1808 KiB |
| `CreTAKE-S2K-BiT512-PLAC512` | exchange | 113604 | 2068 KiB | 2136 KiB |
| `CreTAKE-S2K-BiT512-PLAC512` | init_a | 113604 | 4048 KiB | 4112 KiB |
| `CreTAKE-S2K-BiT512-PLAC512` | init_b | 113604 | 2068 KiB | 2136 KiB |
| `CreTAKE-S2K-BiT512-PLAC512` | pass1 | 113604 | 3904 KiB | 3968 KiB |
| `CreTAKE-S2K-BiT512-PLAC512` | pass2 | 113604 | 2068 KiB | 2136 KiB |
| `CreTAKE-S2K-BiT512-PLAC512` | derive_a | 113604 | 4008 KiB | 4072 KiB |
| `CreTAKE-S2K-BiT512-PLAC512` | derive_b | 113604 | 2068 KiB | 2136 KiB |
| `CreTAKE-S2K-BiT512-ZEN512` | exchange | 90844 | 2032 KiB | 2100 KiB |
| `CreTAKE-S2K-BiT512-ZEN512` | init_a | 90844 | 3880 KiB | 3944 KiB |
| `CreTAKE-S2K-BiT512-ZEN512` | init_b | 90844 | 2032 KiB | 2100 KiB |
| `CreTAKE-S2K-BiT512-ZEN512` | pass1 | 90844 | 2028 KiB | 2096 KiB |
| `CreTAKE-S2K-BiT512-ZEN512` | pass2 | 90844 | 3980 KiB | 4044 KiB |
| `CreTAKE-S2K-BiT512-ZEN512` | derive_a | 90844 | 3988 KiB | 4052 KiB |
| `CreTAKE-S2K-BiT512-ZEN512` | derive_b | 90844 | 4056 KiB | 4120 KiB |
| `CreTAKE-S2S-BiT128-ePLAC128` | exchange | 86964 | 1588 KiB | 1656 KiB |
| `CreTAKE-S2S-BiT128-ePLAC128` | init_a | 86964 | 1588 KiB | 1656 KiB |
| `CreTAKE-S2S-BiT128-ePLAC128` | init_b | 86964 | 1588 KiB | 1656 KiB |
| `CreTAKE-S2S-BiT128-ePLAC128` | pass1 | 86964 | 1588 KiB | 1656 KiB |
| `CreTAKE-S2S-BiT128-ePLAC128` | pass2 | 86964 | 1588 KiB | 1656 KiB |
| `CreTAKE-S2S-BiT128-ePLAC128` | derive_a | 86964 | 1584 KiB | 1652 KiB |
| `CreTAKE-S2S-BiT128-ePLAC128` | derive_b | 86964 | 3608 KiB | 3672 KiB |
| `CreTAKE-S2S-BiT128-eZEN128` | exchange | 71324 | 3540 KiB | 3604 KiB |
| `CreTAKE-S2S-BiT128-eZEN128` | init_a | 71324 | 1564 KiB | 1632 KiB |
| `CreTAKE-S2S-BiT128-eZEN128` | init_b | 71324 | 1564 KiB | 1632 KiB |
| `CreTAKE-S2S-BiT128-eZEN128` | pass1 | 71324 | 1568 KiB | 1636 KiB |
| `CreTAKE-S2S-BiT128-eZEN128` | pass2 | 71324 | 3524 KiB | 3588 KiB |
| `CreTAKE-S2S-BiT128-eZEN128` | derive_a | 71324 | 1568 KiB | 1636 KiB |
| `CreTAKE-S2S-BiT128-eZEN128` | derive_b | 71324 | 3576 KiB | 3640 KiB |
| `CreTAKE-S2S-BiT256-ePLAC256` | exchange | 95796 | 3784 KiB | 3848 KiB |
| `CreTAKE-S2S-BiT256-ePLAC256` | init_a | 95796 | 1800 KiB | 1868 KiB |
| `CreTAKE-S2S-BiT256-ePLAC256` | init_b | 95796 | 1800 KiB | 1868 KiB |
| `CreTAKE-S2S-BiT256-ePLAC256` | pass1 | 95796 | 1796 KiB | 1864 KiB |
| `CreTAKE-S2S-BiT256-ePLAC256` | pass2 | 95796 | 1796 KiB | 1864 KiB |
| `CreTAKE-S2S-BiT256-ePLAC256` | derive_a | 95796 | 1800 KiB | 1868 KiB |
| `CreTAKE-S2S-BiT256-ePLAC256` | derive_b | 95796 | 1796 KiB | 1864 KiB |
| `CreTAKE-S2S-BiT256-eZEN256` | exchange | 77812 | 3780 KiB | 3844 KiB |
| `CreTAKE-S2S-BiT256-eZEN256` | init_a | 77812 | 3640 KiB | 3704 KiB |
| `CreTAKE-S2S-BiT256-eZEN256` | init_b | 77812 | 1764 KiB | 1836 KiB |
| `CreTAKE-S2S-BiT256-eZEN256` | pass1 | 77812 | 1764 KiB | 1836 KiB |
| `CreTAKE-S2S-BiT256-eZEN256` | pass2 | 77812 | 1764 KiB | 1836 KiB |
| `CreTAKE-S2S-BiT256-eZEN256` | derive_a | 77812 | 1764 KiB | 1836 KiB |
| `CreTAKE-S2S-BiT256-eZEN256` | derive_b | 77812 | 1768 KiB | 1840 KiB |
| `CreTAKE-S2S-BiT512-ePLAC512` | exchange | 113524 | 3996 KiB | 4060 KiB |
| `CreTAKE-S2S-BiT512-ePLAC512` | init_a | 113524 | 4148 KiB | 4212 KiB |
| `CreTAKE-S2S-BiT512-ePLAC512` | init_b | 113524 | 4076 KiB | 4140 KiB |
| `CreTAKE-S2S-BiT512-ePLAC512` | pass1 | 113524 | 2128 KiB | 2200 KiB |
| `CreTAKE-S2S-BiT512-ePLAC512` | pass2 | 113524 | 3908 KiB | 3972 KiB |
| `CreTAKE-S2S-BiT512-ePLAC512` | derive_a | 113524 | 3948 KiB | 4012 KiB |
| `CreTAKE-S2S-BiT512-ePLAC512` | derive_b | 113524 | 4056 KiB | 4244 KiB |
| `CreTAKE-S2S-BiT512-eZEN512` | exchange | 90788 | 4064 KiB | 4128 KiB |
| `CreTAKE-S2S-BiT512-eZEN512` | init_a | 90788 | 2100 KiB | 2172 KiB |
| `CreTAKE-S2S-BiT512-eZEN512` | init_b | 90788 | 4032 KiB | 4096 KiB |
| `CreTAKE-S2S-BiT512-eZEN512` | pass1 | 90788 | 2096 KiB | 2168 KiB |
| `CreTAKE-S2S-BiT512-eZEN512` | pass2 | 90788 | 4044 KiB | 4108 KiB |
| `CreTAKE-S2S-BiT512-eZEN512` | derive_a | 90788 | 3952 KiB | 4016 KiB |
| `CreTAKE-S2S-BiT512-eZEN512` | derive_b | 90788 | 2100 KiB | 2172 KiB |

## 6. Transmission and storage overhead

| instance | passes | messages (bytes) | total | long-term pk / sk | shared secret |
|---|---|---|---|---|---|
| `CreTAKE-K2K-PLAC128` | 2 | 1170 / 1280 | 2450 | 530 / 1570 | 32 |
| `CreTAKE-K2K-PLAC256` | 2 | 2340 / 2560 | 4900 | 1060 / 3140 | 64 |
| `CreTAKE-K2K-PLAC512` | 2 | 4676 / 4608 | 9284 | 2116 / 6276 | 128 |
| `CreTAKE-K2K-PLAC512Star` | 2 | 5492 / 5428 | 10920 | 2522 / 6682 | 128 |
| `CreTAKE-K2K-ZEN128` | 2 | 1127 / 1024 | 2151 | 615 / 1303 | 32 |
| `CreTAKE-K2K-ZEN256` | 2 | 2253 / 2048 | 4301 | 1229 / 2605 | 64 |
| `CreTAKE-K2K-ZEN512` | 2 | 4506 / 4096 | 8602 | 2458 / 5210 | 128 |
| `CreTAKE-K2S-PLAC128-BiT128` | 2 | 530 / 2784 | 3314 | 1048 / 1864 | 32 |
| `CreTAKE-K2S-PLAC256-BiT256` | 2 | 1060 / 6016 | 7076 | 2144 / 4160 | 64 |
| `CreTAKE-K2S-PLAC512-BiT512` | 2 | 2116 / 11815 | 13931 | 5056 / 9024 | 128 |
| `CreTAKE-K2S-ZEN128-BiT128` | 2 | 615 / 2528 | 3143 | 1048 / 1864 | 32 |
| `CreTAKE-K2S-ZEN256-BiT256` | 2 | 1229 / 5504 | 6733 | 2144 / 4160 | 64 |
| `CreTAKE-K2S-ZEN512-BiT512` | 2 | 2458 / 10791 | 13249 | 5056 / 9024 | 128 |
| `CreTAKE-S2K-BiT128-PLAC128` | 2 | 2674 / 640 | 3314 | 1048 / 1864 | 32 |
| `CreTAKE-S2K-BiT128-ZEN128` | 2 | 2631 / 512 | 3143 | 1048 / 1864 | 32 |
| `CreTAKE-S2K-BiT256-PLAC256` | 2 | 5796 / 1280 | 7076 | 2144 / 4160 | 64 |
| `CreTAKE-S2K-BiT256-ZEN256` | 2 | 5709 / 1024 | 6733 | 2144 / 4160 | 64 |
| `CreTAKE-S2K-BiT512-PLAC512` | 2 | 11371 / 2560 | 13931 | 5056 / 9024 | 128 |
| `CreTAKE-S2K-BiT512-ZEN512` | 2 | 11201 / 2048 | 13249 | 5056 / 9024 | 128 |
| `CreTAKE-S2S-BiT128-ePLAC128` | 2 | 2034 / 2144 | 4178 | 1048 / 1864 | 32 |
| `CreTAKE-S2S-BiT128-eZEN128` | 2 | 2119 / 2016 | 4135 | 1048 / 1864 | 32 |
| `CreTAKE-S2S-BiT256-ePLAC256` | 2 | 4516 / 4736 | 9252 | 2144 / 4160 | 64 |
| `CreTAKE-S2S-BiT256-eZEN256` | 2 | 4685 / 4480 | 9165 | 2144 / 4160 | 64 |
| `CreTAKE-S2S-BiT512-ePLAC512` | 2 | 8811 / 9255 | 18066 | 5056 / 9024 | 128 |
| `CreTAKE-S2S-BiT512-eZEN512` | 2 | 9153 / 8743 | 17896 | 5056 / 9024 | 128 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — PolarLAC/ZEN/BiT components; Keccak only if BIT_USE_SHAKE=1

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `CreTAKE-K2K-PLAC128` | exchange | 74% | 2.3% | drng 8, pseudoXOF 92.4, pseudohash 2 |
| `CreTAKE-K2K-PLAC256` | exchange | 74% | 1.2% | drng 8, pseudoXOF 92.4, pseudohash 2 |
| `CreTAKE-K2K-PLAC512` | exchange | 82% | 0.3% | drng 7, pseudoXOF 98.7, pseudohash 4 |
| `CreTAKE-K2K-PLAC512Star` | exchange | 84% | 0.3% | drng 7, pseudoXOF 139, pseudohash 4 |
| `CreTAKE-K2K-ZEN128` | exchange | 59% | 2.8% | drng 8, pseudoXOF 26.8, pseudohash 6, sm3hash 6 |
| `CreTAKE-K2K-ZEN256` | exchange | 60% | 1.6% | drng 8, pseudoXOF 27.1, pseudohash 6, sm3hash 6 |
| `CreTAKE-K2K-ZEN512` | exchange | 68% | 0.6% | drng 8, pseudoXOF 29.1, pseudohash 10 |
| `CreTAKE-K2S-PLAC128-BiT128` | exchange | 72% | 1.2% | drng 9, pseudoXOF 146, sm3hash 11 |
| `CreTAKE-K2S-PLAC256-BiT256` | exchange | 75% | 0.6% | drng 9, pseudoXOF 170, pseudohash 6.79 |
| `CreTAKE-K2S-PLAC512-BiT512` | exchange | 79% | 0.2% | drng 9, pseudoXOF 250, pseudohash 7.78 |
| `CreTAKE-K2S-ZEN128-BiT128` | exchange | 69% | 1.2% | drng 9, pseudoXOF 108, pseudohash 2, sm3hash 13.7 |
| `CreTAKE-K2S-ZEN256-BiT256` | exchange | 72% | 0.6% | drng 9, pseudoXOF 137, pseudohash 8.99, sm3hash 3 |
| `CreTAKE-K2S-ZEN512-BiT512` | exchange | 76% | 0.2% | drng 9, pseudoXOF 212, pseudohash 11.6 |
| `CreTAKE-S2K-BiT128-PLAC128` | exchange | 72% | 1.2% | drng 9, pseudoXOF 152, pseudohash 2, sm3hash 10.5 |
| `CreTAKE-S2K-BiT128-ZEN128` | exchange | 68% | 1.2% | drng 9, pseudoXOF 112, pseudohash 4, sm3hash 13.8 |
| `CreTAKE-S2K-BiT256-PLAC256` | exchange | 74% | 0.6% | drng 9, pseudoXOF 182, pseudohash 8.98 |
| `CreTAKE-S2K-BiT256-ZEN256` | exchange | 71% | 0.6% | drng 9, pseudoXOF 139, pseudohash 10.9, sm3hash 3 |
| `CreTAKE-S2K-BiT512-PLAC512` | exchange | 78% | 0.2% | drng 9, pseudoXOF 271, pseudohash 10.6 |
| `CreTAKE-S2K-BiT512-ZEN512` | exchange | 75% | 0.2% | drng 9, pseudoXOF 219, pseudohash 13.9 |
| `CreTAKE-S2S-BiT128-ePLAC128` | exchange | 72% | 0.8% | drng 10, pseudoXOF 216, pseudohash 2, sm3hash 21.4 |
| `CreTAKE-S2S-BiT128-eZEN128` | exchange | 70% | 0.8% | drng 10, pseudoXOF 197, pseudohash 2, sm3hash 21.3 |
| `CreTAKE-S2S-BiT256-ePLAC256` | exchange | 74% | 0.4% | drng 10, pseudoXOF 271, pseudohash 15.8 |
| `CreTAKE-S2S-BiT256-eZEN256` | exchange | 73% | 0.4% | drng 10, pseudoXOF 252, pseudohash 15.8 |
| `CreTAKE-S2S-BiT512-ePLAC512` | exchange | 78% | 0.1% | drng 10, pseudoXOF 435, pseudohash 18 |
| `CreTAKE-S2S-BiT512-eZEN512` | exchange | 77% | 0.1% | drng 10, pseudoXOF 416, pseudohash 18.1 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `CreTAKE-K2K-PLAC128` | KAT log (sha256 `e08c8cb6e89c1938…`) | `kat/kex-03/CreTAKE-K2K-PLAC128.log` |
| `CreTAKE-K2K-PLAC128` | timing derive_a | `records/kex-03/CreTAKE-K2K-PLAC128__derive_a.json` |
| `CreTAKE-K2K-PLAC128` | timing derive_b | `records/kex-03/CreTAKE-K2K-PLAC128__derive_b.json` |
| `CreTAKE-K2K-PLAC128` | timing exchange | `records/kex-03/CreTAKE-K2K-PLAC128__exchange.json` |
| `CreTAKE-K2K-PLAC128` | timing init_a | `records/kex-03/CreTAKE-K2K-PLAC128__init_a.json` |
| `CreTAKE-K2K-PLAC128` | timing init_b | `records/kex-03/CreTAKE-K2K-PLAC128__init_b.json` |
| `CreTAKE-K2K-PLAC128` | timing pass1 | `records/kex-03/CreTAKE-K2K-PLAC128__pass1.json` |
| `CreTAKE-K2K-PLAC128` | timing pass2 | `records/kex-03/CreTAKE-K2K-PLAC128__pass2.json` |
| `CreTAKE-K2K-PLAC128` | hash profile exchange | `profile/kex-03/CreTAKE-K2K-PLAC128__exchange.json` |
| `CreTAKE-K2K-PLAC256` | KAT log (sha256 `6776cc92dfcb13ff…`) | `kat/kex-03/CreTAKE-K2K-PLAC256.log` |
| `CreTAKE-K2K-PLAC256` | timing derive_a | `records/kex-03/CreTAKE-K2K-PLAC256__derive_a.json` |
| `CreTAKE-K2K-PLAC256` | timing derive_b | `records/kex-03/CreTAKE-K2K-PLAC256__derive_b.json` |
| `CreTAKE-K2K-PLAC256` | timing exchange | `records/kex-03/CreTAKE-K2K-PLAC256__exchange.json` |
| `CreTAKE-K2K-PLAC256` | timing init_a | `records/kex-03/CreTAKE-K2K-PLAC256__init_a.json` |
| `CreTAKE-K2K-PLAC256` | timing init_b | `records/kex-03/CreTAKE-K2K-PLAC256__init_b.json` |
| `CreTAKE-K2K-PLAC256` | timing pass1 | `records/kex-03/CreTAKE-K2K-PLAC256__pass1.json` |
| `CreTAKE-K2K-PLAC256` | timing pass2 | `records/kex-03/CreTAKE-K2K-PLAC256__pass2.json` |
| `CreTAKE-K2K-PLAC256` | hash profile exchange | `profile/kex-03/CreTAKE-K2K-PLAC256__exchange.json` |
| `CreTAKE-K2K-PLAC512` | KAT log (sha256 `0ec0534550ac2d11…`) | `kat/kex-03/CreTAKE-K2K-PLAC512.log` |
| `CreTAKE-K2K-PLAC512` | timing derive_a | `records/kex-03/CreTAKE-K2K-PLAC512__derive_a.json` |
| `CreTAKE-K2K-PLAC512` | timing derive_b | `records/kex-03/CreTAKE-K2K-PLAC512__derive_b.json` |
| `CreTAKE-K2K-PLAC512` | timing exchange | `records/kex-03/CreTAKE-K2K-PLAC512__exchange.json` |
| `CreTAKE-K2K-PLAC512` | timing init_a | `records/kex-03/CreTAKE-K2K-PLAC512__init_a.json` |
| `CreTAKE-K2K-PLAC512` | timing init_b | `records/kex-03/CreTAKE-K2K-PLAC512__init_b.json` |
| `CreTAKE-K2K-PLAC512` | timing pass1 | `records/kex-03/CreTAKE-K2K-PLAC512__pass1.json` |
| `CreTAKE-K2K-PLAC512` | timing pass2 | `records/kex-03/CreTAKE-K2K-PLAC512__pass2.json` |
| `CreTAKE-K2K-PLAC512` | hash profile exchange | `profile/kex-03/CreTAKE-K2K-PLAC512__exchange.json` |
| `CreTAKE-K2K-PLAC512Star` | KAT log (sha256 `378269af4e456bd7…`) | `kat/kex-03/CreTAKE-K2K-PLAC512Star.log` |
| `CreTAKE-K2K-PLAC512Star` | timing derive_a | `records/kex-03/CreTAKE-K2K-PLAC512Star__derive_a.json` |
| `CreTAKE-K2K-PLAC512Star` | timing derive_b | `records/kex-03/CreTAKE-K2K-PLAC512Star__derive_b.json` |
| `CreTAKE-K2K-PLAC512Star` | timing exchange | `records/kex-03/CreTAKE-K2K-PLAC512Star__exchange.json` |
| `CreTAKE-K2K-PLAC512Star` | timing init_a | `records/kex-03/CreTAKE-K2K-PLAC512Star__init_a.json` |
| `CreTAKE-K2K-PLAC512Star` | timing init_b | `records/kex-03/CreTAKE-K2K-PLAC512Star__init_b.json` |
| `CreTAKE-K2K-PLAC512Star` | timing pass1 | `records/kex-03/CreTAKE-K2K-PLAC512Star__pass1.json` |
| `CreTAKE-K2K-PLAC512Star` | timing pass2 | `records/kex-03/CreTAKE-K2K-PLAC512Star__pass2.json` |
| `CreTAKE-K2K-PLAC512Star` | hash profile exchange | `profile/kex-03/CreTAKE-K2K-PLAC512Star__exchange.json` |
| `CreTAKE-K2K-ZEN128` | KAT log (sha256 `b8e449db5fef1505…`) | `kat/kex-03/CreTAKE-K2K-ZEN128.log` |
| `CreTAKE-K2K-ZEN128` | timing derive_a | `records/kex-03/CreTAKE-K2K-ZEN128__derive_a.json` |
| `CreTAKE-K2K-ZEN128` | timing derive_b | `records/kex-03/CreTAKE-K2K-ZEN128__derive_b.json` |
| `CreTAKE-K2K-ZEN128` | timing exchange | `records/kex-03/CreTAKE-K2K-ZEN128__exchange.json` |
| `CreTAKE-K2K-ZEN128` | timing init_a | `records/kex-03/CreTAKE-K2K-ZEN128__init_a.json` |
| `CreTAKE-K2K-ZEN128` | timing init_b | `records/kex-03/CreTAKE-K2K-ZEN128__init_b.json` |
| `CreTAKE-K2K-ZEN128` | timing pass1 | `records/kex-03/CreTAKE-K2K-ZEN128__pass1.json` |
| `CreTAKE-K2K-ZEN128` | timing pass2 | `records/kex-03/CreTAKE-K2K-ZEN128__pass2.json` |
| `CreTAKE-K2K-ZEN128` | hash profile exchange | `profile/kex-03/CreTAKE-K2K-ZEN128__exchange.json` |
| `CreTAKE-K2K-ZEN256` | KAT log (sha256 `8f56d874af2c7b47…`) | `kat/kex-03/CreTAKE-K2K-ZEN256.log` |
| `CreTAKE-K2K-ZEN256` | timing derive_a | `records/kex-03/CreTAKE-K2K-ZEN256__derive_a.json` |
| `CreTAKE-K2K-ZEN256` | timing derive_b | `records/kex-03/CreTAKE-K2K-ZEN256__derive_b.json` |
| `CreTAKE-K2K-ZEN256` | timing exchange | `records/kex-03/CreTAKE-K2K-ZEN256__exchange.json` |
| `CreTAKE-K2K-ZEN256` | timing init_a | `records/kex-03/CreTAKE-K2K-ZEN256__init_a.json` |
| `CreTAKE-K2K-ZEN256` | timing init_b | `records/kex-03/CreTAKE-K2K-ZEN256__init_b.json` |
| `CreTAKE-K2K-ZEN256` | timing pass1 | `records/kex-03/CreTAKE-K2K-ZEN256__pass1.json` |
| `CreTAKE-K2K-ZEN256` | timing pass2 | `records/kex-03/CreTAKE-K2K-ZEN256__pass2.json` |
| `CreTAKE-K2K-ZEN256` | hash profile exchange | `profile/kex-03/CreTAKE-K2K-ZEN256__exchange.json` |
| `CreTAKE-K2K-ZEN512` | KAT log (sha256 `da7237ab1b7c88f3…`) | `kat/kex-03/CreTAKE-K2K-ZEN512.log` |
| `CreTAKE-K2K-ZEN512` | timing derive_a | `records/kex-03/CreTAKE-K2K-ZEN512__derive_a.json` |
| `CreTAKE-K2K-ZEN512` | timing derive_b | `records/kex-03/CreTAKE-K2K-ZEN512__derive_b.json` |
| `CreTAKE-K2K-ZEN512` | timing exchange | `records/kex-03/CreTAKE-K2K-ZEN512__exchange.json` |
| `CreTAKE-K2K-ZEN512` | timing init_a | `records/kex-03/CreTAKE-K2K-ZEN512__init_a.json` |
| `CreTAKE-K2K-ZEN512` | timing init_b | `records/kex-03/CreTAKE-K2K-ZEN512__init_b.json` |
| `CreTAKE-K2K-ZEN512` | timing pass1 | `records/kex-03/CreTAKE-K2K-ZEN512__pass1.json` |
| `CreTAKE-K2K-ZEN512` | timing pass2 | `records/kex-03/CreTAKE-K2K-ZEN512__pass2.json` |
| `CreTAKE-K2K-ZEN512` | hash profile exchange | `profile/kex-03/CreTAKE-K2K-ZEN512__exchange.json` |
| `CreTAKE-K2S-PLAC128-BiT128` | KAT log (sha256 `c127506dbf55c8b1…`) | `kat/kex-03/CreTAKE-K2S-PLAC128-BiT128.log` |
| `CreTAKE-K2S-PLAC128-BiT128` | timing derive_a | `records/kex-03/CreTAKE-K2S-PLAC128-BiT128__derive_a.json` |
| `CreTAKE-K2S-PLAC128-BiT128` | timing derive_b | `records/kex-03/CreTAKE-K2S-PLAC128-BiT128__derive_b.json` |
| `CreTAKE-K2S-PLAC128-BiT128` | timing exchange | `records/kex-03/CreTAKE-K2S-PLAC128-BiT128__exchange.json` |
| `CreTAKE-K2S-PLAC128-BiT128` | timing init_a | `records/kex-03/CreTAKE-K2S-PLAC128-BiT128__init_a.json` |
| `CreTAKE-K2S-PLAC128-BiT128` | timing init_b | `records/kex-03/CreTAKE-K2S-PLAC128-BiT128__init_b.json` |
| `CreTAKE-K2S-PLAC128-BiT128` | timing pass1 | `records/kex-03/CreTAKE-K2S-PLAC128-BiT128__pass1.json` |
| `CreTAKE-K2S-PLAC128-BiT128` | timing pass2 | `records/kex-03/CreTAKE-K2S-PLAC128-BiT128__pass2.json` |
| `CreTAKE-K2S-PLAC128-BiT128` | hash profile exchange | `profile/kex-03/CreTAKE-K2S-PLAC128-BiT128__exchange.json` |
| `CreTAKE-K2S-PLAC256-BiT256` | KAT log (sha256 `ef71b168b004d4fa…`) | `kat/kex-03/CreTAKE-K2S-PLAC256-BiT256.log` |
| `CreTAKE-K2S-PLAC256-BiT256` | timing derive_a | `records/kex-03/CreTAKE-K2S-PLAC256-BiT256__derive_a.json` |
| `CreTAKE-K2S-PLAC256-BiT256` | timing derive_b | `records/kex-03/CreTAKE-K2S-PLAC256-BiT256__derive_b.json` |
| `CreTAKE-K2S-PLAC256-BiT256` | timing exchange | `records/kex-03/CreTAKE-K2S-PLAC256-BiT256__exchange.json` |
| `CreTAKE-K2S-PLAC256-BiT256` | timing init_a | `records/kex-03/CreTAKE-K2S-PLAC256-BiT256__init_a.json` |
| `CreTAKE-K2S-PLAC256-BiT256` | timing init_b | `records/kex-03/CreTAKE-K2S-PLAC256-BiT256__init_b.json` |
| `CreTAKE-K2S-PLAC256-BiT256` | timing pass1 | `records/kex-03/CreTAKE-K2S-PLAC256-BiT256__pass1.json` |
| `CreTAKE-K2S-PLAC256-BiT256` | timing pass2 | `records/kex-03/CreTAKE-K2S-PLAC256-BiT256__pass2.json` |
| `CreTAKE-K2S-PLAC256-BiT256` | hash profile exchange | `profile/kex-03/CreTAKE-K2S-PLAC256-BiT256__exchange.json` |
| `CreTAKE-K2S-PLAC512-BiT512` | KAT log (sha256 `a2decb7b0885e271…`) | `kat/kex-03/CreTAKE-K2S-PLAC512-BiT512.log` |
| `CreTAKE-K2S-PLAC512-BiT512` | timing derive_a | `records/kex-03/CreTAKE-K2S-PLAC512-BiT512__derive_a.json` |
| `CreTAKE-K2S-PLAC512-BiT512` | timing derive_b | `records/kex-03/CreTAKE-K2S-PLAC512-BiT512__derive_b.json` |
| `CreTAKE-K2S-PLAC512-BiT512` | timing exchange | `records/kex-03/CreTAKE-K2S-PLAC512-BiT512__exchange.json` |
| `CreTAKE-K2S-PLAC512-BiT512` | timing init_a | `records/kex-03/CreTAKE-K2S-PLAC512-BiT512__init_a.json` |
| `CreTAKE-K2S-PLAC512-BiT512` | timing init_b | `records/kex-03/CreTAKE-K2S-PLAC512-BiT512__init_b.json` |
| `CreTAKE-K2S-PLAC512-BiT512` | timing pass1 | `records/kex-03/CreTAKE-K2S-PLAC512-BiT512__pass1.json` |
| `CreTAKE-K2S-PLAC512-BiT512` | timing pass2 | `records/kex-03/CreTAKE-K2S-PLAC512-BiT512__pass2.json` |
| `CreTAKE-K2S-PLAC512-BiT512` | hash profile exchange | `profile/kex-03/CreTAKE-K2S-PLAC512-BiT512__exchange.json` |
| `CreTAKE-K2S-ZEN128-BiT128` | KAT log (sha256 `1bef2d516968f06b…`) | `kat/kex-03/CreTAKE-K2S-ZEN128-BiT128.log` |
| `CreTAKE-K2S-ZEN128-BiT128` | timing derive_a | `records/kex-03/CreTAKE-K2S-ZEN128-BiT128__derive_a.json` |
| `CreTAKE-K2S-ZEN128-BiT128` | timing derive_b | `records/kex-03/CreTAKE-K2S-ZEN128-BiT128__derive_b.json` |
| `CreTAKE-K2S-ZEN128-BiT128` | timing exchange | `records/kex-03/CreTAKE-K2S-ZEN128-BiT128__exchange.json` |
| `CreTAKE-K2S-ZEN128-BiT128` | timing init_a | `records/kex-03/CreTAKE-K2S-ZEN128-BiT128__init_a.json` |
| `CreTAKE-K2S-ZEN128-BiT128` | timing init_b | `records/kex-03/CreTAKE-K2S-ZEN128-BiT128__init_b.json` |
| `CreTAKE-K2S-ZEN128-BiT128` | timing pass1 | `records/kex-03/CreTAKE-K2S-ZEN128-BiT128__pass1.json` |
| `CreTAKE-K2S-ZEN128-BiT128` | timing pass2 | `records/kex-03/CreTAKE-K2S-ZEN128-BiT128__pass2.json` |
| `CreTAKE-K2S-ZEN128-BiT128` | hash profile exchange | `profile/kex-03/CreTAKE-K2S-ZEN128-BiT128__exchange.json` |
| `CreTAKE-K2S-ZEN256-BiT256` | KAT log (sha256 `9aa8912c0d728f73…`) | `kat/kex-03/CreTAKE-K2S-ZEN256-BiT256.log` |
| `CreTAKE-K2S-ZEN256-BiT256` | timing derive_a | `records/kex-03/CreTAKE-K2S-ZEN256-BiT256__derive_a.json` |
| `CreTAKE-K2S-ZEN256-BiT256` | timing derive_b | `records/kex-03/CreTAKE-K2S-ZEN256-BiT256__derive_b.json` |
| `CreTAKE-K2S-ZEN256-BiT256` | timing exchange | `records/kex-03/CreTAKE-K2S-ZEN256-BiT256__exchange.json` |
| `CreTAKE-K2S-ZEN256-BiT256` | timing init_a | `records/kex-03/CreTAKE-K2S-ZEN256-BiT256__init_a.json` |
| `CreTAKE-K2S-ZEN256-BiT256` | timing init_b | `records/kex-03/CreTAKE-K2S-ZEN256-BiT256__init_b.json` |
| `CreTAKE-K2S-ZEN256-BiT256` | timing pass1 | `records/kex-03/CreTAKE-K2S-ZEN256-BiT256__pass1.json` |
| `CreTAKE-K2S-ZEN256-BiT256` | timing pass2 | `records/kex-03/CreTAKE-K2S-ZEN256-BiT256__pass2.json` |
| `CreTAKE-K2S-ZEN256-BiT256` | hash profile exchange | `profile/kex-03/CreTAKE-K2S-ZEN256-BiT256__exchange.json` |
| `CreTAKE-K2S-ZEN512-BiT512` | KAT log (sha256 `5a2924e0a9ced0b2…`) | `kat/kex-03/CreTAKE-K2S-ZEN512-BiT512.log` |
| `CreTAKE-K2S-ZEN512-BiT512` | timing derive_a | `records/kex-03/CreTAKE-K2S-ZEN512-BiT512__derive_a.json` |
| `CreTAKE-K2S-ZEN512-BiT512` | timing derive_b | `records/kex-03/CreTAKE-K2S-ZEN512-BiT512__derive_b.json` |
| `CreTAKE-K2S-ZEN512-BiT512` | timing exchange | `records/kex-03/CreTAKE-K2S-ZEN512-BiT512__exchange.json` |
| `CreTAKE-K2S-ZEN512-BiT512` | timing init_a | `records/kex-03/CreTAKE-K2S-ZEN512-BiT512__init_a.json` |
| `CreTAKE-K2S-ZEN512-BiT512` | timing init_b | `records/kex-03/CreTAKE-K2S-ZEN512-BiT512__init_b.json` |
| `CreTAKE-K2S-ZEN512-BiT512` | timing pass1 | `records/kex-03/CreTAKE-K2S-ZEN512-BiT512__pass1.json` |
| `CreTAKE-K2S-ZEN512-BiT512` | timing pass2 | `records/kex-03/CreTAKE-K2S-ZEN512-BiT512__pass2.json` |
| `CreTAKE-K2S-ZEN512-BiT512` | hash profile exchange | `profile/kex-03/CreTAKE-K2S-ZEN512-BiT512__exchange.json` |
| `CreTAKE-S2K-BiT128-PLAC128` | KAT log (sha256 `eba54c20bd004e92…`) | `kat/kex-03/CreTAKE-S2K-BiT128-PLAC128.log` |
| `CreTAKE-S2K-BiT128-PLAC128` | timing derive_a | `records/kex-03/CreTAKE-S2K-BiT128-PLAC128__derive_a.json` |
| `CreTAKE-S2K-BiT128-PLAC128` | timing derive_b | `records/kex-03/CreTAKE-S2K-BiT128-PLAC128__derive_b.json` |
| `CreTAKE-S2K-BiT128-PLAC128` | timing exchange | `records/kex-03/CreTAKE-S2K-BiT128-PLAC128__exchange.json` |
| `CreTAKE-S2K-BiT128-PLAC128` | timing init_a | `records/kex-03/CreTAKE-S2K-BiT128-PLAC128__init_a.json` |
| `CreTAKE-S2K-BiT128-PLAC128` | timing init_b | `records/kex-03/CreTAKE-S2K-BiT128-PLAC128__init_b.json` |
| `CreTAKE-S2K-BiT128-PLAC128` | timing pass1 | `records/kex-03/CreTAKE-S2K-BiT128-PLAC128__pass1.json` |
| `CreTAKE-S2K-BiT128-PLAC128` | timing pass2 | `records/kex-03/CreTAKE-S2K-BiT128-PLAC128__pass2.json` |
| `CreTAKE-S2K-BiT128-PLAC128` | hash profile exchange | `profile/kex-03/CreTAKE-S2K-BiT128-PLAC128__exchange.json` |
| `CreTAKE-S2K-BiT128-ZEN128` | KAT log (sha256 `7c70f751e9045bb1…`) | `kat/kex-03/CreTAKE-S2K-BiT128-ZEN128.log` |
| `CreTAKE-S2K-BiT128-ZEN128` | timing derive_a | `records/kex-03/CreTAKE-S2K-BiT128-ZEN128__derive_a.json` |
| `CreTAKE-S2K-BiT128-ZEN128` | timing derive_b | `records/kex-03/CreTAKE-S2K-BiT128-ZEN128__derive_b.json` |
| `CreTAKE-S2K-BiT128-ZEN128` | timing exchange | `records/kex-03/CreTAKE-S2K-BiT128-ZEN128__exchange.json` |
| `CreTAKE-S2K-BiT128-ZEN128` | timing init_a | `records/kex-03/CreTAKE-S2K-BiT128-ZEN128__init_a.json` |
| `CreTAKE-S2K-BiT128-ZEN128` | timing init_b | `records/kex-03/CreTAKE-S2K-BiT128-ZEN128__init_b.json` |
| `CreTAKE-S2K-BiT128-ZEN128` | timing pass1 | `records/kex-03/CreTAKE-S2K-BiT128-ZEN128__pass1.json` |
| `CreTAKE-S2K-BiT128-ZEN128` | timing pass2 | `records/kex-03/CreTAKE-S2K-BiT128-ZEN128__pass2.json` |
| `CreTAKE-S2K-BiT128-ZEN128` | hash profile exchange | `profile/kex-03/CreTAKE-S2K-BiT128-ZEN128__exchange.json` |
| `CreTAKE-S2K-BiT256-PLAC256` | KAT log (sha256 `ecf73bcc0dedc46a…`) | `kat/kex-03/CreTAKE-S2K-BiT256-PLAC256.log` |
| `CreTAKE-S2K-BiT256-PLAC256` | timing derive_a | `records/kex-03/CreTAKE-S2K-BiT256-PLAC256__derive_a.json` |
| `CreTAKE-S2K-BiT256-PLAC256` | timing derive_b | `records/kex-03/CreTAKE-S2K-BiT256-PLAC256__derive_b.json` |
| `CreTAKE-S2K-BiT256-PLAC256` | timing exchange | `records/kex-03/CreTAKE-S2K-BiT256-PLAC256__exchange.json` |
| `CreTAKE-S2K-BiT256-PLAC256` | timing init_a | `records/kex-03/CreTAKE-S2K-BiT256-PLAC256__init_a.json` |
| `CreTAKE-S2K-BiT256-PLAC256` | timing init_b | `records/kex-03/CreTAKE-S2K-BiT256-PLAC256__init_b.json` |
| `CreTAKE-S2K-BiT256-PLAC256` | timing pass1 | `records/kex-03/CreTAKE-S2K-BiT256-PLAC256__pass1.json` |
| `CreTAKE-S2K-BiT256-PLAC256` | timing pass2 | `records/kex-03/CreTAKE-S2K-BiT256-PLAC256__pass2.json` |
| `CreTAKE-S2K-BiT256-PLAC256` | hash profile exchange | `profile/kex-03/CreTAKE-S2K-BiT256-PLAC256__exchange.json` |
| `CreTAKE-S2K-BiT256-ZEN256` | KAT log (sha256 `b044568978fddfeb…`) | `kat/kex-03/CreTAKE-S2K-BiT256-ZEN256.log` |
| `CreTAKE-S2K-BiT256-ZEN256` | timing derive_a | `records/kex-03/CreTAKE-S2K-BiT256-ZEN256__derive_a.json` |
| `CreTAKE-S2K-BiT256-ZEN256` | timing derive_b | `records/kex-03/CreTAKE-S2K-BiT256-ZEN256__derive_b.json` |
| `CreTAKE-S2K-BiT256-ZEN256` | timing exchange | `records/kex-03/CreTAKE-S2K-BiT256-ZEN256__exchange.json` |
| `CreTAKE-S2K-BiT256-ZEN256` | timing init_a | `records/kex-03/CreTAKE-S2K-BiT256-ZEN256__init_a.json` |
| `CreTAKE-S2K-BiT256-ZEN256` | timing init_b | `records/kex-03/CreTAKE-S2K-BiT256-ZEN256__init_b.json` |
| `CreTAKE-S2K-BiT256-ZEN256` | timing pass1 | `records/kex-03/CreTAKE-S2K-BiT256-ZEN256__pass1.json` |
| `CreTAKE-S2K-BiT256-ZEN256` | timing pass2 | `records/kex-03/CreTAKE-S2K-BiT256-ZEN256__pass2.json` |
| `CreTAKE-S2K-BiT256-ZEN256` | hash profile exchange | `profile/kex-03/CreTAKE-S2K-BiT256-ZEN256__exchange.json` |
| `CreTAKE-S2K-BiT512-PLAC512` | KAT log (sha256 `18c7adc0437c6810…`) | `kat/kex-03/CreTAKE-S2K-BiT512-PLAC512.log` |
| `CreTAKE-S2K-BiT512-PLAC512` | timing derive_a | `records/kex-03/CreTAKE-S2K-BiT512-PLAC512__derive_a.json` |
| `CreTAKE-S2K-BiT512-PLAC512` | timing derive_b | `records/kex-03/CreTAKE-S2K-BiT512-PLAC512__derive_b.json` |
| `CreTAKE-S2K-BiT512-PLAC512` | timing exchange | `records/kex-03/CreTAKE-S2K-BiT512-PLAC512__exchange.json` |
| `CreTAKE-S2K-BiT512-PLAC512` | timing init_a | `records/kex-03/CreTAKE-S2K-BiT512-PLAC512__init_a.json` |
| `CreTAKE-S2K-BiT512-PLAC512` | timing init_b | `records/kex-03/CreTAKE-S2K-BiT512-PLAC512__init_b.json` |
| `CreTAKE-S2K-BiT512-PLAC512` | timing pass1 | `records/kex-03/CreTAKE-S2K-BiT512-PLAC512__pass1.json` |
| `CreTAKE-S2K-BiT512-PLAC512` | timing pass2 | `records/kex-03/CreTAKE-S2K-BiT512-PLAC512__pass2.json` |
| `CreTAKE-S2K-BiT512-PLAC512` | hash profile exchange | `profile/kex-03/CreTAKE-S2K-BiT512-PLAC512__exchange.json` |
| `CreTAKE-S2K-BiT512-ZEN512` | KAT log (sha256 `444722ec478abd82…`) | `kat/kex-03/CreTAKE-S2K-BiT512-ZEN512.log` |
| `CreTAKE-S2K-BiT512-ZEN512` | timing derive_a | `records/kex-03/CreTAKE-S2K-BiT512-ZEN512__derive_a.json` |
| `CreTAKE-S2K-BiT512-ZEN512` | timing derive_b | `records/kex-03/CreTAKE-S2K-BiT512-ZEN512__derive_b.json` |
| `CreTAKE-S2K-BiT512-ZEN512` | timing exchange | `records/kex-03/CreTAKE-S2K-BiT512-ZEN512__exchange.json` |
| `CreTAKE-S2K-BiT512-ZEN512` | timing init_a | `records/kex-03/CreTAKE-S2K-BiT512-ZEN512__init_a.json` |
| `CreTAKE-S2K-BiT512-ZEN512` | timing init_b | `records/kex-03/CreTAKE-S2K-BiT512-ZEN512__init_b.json` |
| `CreTAKE-S2K-BiT512-ZEN512` | timing pass1 | `records/kex-03/CreTAKE-S2K-BiT512-ZEN512__pass1.json` |
| `CreTAKE-S2K-BiT512-ZEN512` | timing pass2 | `records/kex-03/CreTAKE-S2K-BiT512-ZEN512__pass2.json` |
| `CreTAKE-S2K-BiT512-ZEN512` | hash profile exchange | `profile/kex-03/CreTAKE-S2K-BiT512-ZEN512__exchange.json` |
| `CreTAKE-S2S-BiT128-ePLAC128` | KAT log (sha256 `bdfe37ee747482cd…`) | `kat/kex-03/CreTAKE-S2S-BiT128-ePLAC128.log` |
| `CreTAKE-S2S-BiT128-ePLAC128` | timing derive_a | `records/kex-03/CreTAKE-S2S-BiT128-ePLAC128__derive_a.json` |
| `CreTAKE-S2S-BiT128-ePLAC128` | timing derive_b | `records/kex-03/CreTAKE-S2S-BiT128-ePLAC128__derive_b.json` |
| `CreTAKE-S2S-BiT128-ePLAC128` | timing exchange | `records/kex-03/CreTAKE-S2S-BiT128-ePLAC128__exchange.json` |
| `CreTAKE-S2S-BiT128-ePLAC128` | timing init_a | `records/kex-03/CreTAKE-S2S-BiT128-ePLAC128__init_a.json` |
| `CreTAKE-S2S-BiT128-ePLAC128` | timing init_b | `records/kex-03/CreTAKE-S2S-BiT128-ePLAC128__init_b.json` |
| `CreTAKE-S2S-BiT128-ePLAC128` | timing pass1 | `records/kex-03/CreTAKE-S2S-BiT128-ePLAC128__pass1.json` |
| `CreTAKE-S2S-BiT128-ePLAC128` | timing pass2 | `records/kex-03/CreTAKE-S2S-BiT128-ePLAC128__pass2.json` |
| `CreTAKE-S2S-BiT128-ePLAC128` | hash profile exchange | `profile/kex-03/CreTAKE-S2S-BiT128-ePLAC128__exchange.json` |
| `CreTAKE-S2S-BiT128-eZEN128` | KAT log (sha256 `d94d4349927a132d…`) | `kat/kex-03/CreTAKE-S2S-BiT128-eZEN128.log` |
| `CreTAKE-S2S-BiT128-eZEN128` | timing derive_a | `records/kex-03/CreTAKE-S2S-BiT128-eZEN128__derive_a.json` |
| `CreTAKE-S2S-BiT128-eZEN128` | timing derive_b | `records/kex-03/CreTAKE-S2S-BiT128-eZEN128__derive_b.json` |
| `CreTAKE-S2S-BiT128-eZEN128` | timing exchange | `records/kex-03/CreTAKE-S2S-BiT128-eZEN128__exchange.json` |
| `CreTAKE-S2S-BiT128-eZEN128` | timing init_a | `records/kex-03/CreTAKE-S2S-BiT128-eZEN128__init_a.json` |
| `CreTAKE-S2S-BiT128-eZEN128` | timing init_b | `records/kex-03/CreTAKE-S2S-BiT128-eZEN128__init_b.json` |
| `CreTAKE-S2S-BiT128-eZEN128` | timing pass1 | `records/kex-03/CreTAKE-S2S-BiT128-eZEN128__pass1.json` |
| `CreTAKE-S2S-BiT128-eZEN128` | timing pass2 | `records/kex-03/CreTAKE-S2S-BiT128-eZEN128__pass2.json` |
| `CreTAKE-S2S-BiT128-eZEN128` | hash profile exchange | `profile/kex-03/CreTAKE-S2S-BiT128-eZEN128__exchange.json` |
| `CreTAKE-S2S-BiT256-ePLAC256` | KAT log (sha256 `a81cb58d084b18a6…`) | `kat/kex-03/CreTAKE-S2S-BiT256-ePLAC256.log` |
| `CreTAKE-S2S-BiT256-ePLAC256` | timing derive_a | `records/kex-03/CreTAKE-S2S-BiT256-ePLAC256__derive_a.json` |
| `CreTAKE-S2S-BiT256-ePLAC256` | timing derive_b | `records/kex-03/CreTAKE-S2S-BiT256-ePLAC256__derive_b.json` |
| `CreTAKE-S2S-BiT256-ePLAC256` | timing exchange | `records/kex-03/CreTAKE-S2S-BiT256-ePLAC256__exchange.json` |
| `CreTAKE-S2S-BiT256-ePLAC256` | timing init_a | `records/kex-03/CreTAKE-S2S-BiT256-ePLAC256__init_a.json` |
| `CreTAKE-S2S-BiT256-ePLAC256` | timing init_b | `records/kex-03/CreTAKE-S2S-BiT256-ePLAC256__init_b.json` |
| `CreTAKE-S2S-BiT256-ePLAC256` | timing pass1 | `records/kex-03/CreTAKE-S2S-BiT256-ePLAC256__pass1.json` |
| `CreTAKE-S2S-BiT256-ePLAC256` | timing pass2 | `records/kex-03/CreTAKE-S2S-BiT256-ePLAC256__pass2.json` |
| `CreTAKE-S2S-BiT256-ePLAC256` | hash profile exchange | `profile/kex-03/CreTAKE-S2S-BiT256-ePLAC256__exchange.json` |
| `CreTAKE-S2S-BiT256-eZEN256` | KAT log (sha256 `ba1cecee5537b330…`) | `kat/kex-03/CreTAKE-S2S-BiT256-eZEN256.log` |
| `CreTAKE-S2S-BiT256-eZEN256` | timing derive_a | `records/kex-03/CreTAKE-S2S-BiT256-eZEN256__derive_a.json` |
| `CreTAKE-S2S-BiT256-eZEN256` | timing derive_b | `records/kex-03/CreTAKE-S2S-BiT256-eZEN256__derive_b.json` |
| `CreTAKE-S2S-BiT256-eZEN256` | timing exchange | `records/kex-03/CreTAKE-S2S-BiT256-eZEN256__exchange.json` |
| `CreTAKE-S2S-BiT256-eZEN256` | timing init_a | `records/kex-03/CreTAKE-S2S-BiT256-eZEN256__init_a.json` |
| `CreTAKE-S2S-BiT256-eZEN256` | timing init_b | `records/kex-03/CreTAKE-S2S-BiT256-eZEN256__init_b.json` |
| `CreTAKE-S2S-BiT256-eZEN256` | timing pass1 | `records/kex-03/CreTAKE-S2S-BiT256-eZEN256__pass1.json` |
| `CreTAKE-S2S-BiT256-eZEN256` | timing pass2 | `records/kex-03/CreTAKE-S2S-BiT256-eZEN256__pass2.json` |
| `CreTAKE-S2S-BiT256-eZEN256` | hash profile exchange | `profile/kex-03/CreTAKE-S2S-BiT256-eZEN256__exchange.json` |
| `CreTAKE-S2S-BiT512-ePLAC512` | KAT log (sha256 `65e997e6fca4bf28…`) | `kat/kex-03/CreTAKE-S2S-BiT512-ePLAC512.log` |
| `CreTAKE-S2S-BiT512-ePLAC512` | timing derive_a | `records/kex-03/CreTAKE-S2S-BiT512-ePLAC512__derive_a.json` |
| `CreTAKE-S2S-BiT512-ePLAC512` | timing derive_b | `records/kex-03/CreTAKE-S2S-BiT512-ePLAC512__derive_b.json` |
| `CreTAKE-S2S-BiT512-ePLAC512` | timing exchange | `records/kex-03/CreTAKE-S2S-BiT512-ePLAC512__exchange.json` |
| `CreTAKE-S2S-BiT512-ePLAC512` | timing init_a | `records/kex-03/CreTAKE-S2S-BiT512-ePLAC512__init_a.json` |
| `CreTAKE-S2S-BiT512-ePLAC512` | timing init_b | `records/kex-03/CreTAKE-S2S-BiT512-ePLAC512__init_b.json` |
| `CreTAKE-S2S-BiT512-ePLAC512` | timing pass1 | `records/kex-03/CreTAKE-S2S-BiT512-ePLAC512__pass1.json` |
| `CreTAKE-S2S-BiT512-ePLAC512` | timing pass2 | `records/kex-03/CreTAKE-S2S-BiT512-ePLAC512__pass2.json` |
| `CreTAKE-S2S-BiT512-ePLAC512` | hash profile exchange | `profile/kex-03/CreTAKE-S2S-BiT512-ePLAC512__exchange.json` |
| `CreTAKE-S2S-BiT512-eZEN512` | KAT log (sha256 `0255bcde72116a9f…`) | `kat/kex-03/CreTAKE-S2S-BiT512-eZEN512.log` |
| `CreTAKE-S2S-BiT512-eZEN512` | timing derive_a | `records/kex-03/CreTAKE-S2S-BiT512-eZEN512__derive_a.json` |
| `CreTAKE-S2S-BiT512-eZEN512` | timing derive_b | `records/kex-03/CreTAKE-S2S-BiT512-eZEN512__derive_b.json` |
| `CreTAKE-S2S-BiT512-eZEN512` | timing exchange | `records/kex-03/CreTAKE-S2S-BiT512-eZEN512__exchange.json` |
| `CreTAKE-S2S-BiT512-eZEN512` | timing init_a | `records/kex-03/CreTAKE-S2S-BiT512-eZEN512__init_a.json` |
| `CreTAKE-S2S-BiT512-eZEN512` | timing init_b | `records/kex-03/CreTAKE-S2S-BiT512-eZEN512__init_b.json` |
| `CreTAKE-S2S-BiT512-eZEN512` | timing pass1 | `records/kex-03/CreTAKE-S2S-BiT512-eZEN512__pass1.json` |
| `CreTAKE-S2S-BiT512-eZEN512` | timing pass2 | `records/kex-03/CreTAKE-S2S-BiT512-eZEN512__pass2.json` |
| `CreTAKE-S2S-BiT512-eZEN512` | hash profile exchange | `profile/kex-03/CreTAKE-S2S-BiT512-eZEN512__exchange.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

