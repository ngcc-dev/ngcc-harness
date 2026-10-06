# kex-07 NEV-AKE — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kex-07` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560626045865984.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key exchange
- Algorithm: NEV-AKE
- Implementation versions measured: reference
- Parameter sets: `NEV_AKE_512_769`, `NEV_AKE_512_769_C`, `NEV_AKE_512_1409`, `NEV_AKE_1024_769`, `NEV_AKE_1024_769_C`, `NEV_AKE_1024_1409`, `NEV_AKE_2048_769`, `NEV_AKE_2048_769_C`, `NEV_AKE_2048_1409`

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
| `NEV_AKE_512_769` | exchange | 889.2 k | 425 µs | 2.35e+03 | 425 µs | 11530 (5 × 2306) |
| `NEV_AKE_512_769` | init_a | 88.6 k | 42.3 µs | 2.37e+04 | 42.3 µs | 11530 (5 × 2306) |
| `NEV_AKE_512_769` | init_b | 88.3 k | 42.8 µs | 2.33e+04 | 42.2 µs | 11530 (5 × 2306) |
| `NEV_AKE_512_769` | pass1 | 150.9 k | 73.3 µs | 1.36e+04 | 72 µs | 11530 (5 × 2306) |
| `NEV_AKE_512_769` | pass2 | 340.1 k | 162 µs | 6.16e+03 | 162 µs | 11530 (5 × 2306) |
| `NEV_AKE_512_769` | derive_a | 237.4 k | 113 µs | 8.83e+03 | 113 µs | 11530 (5 × 2306) |
| `NEV_AKE_512_769` | derive_b | 318 | 51.3 ns | 1.95e+07 | 49.5 ns | 11530 (5 × 2306) |
| `NEV_AKE_512_769_C` | exchange | 847.7 k | 405 µs | 2.47e+03 | 405 µs | 11680 (5 × 2336) |
| `NEV_AKE_512_769_C` | init_a | 88.3 k | 42.2 µs | 2.37e+04 | 42.1 µs | 11680 (5 × 2336) |
| `NEV_AKE_512_769_C` | init_b | 88.2 k | 42 µs | 2.38e+04 | 42.1 µs | 11680 (5 × 2336) |
| `NEV_AKE_512_769_C` | pass1 | 140.8 k | 67.2 µs | 1.49e+04 | 67.2 µs | 11680 (5 × 2336) |
| `NEV_AKE_512_769_C` | pass2 | 319.3 k | 159 µs | 6.31e+03 | 152 µs | 11680 (5 × 2336) |
| `NEV_AKE_512_769_C` | derive_a | 225.5 k | 108 µs | 9.29e+03 | 108 µs | 11680 (5 × 2336) |
| `NEV_AKE_512_769_C` | derive_b | 303 | 50.7 ns | 1.97e+07 | 49.2 ns | 11680 (5 × 2336) |
| `NEV_AKE_512_1409` | exchange | 1.08 M | 517 µs | 1.93e+03 | 517 µs | 9805 (5 × 1961) |
| `NEV_AKE_512_1409` | init_a | 118.4 k | 56.5 µs | 1.77e+04 | 56.5 µs | 9805 (5 × 1961) |
| `NEV_AKE_512_1409` | init_b | 119.0 k | 56.8 µs | 1.76e+04 | 56.7 µs | 9805 (5 × 1961) |
| `NEV_AKE_512_1409` | pass1 | 196.7 k | 93.9 µs | 1.07e+04 | 93.8 µs | 9805 (5 × 1961) |
| `NEV_AKE_512_1409` | pass2 | 396.6 k | 189 µs | 5.28e+03 | 189 µs | 9805 (5 × 1961) |
| `NEV_AKE_512_1409` | derive_a | 257.1 k | 123 µs | 8.15e+03 | 123 µs | 9805 (5 × 1961) |
| `NEV_AKE_512_1409` | derive_b | 302 | 49.2 ns | 2.03e+07 | 49.6 ns | 9805 (5 × 1961) |
| `NEV_AKE_1024_769` | exchange | 1.83 M | 876 µs | 1.14e+03 | 876 µs | 5425 (5 × 1085) |
| `NEV_AKE_1024_769` | init_a | 183.9 k | 87.7 µs | 1.14e+04 | 87.8 µs | 5425 (5 × 1085) |
| `NEV_AKE_1024_769` | init_b | 186.3 k | 91.2 µs | 1.1e+04 | 89 µs | 5425 (5 × 1085) |
| `NEV_AKE_1024_769` | pass1 | 309.1 k | 148 µs | 6.78e+03 | 148 µs | 5425 (5 × 1085) |
| `NEV_AKE_1024_769` | pass2 | 674.0 k | 322 µs | 3.11e+03 | 322 µs | 5425 (5 × 1085) |
| `NEV_AKE_1024_769` | derive_a | 477.7 k | 228 µs | 4.38e+03 | 228 µs | 5425 (5 × 1085) |
| `NEV_AKE_1024_769` | derive_b | 346 | 56 ns | 1.78e+07 | 55.1 ns | 5425 (5 × 1085) |
| `NEV_AKE_1024_769_C` | exchange | 1.76 M | 841 µs | 1.19e+03 | 841 µs | 5910 (5 × 1182) |
| `NEV_AKE_1024_769_C` | init_a | 183.0 k | 87.3 µs | 1.15e+04 | 87.2 µs | 5910 (5 × 1182) |
| `NEV_AKE_1024_769_C` | init_b | 185.8 k | 92 µs | 1.09e+04 | 88.8 µs | 5910 (5 × 1182) |
| `NEV_AKE_1024_769_C` | pass1 | 293.2 k | 140 µs | 7.15e+03 | 140 µs | 5910 (5 × 1182) |
| `NEV_AKE_1024_769_C` | pass2 | 640.4 k | 306 µs | 3.27e+03 | 306 µs | 5910 (5 × 1182) |
| `NEV_AKE_1024_769_C` | derive_a | 460.2 k | 220 µs | 4.55e+03 | 220 µs | 5910 (5 × 1182) |
| `NEV_AKE_1024_769_C` | derive_b | 340 | 55.5 ns | 1.8e+07 | 55.1 ns | 5910 (5 × 1182) |
| `NEV_AKE_1024_1409` | exchange | 2.03 M | 975 µs | 1.03e+03 | 972 µs | 5225 (5 × 1045) |
| `NEV_AKE_1024_1409` | init_a | 224.1 k | 109 µs | 9.18e+03 | 107 µs | 5225 (5 × 1045) |
| `NEV_AKE_1024_1409` | init_b | 225.9 k | 108 µs | 9.28e+03 | 108 µs | 5225 (5 × 1045) |
| `NEV_AKE_1024_1409` | pass1 | 357.1 k | 170 µs | 5.87e+03 | 171 µs | 5225 (5 × 1045) |
| `NEV_AKE_1024_1409` | pass2 | 715.5 k | 342 µs | 2.93e+03 | 342 µs | 5225 (5 × 1045) |
| `NEV_AKE_1024_1409` | derive_a | 502.0 k | 240 µs | 4.17e+03 | 240 µs | 5225 (5 × 1045) |
| `NEV_AKE_1024_1409` | derive_b | 331 | 56.3 ns | 1.78e+07 | 54.6 ns | 5225 (5 × 1045) |
| `NEV_AKE_2048_769` | exchange | 5.18 M | 2.47 ms | 404 | 2.47 ms | 2040 (5 × 408) |
| `NEV_AKE_2048_769` | init_a | 504.8 k | 241 µs | 4.15e+03 | 241 µs | 2040 (5 × 408) |
| `NEV_AKE_2048_769` | init_b | 508.3 k | 243 µs | 4.12e+03 | 243 µs | 2040 (5 × 408) |
| `NEV_AKE_2048_769` | pass1 | 774.7 k | 370 µs | 2.7e+03 | 370 µs | 2040 (5 × 408) |
| `NEV_AKE_2048_769` | pass2 | 1.90 M | 905 µs | 1.1e+03 | 906 µs | 2040 (5 × 408) |
| `NEV_AKE_2048_769` | derive_a | 1.48 M | 727 µs | 1.38e+03 | 706 µs | 2040 (5 × 408) |
| `NEV_AKE_2048_769` | derive_b | 342 | 57.1 ns | 1.75e+07 | 58.4 ns | 2040 (5 × 408) |
| `NEV_AKE_2048_769_C` | exchange | 4.87 M | 2.33 ms | 429 | 2.33 ms | 2155 (5 × 431) |
| `NEV_AKE_2048_769_C` | init_a | 500.1 k | 239 µs | 4.19e+03 | 239 µs | 2155 (5 × 431) |
| `NEV_AKE_2048_769_C` | init_b | 502.4 k | 245 µs | 4.09e+03 | 240 µs | 2155 (5 × 431) |
| `NEV_AKE_2048_769_C` | pass1 | 724.2 k | 346 µs | 2.89e+03 | 346 µs | 2155 (5 × 431) |
| `NEV_AKE_2048_769_C` | pass2 | 1.75 M | 837 µs | 1.19e+03 | 837 µs | 2155 (5 × 431) |
| `NEV_AKE_2048_769_C` | derive_a | 1.38 M | 660 µs | 1.51e+03 | 660 µs | 2155 (5 × 431) |
| `NEV_AKE_2048_769_C` | derive_b | 335 | 55.3 ns | 1.81e+07 | 54.4 ns | 2155 (5 × 431) |
| `NEV_AKE_2048_1409` | exchange | 5.75 M | 2.75 ms | 364 | 2.75 ms | 1865 (5 × 373) |
| `NEV_AKE_2048_1409` | init_a | 547.4 k | 261 µs | 3.83e+03 | 261 µs | 1865 (5 × 373) |
| `NEV_AKE_2048_1409` | init_b | 552.3 k | 264 µs | 3.79e+03 | 263 µs | 1865 (5 × 373) |
| `NEV_AKE_2048_1409` | pass1 | 867.4 k | 422 µs | 2.37e+03 | 414 µs | 1865 (5 × 373) |
| `NEV_AKE_2048_1409` | pass2 | 2.15 M | 1.03 ms | 974 | 1.03 ms | 1865 (5 × 373) |
| `NEV_AKE_2048_1409` | derive_a | 1.62 M | 774 µs | 1.29e+03 | 774 µs | 1865 (5 × 373) |
| `NEV_AKE_2048_1409` | derive_b | 340 | 60.1 ns | 1.66e+07 | 59.3 ns | 1865 (5 × 373) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `NEV_AKE_512_769` | exchange | 45309 | 1756 KiB | 1824 KiB |
| `NEV_AKE_512_769` | init_a | 45309 | 1736 KiB | 1844 KiB |
| `NEV_AKE_512_769` | init_b | 45309 | 1748 KiB | 1816 KiB |
| `NEV_AKE_512_769` | pass1 | 45309 | 1728 KiB | 1840 KiB |
| `NEV_AKE_512_769` | pass2 | 45309 | 1728 KiB | 1836 KiB |
| `NEV_AKE_512_769` | derive_a | 45309 | 1736 KiB | 1804 KiB |
| `NEV_AKE_512_769` | derive_b | 45309 | 1752 KiB | 1836 KiB |
| `NEV_AKE_512_769_C` | exchange | 45309 | 1732 KiB | 1800 KiB |
| `NEV_AKE_512_769_C` | init_a | 45309 | 1712 KiB | 1840 KiB |
| `NEV_AKE_512_769_C` | init_b | 45309 | 1724 KiB | 1820 KiB |
| `NEV_AKE_512_769_C` | pass1 | 45309 | 1740 KiB | 1808 KiB |
| `NEV_AKE_512_769_C` | pass2 | 45309 | 1748 KiB | 1828 KiB |
| `NEV_AKE_512_769_C` | derive_a | 45309 | 1720 KiB | 1788 KiB |
| `NEV_AKE_512_769_C` | derive_b | 45309 | 1744 KiB | 1812 KiB |
| `NEV_AKE_512_1409` | exchange | 50189 | 1752 KiB | 1816 KiB |
| `NEV_AKE_512_1409` | init_a | 50189 | 1768 KiB | 1848 KiB |
| `NEV_AKE_512_1409` | init_b | 50189 | 1752 KiB | 1816 KiB |
| `NEV_AKE_512_1409` | pass1 | 50189 | 1768 KiB | 1836 KiB |
| `NEV_AKE_512_1409` | pass2 | 50189 | 1764 KiB | 1856 KiB |
| `NEV_AKE_512_1409` | derive_a | 50189 | 1752 KiB | 1856 KiB |
| `NEV_AKE_512_1409` | derive_b | 50189 | 1760 KiB | 1844 KiB |
| `NEV_AKE_1024_769` | exchange | 51785 | 1804 KiB | 1888 KiB |
| `NEV_AKE_1024_769` | init_a | 51785 | 1776 KiB | 1884 KiB |
| `NEV_AKE_1024_769` | init_b | 51785 | 1784 KiB | 1884 KiB |
| `NEV_AKE_1024_769` | pass1 | 51785 | 1796 KiB | 1864 KiB |
| `NEV_AKE_1024_769` | pass2 | 51785 | 1772 KiB | 1892 KiB |
| `NEV_AKE_1024_769` | derive_a | 51785 | 1784 KiB | 1880 KiB |
| `NEV_AKE_1024_769` | derive_b | 51785 | 1772 KiB | 1840 KiB |
| `NEV_AKE_1024_769_C` | exchange | 51801 | 1796 KiB | 1864 KiB |
| `NEV_AKE_1024_769_C` | init_a | 51801 | 1764 KiB | 1888 KiB |
| `NEV_AKE_1024_769_C` | init_b | 51801 | 1772 KiB | 1840 KiB |
| `NEV_AKE_1024_769_C` | pass1 | 51801 | 1784 KiB | 1888 KiB |
| `NEV_AKE_1024_769_C` | pass2 | 51801 | 1792 KiB | 1884 KiB |
| `NEV_AKE_1024_769_C` | derive_a | 51801 | 1792 KiB | 1880 KiB |
| `NEV_AKE_1024_769_C` | derive_b | 51801 | 1796 KiB | 1864 KiB |
| `NEV_AKE_1024_1409` | exchange | 57257 | 1812 KiB | 1880 KiB |
| `NEV_AKE_1024_1409` | init_a | 57257 | 1760 KiB | 1892 KiB |
| `NEV_AKE_1024_1409` | init_b | 57257 | 1784 KiB | 1904 KiB |
| `NEV_AKE_1024_1409` | pass1 | 57257 | 1792 KiB | 1860 KiB |
| `NEV_AKE_1024_1409` | pass2 | 57257 | 1808 KiB | 1876 KiB |
| `NEV_AKE_1024_1409` | derive_a | 57257 | 1776 KiB | 1904 KiB |
| `NEV_AKE_1024_1409` | derive_b | 57257 | 1784 KiB | 1904 KiB |
| `NEV_AKE_2048_769` | exchange | 59949 | 1884 KiB | 1960 KiB |
| `NEV_AKE_2048_769` | init_a | 59949 | 1872 KiB | 1968 KiB |
| `NEV_AKE_2048_769` | init_b | 59949 | 1876 KiB | 1944 KiB |
| `NEV_AKE_2048_769` | pass1 | 59949 | 1884 KiB | 1952 KiB |
| `NEV_AKE_2048_769` | pass2 | 59949 | 1884 KiB | 1956 KiB |
| `NEV_AKE_2048_769` | derive_a | 59949 | 1852 KiB | 1976 KiB |
| `NEV_AKE_2048_769` | derive_b | 59949 | 1876 KiB | 1944 KiB |
| `NEV_AKE_2048_769_C` | exchange | 59885 | 1852 KiB | 1968 KiB |
| `NEV_AKE_2048_769_C` | init_a | 59885 | 1864 KiB | 1932 KiB |
| `NEV_AKE_2048_769_C` | init_b | 59885 | 1872 KiB | 1952 KiB |
| `NEV_AKE_2048_769_C` | pass1 | 59885 | 1872 KiB | 1964 KiB |
| `NEV_AKE_2048_769_C` | pass2 | 59885 | 1876 KiB | 1960 KiB |
| `NEV_AKE_2048_769_C` | derive_a | 59885 | 1876 KiB | 1960 KiB |
| `NEV_AKE_2048_769_C` | derive_b | 59885 | 1876 KiB | 1944 KiB |
| `NEV_AKE_2048_1409` | exchange | 65109 | 1904 KiB | 1972 KiB |
| `NEV_AKE_2048_1409` | init_a | 65109 | 1896 KiB | 1964 KiB |
| `NEV_AKE_2048_1409` | init_b | 65109 | 1896 KiB | 1964 KiB |
| `NEV_AKE_2048_1409` | pass1 | 65109 | 1904 KiB | 2000 KiB |
| `NEV_AKE_2048_1409` | pass2 | 65109 | 1904 KiB | 1988 KiB |
| `NEV_AKE_2048_1409` | derive_a | 65109 | 1896 KiB | 1964 KiB |
| `NEV_AKE_2048_1409` | derive_b | 65109 | 1884 KiB | 1996 KiB |

