# hash-11 Garnet — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `hash-11` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Submissions4/pc/content/content_2101538925484527616.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: cryptographic hash algorithm; function: hash
- Algorithm: Garnet
- Implementation versions measured: reference
- Parameter sets: `Garnet_512_Cap512`, `Garnet_512_Cap640`, `Garnet_512_Cap768`, `Garnet_512_Cap896`, `Garnet_512_Cap1024`, `Garnet_768`, `Garnet_1024`, `Garnet_1024_DM4x4`

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
| `Garnet_512_Cap512` | 32 B | 18.6 k | 582.6 | 8.91 µs | 3.6 | 100000 (5 × 20000) |
| `Garnet_512_Cap512` | 128 B | 29.6 k | 231.1 | 14.1 µs | 9.1 | 100000 (5 × 20000) |
| `Garnet_512_Cap512` | 512 B | 62.3 k | 121.6 | 29.7 µs | 17.2 | 100000 (5 × 20000) |
| `Garnet_512_Cap512` | 1024 B | 105.9 k | 103.5 | 50.6 µs | 20.2 | 83600 (5 × 16720) |
| `Garnet_512_Cap512` | 4096 B | 367.3 k | 89.7 | 175 µs | 23.3 | 27125 (5 × 5425) |
| `Garnet_512_Cap512` | 8192 B | 715.9 k | 87.4 | 342 µs | 24.0 | 14190 (5 × 2838) |
| `Garnet_512_Cap512` | 16384 B | 1.41 M | 86.3 | 675 µs | 24.3 | 7255 (5 × 1451) |
| `Garnet_512_Cap512` | 65536 B | 5.61 M | 85.5 | 2.68 ms | 24.5 | 1845 (5 × 369) |
| `Garnet_512_Cap640` | 32 B | 19.0 k | 594.7 | 9.09 µs | 3.5 | 100000 (5 × 20000) |
| `Garnet_512_Cap640` | 128 B | 24.1 k | 188.4 | 11.5 µs | 11.1 | 100000 (5 × 20000) |
| `Garnet_512_Cap640` | 512 B | 51.4 k | 100.4 | 24.5 µs | 20.9 | 100000 (5 × 20000) |
| `Garnet_512_Cap640` | 1024 B | 84.2 k | 82.2 | 40.2 µs | 25.5 | 100000 (5 × 20000) |
| `Garnet_512_Cap640` | 4096 B | 296.6 k | 72.4 | 142 µs | 28.9 | 33220 (5 × 6644) |
| `Garnet_512_Cap640` | 8192 B | 574.5 k | 70.1 | 274 µs | 29.9 | 17550 (5 × 3510) |
| `Garnet_512_Cap640` | 16384 B | 1.13 M | 69.0 | 540 µs | 30.3 | 9025 (5 × 1805) |
| `Garnet_512_Cap640` | 65536 B | 4.58 M | 70.0 | 2.19 ms | 29.9 | 2290 (5 × 458) |
| `Garnet_512_Cap768` | 32 B | 18.6 k | 582.7 | 8.91 µs | 3.6 | 100000 (5 × 20000) |
| `Garnet_512_Cap768` | 128 B | 24.1 k | 188.4 | 11.5 µs | 11.1 | 100000 (5 × 20000) |
| `Garnet_512_Cap768` | 512 B | 46.0 k | 89.8 | 22 µs | 23.3 | 100000 (5 × 20000) |
| `Garnet_512_Cap768` | 1024 B | 73.2 k | 71.5 | 35 µs | 29.3 | 100000 (5 × 20000) |
| `Garnet_512_Cap768` | 4096 B | 247.7 k | 60.5 | 119 µs | 34.3 | 39320 (5 × 7864) |
| `Garnet_512_Cap768` | 8192 B | 482.2 k | 58.9 | 230 µs | 35.6 | 20765 (5 × 4153) |
| `Garnet_512_Cap768` | 16384 B | 945.7 k | 57.7 | 452 µs | 36.3 | 10755 (5 × 2151) |
| `Garnet_512_Cap768` | 65536 B | 3.74 M | 57.1 | 1.79 ms | 36.6 | 2715 (5 × 543) |
| `Garnet_512_Cap896` | 32 B | 18.6 k | 582.2 | 8.9 µs | 3.6 | 100000 (5 × 20000) |
| `Garnet_512_Cap896` | 128 B | 24.1 k | 188.4 | 11.5 µs | 11.1 | 100000 (5 × 20000) |
| `Garnet_512_Cap896` | 512 B | 40.5 k | 79.1 | 19.4 µs | 26.5 | 100000 (5 × 20000) |
| `Garnet_512_Cap896` | 1024 B | 67.8 k | 66.2 | 32.4 µs | 31.6 | 100000 (5 × 20000) |
| `Garnet_512_Cap896` | 4096 B | 215.1 k | 52.5 | 103 µs | 39.9 | 44750 (5 × 8950) |
| `Garnet_512_Cap896` | 8192 B | 416.9 k | 50.9 | 199 µs | 41.1 | 23855 (5 × 4771) |
| `Garnet_512_Cap896` | 16384 B | 826.0 k | 50.4 | 395 µs | 41.5 | 12380 (5 × 2476) |
| `Garnet_512_Cap896` | 65536 B | 3.22 M | 49.1 | 1.54 ms | 42.6 | 3170 (5 × 634) |
| `Garnet_512_Cap1024` | 32 B | 11.3 k | 353.0 | 5.4 µs | 5.9 | 100000 (5 × 20000) |
| `Garnet_512_Cap1024` | 128 B | 14.6 k | 114.0 | 6.97 µs | 18.4 | 100000 (5 × 20000) |
| `Garnet_512_Cap1024` | 512 B | 24.5 k | 47.9 | 11.7 µs | 43.7 | 100000 (5 × 20000) |
| `Garnet_512_Cap1024` | 1024 B | 37.7 k | 36.8 | 18 µs | 56.8 | 100000 (5 × 20000) |
| `Garnet_512_Cap1024` | 4096 B | 116.8 k | 28.5 | 55.8 µs | 73.4 | 76835 (5 × 15367) |
| `Garnet_512_Cap1024` | 8192 B | 222.1 k | 27.1 | 106 µs | 77.2 | 42930 (5 × 8586) |
| `Garnet_512_Cap1024` | 16384 B | 432.9 k | 26.4 | 207 µs | 79.2 | 22475 (5 × 4495) |
| `Garnet_512_Cap1024` | 65536 B | 1.70 M | 26.0 | 813 µs | 80.6 | 5855 (5 × 1171) |
| `Garnet_768` | 32 B | 10.9 k | 339.4 | 5.19 µs | 6.2 | 100000 (5 × 20000) |
| `Garnet_768` | 128 B | 17.3 k | 135.0 | 8.26 µs | 15.5 | 100000 (5 × 20000) |
| `Garnet_768` | 512 B | 36.5 k | 71.3 | 17.4 µs | 29.3 | 37005 (5 × 7401) |
| `Garnet_768` | 1024 B | 62.1 k | 60.6 | 29.7 µs | 34.5 | 100000 (5 × 20000) |
| `Garnet_768` | 4096 B | 215.4 k | 52.6 | 103 µs | 39.8 | 44855 (5 × 8971) |
| `Garnet_768` | 8192 B | 419.7 k | 51.2 | 200 µs | 40.9 | 23645 (5 × 4729) |
| `Garnet_768` | 16384 B | 828.4 k | 50.6 | 396 µs | 41.4 | 12225 (5 × 2445) |
| `Garnet_768` | 65536 B | 3.29 M | 50.3 | 1.57 ms | 41.7 | 3080 (5 × 616) |
| `Garnet_1024` | 32 B | 48.4 k | 1511.6 | 23.1 µs | 1.4 | 100000 (5 × 20000) |
| `Garnet_1024` | 128 B | 62.7 k | 489.5 | 29.9 µs | 4.3 | 100000 (5 × 20000) |
| `Garnet_1024` | 512 B | 106.1 k | 207.2 | 50.7 µs | 10.1 | 80645 (5 × 16129) |
| `Garnet_1024` | 1024 B | 179.7 k | 175.5 | 85.8 µs | 11.9 | 50230 (5 × 10046) |
| `Garnet_1024` | 4096 B | 574.6 k | 140.3 | 274 µs | 14.9 | 17020 (5 × 3404) |
| `Garnet_1024` | 8192 B | 1.12 M | 136.3 | 533 µs | 15.4 | 8890 (5 × 1778) |
| `Garnet_1024` | 16384 B | 2.18 M | 132.9 | 1.04 ms | 15.7 | 4580 (5 × 916) |
| `Garnet_1024` | 65536 B | 8.63 M | 131.6 | 4.12 ms | 15.9 | 1170 (5 × 234) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Garnet_512_Cap512` | hash_32 | 42481 | 1720 KiB | 1788 KiB |
| `Garnet_512_Cap512` | hash_128 | 42481 | 1712 KiB | 1776 KiB |
| `Garnet_512_Cap512` | hash_512 | 42481 | 1716 KiB | 1800 KiB |
| `Garnet_512_Cap512` | hash_1024 | 42481 | 1724 KiB | 1812 KiB |
| `Garnet_512_Cap512` | hash_4096 | 42481 | 1696 KiB | 1764 KiB |
| `Garnet_512_Cap512` | hash_8192 | 42481 | 1708 KiB | 1816 KiB |
| `Garnet_512_Cap512` | hash_16384 | 42481 | 1728 KiB | 1820 KiB |
| `Garnet_512_Cap512` | hash_65536 | 42481 | 1764 KiB | 1936 KiB |
| `Garnet_512_Cap640` | hash_32 | 42481 | 1720 KiB | 1784 KiB |
| `Garnet_512_Cap640` | hash_128 | 42481 | 1696 KiB | 1760 KiB |
| `Garnet_512_Cap640` | hash_512 | 42481 | 1692 KiB | 1788 KiB |
| `Garnet_512_Cap640` | hash_1024 | 42481 | 1704 KiB | 1804 KiB |
| `Garnet_512_Cap640` | hash_4096 | 42481 | 1728 KiB | 1796 KiB |
| `Garnet_512_Cap640` | hash_8192 | 42481 | 1728 KiB | 1816 KiB |
| `Garnet_512_Cap640` | hash_16384 | 42481 | 1740 KiB | 1844 KiB |
| `Garnet_512_Cap640` | hash_65536 | 42481 | 1780 KiB | 1908 KiB |
| `Garnet_512_Cap768` | hash_32 | 42609 | 1720 KiB | 1800 KiB |
| `Garnet_512_Cap768` | hash_128 | 42609 | 1696 KiB | 1804 KiB |
| `Garnet_512_Cap768` | hash_512 | 42609 | 1724 KiB | 1808 KiB |
| `Garnet_512_Cap768` | hash_1024 | 42609 | 1704 KiB | 1816 KiB |
| `Garnet_512_Cap768` | hash_4096 | 42609 | 1720 KiB | 1788 KiB |
| `Garnet_512_Cap768` | hash_8192 | 42609 | 1716 KiB | 1788 KiB |
| `Garnet_512_Cap768` | hash_16384 | 42609 | 1712 KiB | 1792 KiB |
| `Garnet_512_Cap768` | hash_65536 | 42609 | 1788 KiB | 1916 KiB |
| `Garnet_512_Cap896` | hash_32 | 42673 | 1712 KiB | 1788 KiB |
| `Garnet_512_Cap896` | hash_128 | 42673 | 1684 KiB | 1808 KiB |
| `Garnet_512_Cap896` | hash_512 | 42673 | 1712 KiB | 1776 KiB |
| `Garnet_512_Cap896` | hash_1024 | 42673 | 1720 KiB | 1804 KiB |
| `Garnet_512_Cap896` | hash_4096 | 42673 | 1704 KiB | 1808 KiB |
| `Garnet_512_Cap896` | hash_8192 | 42673 | 1708 KiB | 1824 KiB |
| `Garnet_512_Cap896` | hash_16384 | 42673 | 1732 KiB | 1812 KiB |
| `Garnet_512_Cap896` | hash_65536 | 42673 | 1764 KiB | 1940 KiB |
| `Garnet_512_Cap1024` | hash_32 | 41585 | 1700 KiB | 1764 KiB |
| `Garnet_512_Cap1024` | hash_128 | 41585 | 1708 KiB | 1800 KiB |
| `Garnet_512_Cap1024` | hash_512 | 41585 | 1684 KiB | 1808 KiB |
| `Garnet_512_Cap1024` | hash_1024 | 41585 | 1716 KiB | 1792 KiB |
| `Garnet_512_Cap1024` | hash_4096 | 41585 | 1688 KiB | 1816 KiB |
| `Garnet_512_Cap1024` | hash_8192 | 41585 | 1720 KiB | 1792 KiB |
| `Garnet_512_Cap1024` | hash_16384 | 41585 | 1732 KiB | 1844 KiB |
| `Garnet_512_Cap1024` | hash_65536 | 41585 | 1764 KiB | 1928 KiB |
| `Garnet_768` | hash_32 | 41537 | 1724 KiB | 1792 KiB |
| `Garnet_768` | hash_128 | 41537 | 1712 KiB | 1776 KiB |
| `Garnet_768` | hash_512 | 41537 | 1676 KiB | 1804 KiB |
| `Garnet_768` | hash_1024 | 41537 | 1708 KiB | 1776 KiB |
| `Garnet_768` | hash_4096 | 41537 | 1728 KiB | 1796 KiB |
| `Garnet_768` | hash_8192 | 41537 | 1692 KiB | 1824 KiB |
| `Garnet_768` | hash_16384 | 41537 | 1736 KiB | 1836 KiB |
| `Garnet_768` | hash_65536 | 41537 | 1692 KiB | 1884 KiB |
| `Garnet_1024` | hash_32 | 41537 | 1704 KiB | 1800 KiB |
| `Garnet_1024` | hash_128 | 41537 | 1712 KiB | 1804 KiB |
| `Garnet_1024` | hash_512 | 41537 | 1676 KiB | 1804 KiB |
| `Garnet_1024` | hash_1024 | 41537 | 1720 KiB | 1812 KiB |
| `Garnet_1024` | hash_4096 | 41537 | 1708 KiB | 1812 KiB |
| `Garnet_1024` | hash_8192 | 41537 | 1724 KiB | 1828 KiB |
| `Garnet_1024` | hash_16384 | 41537 | 1736 KiB | 1816 KiB |
| `Garnet_1024` | hash_65536 | 41537 | 1768 KiB | 1928 KiB |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Garnet_512_Cap512` | KAT log (sha256 `da167e6706232940…`) | `kat/hash-11/Garnet_512_Cap512.log` |
| `Garnet_512_Cap512` | timing hash_1024 | `records/hash-11/Garnet_512_Cap512__hash_1024.json` |
| `Garnet_512_Cap512` | timing hash_128 | `records/hash-11/Garnet_512_Cap512__hash_128.json` |
| `Garnet_512_Cap512` | timing hash_16384 | `records/hash-11/Garnet_512_Cap512__hash_16384.json` |
| `Garnet_512_Cap512` | timing hash_32 | `records/hash-11/Garnet_512_Cap512__hash_32.json` |
| `Garnet_512_Cap512` | timing hash_4096 | `records/hash-11/Garnet_512_Cap512__hash_4096.json` |
| `Garnet_512_Cap512` | timing hash_512 | `records/hash-11/Garnet_512_Cap512__hash_512.json` |
| `Garnet_512_Cap512` | timing hash_65536 | `records/hash-11/Garnet_512_Cap512__hash_65536.json` |
| `Garnet_512_Cap512` | timing hash_8192 | `records/hash-11/Garnet_512_Cap512__hash_8192.json` |
| `Garnet_512_Cap640` | KAT log (sha256 `0d2d3d1c890af10e…`) | `kat/hash-11/Garnet_512_Cap640.log` |
| `Garnet_512_Cap640` | timing hash_1024 | `records/hash-11/Garnet_512_Cap640__hash_1024.json` |
| `Garnet_512_Cap640` | timing hash_128 | `records/hash-11/Garnet_512_Cap640__hash_128.json` |
| `Garnet_512_Cap640` | timing hash_16384 | `records/hash-11/Garnet_512_Cap640__hash_16384.json` |
| `Garnet_512_Cap640` | timing hash_32 | `records/hash-11/Garnet_512_Cap640__hash_32.json` |
| `Garnet_512_Cap640` | timing hash_4096 | `records/hash-11/Garnet_512_Cap640__hash_4096.json` |
| `Garnet_512_Cap640` | timing hash_512 | `records/hash-11/Garnet_512_Cap640__hash_512.json` |
| `Garnet_512_Cap640` | timing hash_65536 | `records/hash-11/Garnet_512_Cap640__hash_65536.json` |
| `Garnet_512_Cap640` | timing hash_8192 | `records/hash-11/Garnet_512_Cap640__hash_8192.json` |
| `Garnet_512_Cap768` | KAT log (sha256 `bf719ede6d88022e…`) | `kat/hash-11/Garnet_512_Cap768.log` |
| `Garnet_512_Cap768` | timing hash_1024 | `records/hash-11/Garnet_512_Cap768__hash_1024.json` |
| `Garnet_512_Cap768` | timing hash_128 | `records/hash-11/Garnet_512_Cap768__hash_128.json` |
| `Garnet_512_Cap768` | timing hash_16384 | `records/hash-11/Garnet_512_Cap768__hash_16384.json` |
| `Garnet_512_Cap768` | timing hash_32 | `records/hash-11/Garnet_512_Cap768__hash_32.json` |
| `Garnet_512_Cap768` | timing hash_4096 | `records/hash-11/Garnet_512_Cap768__hash_4096.json` |
| `Garnet_512_Cap768` | timing hash_512 | `records/hash-11/Garnet_512_Cap768__hash_512.json` |
| `Garnet_512_Cap768` | timing hash_65536 | `records/hash-11/Garnet_512_Cap768__hash_65536.json` |
| `Garnet_512_Cap768` | timing hash_8192 | `records/hash-11/Garnet_512_Cap768__hash_8192.json` |
| `Garnet_512_Cap896` | KAT log (sha256 `d0fd2e725b7e4aa7…`) | `kat/hash-11/Garnet_512_Cap896.log` |
| `Garnet_512_Cap896` | timing hash_1024 | `records/hash-11/Garnet_512_Cap896__hash_1024.json` |
| `Garnet_512_Cap896` | timing hash_128 | `records/hash-11/Garnet_512_Cap896__hash_128.json` |
| `Garnet_512_Cap896` | timing hash_16384 | `records/hash-11/Garnet_512_Cap896__hash_16384.json` |
| `Garnet_512_Cap896` | timing hash_32 | `records/hash-11/Garnet_512_Cap896__hash_32.json` |
| `Garnet_512_Cap896` | timing hash_4096 | `records/hash-11/Garnet_512_Cap896__hash_4096.json` |
| `Garnet_512_Cap896` | timing hash_512 | `records/hash-11/Garnet_512_Cap896__hash_512.json` |
| `Garnet_512_Cap896` | timing hash_65536 | `records/hash-11/Garnet_512_Cap896__hash_65536.json` |
| `Garnet_512_Cap896` | timing hash_8192 | `records/hash-11/Garnet_512_Cap896__hash_8192.json` |
| `Garnet_512_Cap1024` | KAT log (sha256 `ef0dc35f24546151…`) | `kat/hash-11/Garnet_512_Cap1024.log` |
| `Garnet_512_Cap1024` | timing hash_1024 | `records/hash-11/Garnet_512_Cap1024__hash_1024.json` |
| `Garnet_512_Cap1024` | timing hash_128 | `records/hash-11/Garnet_512_Cap1024__hash_128.json` |
| `Garnet_512_Cap1024` | timing hash_16384 | `records/hash-11/Garnet_512_Cap1024__hash_16384.json` |
| `Garnet_512_Cap1024` | timing hash_32 | `records/hash-11/Garnet_512_Cap1024__hash_32.json` |
| `Garnet_512_Cap1024` | timing hash_4096 | `records/hash-11/Garnet_512_Cap1024__hash_4096.json` |
| `Garnet_512_Cap1024` | timing hash_512 | `records/hash-11/Garnet_512_Cap1024__hash_512.json` |
| `Garnet_512_Cap1024` | timing hash_65536 | `records/hash-11/Garnet_512_Cap1024__hash_65536.json` |
| `Garnet_512_Cap1024` | timing hash_8192 | `records/hash-11/Garnet_512_Cap1024__hash_8192.json` |
| `Garnet_768` | KAT log (sha256 `e0175efed77b1aea…`) | `kat/hash-11/Garnet_768.log` |
| `Garnet_768` | timing hash_1024 | `records/hash-11/Garnet_768__hash_1024.json` |
| `Garnet_768` | timing hash_128 | `records/hash-11/Garnet_768__hash_128.json` |
| `Garnet_768` | timing hash_16384 | `records/hash-11/Garnet_768__hash_16384.json` |
| `Garnet_768` | timing hash_32 | `records/hash-11/Garnet_768__hash_32.json` |
| `Garnet_768` | timing hash_4096 | `records/hash-11/Garnet_768__hash_4096.json` |
| `Garnet_768` | timing hash_512 | `records/hash-11/Garnet_768__hash_512.json` |
| `Garnet_768` | timing hash_65536 | `records/hash-11/Garnet_768__hash_65536.json` |
| `Garnet_768` | timing hash_8192 | `records/hash-11/Garnet_768__hash_8192.json` |
| `Garnet_1024` | KAT log (sha256 `ae9f836b30a461d6…`) | `kat/hash-11/Garnet_1024.log` |
| `Garnet_1024` | timing hash_1024 | `records/hash-11/Garnet_1024__hash_1024.json` |
| `Garnet_1024` | timing hash_128 | `records/hash-11/Garnet_1024__hash_128.json` |
| `Garnet_1024` | timing hash_16384 | `records/hash-11/Garnet_1024__hash_16384.json` |
| `Garnet_1024` | timing hash_32 | `records/hash-11/Garnet_1024__hash_32.json` |
| `Garnet_1024` | timing hash_4096 | `records/hash-11/Garnet_1024__hash_4096.json` |
| `Garnet_1024` | timing hash_512 | `records/hash-11/Garnet_1024__hash_512.json` |
| `Garnet_1024` | timing hash_65536 | `records/hash-11/Garnet_1024__hash_65536.json` |
| `Garnet_1024` | timing hash_8192 | `records/hash-11/Garnet_1024__hash_8192.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

