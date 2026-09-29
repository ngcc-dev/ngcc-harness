# hash-18 Megascon Hash Function — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-18` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101537413073031168.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: Megascon Hash Function
- Implementation versions measured: reference
- Parameter sets: `MEGASCON-384`, `MEGASCON-384-XOF-256`, `MEGASCON-384-XOF-1024`, `MEGASCON-384-XOF-2048`, `MEGASCON-512`, `MEGASCON-512-XOF-256`, `MEGASCON-512-XOF-1024`, `MEGASCON-512-XOF-2048`, `MEGASCON-768`, `MEGASCON-1024`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-18/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `MEGASCON-384` | guide | PASS |
| `MEGASCON-384-XOF-256` | guide | PASS |
| `MEGASCON-384-XOF-1024` | guide | PASS |
| `MEGASCON-384-XOF-2048` | guide | PASS |
| `MEGASCON-512` | guide | PASS |
| `MEGASCON-512-XOF-256` | guide | PASS |
| `MEGASCON-512-XOF-1024` | guide | PASS |
| `MEGASCON-512-XOF-2048` | guide | PASS |
| `MEGASCON-768` | guide | PASS |
| `MEGASCON-1024` | guide | PASS |

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `MEGASCON-384` | 32 B | 2084 | 65.1 | 789 ns | 40.6 | 100000 (5 × 20000) |
| `MEGASCON-384` | 128 B | 2093 | 16.3 | 789 ns | 162.3 | 100000 (5 × 20000) |
| `MEGASCON-384` | 512 B | 8094 | 15.8 | 3.02 µs | 169.3 | 100000 (5 × 20000) |
| `MEGASCON-384` | 1024 B | 13.8 k | 13.5 | 5.15 µs | 198.7 | 100000 (5 × 20000) |
| `MEGASCON-384` | 4096 B | 51.3 k | 12.5 | 19.1 µs | 214.4 | 100000 (5 × 20000) |
| `MEGASCON-384` | 8192 B | 102.1 k | 12.5 | 38 µs | 215.4 | 78820 (5 × 15764) |
| `MEGASCON-384` | 16384 B | 199.8 k | 12.2 | 74.4 µs | 220.2 | 40205 (5 × 8041) |
| `MEGASCON-384` | 65536 B | 801.9 k | 12.2 | 299 µs | 219.5 | 10140 (5 × 2028) |
| `MEGASCON-384-XOF-256` | 32 B | 2200 | 68.8 | 834 ns | 38.3 | 100000 (5 × 20000) |
| `MEGASCON-384-XOF-256` | 128 B | 2163 | 16.9 | 819 ns | 156.3 | 100000 (5 × 20000) |
| `MEGASCON-384-XOF-256` | 512 B | 7927 | 15.5 | 2.96 µs | 172.7 | 100000 (5 × 20000) |
| `MEGASCON-384-XOF-256` | 1024 B | 13.8 k | 13.4 | 5.14 µs | 199.3 | 100000 (5 × 20000) |
| `MEGASCON-384-XOF-256` | 4096 B | 50.9 k | 12.4 | 19 µs | 215.5 | 100000 (5 × 20000) |
| `MEGASCON-384-XOF-256` | 8192 B | 101.7 k | 12.4 | 38 µs | 215.8 | 80810 (5 × 16162) |
| `MEGASCON-384-XOF-256` | 16384 B | 200.0 k | 12.2 | 74.5 µs | 219.9 | 39705 (5 × 7941) |
| `MEGASCON-384-XOF-256` | 65536 B | 798.0 k | 12.2 | 298 µs | 220.0 | 9960 (5 × 1992) |
| `MEGASCON-384-XOF-1024` | 32 B | 2354 | 73.6 | 889 ns | 36.0 | 100000 (5 × 20000) |
| `MEGASCON-384-XOF-1024` | 128 B | 2308 | 18.0 | 867 ns | 147.6 | 100000 (5 × 20000) |
| `MEGASCON-384-XOF-1024` | 512 B | 8173 | 16.0 | 3.05 µs | 167.7 | 100000 (5 × 20000) |
| `MEGASCON-384-XOF-1024` | 1024 B | 14.0 k | 13.7 | 5.22 µs | 196.2 | 100000 (5 × 20000) |
| `MEGASCON-384-XOF-1024` | 4096 B | 51.1 k | 12.5 | 19 µs | 215.1 | 100000 (5 × 20000) |
| `MEGASCON-384-XOF-1024` | 8192 B | 104.0 k | 12.7 | 38.7 µs | 211.6 | 78435 (5 × 15687) |
| `MEGASCON-384-XOF-1024` | 16384 B | 200.0 k | 12.2 | 74.5 µs | 220.0 | 36925 (5 × 7385) |
| `MEGASCON-384-XOF-1024` | 65536 B | 810.1 k | 12.4 | 302 µs | 217.3 | 9930 (5 × 1986) |
| `MEGASCON-384-XOF-2048` | 32 B | 4512 | 141.0 | 1.69 µs | 18.9 | 100000 (5 × 20000) |
| `MEGASCON-384-XOF-2048` | 128 B | 4468 | 34.9 | 1.67 µs | 76.5 | 100000 (5 × 20000) |
| `MEGASCON-384-XOF-2048` | 512 B | 10.3 k | 20.0 | 3.83 µs | 133.6 | 100000 (5 × 20000) |
| `MEGASCON-384-XOF-2048` | 1024 B | 16.2 k | 15.8 | 6.03 µs | 169.9 | 100000 (5 × 20000) |
| `MEGASCON-384-XOF-2048` | 4096 B | 53.1 k | 13.0 | 19.8 µs | 207.1 | 100000 (5 × 20000) |
| `MEGASCON-384-XOF-2048` | 8192 B | 106.1 k | 12.9 | 39.5 µs | 207.4 | 77675 (5 × 15535) |
| `MEGASCON-384-XOF-2048` | 16384 B | 202.0 k | 12.3 | 75.2 µs | 217.8 | 39905 (5 × 7981) |
| `MEGASCON-384-XOF-2048` | 65536 B | 801.9 k | 12.2 | 299 µs | 219.4 | 7505 (5 × 1501) |
| `MEGASCON-512` | 32 B | 2169 | 67.8 | 830 ns | 38.6 | 100000 (5 × 20000) |
| `MEGASCON-512` | 128 B | 4086 | 31.9 | 1.53 µs | 83.4 | 100000 (5 × 20000) |
| `MEGASCON-512` | 512 B | 9982 | 19.5 | 3.74 µs | 137.0 | 100000 (5 × 20000) |
| `MEGASCON-512` | 1024 B | 17.7 k | 17.3 | 6.61 µs | 154.9 | 100000 (5 × 20000) |
| `MEGASCON-512` | 4096 B | 65.0 k | 15.9 | 24.2 µs | 169.3 | 100000 (5 × 20000) |
| `MEGASCON-512` | 8192 B | 131.3 k | 16.0 | 49 µs | 167.3 | 56405 (5 × 11281) |
| `MEGASCON-512` | 16384 B | 254.0 k | 15.5 | 94.8 µs | 172.8 | 32790 (5 × 6558) |
| `MEGASCON-512` | 65536 B | 1.03 M | 15.8 | 384 µs | 170.6 | 7675 (5 × 1535) |
| `MEGASCON-512-XOF-256` | 32 B | 2204 | 68.9 | 834 ns | 38.3 | 100000 (5 × 20000) |
| `MEGASCON-512-XOF-256` | 128 B | 4087 | 31.9 | 1.53 µs | 83.4 | 100000 (5 × 20000) |
| `MEGASCON-512-XOF-256` | 512 B | 10.1 k | 19.7 | 3.76 µs | 136.1 | 100000 (5 × 20000) |
| `MEGASCON-512-XOF-256` | 1024 B | 17.9 k | 17.5 | 6.67 µs | 153.5 | 100000 (5 × 20000) |
| `MEGASCON-512-XOF-256` | 4096 B | 66.7 k | 16.3 | 24.8 µs | 165.1 | 100000 (5 × 20000) |
| `MEGASCON-512-XOF-256` | 8192 B | 127.4 k | 15.6 | 47.4 µs | 172.7 | 64260 (5 × 12852) |
| `MEGASCON-512-XOF-256` | 16384 B | 254.1 k | 15.5 | 94.5 µs | 173.3 | 32810 (5 × 6562) |
| `MEGASCON-512-XOF-256` | 65536 B | 1.01 M | 15.4 | 375 µs | 174.6 | 8235 (5 × 1647) |
| `MEGASCON-512-XOF-1024` | 32 B | 2382 | 74.4 | 910 ns | 35.2 | 100000 (5 × 20000) |
| `MEGASCON-512-XOF-1024` | 128 B | 4563 | 35.6 | 1.72 µs | 74.5 | 100000 (5 × 20000) |
| `MEGASCON-512-XOF-1024` | 512 B | 10.4 k | 20.3 | 3.88 µs | 132.1 | 100000 (5 × 20000) |
| `MEGASCON-512-XOF-1024` | 1024 B | 18.1 k | 17.6 | 6.74 µs | 151.8 | 100000 (5 × 20000) |
| `MEGASCON-512-XOF-1024` | 4096 B | 65.3 k | 15.9 | 24.3 µs | 168.5 | 100000 (5 × 20000) |
| `MEGASCON-512-XOF-1024` | 8192 B | 131.4 k | 16.0 | 49 µs | 167.1 | 64520 (5 × 12904) |
| `MEGASCON-512-XOF-1024` | 16384 B | 253.5 k | 15.5 | 94.6 µs | 173.2 | 32655 (5 × 6531) |
| `MEGASCON-512-XOF-1024` | 65536 B | 1.01 M | 15.4 | 376 µs | 174.3 | 8070 (5 × 1614) |
| `MEGASCON-512-XOF-2048` | 32 B | 4523 | 141.4 | 1.7 µs | 18.9 | 100000 (5 × 20000) |
| `MEGASCON-512-XOF-2048` | 128 B | 6528 | 51.0 | 2.45 µs | 52.3 | 100000 (5 × 20000) |
| `MEGASCON-512-XOF-2048` | 512 B | 12.6 k | 24.6 | 4.71 µs | 108.8 | 100000 (5 × 20000) |
| `MEGASCON-512-XOF-2048` | 1024 B | 20.4 k | 19.9 | 7.6 µs | 134.8 | 100000 (5 × 20000) |
| `MEGASCON-512-XOF-2048` | 4096 B | 67.5 k | 16.5 | 25.1 µs | 163.1 | 100000 (5 × 20000) |
| `MEGASCON-512-XOF-2048` | 8192 B | 131.1 k | 16.0 | 48.9 µs | 167.4 | 60455 (5 × 12091) |
| `MEGASCON-512-XOF-2048` | 16384 B | 260.3 k | 15.9 | 97.1 µs | 168.7 | 31580 (5 × 6316) |
| `MEGASCON-512-XOF-2048` | 65536 B | 1.01 M | 15.4 | 377 µs | 173.9 | 8080 (5 × 1616) |
| `MEGASCON-768` | 32 B | 2355 | 73.6 | 892 ns | 35.9 | 100000 (5 × 20000) |
| `MEGASCON-768` | 128 B | 2322 | 18.1 | 879 ns | 145.6 | 100000 (5 × 20000) |
| `MEGASCON-768` | 512 B | 8895 | 17.4 | 3.33 µs | 154.0 | 100000 (5 × 20000) |
| `MEGASCON-768` | 1024 B | 15.9 k | 15.6 | 5.94 µs | 172.4 | 100000 (5 × 20000) |
| `MEGASCON-768` | 4096 B | 61.1 k | 14.9 | 22.7 µs | 180.2 | 100000 (5 × 20000) |
| `MEGASCON-768` | 8192 B | 122.0 k | 14.9 | 45.4 µs | 180.4 | 67040 (5 × 13408) |
| `MEGASCON-768` | 16384 B | 242.7 k | 14.8 | 90.3 µs | 181.4 | 34265 (5 × 6853) |
| `MEGASCON-768` | 65536 B | 978.2 k | 14.9 | 364 µs | 180.1 | 8265 (5 × 1653) |
| `MEGASCON-1024` | 32 B | 2313 | 72.3 | 871 ns | 36.8 | 100000 (5 × 20000) |
| `MEGASCON-1024` | 128 B | 4523 | 35.3 | 1.7 µs | 75.3 | 100000 (5 × 20000) |
| `MEGASCON-1024` | 512 B | 11.1 k | 21.7 | 4.16 µs | 123.1 | 100000 (5 × 20000) |
| `MEGASCON-1024` | 1024 B | 20.1 k | 19.7 | 7.5 µs | 136.6 | 100000 (5 × 20000) |
| `MEGASCON-1024` | 4096 B | 78.9 k | 19.3 | 29.3 µs | 139.6 | 100000 (5 × 20000) |
| `MEGASCON-1024` | 8192 B | 154.8 k | 18.9 | 57.6 µs | 142.3 | 53875 (5 × 10775) |
| `MEGASCON-1024` | 16384 B | 318.5 k | 19.4 | 118 µs | 138.3 | 27260 (5 × 5452) |
| `MEGASCON-1024` | 65536 B | 1.23 M | 18.7 | 457 µs | 143.5 | 6800 (5 × 1360) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `MEGASCON-384` | hash_32 | 11324 | 1400 KiB | 6936 KiB |
| `MEGASCON-384` | hash_128 | 11324 | 1400 KiB | 6276 KiB |
| `MEGASCON-384` | hash_512 | 11324 | 1396 KiB | 16248 KiB |
| `MEGASCON-384` | hash_1024 | 11324 | 1404 KiB | 25408 KiB |
| `MEGASCON-384` | hash_4096 | 11324 | 1404 KiB | 84680 KiB |
| `MEGASCON-384` | hash_8192 | 11324 | 1404 KiB | 131488 KiB |
| `MEGASCON-384` | hash_16384 | 11324 | 3432 KiB | 132820 KiB |
| `MEGASCON-384` | hash_65536 | 11324 | 1464 KiB | 133248 KiB |
| `MEGASCON-384-XOF-256` | hash_32 | 11556 | 1400 KiB | 6572 KiB |
| `MEGASCON-384-XOF-256` | hash_128 | 11556 | 1400 KiB | 6640 KiB |
| `MEGASCON-384-XOF-256` | hash_512 | 11556 | 1400 KiB | 16120 KiB |
| `MEGASCON-384-XOF-256` | hash_1024 | 11556 | 3440 KiB | 25500 KiB |
| `MEGASCON-384-XOF-256` | hash_4096 | 11556 | 1404 KiB | 84672 KiB |
| `MEGASCON-384-XOF-256` | hash_8192 | 11556 | 1408 KiB | 134304 KiB |
| `MEGASCON-384-XOF-256` | hash_16384 | 11556 | 1416 KiB | 131324 KiB |
| `MEGASCON-384-XOF-256` | hash_65536 | 11556 | 1464 KiB | 130604 KiB |
| `MEGASCON-384-XOF-1024` | hash_32 | 11572 | 1400 KiB | 6436 KiB |
| `MEGASCON-384-XOF-1024` | hash_128 | 11572 | 1400 KiB | 6216 KiB |
| `MEGASCON-384-XOF-1024` | hash_512 | 11572 | 1396 KiB | 16248 KiB |
| `MEGASCON-384-XOF-1024` | hash_1024 | 11572 | 1404 KiB | 25384 KiB |
| `MEGASCON-384-XOF-1024` | hash_4096 | 11572 | 1404 KiB | 84740 KiB |
| `MEGASCON-384-XOF-1024` | hash_8192 | 11572 | 1408 KiB | 130788 KiB |
| `MEGASCON-384-XOF-1024` | hash_16384 | 11572 | 1416 KiB | 122384 KiB |
| `MEGASCON-384-XOF-1024` | hash_65536 | 11572 | 1464 KiB | 130348 KiB |
| `MEGASCON-384-XOF-2048` | hash_32 | 11572 | 1400 KiB | 6396 KiB |
| `MEGASCON-384-XOF-2048` | hash_128 | 11572 | 1400 KiB | 6904 KiB |
| `MEGASCON-384-XOF-2048` | hash_512 | 11572 | 1400 KiB | 15980 KiB |
| `MEGASCON-384-XOF-2048` | hash_1024 | 11572 | 1404 KiB | 25648 KiB |
| `MEGASCON-384-XOF-2048` | hash_4096 | 11572 | 1404 KiB | 84596 KiB |
| `MEGASCON-384-XOF-2048` | hash_8192 | 11572 | 1408 KiB | 129520 KiB |
| `MEGASCON-384-XOF-2048` | hash_16384 | 11572 | 1416 KiB | 132108 KiB |
| `MEGASCON-384-XOF-2048` | hash_65536 | 11572 | 1464 KiB | 99668 KiB |
| `MEGASCON-512` | hash_32 | 11308 | 1400 KiB | 5356 KiB |
| `MEGASCON-512` | hash_128 | 11308 | 1400 KiB | 8484 KiB |
| `MEGASCON-512` | hash_512 | 11308 | 1400 KiB | 16276 KiB |
| `MEGASCON-512` | hash_1024 | 11308 | 1404 KiB | 26268 KiB |
| `MEGASCON-512` | hash_4096 | 11308 | 1404 KiB | 86292 KiB |
| `MEGASCON-512` | hash_8192 | 11308 | 1408 KiB | 94856 KiB |
| `MEGASCON-512` | hash_16384 | 11308 | 1412 KiB | 108932 KiB |
| `MEGASCON-512` | hash_65536 | 11308 | 1464 KiB | 102080 KiB |
| `MEGASCON-512-XOF-256` | hash_32 | 11556 | 1396 KiB | 6116 KiB |
| `MEGASCON-512-XOF-256` | hash_128 | 11556 | 1396 KiB | 8668 KiB |
| `MEGASCON-512-XOF-256` | hash_512 | 11556 | 1400 KiB | 15684 KiB |
| `MEGASCON-512-XOF-256` | hash_1024 | 11556 | 1404 KiB | 25700 KiB |
| `MEGASCON-512-XOF-256` | hash_4096 | 11556 | 1404 KiB | 85916 KiB |
| `MEGASCON-512-XOF-256` | hash_8192 | 11556 | 1408 KiB | 107480 KiB |
| `MEGASCON-512-XOF-256` | hash_16384 | 11556 | 1416 KiB | 109212 KiB |
| `MEGASCON-512-XOF-256` | hash_65536 | 11556 | 1464 KiB | 108576 KiB |
| `MEGASCON-512-XOF-1024` | hash_32 | 11572 | 1400 KiB | 6208 KiB |
| `MEGASCON-512-XOF-1024` | hash_128 | 11572 | 1400 KiB | 8352 KiB |
| `MEGASCON-512-XOF-1024` | hash_512 | 11572 | 1400 KiB | 16252 KiB |
| `MEGASCON-512-XOF-1024` | hash_1024 | 11572 | 1404 KiB | 25880 KiB |
| `MEGASCON-512-XOF-1024` | hash_4096 | 11572 | 1404 KiB | 86312 KiB |
| `MEGASCON-512-XOF-1024` | hash_8192 | 11572 | 1408 KiB | 108368 KiB |
| `MEGASCON-512-XOF-1024` | hash_16384 | 11572 | 1416 KiB | 108988 KiB |
| `MEGASCON-512-XOF-1024` | hash_65536 | 11572 | 1464 KiB | 106684 KiB |
| `MEGASCON-512-XOF-2048` | hash_32 | 11572 | 1400 KiB | 6200 KiB |
| `MEGASCON-512-XOF-2048` | hash_128 | 11572 | 1400 KiB | 8764 KiB |
| `MEGASCON-512-XOF-2048` | hash_512 | 11572 | 1400 KiB | 16304 KiB |
| `MEGASCON-512-XOF-2048` | hash_1024 | 11572 | 1404 KiB | 25932 KiB |
| `MEGASCON-512-XOF-2048` | hash_4096 | 11572 | 1400 KiB | 86084 KiB |
| `MEGASCON-512-XOF-2048` | hash_8192 | 11572 | 1408 KiB | 101896 KiB |
| `MEGASCON-512-XOF-2048` | hash_16384 | 11572 | 1416 KiB | 105224 KiB |
| `MEGASCON-512-XOF-2048` | hash_65536 | 11572 | 1464 KiB | 107400 KiB |
| `MEGASCON-768` | hash_32 | 11636 | 1400 KiB | 6524 KiB |
| `MEGASCON-768` | hash_128 | 11636 | 1400 KiB | 6468 KiB |
| `MEGASCON-768` | hash_512 | 11636 | 1400 KiB | 15596 KiB |
| `MEGASCON-768` | hash_1024 | 11636 | 1404 KiB | 24240 KiB |
| `MEGASCON-768` | hash_4096 | 11636 | 1404 KiB | 83476 KiB |
| `MEGASCON-768` | hash_8192 | 11636 | 1408 KiB | 110848 KiB |
| `MEGASCON-768` | hash_16384 | 11636 | 1416 KiB | 113168 KiB |
| `MEGASCON-768` | hash_65536 | 11636 | 1464 KiB | 109668 KiB |
| `MEGASCON-1024` | hash_32 | 11636 | 1400 KiB | 5880 KiB |
| `MEGASCON-1024` | hash_128 | 11636 | 1400 KiB | 7420 KiB |
| `MEGASCON-1024` | hash_512 | 11636 | 1400 KiB | 15136 KiB |
| `MEGASCON-1024` | hash_1024 | 11636 | 1404 KiB | 24700 KiB |
| `MEGASCON-1024` | hash_4096 | 11636 | 1404 KiB | 85572 KiB |
| `MEGASCON-1024` | hash_8192 | 11636 | 1408 KiB | 89724 KiB |
| `MEGASCON-1024` | hash_16384 | 11636 | 1416 KiB | 90308 KiB |
| `MEGASCON-1024` | hash_65536 | 11636 | 1464 KiB | 90780 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `MEGASCON-384` | KAT log (sha256 `e25f279b0d9f1d47…`) | `kat/hash-18/MEGASCON-384.log` |
| `MEGASCON-384` | timing hash_1024 | `records/hash-18/MEGASCON-384__hash_1024.json` |
| `MEGASCON-384` | timing hash_128 | `records/hash-18/MEGASCON-384__hash_128.json` |
| `MEGASCON-384` | timing hash_16384 | `records/hash-18/MEGASCON-384__hash_16384.json` |
| `MEGASCON-384` | timing hash_32 | `records/hash-18/MEGASCON-384__hash_32.json` |
| `MEGASCON-384` | timing hash_4096 | `records/hash-18/MEGASCON-384__hash_4096.json` |
| `MEGASCON-384` | timing hash_512 | `records/hash-18/MEGASCON-384__hash_512.json` |
| `MEGASCON-384` | timing hash_65536 | `records/hash-18/MEGASCON-384__hash_65536.json` |
| `MEGASCON-384` | timing hash_8192 | `records/hash-18/MEGASCON-384__hash_8192.json` |
| `MEGASCON-384-XOF-256` | KAT log (sha256 `73ce1f2d0afcd6b8…`) | `kat/hash-18/MEGASCON-384-XOF-256.log` |
| `MEGASCON-384-XOF-256` | timing hash_1024 | `records/hash-18/MEGASCON-384-XOF-256__hash_1024.json` |
| `MEGASCON-384-XOF-256` | timing hash_128 | `records/hash-18/MEGASCON-384-XOF-256__hash_128.json` |
| `MEGASCON-384-XOF-256` | timing hash_16384 | `records/hash-18/MEGASCON-384-XOF-256__hash_16384.json` |
| `MEGASCON-384-XOF-256` | timing hash_32 | `records/hash-18/MEGASCON-384-XOF-256__hash_32.json` |
| `MEGASCON-384-XOF-256` | timing hash_4096 | `records/hash-18/MEGASCON-384-XOF-256__hash_4096.json` |
| `MEGASCON-384-XOF-256` | timing hash_512 | `records/hash-18/MEGASCON-384-XOF-256__hash_512.json` |
| `MEGASCON-384-XOF-256` | timing hash_65536 | `records/hash-18/MEGASCON-384-XOF-256__hash_65536.json` |
| `MEGASCON-384-XOF-256` | timing hash_8192 | `records/hash-18/MEGASCON-384-XOF-256__hash_8192.json` |
| `MEGASCON-384-XOF-1024` | KAT log (sha256 `e8ac7b4ce4816a3b…`) | `kat/hash-18/MEGASCON-384-XOF-1024.log` |
| `MEGASCON-384-XOF-1024` | timing hash_1024 | `records/hash-18/MEGASCON-384-XOF-1024__hash_1024.json` |
| `MEGASCON-384-XOF-1024` | timing hash_128 | `records/hash-18/MEGASCON-384-XOF-1024__hash_128.json` |
| `MEGASCON-384-XOF-1024` | timing hash_16384 | `records/hash-18/MEGASCON-384-XOF-1024__hash_16384.json` |
| `MEGASCON-384-XOF-1024` | timing hash_32 | `records/hash-18/MEGASCON-384-XOF-1024__hash_32.json` |
| `MEGASCON-384-XOF-1024` | timing hash_4096 | `records/hash-18/MEGASCON-384-XOF-1024__hash_4096.json` |
| `MEGASCON-384-XOF-1024` | timing hash_512 | `records/hash-18/MEGASCON-384-XOF-1024__hash_512.json` |
| `MEGASCON-384-XOF-1024` | timing hash_65536 | `records/hash-18/MEGASCON-384-XOF-1024__hash_65536.json` |
| `MEGASCON-384-XOF-1024` | timing hash_8192 | `records/hash-18/MEGASCON-384-XOF-1024__hash_8192.json` |
| `MEGASCON-384-XOF-2048` | KAT log (sha256 `3c356bd9b2f45660…`) | `kat/hash-18/MEGASCON-384-XOF-2048.log` |
| `MEGASCON-384-XOF-2048` | timing hash_1024 | `records/hash-18/MEGASCON-384-XOF-2048__hash_1024.json` |
| `MEGASCON-384-XOF-2048` | timing hash_128 | `records/hash-18/MEGASCON-384-XOF-2048__hash_128.json` |
| `MEGASCON-384-XOF-2048` | timing hash_16384 | `records/hash-18/MEGASCON-384-XOF-2048__hash_16384.json` |
| `MEGASCON-384-XOF-2048` | timing hash_32 | `records/hash-18/MEGASCON-384-XOF-2048__hash_32.json` |
| `MEGASCON-384-XOF-2048` | timing hash_4096 | `records/hash-18/MEGASCON-384-XOF-2048__hash_4096.json` |
| `MEGASCON-384-XOF-2048` | timing hash_512 | `records/hash-18/MEGASCON-384-XOF-2048__hash_512.json` |
| `MEGASCON-384-XOF-2048` | timing hash_65536 | `records/hash-18/MEGASCON-384-XOF-2048__hash_65536.json` |
| `MEGASCON-384-XOF-2048` | timing hash_8192 | `records/hash-18/MEGASCON-384-XOF-2048__hash_8192.json` |
| `MEGASCON-512` | KAT log (sha256 `d45309eded630403…`) | `kat/hash-18/MEGASCON-512.log` |
| `MEGASCON-512` | timing hash_1024 | `records/hash-18/MEGASCON-512__hash_1024.json` |
| `MEGASCON-512` | timing hash_128 | `records/hash-18/MEGASCON-512__hash_128.json` |
| `MEGASCON-512` | timing hash_16384 | `records/hash-18/MEGASCON-512__hash_16384.json` |
| `MEGASCON-512` | timing hash_32 | `records/hash-18/MEGASCON-512__hash_32.json` |
| `MEGASCON-512` | timing hash_4096 | `records/hash-18/MEGASCON-512__hash_4096.json` |
| `MEGASCON-512` | timing hash_512 | `records/hash-18/MEGASCON-512__hash_512.json` |
| `MEGASCON-512` | timing hash_65536 | `records/hash-18/MEGASCON-512__hash_65536.json` |
| `MEGASCON-512` | timing hash_8192 | `records/hash-18/MEGASCON-512__hash_8192.json` |
| `MEGASCON-512-XOF-256` | KAT log (sha256 `87150b6293a7c9d7…`) | `kat/hash-18/MEGASCON-512-XOF-256.log` |
| `MEGASCON-512-XOF-256` | timing hash_1024 | `records/hash-18/MEGASCON-512-XOF-256__hash_1024.json` |
| `MEGASCON-512-XOF-256` | timing hash_128 | `records/hash-18/MEGASCON-512-XOF-256__hash_128.json` |
| `MEGASCON-512-XOF-256` | timing hash_16384 | `records/hash-18/MEGASCON-512-XOF-256__hash_16384.json` |
| `MEGASCON-512-XOF-256` | timing hash_32 | `records/hash-18/MEGASCON-512-XOF-256__hash_32.json` |
| `MEGASCON-512-XOF-256` | timing hash_4096 | `records/hash-18/MEGASCON-512-XOF-256__hash_4096.json` |
| `MEGASCON-512-XOF-256` | timing hash_512 | `records/hash-18/MEGASCON-512-XOF-256__hash_512.json` |
| `MEGASCON-512-XOF-256` | timing hash_65536 | `records/hash-18/MEGASCON-512-XOF-256__hash_65536.json` |
| `MEGASCON-512-XOF-256` | timing hash_8192 | `records/hash-18/MEGASCON-512-XOF-256__hash_8192.json` |
| `MEGASCON-512-XOF-1024` | KAT log (sha256 `1ecd6372ed0d0622…`) | `kat/hash-18/MEGASCON-512-XOF-1024.log` |
| `MEGASCON-512-XOF-1024` | timing hash_1024 | `records/hash-18/MEGASCON-512-XOF-1024__hash_1024.json` |
| `MEGASCON-512-XOF-1024` | timing hash_128 | `records/hash-18/MEGASCON-512-XOF-1024__hash_128.json` |
| `MEGASCON-512-XOF-1024` | timing hash_16384 | `records/hash-18/MEGASCON-512-XOF-1024__hash_16384.json` |
| `MEGASCON-512-XOF-1024` | timing hash_32 | `records/hash-18/MEGASCON-512-XOF-1024__hash_32.json` |
| `MEGASCON-512-XOF-1024` | timing hash_4096 | `records/hash-18/MEGASCON-512-XOF-1024__hash_4096.json` |
| `MEGASCON-512-XOF-1024` | timing hash_512 | `records/hash-18/MEGASCON-512-XOF-1024__hash_512.json` |
| `MEGASCON-512-XOF-1024` | timing hash_65536 | `records/hash-18/MEGASCON-512-XOF-1024__hash_65536.json` |
| `MEGASCON-512-XOF-1024` | timing hash_8192 | `records/hash-18/MEGASCON-512-XOF-1024__hash_8192.json` |
| `MEGASCON-512-XOF-2048` | KAT log (sha256 `d410ab24572b8b50…`) | `kat/hash-18/MEGASCON-512-XOF-2048.log` |
| `MEGASCON-512-XOF-2048` | timing hash_1024 | `records/hash-18/MEGASCON-512-XOF-2048__hash_1024.json` |
| `MEGASCON-512-XOF-2048` | timing hash_128 | `records/hash-18/MEGASCON-512-XOF-2048__hash_128.json` |
| `MEGASCON-512-XOF-2048` | timing hash_16384 | `records/hash-18/MEGASCON-512-XOF-2048__hash_16384.json` |
| `MEGASCON-512-XOF-2048` | timing hash_32 | `records/hash-18/MEGASCON-512-XOF-2048__hash_32.json` |
| `MEGASCON-512-XOF-2048` | timing hash_4096 | `records/hash-18/MEGASCON-512-XOF-2048__hash_4096.json` |
| `MEGASCON-512-XOF-2048` | timing hash_512 | `records/hash-18/MEGASCON-512-XOF-2048__hash_512.json` |
| `MEGASCON-512-XOF-2048` | timing hash_65536 | `records/hash-18/MEGASCON-512-XOF-2048__hash_65536.json` |
| `MEGASCON-512-XOF-2048` | timing hash_8192 | `records/hash-18/MEGASCON-512-XOF-2048__hash_8192.json` |
| `MEGASCON-768` | KAT log (sha256 `e54dd8b935802c8c…`) | `kat/hash-18/MEGASCON-768.log` |
| `MEGASCON-768` | timing hash_1024 | `records/hash-18/MEGASCON-768__hash_1024.json` |
| `MEGASCON-768` | timing hash_128 | `records/hash-18/MEGASCON-768__hash_128.json` |
| `MEGASCON-768` | timing hash_16384 | `records/hash-18/MEGASCON-768__hash_16384.json` |
| `MEGASCON-768` | timing hash_32 | `records/hash-18/MEGASCON-768__hash_32.json` |
| `MEGASCON-768` | timing hash_4096 | `records/hash-18/MEGASCON-768__hash_4096.json` |
| `MEGASCON-768` | timing hash_512 | `records/hash-18/MEGASCON-768__hash_512.json` |
| `MEGASCON-768` | timing hash_65536 | `records/hash-18/MEGASCON-768__hash_65536.json` |
| `MEGASCON-768` | timing hash_8192 | `records/hash-18/MEGASCON-768__hash_8192.json` |
| `MEGASCON-1024` | KAT log (sha256 `fc2b9fe79410036e…`) | `kat/hash-18/MEGASCON-1024.log` |
| `MEGASCON-1024` | timing hash_1024 | `records/hash-18/MEGASCON-1024__hash_1024.json` |
| `MEGASCON-1024` | timing hash_128 | `records/hash-18/MEGASCON-1024__hash_128.json` |
| `MEGASCON-1024` | timing hash_16384 | `records/hash-18/MEGASCON-1024__hash_16384.json` |
| `MEGASCON-1024` | timing hash_32 | `records/hash-18/MEGASCON-1024__hash_32.json` |
| `MEGASCON-1024` | timing hash_4096 | `records/hash-18/MEGASCON-1024__hash_4096.json` |
| `MEGASCON-1024` | timing hash_512 | `records/hash-18/MEGASCON-1024__hash_512.json` |
| `MEGASCON-1024` | timing hash_65536 | `records/hash-18/MEGASCON-1024__hash_65536.json` |
| `MEGASCON-1024` | timing hash_8192 | `records/hash-18/MEGASCON-1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