## 6. Transmission and storage overhead

Bandwidth counts all specified protocol messages and each required public key once. Public keys are transmitted bytes too. Certificates and transport framing are excluded. The published raw timing records are unchanged.

| instance | passes | messages (bytes; raw API) | protocol-message bytes | public key A / B | bandwidth (bytes) | long-term sk (API cap) | shared secret |
|---|---|---|---|---|---|---|---|
| `NEV_AKE_512_769` | 2 | 1230 / 1230 | 2460 | 615 / 615 | 3690 | 1246 | 16 |
| `NEV_AKE_512_769_C` | 2 | 1127 / 1127 | 2254 | 615 / 615 | 3484 | 1246 | 16 |
| `NEV_AKE_512_1409` | 2 | 1344 / 1344 | 2688 | 672 / 672 | 4032 | 1360 | 16 |
| `NEV_AKE_1024_769` | 2 | 2458 / 2458 | 4916 | 1229 / 1229 | 7374 | 2490 | 32 |
| `NEV_AKE_1024_769_C` | 2 | 2253 / 2253 | 4506 | 1229 / 1229 | 6964 | 2490 | 32 |
| `NEV_AKE_1024_1409` | 2 | 2688 / 2688 | 5376 | 1344 / 1344 | 8064 | 2720 | 32 |
| `NEV_AKE_2048_769` | 2 | 4916 / 4916 | 9832 | 2458 / 2458 | 14748 | 4980 | 64 |
| `NEV_AKE_2048_769_C` | 2 | 4506 / 4506 | 9012 | 2458 / 2458 | 13928 | 4980 | 64 |
| `NEV_AKE_2048_1409` | 2 | 5376 / 5376 | 10752 | 2688 / 2688 | 16128 | 5440 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — own fips202.c compiled but unreachable

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `NEV_AKE_512_769` | exchange | 45% | 4.1% | drng 8, pseudoXOF 22, sm3hash 25 |
| `NEV_AKE_512_769_C` | exchange | 42% | 4.3% | drng 8, pseudoXOF 22, sm3hash 5 |
| `NEV_AKE_512_1409` | exchange | 52% | 3.5% | drng 8, pseudoXOF 27 |
| `NEV_AKE_1024_769` | exchange | 42% | 2.1% | drng 8, pseudoXOF 19, sm3hash 43 |
| `NEV_AKE_1024_769_C` | exchange | 38% | 2.1% | drng 8, pseudoXOF 19, sm3hash 11 |
| `NEV_AKE_1024_1409` | exchange | 45% | 1.9% | drng 8, pseudoXOF 24, sm3hash 3 |
| `NEV_AKE_2048_769` | exchange | 55% | 1.0% | drng 8, pseudoXOF 19, pseudohash 3, sm3hash 70 |
| `NEV_AKE_2048_769_C` | exchange | 51% | 1.0% | drng 8, pseudoXOF 19, pseudohash 3, sm3hash 14 |
| `NEV_AKE_2048_1409` | exchange | 53% | 0.9% | drng 8, pseudoXOF 13, pseudohash 3, sm3hash 154 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `NEV_AKE_512_769` | KAT log (sha256 `c36c43d23a3f83c8…`) | `kat/kex-07/NEV_AKE_512_769.log` |
| `NEV_AKE_512_769` | timing derive_a | `records/kex-07/NEV_AKE_512_769__derive_a.json` |
| `NEV_AKE_512_769` | timing derive_b | `records/kex-07/NEV_AKE_512_769__derive_b.json` |
| `NEV_AKE_512_769` | timing exchange | `records/kex-07/NEV_AKE_512_769__exchange.json` |
| `NEV_AKE_512_769` | timing init_a | `records/kex-07/NEV_AKE_512_769__init_a.json` |
| `NEV_AKE_512_769` | timing init_b | `records/kex-07/NEV_AKE_512_769__init_b.json` |
| `NEV_AKE_512_769` | timing pass1 | `records/kex-07/NEV_AKE_512_769__pass1.json` |
| `NEV_AKE_512_769` | timing pass2 | `records/kex-07/NEV_AKE_512_769__pass2.json` |
| `NEV_AKE_512_769` | hash profile exchange | `profile/kex-07/NEV_AKE_512_769__exchange.json` |
| `NEV_AKE_512_769_C` | KAT log (sha256 `1a314fcd81a22801…`) | `kat/kex-07/NEV_AKE_512_769_C.log` |
| `NEV_AKE_512_769_C` | timing derive_a | `records/kex-07/NEV_AKE_512_769_C__derive_a.json` |
| `NEV_AKE_512_769_C` | timing derive_b | `records/kex-07/NEV_AKE_512_769_C__derive_b.json` |
| `NEV_AKE_512_769_C` | timing exchange | `records/kex-07/NEV_AKE_512_769_C__exchange.json` |
| `NEV_AKE_512_769_C` | timing init_a | `records/kex-07/NEV_AKE_512_769_C__init_a.json` |
| `NEV_AKE_512_769_C` | timing init_b | `records/kex-07/NEV_AKE_512_769_C__init_b.json` |
| `NEV_AKE_512_769_C` | timing pass1 | `records/kex-07/NEV_AKE_512_769_C__pass1.json` |
| `NEV_AKE_512_769_C` | timing pass2 | `records/kex-07/NEV_AKE_512_769_C__pass2.json` |
| `NEV_AKE_512_769_C` | hash profile exchange | `profile/kex-07/NEV_AKE_512_769_C__exchange.json` |
| `NEV_AKE_512_1409` | KAT log (sha256 `10cb963dd0644494…`) | `kat/kex-07/NEV_AKE_512_1409.log` |
| `NEV_AKE_512_1409` | timing derive_a | `records/kex-07/NEV_AKE_512_1409__derive_a.json` |
| `NEV_AKE_512_1409` | timing derive_b | `records/kex-07/NEV_AKE_512_1409__derive_b.json` |
| `NEV_AKE_512_1409` | timing exchange | `records/kex-07/NEV_AKE_512_1409__exchange.json` |
| `NEV_AKE_512_1409` | timing init_a | `records/kex-07/NEV_AKE_512_1409__init_a.json` |
| `NEV_AKE_512_1409` | timing init_b | `records/kex-07/NEV_AKE_512_1409__init_b.json` |
| `NEV_AKE_512_1409` | timing pass1 | `records/kex-07/NEV_AKE_512_1409__pass1.json` |
| `NEV_AKE_512_1409` | timing pass2 | `records/kex-07/NEV_AKE_512_1409__pass2.json` |
| `NEV_AKE_512_1409` | hash profile exchange | `profile/kex-07/NEV_AKE_512_1409__exchange.json` |
| `NEV_AKE_1024_769` | KAT log (sha256 `dbc86f11f0e9424b…`) | `kat/kex-07/NEV_AKE_1024_769.log` |
| `NEV_AKE_1024_769` | timing derive_a | `records/kex-07/NEV_AKE_1024_769__derive_a.json` |
| `NEV_AKE_1024_769` | timing derive_b | `records/kex-07/NEV_AKE_1024_769__derive_b.json` |
| `NEV_AKE_1024_769` | timing exchange | `records/kex-07/NEV_AKE_1024_769__exchange.json` |
| `NEV_AKE_1024_769` | timing init_a | `records/kex-07/NEV_AKE_1024_769__init_a.json` |
| `NEV_AKE_1024_769` | timing init_b | `records/kex-07/NEV_AKE_1024_769__init_b.json` |
| `NEV_AKE_1024_769` | timing pass1 | `records/kex-07/NEV_AKE_1024_769__pass1.json` |
| `NEV_AKE_1024_769` | timing pass2 | `records/kex-07/NEV_AKE_1024_769__pass2.json` |
| `NEV_AKE_1024_769` | hash profile exchange | `profile/kex-07/NEV_AKE_1024_769__exchange.json` |
| `NEV_AKE_1024_769_C` | KAT log (sha256 `852e968bd22d0fc1…`) | `kat/kex-07/NEV_AKE_1024_769_C.log` |
| `NEV_AKE_1024_769_C` | timing derive_a | `records/kex-07/NEV_AKE_1024_769_C__derive_a.json` |
| `NEV_AKE_1024_769_C` | timing derive_b | `records/kex-07/NEV_AKE_1024_769_C__derive_b.json` |
| `NEV_AKE_1024_769_C` | timing exchange | `records/kex-07/NEV_AKE_1024_769_C__exchange.json` |
| `NEV_AKE_1024_769_C` | timing init_a | `records/kex-07/NEV_AKE_1024_769_C__init_a.json` |
| `NEV_AKE_1024_769_C` | timing init_b | `records/kex-07/NEV_AKE_1024_769_C__init_b.json` |
| `NEV_AKE_1024_769_C` | timing pass1 | `records/kex-07/NEV_AKE_1024_769_C__pass1.json` |
| `NEV_AKE_1024_769_C` | timing pass2 | `records/kex-07/NEV_AKE_1024_769_C__pass2.json` |
| `NEV_AKE_1024_769_C` | hash profile exchange | `profile/kex-07/NEV_AKE_1024_769_C__exchange.json` |
| `NEV_AKE_1024_1409` | KAT log (sha256 `f4c5077b88d2ea5f…`) | `kat/kex-07/NEV_AKE_1024_1409.log` |
| `NEV_AKE_1024_1409` | timing derive_a | `records/kex-07/NEV_AKE_1024_1409__derive_a.json` |
| `NEV_AKE_1024_1409` | timing derive_b | `records/kex-07/NEV_AKE_1024_1409__derive_b.json` |
| `NEV_AKE_1024_1409` | timing exchange | `records/kex-07/NEV_AKE_1024_1409__exchange.json` |
| `NEV_AKE_1024_1409` | timing init_a | `records/kex-07/NEV_AKE_1024_1409__init_a.json` |
| `NEV_AKE_1024_1409` | timing init_b | `records/kex-07/NEV_AKE_1024_1409__init_b.json` |
| `NEV_AKE_1024_1409` | timing pass1 | `records/kex-07/NEV_AKE_1024_1409__pass1.json` |
| `NEV_AKE_1024_1409` | timing pass2 | `records/kex-07/NEV_AKE_1024_1409__pass2.json` |
| `NEV_AKE_1024_1409` | hash profile exchange | `profile/kex-07/NEV_AKE_1024_1409__exchange.json` |
| `NEV_AKE_2048_769` | KAT log (sha256 `df88a77628968ca3…`) | `kat/kex-07/NEV_AKE_2048_769.log` |
| `NEV_AKE_2048_769` | timing derive_a | `records/kex-07/NEV_AKE_2048_769__derive_a.json` |
| `NEV_AKE_2048_769` | timing derive_b | `records/kex-07/NEV_AKE_2048_769__derive_b.json` |
| `NEV_AKE_2048_769` | timing exchange | `records/kex-07/NEV_AKE_2048_769__exchange.json` |
| `NEV_AKE_2048_769` | timing init_a | `records/kex-07/NEV_AKE_2048_769__init_a.json` |
| `NEV_AKE_2048_769` | timing init_b | `records/kex-07/NEV_AKE_2048_769__init_b.json` |
| `NEV_AKE_2048_769` | timing pass1 | `records/kex-07/NEV_AKE_2048_769__pass1.json` |
| `NEV_AKE_2048_769` | timing pass2 | `records/kex-07/NEV_AKE_2048_769__pass2.json` |
| `NEV_AKE_2048_769` | hash profile exchange | `profile/kex-07/NEV_AKE_2048_769__exchange.json` |
| `NEV_AKE_2048_769_C` | KAT log (sha256 `50e9401e025381f7…`) | `kat/kex-07/NEV_AKE_2048_769_C.log` |
| `NEV_AKE_2048_769_C` | timing derive_a | `records/kex-07/NEV_AKE_2048_769_C__derive_a.json` |
| `NEV_AKE_2048_769_C` | timing derive_b | `records/kex-07/NEV_AKE_2048_769_C__derive_b.json` |
| `NEV_AKE_2048_769_C` | timing exchange | `records/kex-07/NEV_AKE_2048_769_C__exchange.json` |
| `NEV_AKE_2048_769_C` | timing init_a | `records/kex-07/NEV_AKE_2048_769_C__init_a.json` |
| `NEV_AKE_2048_769_C` | timing init_b | `records/kex-07/NEV_AKE_2048_769_C__init_b.json` |
| `NEV_AKE_2048_769_C` | timing pass1 | `records/kex-07/NEV_AKE_2048_769_C__pass1.json` |
| `NEV_AKE_2048_769_C` | timing pass2 | `records/kex-07/NEV_AKE_2048_769_C__pass2.json` |
| `NEV_AKE_2048_769_C` | hash profile exchange | `profile/kex-07/NEV_AKE_2048_769_C__exchange.json` |
| `NEV_AKE_2048_1409` | KAT log (sha256 `30531dc432374794…`) | `kat/kex-07/NEV_AKE_2048_1409.log` |
| `NEV_AKE_2048_1409` | timing derive_a | `records/kex-07/NEV_AKE_2048_1409__derive_a.json` |
| `NEV_AKE_2048_1409` | timing derive_b | `records/kex-07/NEV_AKE_2048_1409__derive_b.json` |
| `NEV_AKE_2048_1409` | timing exchange | `records/kex-07/NEV_AKE_2048_1409__exchange.json` |
| `NEV_AKE_2048_1409` | timing init_a | `records/kex-07/NEV_AKE_2048_1409__init_a.json` |
| `NEV_AKE_2048_1409` | timing init_b | `records/kex-07/NEV_AKE_2048_1409__init_b.json` |
| `NEV_AKE_2048_1409` | timing pass1 | `records/kex-07/NEV_AKE_2048_1409__pass1.json` |
| `NEV_AKE_2048_1409` | timing pass2 | `records/kex-07/NEV_AKE_2048_1409__pass2.json` |
| `NEV_AKE_2048_1409` | hash profile exchange | `profile/kex-07/NEV_AKE_2048_1409__exchange.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

