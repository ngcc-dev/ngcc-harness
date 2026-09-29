# hash-20 Mozi Hash Function — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-20` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101537013422968832.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: Mozi Hash Function
- Implementation versions measured: reference
- Parameter sets: `MOZI-384`, `MOZI-384-XOF-256`, `MOZI-384-XOF-1024`, `MOZI-384-XOF-2048`, `MOZI-512`, `MOZI-512-XOF-256`, `MOZI-512-XOF-1024`, `MOZI-512-XOF-2048`, `MOZI-768`, `MOZI-1024`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-20/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `MOZI-384` | guide | PASS |
| `MOZI-384-XOF-256` | guide | PASS |
| `MOZI-384-XOF-1024` | guide | PASS |
| `MOZI-384-XOF-2048` | guide | PASS |
| `MOZI-512` | guide | PASS |
| `MOZI-512-XOF-256` | guide | PASS |
| `MOZI-512-XOF-1024` | guide | PASS |
| `MOZI-512-XOF-2048` | guide | PASS |
| `MOZI-768` | guide | PASS |
| `MOZI-1024` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `MOZI-384` | 32 B | 2350 | 73.4 | 887 ns | 36.1 | 100000 (5 × 20000) |
| `MOZI-384` | 128 B | 2393 | 18.7 | 903 ns | 141.7 | 100000 (5 × 20000) |
| `MOZI-384` | 512 B | 9378 | 18.3 | 3.51 µs | 145.9 | 100000 (5 × 20000) |
| `MOZI-384` | 1024 B | 16.0 k | 15.6 | 5.96 µs | 171.7 | 100000 (5 × 20000) |
| `MOZI-384` | 4096 B | 59.1 k | 14.4 | 22 µs | 186.1 | 100000 (5 × 20000) |
| `MOZI-384` | 8192 B | 118.1 k | 14.4 | 44 µs | 186.3 | 66115 (5 × 13223) |
| `MOZI-384` | 16384 B | 233.3 k | 14.2 | 86.8 µs | 188.7 | 34460 (5 × 6892) |
| `MOZI-384` | 65536 B | 956.5 k | 14.6 | 356 µs | 184.1 | 8590 (5 × 1718) |
| `MOZI-384-XOF-256` | 32 B | 2485 | 77.7 | 936 ns | 34.2 | 100000 (5 × 20000) |
| `MOZI-384-XOF-256` | 128 B | 2482 | 19.4 | 941 ns | 136.1 | 100000 (5 × 20000) |
| `MOZI-384-XOF-256` | 512 B | 9756 | 19.1 | 3.64 µs | 140.7 | 100000 (5 × 20000) |
| `MOZI-384-XOF-256` | 1024 B | 16.7 k | 16.3 | 6.21 µs | 164.8 | 100000 (5 × 20000) |
| `MOZI-384-XOF-256` | 4096 B | 61.6 k | 15.0 | 22.9 µs | 178.7 | 100000 (5 × 20000) |
| `MOZI-384-XOF-256` | 8192 B | 123.1 k | 15.0 | 45.8 µs | 178.8 | 62995 (5 × 12599) |
| `MOZI-384-XOF-256` | 16384 B | 247.3 k | 15.1 | 92.1 µs | 177.9 | 32655 (5 × 6531) |
| `MOZI-384-XOF-256` | 65536 B | 990.7 k | 15.1 | 369 µs | 177.8 | 8395 (5 × 1679) |
| `MOZI-384-XOF-1024` | 32 B | 2707 | 84.6 | 1.02 µs | 31.4 | 100000 (5 × 20000) |
| `MOZI-384-XOF-1024` | 128 B | 2786 | 21.8 | 1.06 µs | 120.6 | 100000 (5 × 20000) |
| `MOZI-384-XOF-1024` | 512 B | 9768 | 19.1 | 3.65 µs | 140.1 | 100000 (5 × 20000) |
| `MOZI-384-XOF-1024` | 1024 B | 16.9 k | 16.5 | 6.3 µs | 162.6 | 100000 (5 × 20000) |
| `MOZI-384-XOF-1024` | 4096 B | 63.0 k | 15.4 | 23.4 µs | 174.7 | 100000 (5 × 20000) |
| `MOZI-384-XOF-1024` | 8192 B | 123.4 k | 15.1 | 46 µs | 178.0 | 62995 (5 × 12599) |
| `MOZI-384-XOF-1024` | 16384 B | 243.1 k | 14.8 | 90.5 µs | 181.1 | 31645 (5 × 6329) |
| `MOZI-384-XOF-1024` | 65536 B | 969.2 k | 14.8 | 361 µs | 181.6 | 7610 (5 × 1522) |
| `MOZI-384-XOF-2048` | 32 B | 5286 | 165.2 | 1.98 µs | 16.1 | 100000 (5 × 20000) |
| `MOZI-384-XOF-2048` | 128 B | 5290 | 41.3 | 1.98 µs | 64.5 | 100000 (5 × 20000) |
| `MOZI-384-XOF-2048` | 512 B | 12.3 k | 24.1 | 4.61 µs | 111.1 | 100000 (5 × 20000) |
| `MOZI-384-XOF-2048` | 1024 B | 19.5 k | 19.0 | 7.28 µs | 140.6 | 100000 (5 × 20000) |
| `MOZI-384-XOF-2048` | 4096 B | 65.7 k | 16.0 | 24.4 µs | 167.6 | 100000 (5 × 20000) |
| `MOZI-384-XOF-2048` | 8192 B | 125.9 k | 15.4 | 47 µs | 174.3 | 55690 (5 × 11138) |
| `MOZI-384-XOF-2048` | 16384 B | 252.1 k | 15.4 | 93.8 µs | 174.7 | 28730 (5 × 5746) |
| `MOZI-384-XOF-2048` | 65536 B | 971.9 k | 14.8 | 362 µs | 181.1 | 8350 (5 × 1670) |
| `MOZI-512` | 32 B | 2331 | 72.8 | 882 ns | 36.3 | 100000 (5 × 20000) |
| `MOZI-512` | 128 B | 4617 | 36.1 | 1.73 µs | 74.0 | 100000 (5 × 20000) |
| `MOZI-512` | 512 B | 11.4 k | 22.2 | 4.25 µs | 120.4 | 100000 (5 × 20000) |
| `MOZI-512` | 1024 B | 21.6 k | 21.1 | 8.03 µs | 127.6 | 100000 (5 × 20000) |
| `MOZI-512` | 4096 B | 74.8 k | 18.3 | 27.8 µs | 147.1 | 83770 (5 × 16754) |
| `MOZI-512` | 8192 B | 147.5 k | 18.0 | 54.9 µs | 149.3 | 53515 (5 × 10703) |
| `MOZI-512` | 16384 B | 298.7 k | 18.2 | 111 µs | 147.4 | 27635 (5 × 5527) |
| `MOZI-512` | 65536 B | 1.16 M | 17.7 | 432 µs | 151.5 | 7010 (5 × 1402) |
| `MOZI-512-XOF-256` | 32 B | 2478 | 77.4 | 939 ns | 34.1 | 100000 (5 × 20000) |
| `MOZI-512-XOF-256` | 128 B | 4858 | 38.0 | 1.82 µs | 70.5 | 100000 (5 × 20000) |
| `MOZI-512-XOF-256` | 512 B | 12.1 k | 23.7 | 4.53 µs | 113.1 | 100000 (5 × 20000) |
| `MOZI-512-XOF-256` | 1024 B | 21.3 k | 20.8 | 7.95 µs | 128.9 | 100000 (5 × 20000) |
| `MOZI-512-XOF-256` | 4096 B | 78.0 k | 19.0 | 29 µs | 141.2 | 95240 (5 × 19048) |
| `MOZI-512-XOF-256` | 8192 B | 153.5 k | 18.7 | 57.1 µs | 143.5 | 46925 (5 × 9385) |
| `MOZI-512-XOF-256` | 16384 B | 310.1 k | 18.9 | 115 µs | 142.0 | 25440 (5 × 5088) |
| `MOZI-512-XOF-256` | 65536 B | 1.26 M | 19.3 | 470 µs | 139.5 | 6100 (5 × 1220) |
| `MOZI-512-XOF-1024` | 32 B | 2707 | 84.6 | 1.02 µs | 31.3 | 100000 (5 × 20000) |
| `MOZI-512-XOF-1024` | 128 B | 5088 | 39.7 | 1.91 µs | 66.9 | 100000 (5 × 20000) |
| `MOZI-512-XOF-1024` | 512 B | 12.6 k | 24.6 | 4.71 µs | 108.6 | 100000 (5 × 20000) |
| `MOZI-512-XOF-1024` | 1024 B | 21.6 k | 21.1 | 8.05 µs | 127.2 | 100000 (5 × 20000) |
| `MOZI-512-XOF-1024` | 4096 B | 78.2 k | 19.1 | 29.1 µs | 140.7 | 93570 (5 × 18714) |
| `MOZI-512-XOF-1024` | 8192 B | 153.7 k | 18.8 | 57.3 µs | 142.9 | 50690 (5 × 10138) |
| `MOZI-512-XOF-1024` | 16384 B | 304.5 k | 18.6 | 113 µs | 144.4 | 26375 (5 × 5275) |
| `MOZI-512-XOF-1024` | 65536 B | 1.21 M | 18.5 | 450 µs | 145.6 | 6645 (5 × 1329) |
| `MOZI-512-XOF-2048` | 32 B | 5399 | 168.7 | 2.02 µs | 15.9 | 100000 (5 × 20000) |
| `MOZI-512-XOF-2048` | 128 B | 7656 | 59.8 | 2.86 µs | 44.8 | 100000 (5 × 20000) |
| `MOZI-512-XOF-2048` | 512 B | 15.0 k | 29.2 | 5.58 µs | 91.8 | 100000 (5 × 20000) |
| `MOZI-512-XOF-2048` | 1024 B | 24.7 k | 24.1 | 9.18 µs | 111.5 | 100000 (5 × 20000) |
| `MOZI-512-XOF-2048` | 4096 B | 82.5 k | 20.1 | 30.7 µs | 133.5 | 90910 (5 × 18182) |
| `MOZI-512-XOF-2048` | 8192 B | 156.3 k | 19.1 | 58.1 µs | 140.9 | 50210 (5 × 10042) |
| `MOZI-512-XOF-2048` | 16384 B | 314.3 k | 19.2 | 117 µs | 140.2 | 25920 (5 × 5184) |
| `MOZI-512-XOF-2048` | 65536 B | 1.21 M | 18.5 | 451 µs | 145.3 | 6595 (5 × 1319) |
| `MOZI-768` | 32 B | 2747 | 85.9 | 1.03 µs | 31.0 | 100000 (5 × 20000) |
| `MOZI-768` | 128 B | 2916 | 22.8 | 1.1 µs | 116.6 | 100000 (5 × 20000) |
| `MOZI-768` | 512 B | 11.0 k | 21.6 | 4.12 µs | 124.3 | 100000 (5 × 20000) |
| `MOZI-768` | 1024 B | 18.8 k | 18.3 | 6.99 µs | 146.5 | 100000 (5 × 20000) |
| `MOZI-768` | 4096 B | 72.3 k | 17.6 | 26.9 µs | 152.3 | 92130 (5 × 18426) |
| `MOZI-768` | 8192 B | 152.0 k | 18.6 | 56.6 µs | 144.8 | 52405 (5 × 10481) |
| `MOZI-768` | 16384 B | 287.5 k | 17.5 | 107 µs | 153.2 | 25120 (5 × 5024) |
| `MOZI-768` | 65536 B | 1.15 M | 17.6 | 428 µs | 153.0 | 7125 (5 × 1425) |
| `MOZI-1024` | 32 B | 2742 | 85.7 | 1.03 µs | 31.0 | 100000 (5 × 20000) |
| `MOZI-1024` | 128 B | 5574 | 43.5 | 2.09 µs | 61.4 | 100000 (5 × 20000) |
| `MOZI-1024` | 512 B | 13.8 k | 27.0 | 5.15 µs | 99.4 | 100000 (5 × 20000) |
| `MOZI-1024` | 1024 B | 24.1 k | 23.5 | 8.97 µs | 114.2 | 100000 (5 × 20000) |
| `MOZI-1024` | 4096 B | 93.5 k | 22.8 | 34.8 µs | 117.8 | 83625 (5 × 16725) |
| `MOZI-1024` | 8192 B | 189.6 k | 23.1 | 70.5 µs | 116.2 | 43600 (5 × 8720) |
| `MOZI-1024` | 16384 B | 364.7 k | 22.3 | 136 µs | 120.7 | 14800 (5 × 2960) |
| `MOZI-1024` | 65536 B | 1.46 M | 22.2 | 542 µs | 120.9 | 5610 (5 × 1122) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `MOZI-384` | hash_32 | 11380 | 1400 KiB | 6392 KiB |
| `MOZI-384` | hash_128 | 11380 | 1400 KiB | 6176 KiB |
| `MOZI-384` | hash_512 | 11380 | 1400 KiB | 16204 KiB |
| `MOZI-384` | hash_1024 | 11380 | 1404 KiB | 25624 KiB |
| `MOZI-384` | hash_4096 | 11380 | 1404 KiB | 84760 KiB |
| `MOZI-384` | hash_8192 | 11380 | 1408 KiB | 111004 KiB |
| `MOZI-384` | hash_16384 | 11380 | 1416 KiB | 114428 KiB |
| `MOZI-384` | hash_65536 | 11380 | 1464 KiB | 113792 KiB |
| `MOZI-384-XOF-256` | hash_32 | 11644 | 1400 KiB | 6916 KiB |
| `MOZI-384-XOF-256` | hash_128 | 11644 | 1400 KiB | 6932 KiB |
| `MOZI-384-XOF-256` | hash_512 | 11644 | 1400 KiB | 16172 KiB |
| `MOZI-384-XOF-256` | hash_1024 | 11644 | 1404 KiB | 25412 KiB |
| `MOZI-384-XOF-256` | hash_4096 | 11644 | 1400 KiB | 84636 KiB |
| `MOZI-384-XOF-256` | hash_8192 | 11644 | 1408 KiB | 105688 KiB |
| `MOZI-384-XOF-256` | hash_16384 | 11644 | 1416 KiB | 108740 KiB |
| `MOZI-384-XOF-256` | hash_65536 | 11644 | 1464 KiB | 111004 KiB |
| `MOZI-384-XOF-1024` | hash_32 | 11644 | 1400 KiB | 6528 KiB |
| `MOZI-384-XOF-1024` | hash_128 | 11644 | 1400 KiB | 6904 KiB |
| `MOZI-384-XOF-1024` | hash_512 | 11644 | 1400 KiB | 16072 KiB |
| `MOZI-384-XOF-1024` | hash_1024 | 11644 | 1404 KiB | 25504 KiB |
| `MOZI-384-XOF-1024` | hash_4096 | 11644 | 1404 KiB | 84640 KiB |
| `MOZI-384-XOF-1024` | hash_8192 | 11644 | 1408 KiB | 106020 KiB |
| `MOZI-384-XOF-1024` | hash_16384 | 11644 | 1416 KiB | 104972 KiB |
| `MOZI-384-XOF-1024` | hash_65536 | 11644 | 1464 KiB | 101164 KiB |
| `MOZI-384-XOF-2048` | hash_32 | 11644 | 1400 KiB | 6580 KiB |
| `MOZI-384-XOF-2048` | hash_128 | 11644 | 1400 KiB | 6624 KiB |
| `MOZI-384-XOF-2048` | hash_512 | 11644 | 1400 KiB | 16132 KiB |
| `MOZI-384-XOF-2048` | hash_1024 | 11644 | 1404 KiB | 25620 KiB |
| `MOZI-384-XOF-2048` | hash_4096 | 11644 | 1404 KiB | 84640 KiB |
| `MOZI-384-XOF-2048` | hash_8192 | 11644 | 1408 KiB | 93592 KiB |
| `MOZI-384-XOF-2048` | hash_16384 | 11644 | 1412 KiB | 96076 KiB |
| `MOZI-384-XOF-2048` | hash_65536 | 11644 | 1464 KiB | 109624 KiB |
| `MOZI-512` | hash_32 | 11364 | 1400 KiB | 6200 KiB |
| `MOZI-512` | hash_128 | 11364 | 1400 KiB | 8364 KiB |
| `MOZI-512` | hash_512 | 11364 | 1400 KiB | 15804 KiB |
| `MOZI-512` | hash_1024 | 11364 | 1400 KiB | 25320 KiB |
| `MOZI-512` | hash_4096 | 11364 | 1404 KiB | 72572 KiB |
| `MOZI-512` | hash_8192 | 11364 | 1408 KiB | 89952 KiB |
| `MOZI-512` | hash_16384 | 11364 | 1416 KiB | 92308 KiB |
| `MOZI-512` | hash_65536 | 11364 | 1464 KiB | 93480 KiB |
| `MOZI-512-XOF-256` | hash_32 | 11644 | 1400 KiB | 5492 KiB |
| `MOZI-512-XOF-256` | hash_128 | 11644 | 3440 KiB | 7944 KiB |
| `MOZI-512-XOF-256` | hash_512 | 11644 | 1400 KiB | 16312 KiB |
| `MOZI-512-XOF-256` | hash_1024 | 11644 | 1404 KiB | 24996 KiB |
| `MOZI-512-XOF-256` | hash_4096 | 11644 | 1404 KiB | 82304 KiB |
| `MOZI-512-XOF-256` | hash_8192 | 11644 | 1408 KiB | 79812 KiB |
| `MOZI-512-XOF-256` | hash_16384 | 11644 | 1416 KiB | 85388 KiB |
| `MOZI-512-XOF-256` | hash_65536 | 11644 | 3464 KiB | 81868 KiB |
| `MOZI-512-XOF-1024` | hash_32 | 11644 | 1400 KiB | 5212 KiB |
| `MOZI-512-XOF-1024` | hash_128 | 11644 | 1400 KiB | 8076 KiB |
| `MOZI-512-XOF-1024` | hash_512 | 11644 | 1396 KiB | 16040 KiB |
| `MOZI-512-XOF-1024` | hash_1024 | 11644 | 1404 KiB | 26068 KiB |
| `MOZI-512-XOF-1024` | hash_4096 | 11644 | 3444 KiB | 80188 KiB |
| `MOZI-512-XOF-1024` | hash_8192 | 11644 | 1408 KiB | 85224 KiB |
| `MOZI-512-XOF-1024` | hash_16384 | 11644 | 3444 KiB | 88468 KiB |
| `MOZI-512-XOF-1024` | hash_65536 | 11644 | 3472 KiB | 88636 KiB |
| `MOZI-512-XOF-2048` | hash_32 | 11644 | 1400 KiB | 6168 KiB |
| `MOZI-512-XOF-2048` | hash_128 | 11644 | 1400 KiB | 8796 KiB |
| `MOZI-512-XOF-2048` | hash_512 | 11644 | 1400 KiB | 16264 KiB |
| `MOZI-512-XOF-2048` | hash_1024 | 11644 | 1404 KiB | 25904 KiB |
| `MOZI-512-XOF-2048` | hash_4096 | 11644 | 1400 KiB | 78368 KiB |
| `MOZI-512-XOF-2048` | hash_8192 | 11644 | 1408 KiB | 84884 KiB |
| `MOZI-512-XOF-2048` | hash_16384 | 11644 | 1416 KiB | 87200 KiB |
| `MOZI-512-XOF-2048` | hash_65536 | 11644 | 3508 KiB | 88188 KiB |
| `MOZI-768` | hash_32 | 11692 | 1400 KiB | 6356 KiB |
| `MOZI-768` | hash_128 | 11692 | 1400 KiB | 5888 KiB |
| `MOZI-768` | hash_512 | 11692 | 1396 KiB | 15432 KiB |
| `MOZI-768` | hash_1024 | 11692 | 1404 KiB | 24072 KiB |
| `MOZI-768` | hash_4096 | 11692 | 1404 KiB | 77152 KiB |
| `MOZI-768` | hash_8192 | 11692 | 1408 KiB | 87084 KiB |
| `MOZI-768` | hash_16384 | 11692 | 1416 KiB | 83548 KiB |
| `MOZI-768` | hash_65536 | 11692 | 1464 KiB | 94892 KiB |
| `MOZI-1024` | hash_32 | 11708 | 1400 KiB | 4752 KiB |
| `MOZI-1024` | hash_128 | 11708 | 1400 KiB | 8472 KiB |
| `MOZI-1024` | hash_512 | 11708 | 1400 KiB | 15264 KiB |
| `MOZI-1024` | hash_1024 | 11708 | 1404 KiB | 24536 KiB |
| `MOZI-1024` | hash_4096 | 11708 | 1404 KiB | 72188 KiB |
| `MOZI-1024` | hash_8192 | 11708 | 1408 KiB | 73752 KiB |
| `MOZI-1024` | hash_16384 | 11708 | 1416 KiB | 50956 KiB |
| `MOZI-1024` | hash_65536 | 11708 | 1464 KiB | 75004 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `MOZI-384` | KAT log (sha256 `e3f5cacbd8355fb3…`) | `kat/hash-20/MOZI-384.log` |
| `MOZI-384` | timing hash_1024 | `records/hash-20/MOZI-384__hash_1024.json` |
| `MOZI-384` | timing hash_128 | `records/hash-20/MOZI-384__hash_128.json` |
| `MOZI-384` | timing hash_16384 | `records/hash-20/MOZI-384__hash_16384.json` |
| `MOZI-384` | timing hash_32 | `records/hash-20/MOZI-384__hash_32.json` |
| `MOZI-384` | timing hash_4096 | `records/hash-20/MOZI-384__hash_4096.json` |
| `MOZI-384` | timing hash_512 | `records/hash-20/MOZI-384__hash_512.json` |
| `MOZI-384` | timing hash_65536 | `records/hash-20/MOZI-384__hash_65536.json` |
| `MOZI-384` | timing hash_8192 | `records/hash-20/MOZI-384__hash_8192.json` |
| `MOZI-384-XOF-256` | KAT log (sha256 `90a0db86ea98fb91…`) | `kat/hash-20/MOZI-384-XOF-256.log` |
| `MOZI-384-XOF-256` | timing hash_1024 | `records/hash-20/MOZI-384-XOF-256__hash_1024.json` |
| `MOZI-384-XOF-256` | timing hash_128 | `records/hash-20/MOZI-384-XOF-256__hash_128.json` |
| `MOZI-384-XOF-256` | timing hash_16384 | `records/hash-20/MOZI-384-XOF-256__hash_16384.json` |
| `MOZI-384-XOF-256` | timing hash_32 | `records/hash-20/MOZI-384-XOF-256__hash_32.json` |
| `MOZI-384-XOF-256` | timing hash_4096 | `records/hash-20/MOZI-384-XOF-256__hash_4096.json` |
| `MOZI-384-XOF-256` | timing hash_512 | `records/hash-20/MOZI-384-XOF-256__hash_512.json` |
| `MOZI-384-XOF-256` | timing hash_65536 | `records/hash-20/MOZI-384-XOF-256__hash_65536.json` |
| `MOZI-384-XOF-256` | timing hash_8192 | `records/hash-20/MOZI-384-XOF-256__hash_8192.json` |
| `MOZI-384-XOF-1024` | KAT log (sha256 `989ffb66a772ebc0…`) | `kat/hash-20/MOZI-384-XOF-1024.log` |
| `MOZI-384-XOF-1024` | timing hash_1024 | `records/hash-20/MOZI-384-XOF-1024__hash_1024.json` |
| `MOZI-384-XOF-1024` | timing hash_128 | `records/hash-20/MOZI-384-XOF-1024__hash_128.json` |
| `MOZI-384-XOF-1024` | timing hash_16384 | `records/hash-20/MOZI-384-XOF-1024__hash_16384.json` |
| `MOZI-384-XOF-1024` | timing hash_32 | `records/hash-20/MOZI-384-XOF-1024__hash_32.json` |
| `MOZI-384-XOF-1024` | timing hash_4096 | `records/hash-20/MOZI-384-XOF-1024__hash_4096.json` |
| `MOZI-384-XOF-1024` | timing hash_512 | `records/hash-20/MOZI-384-XOF-1024__hash_512.json` |
| `MOZI-384-XOF-1024` | timing hash_65536 | `records/hash-20/MOZI-384-XOF-1024__hash_65536.json` |
| `MOZI-384-XOF-1024` | timing hash_8192 | `records/hash-20/MOZI-384-XOF-1024__hash_8192.json` |
| `MOZI-384-XOF-2048` | KAT log (sha256 `091ef026687c9596…`) | `kat/hash-20/MOZI-384-XOF-2048.log` |
| `MOZI-384-XOF-2048` | timing hash_1024 | `records/hash-20/MOZI-384-XOF-2048__hash_1024.json` |
| `MOZI-384-XOF-2048` | timing hash_128 | `records/hash-20/MOZI-384-XOF-2048__hash_128.json` |
| `MOZI-384-XOF-2048` | timing hash_16384 | `records/hash-20/MOZI-384-XOF-2048__hash_16384.json` |
| `MOZI-384-XOF-2048` | timing hash_32 | `records/hash-20/MOZI-384-XOF-2048__hash_32.json` |
| `MOZI-384-XOF-2048` | timing hash_4096 | `records/hash-20/MOZI-384-XOF-2048__hash_4096.json` |
| `MOZI-384-XOF-2048` | timing hash_512 | `records/hash-20/MOZI-384-XOF-2048__hash_512.json` |
| `MOZI-384-XOF-2048` | timing hash_65536 | `records/hash-20/MOZI-384-XOF-2048__hash_65536.json` |
| `MOZI-384-XOF-2048` | timing hash_8192 | `records/hash-20/MOZI-384-XOF-2048__hash_8192.json` |
| `MOZI-512` | KAT log (sha256 `bf18e6d6b6789188…`) | `kat/hash-20/MOZI-512.log` |
| `MOZI-512` | timing hash_1024 | `records/hash-20/MOZI-512__hash_1024.json` |
| `MOZI-512` | timing hash_128 | `records/hash-20/MOZI-512__hash_128.json` |
| `MOZI-512` | timing hash_16384 | `records/hash-20/MOZI-512__hash_16384.json` |
| `MOZI-512` | timing hash_32 | `records/hash-20/MOZI-512__hash_32.json` |
| `MOZI-512` | timing hash_4096 | `records/hash-20/MOZI-512__hash_4096.json` |
| `MOZI-512` | timing hash_512 | `records/hash-20/MOZI-512__hash_512.json` |
| `MOZI-512` | timing hash_65536 | `records/hash-20/MOZI-512__hash_65536.json` |
| `MOZI-512` | timing hash_8192 | `records/hash-20/MOZI-512__hash_8192.json` |
| `MOZI-512-XOF-256` | KAT log (sha256 `e1eee2323153b9a1…`) | `kat/hash-20/MOZI-512-XOF-256.log` |
| `MOZI-512-XOF-256` | timing hash_1024 | `records/hash-20/MOZI-512-XOF-256__hash_1024.json` |
| `MOZI-512-XOF-256` | timing hash_128 | `records/hash-20/MOZI-512-XOF-256__hash_128.json` |
| `MOZI-512-XOF-256` | timing hash_16384 | `records/hash-20/MOZI-512-XOF-256__hash_16384.json` |
| `MOZI-512-XOF-256` | timing hash_32 | `records/hash-20/MOZI-512-XOF-256__hash_32.json` |
| `MOZI-512-XOF-256` | timing hash_4096 | `records/hash-20/MOZI-512-XOF-256__hash_4096.json` |
| `MOZI-512-XOF-256` | timing hash_512 | `records/hash-20/MOZI-512-XOF-256__hash_512.json` |
| `MOZI-512-XOF-256` | timing hash_65536 | `records/hash-20/MOZI-512-XOF-256__hash_65536.json` |
| `MOZI-512-XOF-256` | timing hash_8192 | `records/hash-20/MOZI-512-XOF-256__hash_8192.json` |
| `MOZI-512-XOF-1024` | KAT log (sha256 `ed9d555f5f26ffa7…`) | `kat/hash-20/MOZI-512-XOF-1024.log` |
| `MOZI-512-XOF-1024` | timing hash_1024 | `records/hash-20/MOZI-512-XOF-1024__hash_1024.json` |
| `MOZI-512-XOF-1024` | timing hash_128 | `records/hash-20/MOZI-512-XOF-1024__hash_128.json` |
| `MOZI-512-XOF-1024` | timing hash_16384 | `records/hash-20/MOZI-512-XOF-1024__hash_16384.json` |
| `MOZI-512-XOF-1024` | timing hash_32 | `records/hash-20/MOZI-512-XOF-1024__hash_32.json` |
| `MOZI-512-XOF-1024` | timing hash_4096 | `records/hash-20/MOZI-512-XOF-1024__hash_4096.json` |
| `MOZI-512-XOF-1024` | timing hash_512 | `records/hash-20/MOZI-512-XOF-1024__hash_512.json` |
| `MOZI-512-XOF-1024` | timing hash_65536 | `records/hash-20/MOZI-512-XOF-1024__hash_65536.json` |
| `MOZI-512-XOF-1024` | timing hash_8192 | `records/hash-20/MOZI-512-XOF-1024__hash_8192.json` |
| `MOZI-512-XOF-2048` | KAT log (sha256 `74e767b30edefff5…`) | `kat/hash-20/MOZI-512-XOF-2048.log` |
| `MOZI-512-XOF-2048` | timing hash_1024 | `records/hash-20/MOZI-512-XOF-2048__hash_1024.json` |
| `MOZI-512-XOF-2048` | timing hash_128 | `records/hash-20/MOZI-512-XOF-2048__hash_128.json` |
| `MOZI-512-XOF-2048` | timing hash_16384 | `records/hash-20/MOZI-512-XOF-2048__hash_16384.json` |
| `MOZI-512-XOF-2048` | timing hash_32 | `records/hash-20/MOZI-512-XOF-2048__hash_32.json` |
| `MOZI-512-XOF-2048` | timing hash_4096 | `records/hash-20/MOZI-512-XOF-2048__hash_4096.json` |
| `MOZI-512-XOF-2048` | timing hash_512 | `records/hash-20/MOZI-512-XOF-2048__hash_512.json` |
| `MOZI-512-XOF-2048` | timing hash_65536 | `records/hash-20/MOZI-512-XOF-2048__hash_65536.json` |
| `MOZI-512-XOF-2048` | timing hash_8192 | `records/hash-20/MOZI-512-XOF-2048__hash_8192.json` |
| `MOZI-768` | KAT log (sha256 `49cb7b0643a13848…`) | `kat/hash-20/MOZI-768.log` |
| `MOZI-768` | timing hash_1024 | `records/hash-20/MOZI-768__hash_1024.json` |
| `MOZI-768` | timing hash_128 | `records/hash-20/MOZI-768__hash_128.json` |
| `MOZI-768` | timing hash_16384 | `records/hash-20/MOZI-768__hash_16384.json` |
| `MOZI-768` | timing hash_32 | `records/hash-20/MOZI-768__hash_32.json` |
| `MOZI-768` | timing hash_4096 | `records/hash-20/MOZI-768__hash_4096.json` |
| `MOZI-768` | timing hash_512 | `records/hash-20/MOZI-768__hash_512.json` |
| `MOZI-768` | timing hash_65536 | `records/hash-20/MOZI-768__hash_65536.json` |
| `MOZI-768` | timing hash_8192 | `records/hash-20/MOZI-768__hash_8192.json` |
| `MOZI-1024` | KAT log (sha256 `48ba6e860bf244da…`) | `kat/hash-20/MOZI-1024.log` |
| `MOZI-1024` | timing hash_1024 | `records/hash-20/MOZI-1024__hash_1024.json` |
| `MOZI-1024` | timing hash_128 | `records/hash-20/MOZI-1024__hash_128.json` |
| `MOZI-1024` | timing hash_16384 | `records/hash-20/MOZI-1024__hash_16384.json` |
| `MOZI-1024` | timing hash_32 | `records/hash-20/MOZI-1024__hash_32.json` |
| `MOZI-1024` | timing hash_4096 | `records/hash-20/MOZI-1024__hash_4096.json` |
| `MOZI-1024` | timing hash_512 | `records/hash-20/MOZI-1024__hash_512.json` |
| `MOZI-1024` | timing hash_65536 | `records/hash-20/MOZI-1024__hash_65536.json` |
| `MOZI-1024` | timing hash_8192 | `records/hash-20/MOZI-1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

