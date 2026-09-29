# hash-11 Garnet — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `hash-11` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101538925484527616.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: Garnet
- Implementation versions measured: reference
- Parameter sets: `Garnet_512_Cap512`, `Garnet_512_Cap640`, `Garnet_512_Cap768`, `Garnet_512_Cap896`, `Garnet_512_Cap1024`, `Garnet_768`, `Garnet_1024`, `Garnet_1024_DM4x4`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `hash-11/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Garnet_512_Cap512` | guide | PASS |
| `Garnet_512_Cap640` | guide | PASS |
| `Garnet_512_Cap768` | guide | PASS |
| `Garnet_512_Cap896` | guide | PASS |
| `Garnet_512_Cap1024` | guide | PASS |
| `Garnet_768` | guide | PASS |
| `Garnet_1024` | guide | PASS |
| `Garnet_1024_DM4x4` | harness-default | MISMATCH [1] (not timed) |

[1] No reference source: the submission ships a second 1024-bit KAT set (DM4x4, apparently a 512-bit-rate variant) whose only code is x86-64 assembly in the optimized tree; the harness label reuses the Garnet_1024 C sources, so its timing would duplicate Garnet_1024 (hash-11/pseudocode.md, discrepancy 3).

## 4. Performance

| instance | message | mean cycles | cycles/byte | mean time | MB/s | n |
|---|---|---|---|---|---|---|
| `Garnet_512_Cap512` | 32 B | 8870 | 277.2 | 3.29 µs | 9.7 | 100000 (5 × 20000) |
| `Garnet_512_Cap512` | 128 B | 14.1 k | 109.9 | 5.22 µs | 24.5 | 100000 (5 × 20000) |
| `Garnet_512_Cap512` | 512 B | 29.5 k | 57.7 | 11 µs | 46.7 | 100000 (5 × 20000) |
| `Garnet_512_Cap512` | 1024 B | 50.2 k | 49.1 | 18.6 µs | 54.9 | 100000 (5 × 20000) |
| `Garnet_512_Cap512` | 4096 B | 174.2 k | 42.5 | 64.7 µs | 63.3 | 46110 (5 × 9222) |
| `Garnet_512_Cap512` | 8192 B | 339.6 k | 41.5 | 126 µs | 65.0 | 24355 (5 × 4871) |
| `Garnet_512_Cap512` | 16384 B | 669.9 k | 40.9 | 249 µs | 65.9 | 12425 (5 × 2485) |
| `Garnet_512_Cap512` | 65536 B | 2.65 M | 40.5 | 985 µs | 66.5 | 3120 (5 × 624) |
| `Garnet_512_Cap640` | 32 B | 8878 | 277.5 | 3.3 µs | 9.7 | 100000 (5 × 20000) |
| `Garnet_512_Cap640` | 128 B | 11.5 k | 89.6 | 4.26 µs | 30.1 | 100000 (5 × 20000) |
| `Garnet_512_Cap640` | 512 B | 24.4 k | 47.7 | 9.06 µs | 56.5 | 100000 (5 × 20000) |
| `Garnet_512_Cap640` | 1024 B | 39.9 k | 39.0 | 14.8 µs | 69.1 | 100000 (5 × 20000) |
| `Garnet_512_Cap640` | 4096 B | 140.9 k | 34.4 | 52.3 µs | 78.4 | 55240 (5 × 11048) |
| `Garnet_512_Cap640` | 8192 B | 272.7 k | 33.3 | 101 µs | 81.0 | 29470 (5 × 5894) |
| `Garnet_512_Cap640` | 16384 B | 536.3 k | 32.7 | 199 µs | 82.3 | 15415 (5 × 3083) |
| `Garnet_512_Cap640` | 65536 B | 2.13 M | 32.4 | 789 µs | 83.0 | 3395 (5 × 679) |
| `Garnet_512_Cap768` | 32 B | 8879 | 277.5 | 3.3 µs | 9.7 | 100000 (5 × 20000) |
| `Garnet_512_Cap768` | 128 B | 11.5 k | 89.8 | 4.27 µs | 30.0 | 100000 (5 × 20000) |
| `Garnet_512_Cap768` | 512 B | 21.9 k | 42.7 | 8.11 µs | 63.1 | 100000 (5 × 20000) |
| `Garnet_512_Cap768` | 1024 B | 34.8 k | 34.0 | 12.9 µs | 79.2 | 100000 (5 × 20000) |
| `Garnet_512_Cap768` | 4096 B | 117.7 k | 28.7 | 43.7 µs | 93.8 | 67895 (5 × 13579) |
| `Garnet_512_Cap768` | 8192 B | 229.3 k | 28.0 | 85.2 µs | 96.2 | 35195 (5 × 7039) |
| `Garnet_512_Cap768` | 16384 B | 448.7 k | 27.4 | 167 µs | 98.4 | 17700 (5 × 3540) |
| `Garnet_512_Cap768` | 65536 B | 1.77 M | 27.1 | 658 µs | 99.6 | 4515 (5 × 903) |
| `Garnet_512_Cap896` | 32 B | 8879 | 277.5 | 3.3 µs | 9.7 | 100000 (5 × 20000) |
| `Garnet_512_Cap896` | 128 B | 11.5 k | 89.8 | 4.27 µs | 30.0 | 100000 (5 × 20000) |
| `Garnet_512_Cap896` | 512 B | 19.3 k | 37.6 | 7.15 µs | 71.6 | 100000 (5 × 20000) |
| `Garnet_512_Cap896` | 1024 B | 32.2 k | 31.5 | 12 µs | 85.6 | 100000 (5 × 20000) |
| `Garnet_512_Cap896` | 4096 B | 102.2 k | 25.0 | 37.9 µs | 108.0 | 77425 (5 × 15485) |
| `Garnet_512_Cap896` | 8192 B | 198.0 k | 24.2 | 73.5 µs | 111.5 | 40820 (5 × 8164) |
| `Garnet_512_Cap896` | 16384 B | 386.7 k | 23.6 | 144 µs | 114.2 | 19140 (5 × 3828) |
| `Garnet_512_Cap896` | 65536 B | 1.52 M | 23.3 | 566 µs | 115.9 | 5345 (5 × 1069) |
| `Garnet_512_Cap1024` | 32 B | 8609 | 269.0 | 3.2 µs | 10.0 | 100000 (5 × 20000) |
| `Garnet_512_Cap1024` | 128 B | 11.1 k | 86.9 | 4.13 µs | 31.0 | 100000 (5 × 20000) |
| `Garnet_512_Cap1024` | 512 B | 18.6 k | 36.4 | 6.92 µs | 74.0 | 100000 (5 × 20000) |
| `Garnet_512_Cap1024` | 1024 B | 28.7 k | 28.0 | 10.7 µs | 96.1 | 100000 (5 × 20000) |
| `Garnet_512_Cap1024` | 4096 B | 88.9 k | 21.7 | 33 µs | 124.2 | 88400 (5 × 17680) |
| `Garnet_512_Cap1024` | 8192 B | 169.2 k | 20.7 | 62.8 µs | 130.5 | 47525 (5 × 9505) |
| `Garnet_512_Cap1024` | 16384 B | 330.5 k | 20.2 | 123 µs | 133.4 | 24480 (5 × 4896) |
| `Garnet_512_Cap1024` | 65536 B | 1.29 M | 19.8 | 480 µs | 136.4 | 6290 (5 × 1258) |
| `Garnet_768` | 32 B | 10.7 k | 334.6 | 3.98 µs | 8.0 | 100000 (5 × 20000) |
| `Garnet_768` | 128 B | 14.1 k | 110.5 | 5.25 µs | 24.4 | 100000 (5 × 20000) |
| `Garnet_768` | 512 B | 29.9 k | 58.3 | 11.1 µs | 46.2 | 100000 (5 × 20000) |
| `Garnet_768` | 1024 B | 50.9 k | 49.7 | 18.9 µs | 54.2 | 100000 (5 × 20000) |
| `Garnet_768` | 4096 B | 216.7 k | 52.9 | 80.4 µs | 50.9 | 46025 (5 × 9205) |
| `Garnet_768` | 8192 B | 344.2 k | 42.0 | 128 µs | 64.1 | 24135 (5 × 4827) |
| `Garnet_768` | 16384 B | 679.5 k | 41.5 | 252 µs | 65.0 | 12300 (5 × 2460) |
| `Garnet_768` | 65536 B | 2.69 M | 41.1 | 999 µs | 65.6 | 3095 (5 × 619) |
| `Garnet_1024` | 32 B | 43.1 k | 1347.0 | 16 µs | 2.0 | 100000 (5 × 20000) |
| `Garnet_1024` | 128 B | 58.5 k | 457.0 | 21.7 µs | 5.9 | 100000 (5 × 20000) |
| `Garnet_1024` | 512 B | 99.5 k | 194.3 | 36.9 µs | 13.9 | 81220 (5 × 16244) |
| `Garnet_1024` | 1024 B | 159.6 k | 155.9 | 59.2 µs | 17.3 | 51505 (5 × 10301) |
| `Garnet_1024` | 4096 B | 509.2 k | 124.3 | 189 µs | 21.7 | 16480 (5 × 3296) |
| `Garnet_1024` | 8192 B | 1.07 M | 130.8 | 398 µs | 20.6 | 8410 (5 × 1682) |
| `Garnet_1024` | 16384 B | 1.93 M | 118.0 | 717 µs | 22.8 | 4025 (5 × 805) |
| `Garnet_1024` | 65536 B | 7.62 M | 116.2 | 2.83 ms | 23.2 | 1110 (5 × 222) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Garnet_512_Cap512` | hash_32 | 36516 | 1424 KiB | 1488 KiB |
| `Garnet_512_Cap512` | hash_128 | 36516 | 1424 KiB | 1488 KiB |
| `Garnet_512_Cap512` | hash_512 | 36516 | 1424 KiB | 1492 KiB |
| `Garnet_512_Cap512` | hash_1024 | 36516 | 1428 KiB | 1492 KiB |
| `Garnet_512_Cap512` | hash_4096 | 36516 | 1428 KiB | 1496 KiB |
| `Garnet_512_Cap512` | hash_8192 | 36516 | 1432 KiB | 1504 KiB |
| `Garnet_512_Cap512` | hash_16384 | 36516 | 1440 KiB | 1520 KiB |
| `Garnet_512_Cap512` | hash_65536 | 36516 | 1488 KiB | 1616 KiB |
| `Garnet_512_Cap640` | hash_32 | 36580 | 1424 KiB | 1488 KiB |
| `Garnet_512_Cap640` | hash_128 | 36580 | 1424 KiB | 1488 KiB |
| `Garnet_512_Cap640` | hash_512 | 36580 | 3460 KiB | 3524 KiB |
| `Garnet_512_Cap640` | hash_1024 | 36580 | 1428 KiB | 1492 KiB |
| `Garnet_512_Cap640` | hash_4096 | 36580 | 1428 KiB | 1496 KiB |
| `Garnet_512_Cap640` | hash_8192 | 36580 | 1432 KiB | 1504 KiB |
| `Garnet_512_Cap640` | hash_16384 | 36580 | 1436 KiB | 1516 KiB |
| `Garnet_512_Cap640` | hash_65536 | 36580 | 3496 KiB | 3560 KiB |
| `Garnet_512_Cap768` | hash_32 | 36580 | 1424 KiB | 1488 KiB |
| `Garnet_512_Cap768` | hash_128 | 36580 | 1424 KiB | 1488 KiB |
| `Garnet_512_Cap768` | hash_512 | 36580 | 1424 KiB | 1492 KiB |
| `Garnet_512_Cap768` | hash_1024 | 36580 | 1428 KiB | 1492 KiB |
| `Garnet_512_Cap768` | hash_4096 | 36580 | 1428 KiB | 1496 KiB |
| `Garnet_512_Cap768` | hash_8192 | 36580 | 1432 KiB | 1504 KiB |
| `Garnet_512_Cap768` | hash_16384 | 36580 | 1440 KiB | 1520 KiB |
| `Garnet_512_Cap768` | hash_65536 | 36580 | 3488 KiB | 3552 KiB |
| `Garnet_512_Cap896` | hash_32 | 36644 | 1424 KiB | 1488 KiB |
| `Garnet_512_Cap896` | hash_128 | 36644 | 1424 KiB | 1488 KiB |
| `Garnet_512_Cap896` | hash_512 | 36644 | 1424 KiB | 1492 KiB |
| `Garnet_512_Cap896` | hash_1024 | 36644 | 1428 KiB | 1492 KiB |
| `Garnet_512_Cap896` | hash_4096 | 36644 | 1428 KiB | 1496 KiB |
| `Garnet_512_Cap896` | hash_8192 | 36644 | 1432 KiB | 1504 KiB |
| `Garnet_512_Cap896` | hash_16384 | 36644 | 1440 KiB | 1520 KiB |
| `Garnet_512_Cap896` | hash_65536 | 36644 | 3480 KiB | 3544 KiB |
| `Garnet_512_Cap1024` | hash_32 | 35988 | 1424 KiB | 1488 KiB |
| `Garnet_512_Cap1024` | hash_128 | 35988 | 1424 KiB | 1488 KiB |
| `Garnet_512_Cap1024` | hash_512 | 35988 | 1424 KiB | 1492 KiB |
| `Garnet_512_Cap1024` | hash_1024 | 35988 | 1428 KiB | 1492 KiB |
| `Garnet_512_Cap1024` | hash_4096 | 35988 | 1428 KiB | 1496 KiB |
| `Garnet_512_Cap1024` | hash_8192 | 35988 | 1432 KiB | 1504 KiB |
| `Garnet_512_Cap1024` | hash_16384 | 35988 | 1440 KiB | 1520 KiB |
| `Garnet_512_Cap1024` | hash_65536 | 35988 | 1488 KiB | 1616 KiB |
| `Garnet_768` | hash_32 | 35972 | 3448 KiB | 3512 KiB |
| `Garnet_768` | hash_128 | 35972 | 1424 KiB | 1488 KiB |
| `Garnet_768` | hash_512 | 35972 | 1420 KiB | 1488 KiB |
| `Garnet_768` | hash_1024 | 35972 | 1428 KiB | 1492 KiB |
| `Garnet_768` | hash_4096 | 35972 | 1428 KiB | 1496 KiB |
| `Garnet_768` | hash_8192 | 35972 | 1432 KiB | 1504 KiB |
| `Garnet_768` | hash_16384 | 35972 | 1440 KiB | 1520 KiB |
| `Garnet_768` | hash_65536 | 35972 | 1488 KiB | 1616 KiB |
| `Garnet_1024` | hash_32 | 35972 | 1424 KiB | 1488 KiB |
| `Garnet_1024` | hash_128 | 35972 | 1424 KiB | 1488 KiB |
| `Garnet_1024` | hash_512 | 35972 | 1424 KiB | 1492 KiB |
| `Garnet_1024` | hash_1024 | 35972 | 1428 KiB | 1492 KiB |
| `Garnet_1024` | hash_4096 | 35972 | 1428 KiB | 1496 KiB |
| `Garnet_1024` | hash_8192 | 35972 | 1432 KiB | 1504 KiB |
| `Garnet_1024` | hash_16384 | 35972 | 1440 KiB | 1520 KiB |
| `Garnet_1024` | hash_65536 | 35972 | 1488 KiB | 1616 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Garnet_512_Cap512` | KAT log (sha256 `1f9dfe5152e3e240…`) | `kat/hash-11/Garnet_512_Cap512.log` |
| `Garnet_512_Cap512` | timing hash_1024 | `records/hash-11/Garnet_512_Cap512__hash_1024.json` |
| `Garnet_512_Cap512` | timing hash_128 | `records/hash-11/Garnet_512_Cap512__hash_128.json` |
| `Garnet_512_Cap512` | timing hash_16384 | `records/hash-11/Garnet_512_Cap512__hash_16384.json` |
| `Garnet_512_Cap512` | timing hash_32 | `records/hash-11/Garnet_512_Cap512__hash_32.json` |
| `Garnet_512_Cap512` | timing hash_4096 | `records/hash-11/Garnet_512_Cap512__hash_4096.json` |
| `Garnet_512_Cap512` | timing hash_512 | `records/hash-11/Garnet_512_Cap512__hash_512.json` |
| `Garnet_512_Cap512` | timing hash_65536 | `records/hash-11/Garnet_512_Cap512__hash_65536.json` |
| `Garnet_512_Cap512` | timing hash_8192 | `records/hash-11/Garnet_512_Cap512__hash_8192.json` |
| `Garnet_512_Cap640` | KAT log (sha256 `836f2ea099ed527f…`) | `kat/hash-11/Garnet_512_Cap640.log` |
| `Garnet_512_Cap640` | timing hash_1024 | `records/hash-11/Garnet_512_Cap640__hash_1024.json` |
| `Garnet_512_Cap640` | timing hash_128 | `records/hash-11/Garnet_512_Cap640__hash_128.json` |
| `Garnet_512_Cap640` | timing hash_16384 | `records/hash-11/Garnet_512_Cap640__hash_16384.json` |
| `Garnet_512_Cap640` | timing hash_32 | `records/hash-11/Garnet_512_Cap640__hash_32.json` |
| `Garnet_512_Cap640` | timing hash_4096 | `records/hash-11/Garnet_512_Cap640__hash_4096.json` |
| `Garnet_512_Cap640` | timing hash_512 | `records/hash-11/Garnet_512_Cap640__hash_512.json` |
| `Garnet_512_Cap640` | timing hash_65536 | `records/hash-11/Garnet_512_Cap640__hash_65536.json` |
| `Garnet_512_Cap640` | timing hash_8192 | `records/hash-11/Garnet_512_Cap640__hash_8192.json` |
| `Garnet_512_Cap768` | KAT log (sha256 `9f0bb370112cc687…`) | `kat/hash-11/Garnet_512_Cap768.log` |
| `Garnet_512_Cap768` | timing hash_1024 | `records/hash-11/Garnet_512_Cap768__hash_1024.json` |
| `Garnet_512_Cap768` | timing hash_128 | `records/hash-11/Garnet_512_Cap768__hash_128.json` |
| `Garnet_512_Cap768` | timing hash_16384 | `records/hash-11/Garnet_512_Cap768__hash_16384.json` |
| `Garnet_512_Cap768` | timing hash_32 | `records/hash-11/Garnet_512_Cap768__hash_32.json` |
| `Garnet_512_Cap768` | timing hash_4096 | `records/hash-11/Garnet_512_Cap768__hash_4096.json` |
| `Garnet_512_Cap768` | timing hash_512 | `records/hash-11/Garnet_512_Cap768__hash_512.json` |
| `Garnet_512_Cap768` | timing hash_65536 | `records/hash-11/Garnet_512_Cap768__hash_65536.json` |
| `Garnet_512_Cap768` | timing hash_8192 | `records/hash-11/Garnet_512_Cap768__hash_8192.json` |
| `Garnet_512_Cap896` | KAT log (sha256 `ad262571d2926a39…`) | `kat/hash-11/Garnet_512_Cap896.log` |
| `Garnet_512_Cap896` | timing hash_1024 | `records/hash-11/Garnet_512_Cap896__hash_1024.json` |
| `Garnet_512_Cap896` | timing hash_128 | `records/hash-11/Garnet_512_Cap896__hash_128.json` |
| `Garnet_512_Cap896` | timing hash_16384 | `records/hash-11/Garnet_512_Cap896__hash_16384.json` |
| `Garnet_512_Cap896` | timing hash_32 | `records/hash-11/Garnet_512_Cap896__hash_32.json` |
| `Garnet_512_Cap896` | timing hash_4096 | `records/hash-11/Garnet_512_Cap896__hash_4096.json` |
| `Garnet_512_Cap896` | timing hash_512 | `records/hash-11/Garnet_512_Cap896__hash_512.json` |
| `Garnet_512_Cap896` | timing hash_65536 | `records/hash-11/Garnet_512_Cap896__hash_65536.json` |
| `Garnet_512_Cap896` | timing hash_8192 | `records/hash-11/Garnet_512_Cap896__hash_8192.json` |
| `Garnet_512_Cap1024` | KAT log (sha256 `0638c27897f390bc…`) | `kat/hash-11/Garnet_512_Cap1024.log` |
| `Garnet_512_Cap1024` | timing hash_1024 | `records/hash-11/Garnet_512_Cap1024__hash_1024.json` |
| `Garnet_512_Cap1024` | timing hash_128 | `records/hash-11/Garnet_512_Cap1024__hash_128.json` |
| `Garnet_512_Cap1024` | timing hash_16384 | `records/hash-11/Garnet_512_Cap1024__hash_16384.json` |
| `Garnet_512_Cap1024` | timing hash_32 | `records/hash-11/Garnet_512_Cap1024__hash_32.json` |
| `Garnet_512_Cap1024` | timing hash_4096 | `records/hash-11/Garnet_512_Cap1024__hash_4096.json` |
| `Garnet_512_Cap1024` | timing hash_512 | `records/hash-11/Garnet_512_Cap1024__hash_512.json` |
| `Garnet_512_Cap1024` | timing hash_65536 | `records/hash-11/Garnet_512_Cap1024__hash_65536.json` |
| `Garnet_512_Cap1024` | timing hash_8192 | `records/hash-11/Garnet_512_Cap1024__hash_8192.json` |
| `Garnet_768` | KAT log (sha256 `8e1382ae0a2a9f7a…`) | `kat/hash-11/Garnet_768.log` |
| `Garnet_768` | timing hash_1024 | `records/hash-11/Garnet_768__hash_1024.json` |
| `Garnet_768` | timing hash_128 | `records/hash-11/Garnet_768__hash_128.json` |
| `Garnet_768` | timing hash_16384 | `records/hash-11/Garnet_768__hash_16384.json` |
| `Garnet_768` | timing hash_32 | `records/hash-11/Garnet_768__hash_32.json` |
| `Garnet_768` | timing hash_4096 | `records/hash-11/Garnet_768__hash_4096.json` |
| `Garnet_768` | timing hash_512 | `records/hash-11/Garnet_768__hash_512.json` |
| `Garnet_768` | timing hash_65536 | `records/hash-11/Garnet_768__hash_65536.json` |
| `Garnet_768` | timing hash_8192 | `records/hash-11/Garnet_768__hash_8192.json` |
| `Garnet_1024` | KAT log (sha256 `33e8b15504a5dda1…`) | `kat/hash-11/Garnet_1024.log` |
| `Garnet_1024` | timing hash_1024 | `records/hash-11/Garnet_1024__hash_1024.json` |
| `Garnet_1024` | timing hash_128 | `records/hash-11/Garnet_1024__hash_128.json` |
| `Garnet_1024` | timing hash_16384 | `records/hash-11/Garnet_1024__hash_16384.json` |
| `Garnet_1024` | timing hash_32 | `records/hash-11/Garnet_1024__hash_32.json` |
| `Garnet_1024` | timing hash_4096 | `records/hash-11/Garnet_1024__hash_4096.json` |
| `Garnet_1024` | timing hash_512 | `records/hash-11/Garnet_1024__hash_512.json` |
| `Garnet_1024` | timing hash_65536 | `records/hash-11/Garnet_1024__hash_65536.json` |
| `Garnet_1024` | timing hash_8192 | `records/hash-11/Garnet_1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

