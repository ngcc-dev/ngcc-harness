# kem-35 Scloud+ — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-35` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560881269264384.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: Scloud+
- Implementation versions measured: optimized (AVX2), reference
- Parameter sets: `Scloudplus-128-AES-packed10`, `Scloudplus-128-SHAKE-avx2`, `Scloudplus-128-SHAKE-packed10`, `Scloudplus-128-SM3-packed10`, `Scloudplus-192-AES-packed10`, `Scloudplus-192-SHAKE-packed10`, `Scloudplus-192-SM3-packed10`, `Scloudplus-256-AES-packed10`, `Scloudplus-256-SHAKE-packed10`, `Scloudplus-256-SM3-packed10`, `Scloudplus-384-AES-packed10`, `Scloudplus-384-SHAKE-packed10`, `Scloudplus-384-SM3-packed10`, `Scloudplus-512-AES-packed10`, `Scloudplus-512-SHAKE-packed10`, `Scloudplus-512-SM3-packed10`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-35/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Scloudplus-128-AES-packed10` | guide | PASS |
| `Scloudplus-128-SHAKE-avx2` | guide-performance | PASS |
| `Scloudplus-128-SHAKE-packed10` | guide | PASS |
| `Scloudplus-128-SM3-packed10` | guide | PASS |
| `Scloudplus-192-AES-packed10` | guide | PASS |
| `Scloudplus-192-SHAKE-packed10` | guide | PASS |
| `Scloudplus-192-SM3-packed10` | guide | PASS |
| `Scloudplus-256-AES-packed10` | guide | PASS |
| `Scloudplus-256-SHAKE-packed10` | guide | PASS |
| `Scloudplus-256-SM3-packed10` | guide | PASS |
| `Scloudplus-384-AES-packed10` | guide | PASS |
| `Scloudplus-384-SHAKE-packed10` | guide | PASS |
| `Scloudplus-384-SM3-packed10` | guide | PASS |
| `Scloudplus-512-AES-packed10` | guide | PASS |
| `Scloudplus-512-SHAKE-packed10` | guide | PASS |
| `Scloudplus-512-SM3-packed10` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Scloudplus-128-AES-packed10` | keygen | 41.90 M | 20 ms | 50 | 20 ms | 250 (5 × 50) |
| `Scloudplus-128-AES-packed10` | enc | 42.02 M | 20.1 ms | 49.8 | 20.1 ms | 250 (5 × 50) |
| `Scloudplus-128-AES-packed10` | dec | 42.02 M | 20.1 ms | 49.8 | 20.1 ms | 250 (5 × 50) |
| `Scloudplus-128-SHAKE-avx2` | keygen | 1.68 M | 804 µs | 1.24e+03 | 804 µs | 5930 (5 × 1186) |
| `Scloudplus-128-SHAKE-avx2` | enc | 1.73 M | 830 µs | 1.21e+03 | 829 µs | 5970 (5 × 1194) |
| `Scloudplus-128-SHAKE-avx2` | dec | 1.64 M | 785 µs | 1.27e+03 | 785 µs | 6305 (5 × 1261) |
| `Scloudplus-128-SHAKE-packed10` | keygen | 6.31 M | 3.01 ms | 332 | 3.02 ms | 1655 (5 × 331) |
| `Scloudplus-128-SHAKE-packed10` | enc | 6.43 M | 3.07 ms | 325 | 3.08 ms | 1565 (5 × 313) |
| `Scloudplus-128-SHAKE-packed10` | dec | 6.44 M | 3.08 ms | 325 | 3.08 ms | 1630 (5 × 326) |
| `Scloudplus-128-SM3-packed10` | keygen | 23.68 M | 11.3 ms | 88.4 | 11.3 ms | 440 (5 × 88) |
| `Scloudplus-128-SM3-packed10` | enc | 23.93 M | 11.4 ms | 87.5 | 11.4 ms | 440 (5 × 88) |
| `Scloudplus-128-SM3-packed10` | dec | 23.78 M | 11.4 ms | 88 | 11.3 ms | 445 (5 × 89) |
| `Scloudplus-192-AES-packed10` | keygen | 77.94 M | 37.2 ms | 26.9 | 37.2 ms | 135 (5 × 27) |
| `Scloudplus-192-AES-packed10` | enc | 78.49 M | 37.5 ms | 26.7 | 37.5 ms | 135 (5 × 27) |
| `Scloudplus-192-AES-packed10` | dec | 78.61 M | 37.5 ms | 26.6 | 37.6 ms | 135 (5 × 27) |
| `Scloudplus-192-SHAKE-packed10` | keygen | 10.50 M | 5.02 ms | 199 | 5.01 ms | 840 (5 × 168) |
| `Scloudplus-192-SHAKE-packed10` | enc | 11.04 M | 5.27 ms | 190 | 5.27 ms | 905 (5 × 181) |
| `Scloudplus-192-SHAKE-packed10` | dec | 11.28 M | 5.39 ms | 185 | 5.38 ms | 770 (5 × 154) |
| `Scloudplus-192-SM3-packed10` | keygen | 44.44 M | 21.2 ms | 47.1 | 21.2 ms | 235 (5 × 47) |
| `Scloudplus-192-SM3-packed10` | enc | 45.14 M | 21.6 ms | 46.4 | 21.6 ms | 235 (5 × 47) |
| `Scloudplus-192-SM3-packed10` | dec | 45.03 M | 21.5 ms | 46.5 | 21.5 ms | 235 (5 × 47) |
| `Scloudplus-256-AES-packed10` | keygen | 158.93 M | 75.9 ms | 13.2 | 75.9 ms | 100 (5 × 20) |
| `Scloudplus-256-AES-packed10` | enc | 159.85 M | 76.4 ms | 13.1 | 76.4 ms | 100 (5 × 20) |
| `Scloudplus-256-AES-packed10` | dec | 159.98 M | 76.4 ms | 13.1 | 76.4 ms | 100 (5 × 20) |
| `Scloudplus-256-SHAKE-packed10` | keygen | 23.56 M | 11.3 ms | 88.9 | 11.3 ms | 435 (5 × 87) |
| `Scloudplus-256-SHAKE-packed10` | enc | 24.61 M | 11.8 ms | 85.1 | 11.8 ms | 420 (5 × 84) |
| `Scloudplus-256-SHAKE-packed10` | dec | 24.83 M | 11.9 ms | 84.3 | 11.9 ms | 400 (5 × 80) |
| `Scloudplus-256-SM3-packed10` | keygen | 90.93 M | 43.4 ms | 23 | 43.4 ms | 115 (5 × 23) |
| `Scloudplus-256-SM3-packed10` | enc | 92.25 M | 44.1 ms | 22.7 | 44.1 ms | 115 (5 × 23) |
| `Scloudplus-256-SM3-packed10` | dec | 92.03 M | 44 ms | 22.7 | 44 ms | 115 (5 × 23) |
| `Scloudplus-384-AES-packed10` | keygen | 315.05 M | 152 ms | 6.59 | 151 ms | 100 (5 × 20) |
| `Scloudplus-384-AES-packed10` | enc | 316.43 M | 151 ms | 6.62 | 151 ms | 100 (5 × 20) |
| `Scloudplus-384-AES-packed10` | dec | 317.13 M | 153 ms | 6.54 | 151 ms | 100 (5 × 20) |
| `Scloudplus-384-SHAKE-packed10` | keygen | 46.78 M | 22.3 ms | 44.8 | 22.3 ms | 215 (5 × 43) |
| `Scloudplus-384-SHAKE-packed10` | enc | 48.27 M | 23.1 ms | 43.4 | 23.1 ms | 205 (5 × 41) |
| `Scloudplus-384-SHAKE-packed10` | dec | 49.53 M | 23.7 ms | 42.2 | 23.8 ms | 205 (5 × 41) |
| `Scloudplus-384-SM3-packed10` | keygen | 179.31 M | 85.7 ms | 11.7 | 85.7 ms | 100 (5 × 20) |
| `Scloudplus-384-SM3-packed10` | enc | 182.22 M | 87.1 ms | 11.5 | 87 ms | 100 (5 × 20) |
| `Scloudplus-384-SM3-packed10` | dec | 181.76 M | 86.8 ms | 11.5 | 86.8 ms | 100 (5 × 20) |
| `Scloudplus-512-AES-packed10` | keygen | 656.92 M | 315 ms | 3.17 | 314 ms | 100 (5 × 20) |
| `Scloudplus-512-AES-packed10` | enc | 657.55 M | 315 ms | 3.17 | 314 ms | 100 (5 × 20) |
| `Scloudplus-512-AES-packed10` | dec | 658.67 M | 316 ms | 3.16 | 315 ms | 100 (5 × 20) |
| `Scloudplus-512-SHAKE-packed10` | keygen | 99.62 M | 47.6 ms | 21 | 47.6 ms | 105 (5 × 21) |
| `Scloudplus-512-SHAKE-packed10` | enc | 101.20 M | 48.4 ms | 20.7 | 48.4 ms | 110 (5 × 22) |
| `Scloudplus-512-SHAKE-packed10` | dec | 102.35 M | 48.9 ms | 20.4 | 49 ms | 105 (5 × 21) |
| `Scloudplus-512-SM3-packed10` | keygen | 376.23 M | 181 ms | 5.52 | 180 ms | 100 (5 × 20) |
| `Scloudplus-512-SM3-packed10` | enc | 379.05 M | 181 ms | 5.52 | 181 ms | 100 (5 × 20) |
| `Scloudplus-512-SM3-packed10` | dec | 377.95 M | 181 ms | 5.54 | 181 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Scloudplus-128-AES-packed10` | keygen | 28569 | 1728 KiB | 1812 KiB |
| `Scloudplus-128-AES-packed10` | enc | 28569 | 1748 KiB | 1816 KiB |
| `Scloudplus-128-AES-packed10` | dec | 28569 | 1748 KiB | 1812 KiB |
| `Scloudplus-128-SHAKE-avx2` | keygen | 82413 | 1772 KiB | 1868 KiB |
| `Scloudplus-128-SHAKE-avx2` | enc | 82413 | 1784 KiB | 1908 KiB |
| `Scloudplus-128-SHAKE-avx2` | dec | 82413 | 1820 KiB | 1884 KiB |
| `Scloudplus-128-SHAKE-packed10` | keygen | 26793 | 1724 KiB | 1812 KiB |
| `Scloudplus-128-SHAKE-packed10` | enc | 26793 | 1748 KiB | 1832 KiB |
| `Scloudplus-128-SHAKE-packed10` | dec | 26793 | 1744 KiB | 1808 KiB |
| `Scloudplus-128-SM3-packed10` | keygen | 31705 | 1728 KiB | 1828 KiB |
| `Scloudplus-128-SM3-packed10` | enc | 31705 | 1732 KiB | 1864 KiB |
| `Scloudplus-128-SM3-packed10` | dec | 31705 | 1732 KiB | 1856 KiB |
| `Scloudplus-192-AES-packed10` | keygen | 28185 | 1720 KiB | 1864 KiB |
| `Scloudplus-192-AES-packed10` | enc | 28185 | 1780 KiB | 1872 KiB |
| `Scloudplus-192-AES-packed10` | dec | 28185 | 1744 KiB | 1872 KiB |
| `Scloudplus-192-SHAKE-packed10` | keygen | 26473 | 1724 KiB | 1856 KiB |
| `Scloudplus-192-SHAKE-packed10` | enc | 26473 | 1784 KiB | 1848 KiB |
| `Scloudplus-192-SHAKE-packed10` | dec | 26473 | 1752 KiB | 1876 KiB |
| `Scloudplus-192-SM3-packed10` | keygen | 31449 | 1724 KiB | 1904 KiB |
| `Scloudplus-192-SM3-packed10` | enc | 31449 | 1812 KiB | 1916 KiB |
| `Scloudplus-192-SM3-packed10` | dec | 31449 | 1824 KiB | 1912 KiB |
| `Scloudplus-256-AES-packed10` | keygen | 29057 | 1756 KiB | 1900 KiB |
| `Scloudplus-256-AES-packed10` | enc | 29057 | 1828 KiB | 1892 KiB |
| `Scloudplus-256-AES-packed10` | dec | 29057 | 1808 KiB | 1920 KiB |
| `Scloudplus-256-SHAKE-packed10` | keygen | 27281 | 1756 KiB | 1872 KiB |
| `Scloudplus-256-SHAKE-packed10` | enc | 27281 | 1812 KiB | 1912 KiB |
| `Scloudplus-256-SHAKE-packed10` | dec | 27281 | 1808 KiB | 1904 KiB |
| `Scloudplus-256-SM3-packed10` | keygen | 32257 | 1752 KiB | 1916 KiB |
| `Scloudplus-256-SM3-packed10` | enc | 32257 | 1880 KiB | 1956 KiB |
| `Scloudplus-256-SM3-packed10` | dec | 32257 | 1868 KiB | 1932 KiB |
| `Scloudplus-384-AES-packed10` | keygen | 28761 | 1812 KiB | 1984 KiB |
| `Scloudplus-384-AES-packed10` | enc | 28761 | 1948 KiB | 2020 KiB |
| `Scloudplus-384-AES-packed10` | dec | 28761 | 1956 KiB | 2020 KiB |
| `Scloudplus-384-SHAKE-packed10` | keygen | 26985 | 1808 KiB | 1980 KiB |
| `Scloudplus-384-SHAKE-packed10` | enc | 26985 | 1924 KiB | 2016 KiB |
| `Scloudplus-384-SHAKE-packed10` | dec | 26985 | 1952 KiB | 2040 KiB |
| `Scloudplus-384-SM3-packed10` | keygen | 32025 | 1792 KiB | 2064 KiB |
| `Scloudplus-384-SM3-packed10` | enc | 32025 | 2044 KiB | 2124 KiB |
| `Scloudplus-384-SM3-packed10` | dec | 32025 | 2040 KiB | 2104 KiB |
| `Scloudplus-512-AES-packed10` | keygen | 28953 | 1832 KiB | 2084 KiB |
| `Scloudplus-512-AES-packed10` | enc | 28953 | 2032 KiB | 2148 KiB |
| `Scloudplus-512-AES-packed10` | dec | 28953 | 2060 KiB | 2124 KiB |
| `Scloudplus-512-SHAKE-packed10` | keygen | 27177 | 1824 KiB | 2076 KiB |
| `Scloudplus-512-SHAKE-packed10` | enc | 27177 | 2044 KiB | 2144 KiB |
| `Scloudplus-512-SHAKE-packed10` | dec | 27177 | 2040 KiB | 2152 KiB |
| `Scloudplus-512-SM3-packed10` | keygen | 32281 | 1836 KiB | 2256 KiB |
| `Scloudplus-512-SM3-packed10` | enc | 32281 | 2248 KiB | 2324 KiB |
| `Scloudplus-512-SM3-packed10` | dec | 32281 | 2232 KiB | 2328 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `Scloudplus-128-AES-packed10` | 6096 | 7440 | 6160 | 16 |
| `Scloudplus-128-SHAKE-avx2` | 6096 | 7440 | 6160 | 16 |
| `Scloudplus-128-SHAKE-packed10` | 6096 | 7440 | 6160 | 16 |
| `Scloudplus-128-SM3-packed10` | 6096 | 7440 | 6160 | 16 |
| `Scloudplus-192-AES-packed10` | 11456 | 13872 | 12645 | 24 |
| `Scloudplus-192-SHAKE-packed10` | 11456 | 13872 | 12645 | 24 |
| `Scloudplus-192-SM3-packed10` | 11456 | 13872 | 12645 | 24 |
| `Scloudplus-256-AES-packed10` | 16296 | 19680 | 17925 | 32 |
| `Scloudplus-256-SHAKE-packed10` | 16296 | 19680 | 17925 | 32 |
| `Scloudplus-256-SM3-packed10` | 16296 | 19680 | 17925 | 32 |
| `Scloudplus-384-AES-packed10` | 33296 | 40144 | 33600 | 48 |
| `Scloudplus-384-SHAKE-packed10` | 33296 | 40144 | 33600 | 48 |
| `Scloudplus-384-SM3-packed10` | 33296 | 40144 | 33600 | 48 |
| `Scloudplus-512-AES-packed10` | 48016 | 57872 | 48320 | 64 |
| `Scloudplus-512-SHAKE-packed10` | 48016 | 57872 | 48320 | 64 |
| `Scloudplus-512-SM3-packed10` | 48016 | 57872 | 48320 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **instance-dependent** — SM3 sets: private modified copy of auxfunc (official not linked); AES/SHAKE sets: own AES/Keccak (bypass); benchmark pilot uses SHAKE

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Scloudplus-128-AES-packed10` | keygen | 0.0% | 0.0% | drng 2 |
| `Scloudplus-128-AES-packed10` | enc | 0.0% | 0.0% | drng 1 |
| `Scloudplus-128-AES-packed10` | dec | 0.0% | 0.0% | – |
| `Scloudplus-128-SHAKE-packed10` | keygen | 0.0% | 0.2% | drng 2 |
| `Scloudplus-128-SHAKE-packed10` | enc | 0.0% | 0.1% | drng 1 |
| `Scloudplus-128-SHAKE-packed10` | dec | 0.0% | 0.0% | – |
| `Scloudplus-128-SM3-packed10` | keygen | 92% | 0.1% | drng 2, pseudoXOF 609, pseudohash 1 |
| `Scloudplus-128-SM3-packed10` | enc | 92% | 0.0% | drng 1, pseudoXOF 611, pseudohash 1 |
| `Scloudplus-128-SM3-packed10` | dec | 92% | 0.0% | pseudoXOF 611 |
| `Scloudplus-192-AES-packed10` | keygen | 0.0% | 0.0% | drng 2 |
| `Scloudplus-192-AES-packed10` | enc | 0.0% | 0.0% | drng 1 |
| `Scloudplus-192-AES-packed10` | dec | 0.0% | 0.0% | – |
| `Scloudplus-192-SHAKE-packed10` | keygen | 0.0% | 0.1% | drng 2 |
| `Scloudplus-192-SHAKE-packed10` | enc | 0.0% | 0.0% | drng 1 |
| `Scloudplus-192-SHAKE-packed10` | dec | 0.0% | 0.0% | – |
| `Scloudplus-192-SM3-packed10` | keygen | 93% | 0.0% | drng 2, pseudoXOF 833, pseudohash 1 |
| `Scloudplus-192-SM3-packed10` | enc | 92% | 0.0% | drng 1, pseudoXOF 835, pseudohash 1 |
| `Scloudplus-192-SM3-packed10` | dec | 91% | 0.0% | pseudoXOF 835 |
| `Scloudplus-256-AES-packed10` | keygen | 0.0% | 0.0% | drng 2 |
| `Scloudplus-256-AES-packed10` | enc | 0.0% | 0.0% | drng 1 |
| `Scloudplus-256-AES-packed10` | dec | 0.0% | 0.0% | – |
| `Scloudplus-256-SHAKE-packed10` | keygen | 0.0% | 0.1% | drng 2 |
| `Scloudplus-256-SHAKE-packed10` | enc | 0.0% | 0.0% | drng 1 |
| `Scloudplus-256-SHAKE-packed10` | dec | 0.0% | 0.0% | – |
| `Scloudplus-256-SM3-packed10` | keygen | 91% | 0.0% | drng 2, pseudoXOF 1.18e+03, pseudohash 1 |
| `Scloudplus-256-SM3-packed10` | enc | 91% | 0.0% | drng 1, pseudoXOF 1.19e+03, pseudohash 1 |
| `Scloudplus-256-SM3-packed10` | dec | 90% | 0.0% | pseudoXOF 1.19e+03 |
| `Scloudplus-384-AES-packed10` | keygen | 0.0% | 0.0% | drng 2 |
| `Scloudplus-384-AES-packed10` | enc | 0.0% | 0.0% | drng 1 |
| `Scloudplus-384-AES-packed10` | dec | 0.0% | 0.0% | – |
| `Scloudplus-384-SHAKE-packed10` | keygen | 0.0% | 0.0% | drng 2 |
| `Scloudplus-384-SHAKE-packed10` | enc | 0.0% | 0.0% | drng 1 |
| `Scloudplus-384-SHAKE-packed10` | dec | 0.0% | 0.0% | – |
| `Scloudplus-384-SM3-packed10` | keygen | 90% | 0.0% | drng 2, pseudoXOF 1.66e+03, pseudohash 1 |
| `Scloudplus-384-SM3-packed10` | enc | 90% | 0.0% | drng 1, pseudoXOF 1.67e+03, pseudohash 1 |
| `Scloudplus-384-SM3-packed10` | dec | 89% | 0.0% | pseudoXOF 1.67e+03 |
| `Scloudplus-512-AES-packed10` | keygen | 0.0% | 0.0% | drng 2 |
| `Scloudplus-512-AES-packed10` | enc | 0.0% | 0.0% | drng 1 |
| `Scloudplus-512-AES-packed10` | dec | 0.0% | 0.0% | – |
| `Scloudplus-512-SHAKE-packed10` | keygen | 0.0% | 0.0% | drng 2 |
| `Scloudplus-512-SHAKE-packed10` | enc | 0.0% | 0.0% | drng 1 |
| `Scloudplus-512-SHAKE-packed10` | dec | 0.0% | 0.0% | – |
| `Scloudplus-512-SM3-packed10` | keygen | 89% | 0.0% | drng 2, pseudoXOF 2.4e+03, pseudohash 1 |
| `Scloudplus-512-SM3-packed10` | enc | 89% | 0.0% | drng 1, pseudoXOF 2.4e+03, pseudohash 1 |
| `Scloudplus-512-SM3-packed10` | dec | 89% | 0.0% | pseudoXOF 2.4e+03 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Scloudplus-128-AES-packed10` | KAT log (sha256 `c672e793e6304494…`) | `kat/kem-35/Scloudplus-128-AES-packed10.log` |
| `Scloudplus-128-AES-packed10` | timing dec | `records/kem-35/Scloudplus-128-AES-packed10__dec.json` |
| `Scloudplus-128-AES-packed10` | timing enc | `records/kem-35/Scloudplus-128-AES-packed10__enc.json` |
| `Scloudplus-128-AES-packed10` | timing keygen | `records/kem-35/Scloudplus-128-AES-packed10__keygen.json` |
| `Scloudplus-128-AES-packed10` | hash profile dec | `profile/kem-35/Scloudplus-128-AES-packed10__dec.json` |
| `Scloudplus-128-AES-packed10` | hash profile enc | `profile/kem-35/Scloudplus-128-AES-packed10__enc.json` |
| `Scloudplus-128-AES-packed10` | hash profile keygen | `profile/kem-35/Scloudplus-128-AES-packed10__keygen.json` |
| `Scloudplus-128-SHAKE-avx2` | KAT log (sha256 `2a192259d13d6f43…`) | `kat/kem-35/Scloudplus-128-SHAKE-avx2.log` |
| `Scloudplus-128-SHAKE-avx2` | timing dec | `records/kem-35/Scloudplus-128-SHAKE-avx2__dec.json` |
| `Scloudplus-128-SHAKE-avx2` | timing enc | `records/kem-35/Scloudplus-128-SHAKE-avx2__enc.json` |
| `Scloudplus-128-SHAKE-avx2` | timing keygen | `records/kem-35/Scloudplus-128-SHAKE-avx2__keygen.json` |
| `Scloudplus-128-SHAKE-packed10` | KAT log (sha256 `271091ceb6e314e6…`) | `kat/kem-35/Scloudplus-128-SHAKE-packed10.log` |
| `Scloudplus-128-SHAKE-packed10` | timing dec | `records/kem-35/Scloudplus-128-SHAKE-packed10__dec.json` |
| `Scloudplus-128-SHAKE-packed10` | timing enc | `records/kem-35/Scloudplus-128-SHAKE-packed10__enc.json` |
| `Scloudplus-128-SHAKE-packed10` | timing keygen | `records/kem-35/Scloudplus-128-SHAKE-packed10__keygen.json` |
| `Scloudplus-128-SHAKE-packed10` | hash profile dec | `profile/kem-35/Scloudplus-128-SHAKE-packed10__dec.json` |
| `Scloudplus-128-SHAKE-packed10` | hash profile enc | `profile/kem-35/Scloudplus-128-SHAKE-packed10__enc.json` |
| `Scloudplus-128-SHAKE-packed10` | hash profile keygen | `profile/kem-35/Scloudplus-128-SHAKE-packed10__keygen.json` |
| `Scloudplus-128-SM3-packed10` | KAT log (sha256 `f4c6078f9a047790…`) | `kat/kem-35/Scloudplus-128-SM3-packed10.log` |
| `Scloudplus-128-SM3-packed10` | timing dec | `records/kem-35/Scloudplus-128-SM3-packed10__dec.json` |
| `Scloudplus-128-SM3-packed10` | timing enc | `records/kem-35/Scloudplus-128-SM3-packed10__enc.json` |
| `Scloudplus-128-SM3-packed10` | timing keygen | `records/kem-35/Scloudplus-128-SM3-packed10__keygen.json` |
| `Scloudplus-128-SM3-packed10` | hash profile dec | `profile/kem-35/Scloudplus-128-SM3-packed10__dec.json` |
| `Scloudplus-128-SM3-packed10` | hash profile enc | `profile/kem-35/Scloudplus-128-SM3-packed10__enc.json` |
| `Scloudplus-128-SM3-packed10` | hash profile keygen | `profile/kem-35/Scloudplus-128-SM3-packed10__keygen.json` |
| `Scloudplus-192-AES-packed10` | KAT log (sha256 `891ea624590a8526…`) | `kat/kem-35/Scloudplus-192-AES-packed10.log` |
| `Scloudplus-192-AES-packed10` | timing dec | `records/kem-35/Scloudplus-192-AES-packed10__dec.json` |
| `Scloudplus-192-AES-packed10` | timing enc | `records/kem-35/Scloudplus-192-AES-packed10__enc.json` |
| `Scloudplus-192-AES-packed10` | timing keygen | `records/kem-35/Scloudplus-192-AES-packed10__keygen.json` |
| `Scloudplus-192-AES-packed10` | hash profile dec | `profile/kem-35/Scloudplus-192-AES-packed10__dec.json` |
| `Scloudplus-192-AES-packed10` | hash profile enc | `profile/kem-35/Scloudplus-192-AES-packed10__enc.json` |
| `Scloudplus-192-AES-packed10` | hash profile keygen | `profile/kem-35/Scloudplus-192-AES-packed10__keygen.json` |
| `Scloudplus-192-SHAKE-packed10` | KAT log (sha256 `8d30f5452fd61110…`) | `kat/kem-35/Scloudplus-192-SHAKE-packed10.log` |
| `Scloudplus-192-SHAKE-packed10` | timing dec | `records/kem-35/Scloudplus-192-SHAKE-packed10__dec.json` |
| `Scloudplus-192-SHAKE-packed10` | timing enc | `records/kem-35/Scloudplus-192-SHAKE-packed10__enc.json` |
| `Scloudplus-192-SHAKE-packed10` | timing keygen | `records/kem-35/Scloudplus-192-SHAKE-packed10__keygen.json` |
| `Scloudplus-192-SHAKE-packed10` | hash profile dec | `profile/kem-35/Scloudplus-192-SHAKE-packed10__dec.json` |
| `Scloudplus-192-SHAKE-packed10` | hash profile enc | `profile/kem-35/Scloudplus-192-SHAKE-packed10__enc.json` |
| `Scloudplus-192-SHAKE-packed10` | hash profile keygen | `profile/kem-35/Scloudplus-192-SHAKE-packed10__keygen.json` |
| `Scloudplus-192-SM3-packed10` | KAT log (sha256 `0bbc1ca140f5fbf2…`) | `kat/kem-35/Scloudplus-192-SM3-packed10.log` |
| `Scloudplus-192-SM3-packed10` | timing dec | `records/kem-35/Scloudplus-192-SM3-packed10__dec.json` |
| `Scloudplus-192-SM3-packed10` | timing enc | `records/kem-35/Scloudplus-192-SM3-packed10__enc.json` |
| `Scloudplus-192-SM3-packed10` | timing keygen | `records/kem-35/Scloudplus-192-SM3-packed10__keygen.json` |
| `Scloudplus-192-SM3-packed10` | hash profile dec | `profile/kem-35/Scloudplus-192-SM3-packed10__dec.json` |
| `Scloudplus-192-SM3-packed10` | hash profile enc | `profile/kem-35/Scloudplus-192-SM3-packed10__enc.json` |
| `Scloudplus-192-SM3-packed10` | hash profile keygen | `profile/kem-35/Scloudplus-192-SM3-packed10__keygen.json` |
| `Scloudplus-256-AES-packed10` | KAT log (sha256 `01d9fc265d46ecf3…`) | `kat/kem-35/Scloudplus-256-AES-packed10.log` |
| `Scloudplus-256-AES-packed10` | timing dec | `records/kem-35/Scloudplus-256-AES-packed10__dec.json` |
| `Scloudplus-256-AES-packed10` | timing enc | `records/kem-35/Scloudplus-256-AES-packed10__enc.json` |
| `Scloudplus-256-AES-packed10` | timing keygen | `records/kem-35/Scloudplus-256-AES-packed10__keygen.json` |
| `Scloudplus-256-AES-packed10` | hash profile dec | `profile/kem-35/Scloudplus-256-AES-packed10__dec.json` |
| `Scloudplus-256-AES-packed10` | hash profile enc | `profile/kem-35/Scloudplus-256-AES-packed10__enc.json` |
| `Scloudplus-256-AES-packed10` | hash profile keygen | `profile/kem-35/Scloudplus-256-AES-packed10__keygen.json` |
| `Scloudplus-256-SHAKE-packed10` | KAT log (sha256 `95d3c6850fcaa633…`) | `kat/kem-35/Scloudplus-256-SHAKE-packed10.log` |
| `Scloudplus-256-SHAKE-packed10` | timing dec | `records/kem-35/Scloudplus-256-SHAKE-packed10__dec.json` |
| `Scloudplus-256-SHAKE-packed10` | timing enc | `records/kem-35/Scloudplus-256-SHAKE-packed10__enc.json` |
| `Scloudplus-256-SHAKE-packed10` | timing keygen | `records/kem-35/Scloudplus-256-SHAKE-packed10__keygen.json` |
| `Scloudplus-256-SHAKE-packed10` | hash profile dec | `profile/kem-35/Scloudplus-256-SHAKE-packed10__dec.json` |
| `Scloudplus-256-SHAKE-packed10` | hash profile enc | `profile/kem-35/Scloudplus-256-SHAKE-packed10__enc.json` |
| `Scloudplus-256-SHAKE-packed10` | hash profile keygen | `profile/kem-35/Scloudplus-256-SHAKE-packed10__keygen.json` |
| `Scloudplus-256-SM3-packed10` | KAT log (sha256 `9f5ce88c00b2e829…`) | `kat/kem-35/Scloudplus-256-SM3-packed10.log` |
| `Scloudplus-256-SM3-packed10` | timing dec | `records/kem-35/Scloudplus-256-SM3-packed10__dec.json` |
| `Scloudplus-256-SM3-packed10` | timing enc | `records/kem-35/Scloudplus-256-SM3-packed10__enc.json` |
| `Scloudplus-256-SM3-packed10` | timing keygen | `records/kem-35/Scloudplus-256-SM3-packed10__keygen.json` |
| `Scloudplus-256-SM3-packed10` | hash profile dec | `profile/kem-35/Scloudplus-256-SM3-packed10__dec.json` |
| `Scloudplus-256-SM3-packed10` | hash profile enc | `profile/kem-35/Scloudplus-256-SM3-packed10__enc.json` |
| `Scloudplus-256-SM3-packed10` | hash profile keygen | `profile/kem-35/Scloudplus-256-SM3-packed10__keygen.json` |
| `Scloudplus-384-AES-packed10` | KAT log (sha256 `59e72d0f0c025811…`) | `kat/kem-35/Scloudplus-384-AES-packed10.log` |
| `Scloudplus-384-AES-packed10` | timing dec | `records/kem-35/Scloudplus-384-AES-packed10__dec.json` |
| `Scloudplus-384-AES-packed10` | timing enc | `records/kem-35/Scloudplus-384-AES-packed10__enc.json` |
| `Scloudplus-384-AES-packed10` | timing keygen | `records/kem-35/Scloudplus-384-AES-packed10__keygen.json` |
| `Scloudplus-384-AES-packed10` | hash profile dec | `profile/kem-35/Scloudplus-384-AES-packed10__dec.json` |
| `Scloudplus-384-AES-packed10` | hash profile enc | `profile/kem-35/Scloudplus-384-AES-packed10__enc.json` |
| `Scloudplus-384-AES-packed10` | hash profile keygen | `profile/kem-35/Scloudplus-384-AES-packed10__keygen.json` |
| `Scloudplus-384-SHAKE-packed10` | KAT log (sha256 `7d5251c696676bd8…`) | `kat/kem-35/Scloudplus-384-SHAKE-packed10.log` |
| `Scloudplus-384-SHAKE-packed10` | timing dec | `records/kem-35/Scloudplus-384-SHAKE-packed10__dec.json` |
| `Scloudplus-384-SHAKE-packed10` | timing enc | `records/kem-35/Scloudplus-384-SHAKE-packed10__enc.json` |
| `Scloudplus-384-SHAKE-packed10` | timing keygen | `records/kem-35/Scloudplus-384-SHAKE-packed10__keygen.json` |
| `Scloudplus-384-SHAKE-packed10` | hash profile dec | `profile/kem-35/Scloudplus-384-SHAKE-packed10__dec.json` |
| `Scloudplus-384-SHAKE-packed10` | hash profile enc | `profile/kem-35/Scloudplus-384-SHAKE-packed10__enc.json` |
| `Scloudplus-384-SHAKE-packed10` | hash profile keygen | `profile/kem-35/Scloudplus-384-SHAKE-packed10__keygen.json` |
| `Scloudplus-384-SM3-packed10` | KAT log (sha256 `d944b6f3f6f6f404…`) | `kat/kem-35/Scloudplus-384-SM3-packed10.log` |
| `Scloudplus-384-SM3-packed10` | timing dec | `records/kem-35/Scloudplus-384-SM3-packed10__dec.json` |
| `Scloudplus-384-SM3-packed10` | timing enc | `records/kem-35/Scloudplus-384-SM3-packed10__enc.json` |
| `Scloudplus-384-SM3-packed10` | timing keygen | `records/kem-35/Scloudplus-384-SM3-packed10__keygen.json` |
| `Scloudplus-384-SM3-packed10` | hash profile dec | `profile/kem-35/Scloudplus-384-SM3-packed10__dec.json` |
| `Scloudplus-384-SM3-packed10` | hash profile enc | `profile/kem-35/Scloudplus-384-SM3-packed10__enc.json` |
| `Scloudplus-384-SM3-packed10` | hash profile keygen | `profile/kem-35/Scloudplus-384-SM3-packed10__keygen.json` |
| `Scloudplus-512-AES-packed10` | KAT log (sha256 `0f0aeee32d69d6ec…`) | `kat/kem-35/Scloudplus-512-AES-packed10.log` |
| `Scloudplus-512-AES-packed10` | timing dec | `records/kem-35/Scloudplus-512-AES-packed10__dec.json` |
| `Scloudplus-512-AES-packed10` | timing enc | `records/kem-35/Scloudplus-512-AES-packed10__enc.json` |
| `Scloudplus-512-AES-packed10` | timing keygen | `records/kem-35/Scloudplus-512-AES-packed10__keygen.json` |
| `Scloudplus-512-AES-packed10` | hash profile dec | `profile/kem-35/Scloudplus-512-AES-packed10__dec.json` |
| `Scloudplus-512-AES-packed10` | hash profile enc | `profile/kem-35/Scloudplus-512-AES-packed10__enc.json` |
| `Scloudplus-512-AES-packed10` | hash profile keygen | `profile/kem-35/Scloudplus-512-AES-packed10__keygen.json` |
| `Scloudplus-512-SHAKE-packed10` | KAT log (sha256 `8864cfbb83401c4b…`) | `kat/kem-35/Scloudplus-512-SHAKE-packed10.log` |
| `Scloudplus-512-SHAKE-packed10` | timing dec | `records/kem-35/Scloudplus-512-SHAKE-packed10__dec.json` |
| `Scloudplus-512-SHAKE-packed10` | timing enc | `records/kem-35/Scloudplus-512-SHAKE-packed10__enc.json` |
| `Scloudplus-512-SHAKE-packed10` | timing keygen | `records/kem-35/Scloudplus-512-SHAKE-packed10__keygen.json` |
| `Scloudplus-512-SHAKE-packed10` | hash profile dec | `profile/kem-35/Scloudplus-512-SHAKE-packed10__dec.json` |
| `Scloudplus-512-SHAKE-packed10` | hash profile enc | `profile/kem-35/Scloudplus-512-SHAKE-packed10__enc.json` |
| `Scloudplus-512-SHAKE-packed10` | hash profile keygen | `profile/kem-35/Scloudplus-512-SHAKE-packed10__keygen.json` |
| `Scloudplus-512-SM3-packed10` | KAT log (sha256 `e3a7622501060e31…`) | `kat/kem-35/Scloudplus-512-SM3-packed10.log` |
| `Scloudplus-512-SM3-packed10` | timing dec | `records/kem-35/Scloudplus-512-SM3-packed10__dec.json` |
| `Scloudplus-512-SM3-packed10` | timing enc | `records/kem-35/Scloudplus-512-SM3-packed10__enc.json` |
| `Scloudplus-512-SM3-packed10` | timing keygen | `records/kem-35/Scloudplus-512-SM3-packed10__keygen.json` |
| `Scloudplus-512-SM3-packed10` | hash profile dec | `profile/kem-35/Scloudplus-512-SM3-packed10__dec.json` |
| `Scloudplus-512-SM3-packed10` | hash profile enc | `profile/kem-35/Scloudplus-512-SM3-packed10__enc.json` |
| `Scloudplus-512-SM3-packed10` | hash profile keygen | `profile/kem-35/Scloudplus-512-SM3-packed10__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

