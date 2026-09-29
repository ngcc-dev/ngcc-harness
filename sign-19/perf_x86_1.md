# sign-19 Phoenix — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-19` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561087113121792.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: Phoenix
- Implementation versions measured: reference
- Parameter sets: `Phoenix-SHAKE-128f`, `Phoenix-SHAKE-128s`, `Phoenix-SHAKE-192f`, `Phoenix-SHAKE-192s`, `Phoenix-SHAKE-256f`, `Phoenix-SHAKE-256s`, `Phoenix-SHAKE-384f`, `Phoenix-SHAKE-384s`, `Phoenix-SHAKE-512f`, `Phoenix-SHAKE-512s`, `Phoenix-SM3-128f`, `Phoenix-SM3-128s`, `Phoenix-SM3-192f`, `Phoenix-SM3-192s`, `Phoenix-SM3-256f`, `Phoenix-SM3-256s`, `Phoenix-SM3-384f`, `Phoenix-SM3-384s`, `Phoenix-SM3-512f`, `Phoenix-SM3-512s`

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
| `Phoenix-SHAKE-128f` | keygen | 9.77 M | 4.7 ms | 213 | 4.7 ms | 995 (5 × 199) |
| `Phoenix-SHAKE-128f` | sign | 186.43 M | 89.6 ms | 11.2 | 89.7 ms | 100 (5 × 20) |
| `Phoenix-SHAKE-128f` | verify | 4.99 M | 2.38 ms | 419 | 2.38 ms | 2050 (5 × 410) |
| `Phoenix-SHAKE-128s` | keygen | 307.13 M | 147 ms | 6.81 | 147 ms | 100 (5 × 20) |
| `Phoenix-SHAKE-128s` | sign | 3.27 G | 1.57 s | 0.637 | 1.57 s | 100 (5 × 20) |
| `Phoenix-SHAKE-128s` | verify | 12.11 M | 5.78 ms | 173 | 5.79 ms | 855 (5 × 171) |
| `Phoenix-SHAKE-192f` | keygen | 14.64 M | 7.03 ms | 142 | 7.02 ms | 735 (5 × 147) |
| `Phoenix-SHAKE-192f` | sign | 290.69 M | 139 ms | 7.17 | 139 ms | 100 (5 × 20) |
| `Phoenix-SHAKE-192f` | verify | 7.53 M | 3.6 ms | 278 | 3.6 ms | 1375 (5 × 275) |
| `Phoenix-SHAKE-192s` | keygen | 538.45 M | 257 ms | 3.89 | 257 ms | 100 (5 × 20) |
| `Phoenix-SHAKE-192s` | sign | 5.59 G | 2.68 s | 0.373 | 2.68 s | 100 (5 × 20) |
| `Phoenix-SHAKE-192s` | verify | 19.20 M | 9.17 ms | 109 | 9.18 ms | 535 (5 × 107) |
| `Phoenix-SHAKE-256f` | keygen | 29.75 M | 14.2 ms | 70.2 | 14.2 ms | 345 (5 × 69) |
| `Phoenix-SHAKE-256f` | sign | 576.55 M | 276 ms | 3.62 | 276 ms | 100 (5 × 20) |
| `Phoenix-SHAKE-256f` | verify | 14.99 M | 7.16 ms | 140 | 7.15 ms | 695 (5 × 139) |
| `Phoenix-SHAKE-256s` | keygen | 387.92 M | 185 ms | 5.4 | 185 ms | 100 (5 × 20) |
| `Phoenix-SHAKE-256s` | sign | 5.15 G | 2.47 s | 0.405 | 2.47 s | 100 (5 × 20) |
| `Phoenix-SHAKE-256s` | verify | 33.58 M | 16 ms | 62.3 | 16 ms | 310 (5 × 62) |
| `Phoenix-SHAKE-384f` | keygen | 65.91 M | 31.5 ms | 31.7 | 31.5 ms | 160 (5 × 32) |
| `Phoenix-SHAKE-384f` | sign | 2.05 G | 983 ms | 1.02 | 985 ms | 100 (5 × 20) |
| `Phoenix-SHAKE-384f` | verify | 36.17 M | 17.3 ms | 57.9 | 17.3 ms | 295 (5 × 59) |
| `Phoenix-SHAKE-384s` | keygen | 651.57 M | 313 ms | 3.2 | 311 ms | 100 (5 × 20) |
| `Phoenix-SHAKE-384s` | sign | 9.03 G | 4.33 s | 0.231 | 4.33 s | 100 (5 × 20) |
| `Phoenix-SHAKE-384s` | verify | 57.15 M | 27.6 ms | 36.2 | 27.6 ms | 185 (5 × 37) |
| `Phoenix-SHAKE-512f` | keygen | 231.73 M | 111 ms | 9.03 | 111 ms | 100 (5 × 20) |
| `Phoenix-SHAKE-512f` | sign | 5.34 G | 2.56 s | 0.391 | 2.56 s | 100 (5 × 20) |
| `Phoenix-SHAKE-512f` | verify | 123.82 M | 59.7 ms | 16.7 | 59.7 ms | 100 (5 × 20) |
| `Phoenix-SHAKE-512s` | keygen | 2.41 G | 1.16 s | 0.864 | 1.16 s | 100 (5 × 20) |
| `Phoenix-SHAKE-512s` | sign | 22.26 G | 10.7 s | 0.0936 | 10.7 s | 80 (5 × 16) |
| `Phoenix-SHAKE-512s` | verify | 39.06 M | 18.8 ms | 53.2 | 18.8 ms | 265 (5 × 53) |
| `Phoenix-SM3-128f` | keygen | 10.80 M | 5.16 ms | 194 | 5.16 ms | 970 (5 × 194) |
| `Phoenix-SM3-128f` | sign | 219.91 M | 105 ms | 9.52 | 105 ms | 100 (5 × 20) |
| `Phoenix-SM3-128f` | verify | 6.08 M | 2.9 ms | 345 | 2.9 ms | 1715 (5 × 343) |
| `Phoenix-SM3-128s` | keygen | 343.22 M | 164 ms | 6.1 | 164 ms | 100 (5 × 20) |
| `Phoenix-SM3-128s` | sign | 3.90 G | 1.87 s | 0.534 | 1.87 s | 100 (5 × 20) |
| `Phoenix-SM3-128s` | verify | 13.88 M | 6.63 ms | 151 | 6.63 ms | 755 (5 × 151) |
| `Phoenix-SM3-192f` | keygen | 29.84 M | 14.3 ms | 70.2 | 14.3 ms | 350 (5 × 70) |
| `Phoenix-SM3-192f` | sign | 587.62 M | 282 ms | 3.55 | 281 ms | 100 (5 × 20) |
| `Phoenix-SM3-192f` | verify | 15.70 M | 7.5 ms | 133 | 7.5 ms | 670 (5 × 134) |
| `Phoenix-SM3-192s` | keygen | 1.11 G | 534 ms | 1.87 | 533 ms | 100 (5 × 20) |
| `Phoenix-SM3-192s` | sign | 11.52 G | 5.53 s | 0.181 | 5.53 s | 100 (5 × 20) |
| `Phoenix-SM3-192s` | verify | 39.76 M | 19 ms | 52.7 | 19 ms | 265 (5 × 53) |
| `Phoenix-SM3-256f` | keygen | 59.14 M | 28.2 ms | 35.4 | 28.3 ms | 180 (5 × 36) |
| `Phoenix-SM3-256f` | sign | 1.12 G | 538 ms | 1.86 | 538 ms | 100 (5 × 20) |
| `Phoenix-SM3-256f` | verify | 30.04 M | 14.4 ms | 69.7 | 14.3 ms | 350 (5 × 70) |
| `Phoenix-SM3-256s` | keygen | 777.96 M | 373 ms | 2.68 | 372 ms | 100 (5 × 20) |
| `Phoenix-SM3-256s` | sign | 10.54 G | 5.05 s | 0.198 | 5.05 s | 100 (5 × 20) |
| `Phoenix-SM3-256s` | verify | 67.56 M | 32.3 ms | 31 | 32.3 ms | 155 (5 × 31) |
| `Phoenix-SM3-384f` | keygen | 124.61 M | 59.5 ms | 16.8 | 59.6 ms | 100 (5 × 20) |
| `Phoenix-SM3-384f` | sign | 3.55 G | 1.7 s | 0.587 | 1.7 s | 100 (5 × 20) |
| `Phoenix-SM3-384f` | verify | 67.63 M | 32.3 ms | 31 | 32.3 ms | 155 (5 × 31) |
| `Phoenix-SM3-384s` | keygen | 1.22 G | 583 ms | 1.72 | 583 ms | 100 (5 × 20) |
| `Phoenix-SM3-384s` | sign | 16.64 G | 7.98 s | 0.125 | 7.98 s | 100 (5 × 20) |
| `Phoenix-SM3-384s` | verify | 106.66 M | 50.9 ms | 19.6 | 51 ms | 100 (5 × 20) |
| `Phoenix-SM3-512f` | keygen | 248.00 M | 118 ms | 8.44 | 119 ms | 100 (5 × 20) |
| `Phoenix-SM3-512f` | sign | 6.03 G | 2.89 s | 0.346 | 2.89 s | 100 (5 × 20) |
| `Phoenix-SM3-512f` | verify | 133.86 M | 63.9 ms | 15.6 | 63.9 ms | 100 (5 × 20) |
| `Phoenix-SM3-512s` | keygen | 2.58 G | 1.24 s | 0.808 | 1.24 s | 100 (5 × 20) |
| `Phoenix-SM3-512s` | sign | 25.75 G | 12.3 s | 0.081 | 12.3 s | 70 (5 × 14) |
| `Phoenix-SM3-512s` | verify | 43.19 M | 20.6 ms | 48.5 | 20.6 ms | 245 (5 × 49) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Phoenix-SHAKE-128f` | keygen | 36157 | 1720 KiB | 1824 KiB |
| `Phoenix-SHAKE-128f` | sign | 36157 | 1708 KiB | 1820 KiB |
| `Phoenix-SHAKE-128f` | verify | 36157 | 1712 KiB | 1776 KiB |
| `Phoenix-SHAKE-128s` | keygen | 36285 | 1676 KiB | 1804 KiB |
| `Phoenix-SHAKE-128s` | sign | 36285 | 1712 KiB | 1776 KiB |
| `Phoenix-SHAKE-128s` | verify | 36285 | 1716 KiB | 1780 KiB |
| `Phoenix-SHAKE-192f` | keygen | 37101 | 1744 KiB | 1820 KiB |
| `Phoenix-SHAKE-192f` | sign | 37101 | 1744 KiB | 1832 KiB |
| `Phoenix-SHAKE-192f` | verify | 37101 | 1744 KiB | 1812 KiB |
| `Phoenix-SHAKE-192s` | keygen | 37037 | 1724 KiB | 1812 KiB |
| `Phoenix-SHAKE-192s` | sign | 37037 | 1724 KiB | 1792 KiB |
| `Phoenix-SHAKE-192s` | verify | 37037 | 1732 KiB | 1800 KiB |
| `Phoenix-SHAKE-256f` | keygen | 37165 | 1764 KiB | 1832 KiB |
| `Phoenix-SHAKE-256f` | sign | 37165 | 1768 KiB | 1832 KiB |
| `Phoenix-SHAKE-256f` | verify | 37165 | 1740 KiB | 1860 KiB |
| `Phoenix-SHAKE-256s` | keygen | 37101 | 1744 KiB | 1832 KiB |
| `Phoenix-SHAKE-256s` | sign | 37101 | 1728 KiB | 1824 KiB |
| `Phoenix-SHAKE-256s` | verify | 37101 | 1744 KiB | 1812 KiB |
| `Phoenix-SHAKE-384f` | keygen | 37741 | 1788 KiB | 1880 KiB |
| `Phoenix-SHAKE-384f` | sign | 37741 | 1804 KiB | 1868 KiB |
| `Phoenix-SHAKE-384f` | verify | 37741 | 1764 KiB | 1900 KiB |
| `Phoenix-SHAKE-384s` | keygen | 37861 | 1740 KiB | 1868 KiB |
| `Phoenix-SHAKE-384s` | sign | 37861 | 1720 KiB | 1848 KiB |
| `Phoenix-SHAKE-384s` | verify | 37861 | 1772 KiB | 1844 KiB |
| `Phoenix-SHAKE-512f` | keygen | 38437 | 1656 KiB | 1796 KiB |
| `Phoenix-SHAKE-512f` | sign | 38437 | 1708 KiB | 1940 KiB |
| `Phoenix-SHAKE-512f` | verify | 38437 | 1856 KiB | 1932 KiB |
| `Phoenix-SHAKE-512s` | keygen | 38317 | 1812 KiB | 1916 KiB |
| `Phoenix-SHAKE-512s` | sign | 38317 | 1812 KiB | 1880 KiB |
| `Phoenix-SHAKE-512s` | verify | 38317 | 1808 KiB | 1924 KiB |
| `Phoenix-SM3-128f` | keygen | – | 1736 KiB | 1800 KiB |
| `Phoenix-SM3-128f` | sign | – | 1720 KiB | 1804 KiB |
| `Phoenix-SM3-128f` | verify | – | 1728 KiB | 1792 KiB |
| `Phoenix-SM3-128s` | keygen | – | 1728 KiB | 1792 KiB |
| `Phoenix-SM3-128s` | sign | – | 1716 KiB | 1780 KiB |
| `Phoenix-SM3-128s` | verify | – | 1708 KiB | 1816 KiB |
| `Phoenix-SM3-192f` | keygen | – | 1724 KiB | 1840 KiB |
| `Phoenix-SM3-192f` | sign | – | 1740 KiB | 1804 KiB |
| `Phoenix-SM3-192f` | verify | – | 1732 KiB | 1840 KiB |
| `Phoenix-SM3-192s` | keygen | – | 1728 KiB | 1796 KiB |
| `Phoenix-SM3-192s` | sign | – | 1724 KiB | 1792 KiB |
| `Phoenix-SM3-192s` | verify | – | 1716 KiB | 1780 KiB |
| `Phoenix-SM3-256f` | keygen | – | 1732 KiB | 1856 KiB |
| `Phoenix-SM3-256f` | sign | – | 1744 KiB | 1844 KiB |
| `Phoenix-SM3-256f` | verify | – | 1768 KiB | 1852 KiB |
| `Phoenix-SM3-256s` | keygen | – | 1724 KiB | 1824 KiB |
| `Phoenix-SM3-256s` | sign | – | 1732 KiB | 1836 KiB |
| `Phoenix-SM3-256s` | verify | – | 1736 KiB | 1816 KiB |
| `Phoenix-SM3-384f` | keygen | – | 1776 KiB | 1884 KiB |
| `Phoenix-SM3-384f` | sign | – | 1804 KiB | 1904 KiB |
| `Phoenix-SM3-384f` | verify | – | 1812 KiB | 1880 KiB |
| `Phoenix-SM3-384s` | keygen | – | 1756 KiB | 1828 KiB |
| `Phoenix-SM3-384s` | sign | – | 1756 KiB | 1820 KiB |
| `Phoenix-SM3-384s` | verify | – | 1692 KiB | 1824 KiB |
| `Phoenix-SM3-512f` | keygen | – | 1700 KiB | 1800 KiB |
| `Phoenix-SM3-512f` | sign | – | 1736 KiB | 1936 KiB |
| `Phoenix-SM3-512f` | verify | – | 1832 KiB | 1964 KiB |
| `Phoenix-SM3-512s` | keygen | – | 1808 KiB | 1892 KiB |
| `Phoenix-SM3-512s` | sign | – | 1836 KiB | 1924 KiB |
| `Phoenix-SM3-512s` | verify | – | 1840 KiB | 1924 KiB |

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
| `Phoenix-SM3-128f` | sign | 4.1% | 0.0% | drng 1, pseudoXOF 618 |
| `Phoenix-SM3-128f` | verify | 1.3% | 0.0% | pseudoXOF 20 |
| `Phoenix-SM3-128s` | keygen | 0.0% | 0.0% | drng 1 |
| `Phoenix-SM3-128s` | sign | 5.3% | 0.0% | drng 1, pseudoXOF 1.3e+04 |
| `Phoenix-SM3-128s` | verify | 0.5% | 0.0% | pseudoXOF 13 |
| `Phoenix-SM3-192f` | keygen | 0.0% | 0.0% | drng 1 |
| `Phoenix-SM3-192f` | sign | 0.3% | 0.0% | drng 1, pseudoXOF 144 |
| `Phoenix-SM3-192f` | verify | 0.7% | 0.0% | pseudoXOF 20 |
| `Phoenix-SM3-192s` | keygen | 0.0% | 0.0% | drng 1 |
| `Phoenix-SM3-192s` | sign | 0.9% | 0.0% | drng 1, pseudoXOF 7.13e+03 |
| `Phoenix-SM3-192s` | verify | 0.2% | 0.0% | pseudoXOF 12 |
| `Phoenix-SM3-256f` | keygen | 0.0% | 0.0% | drng 1 |
| `Phoenix-SM3-256f` | sign | 2.0% | 0.0% | drng 1, pseudoXOF 851 |
| `Phoenix-SM3-256f` | verify | 0.5% | 0.0% | pseudoXOF 19 |
| `Phoenix-SM3-256s` | keygen | 0.0% | 0.0% | drng 1 |
| `Phoenix-SM3-256s` | sign | 12% | 0.0% | drng 1, pseudoXOF 4.83e+04 |
| `Phoenix-SM3-256s` | verify | 0.2% | 0.0% | pseudoXOF 14 |
| `Phoenix-SM3-384f` | keygen | 0.0% | 0.0% | drng 1 |
| `Phoenix-SM3-384f` | sign | 0.0% | 0.0% | drng 1, pseudoXOF 19 |
| `Phoenix-SM3-384f` | verify | 0.3% | 0.0% | pseudoXOF 20 |
| `Phoenix-SM3-384s` | keygen | 0.0% | 0.0% | drng 1 |
| `Phoenix-SM3-384s` | sign | 2.0% | 0.0% | drng 1, pseudoXOF 1.08e+04 |
| `Phoenix-SM3-384s` | verify | 0.2% | 0.0% | pseudoXOF 14 |
| `Phoenix-SM3-512f` | keygen | 0.0% | 0.0% | drng 1 |
| `Phoenix-SM3-512f` | sign | 4.8% | 0.0% | drng 1, pseudoXOF 6.51e+03 |
| `Phoenix-SM3-512f` | verify | 0.2% | 0.0% | pseudoXOF 20 |
| `Phoenix-SM3-512s` | keygen | 0.0% | 0.0% | drng 1 |
| `Phoenix-SM3-512s` | sign | 8.1% | 0.0% | drng 1, pseudoXOF 4.78e+04 |
| `Phoenix-SM3-512s` | verify | 0.5% | 0.0% | pseudoXOF 11 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Phoenix-SHAKE-128f` | KAT log (sha256 `33712e9ccf79c79d…`) | `kat/sign-19/Phoenix-SHAKE-128f.log` |
| `Phoenix-SHAKE-128f` | timing keygen | `records/sign-19/Phoenix-SHAKE-128f__keygen.json` |
| `Phoenix-SHAKE-128f` | timing sign | `records/sign-19/Phoenix-SHAKE-128f__sign.json` |
| `Phoenix-SHAKE-128f` | timing verify | `records/sign-19/Phoenix-SHAKE-128f__verify.json` |
| `Phoenix-SHAKE-128f` | hash profile keygen | `profile/sign-19/Phoenix-SHAKE-128f__keygen.json` |
| `Phoenix-SHAKE-128f` | hash profile sign | `profile/sign-19/Phoenix-SHAKE-128f__sign.json` |
| `Phoenix-SHAKE-128f` | hash profile verify | `profile/sign-19/Phoenix-SHAKE-128f__verify.json` |
| `Phoenix-SHAKE-128s` | KAT log (sha256 `d9d52d2b659bdf88…`) | `kat/sign-19/Phoenix-SHAKE-128s.log` |
| `Phoenix-SHAKE-128s` | timing keygen | `records/sign-19/Phoenix-SHAKE-128s__keygen.json` |
| `Phoenix-SHAKE-128s` | timing sign | `records/sign-19/Phoenix-SHAKE-128s__sign.json` |
| `Phoenix-SHAKE-128s` | timing verify | `records/sign-19/Phoenix-SHAKE-128s__verify.json` |
| `Phoenix-SHAKE-128s` | hash profile keygen | `profile/sign-19/Phoenix-SHAKE-128s__keygen.json` |
| `Phoenix-SHAKE-128s` | hash profile sign | `profile/sign-19/Phoenix-SHAKE-128s__sign.json` |
| `Phoenix-SHAKE-128s` | hash profile verify | `profile/sign-19/Phoenix-SHAKE-128s__verify.json` |
| `Phoenix-SHAKE-192f` | KAT log (sha256 `f4c56f03e2032031…`) | `kat/sign-19/Phoenix-SHAKE-192f.log` |
| `Phoenix-SHAKE-192f` | timing keygen | `records/sign-19/Phoenix-SHAKE-192f__keygen.json` |
| `Phoenix-SHAKE-192f` | timing sign | `records/sign-19/Phoenix-SHAKE-192f__sign.json` |
| `Phoenix-SHAKE-192f` | timing verify | `records/sign-19/Phoenix-SHAKE-192f__verify.json` |
| `Phoenix-SHAKE-192f` | hash profile keygen | `profile/sign-19/Phoenix-SHAKE-192f__keygen.json` |
| `Phoenix-SHAKE-192f` | hash profile sign | `profile/sign-19/Phoenix-SHAKE-192f__sign.json` |
| `Phoenix-SHAKE-192f` | hash profile verify | `profile/sign-19/Phoenix-SHAKE-192f__verify.json` |
| `Phoenix-SHAKE-192s` | KAT log (sha256 `57c550c743e5ce11…`) | `kat/sign-19/Phoenix-SHAKE-192s.log` |
| `Phoenix-SHAKE-192s` | timing keygen | `records/sign-19/Phoenix-SHAKE-192s__keygen.json` |
| `Phoenix-SHAKE-192s` | timing sign | `records/sign-19/Phoenix-SHAKE-192s__sign.json` |
| `Phoenix-SHAKE-192s` | timing verify | `records/sign-19/Phoenix-SHAKE-192s__verify.json` |
| `Phoenix-SHAKE-192s` | hash profile keygen | `profile/sign-19/Phoenix-SHAKE-192s__keygen.json` |
| `Phoenix-SHAKE-192s` | hash profile sign | `profile/sign-19/Phoenix-SHAKE-192s__sign.json` |
| `Phoenix-SHAKE-192s` | hash profile verify | `profile/sign-19/Phoenix-SHAKE-192s__verify.json` |
| `Phoenix-SHAKE-256f` | KAT log (sha256 `9ad07dbd3b6592cc…`) | `kat/sign-19/Phoenix-SHAKE-256f.log` |
| `Phoenix-SHAKE-256f` | timing keygen | `records/sign-19/Phoenix-SHAKE-256f__keygen.json` |
| `Phoenix-SHAKE-256f` | timing sign | `records/sign-19/Phoenix-SHAKE-256f__sign.json` |
| `Phoenix-SHAKE-256f` | timing verify | `records/sign-19/Phoenix-SHAKE-256f__verify.json` |
| `Phoenix-SHAKE-256f` | hash profile keygen | `profile/sign-19/Phoenix-SHAKE-256f__keygen.json` |
| `Phoenix-SHAKE-256f` | hash profile sign | `profile/sign-19/Phoenix-SHAKE-256f__sign.json` |
| `Phoenix-SHAKE-256f` | hash profile verify | `profile/sign-19/Phoenix-SHAKE-256f__verify.json` |
| `Phoenix-SHAKE-256s` | KAT log (sha256 `abaf9597cf1bbbdb…`) | `kat/sign-19/Phoenix-SHAKE-256s.log` |
| `Phoenix-SHAKE-256s` | timing keygen | `records/sign-19/Phoenix-SHAKE-256s__keygen.json` |
| `Phoenix-SHAKE-256s` | timing sign | `records/sign-19/Phoenix-SHAKE-256s__sign.json` |
| `Phoenix-SHAKE-256s` | timing verify | `records/sign-19/Phoenix-SHAKE-256s__verify.json` |
| `Phoenix-SHAKE-256s` | hash profile keygen | `profile/sign-19/Phoenix-SHAKE-256s__keygen.json` |
| `Phoenix-SHAKE-256s` | hash profile sign | `profile/sign-19/Phoenix-SHAKE-256s__sign.json` |
| `Phoenix-SHAKE-256s` | hash profile verify | `profile/sign-19/Phoenix-SHAKE-256s__verify.json` |
| `Phoenix-SHAKE-384f` | KAT log (sha256 `ffa3f642ceba8c0a…`) | `kat/sign-19/Phoenix-SHAKE-384f.log` |
| `Phoenix-SHAKE-384f` | timing keygen | `records/sign-19/Phoenix-SHAKE-384f__keygen.json` |
| `Phoenix-SHAKE-384f` | timing sign | `records/sign-19/Phoenix-SHAKE-384f__sign.json` |
| `Phoenix-SHAKE-384f` | timing verify | `records/sign-19/Phoenix-SHAKE-384f__verify.json` |
| `Phoenix-SHAKE-384f` | hash profile keygen | `profile/sign-19/Phoenix-SHAKE-384f__keygen.json` |
| `Phoenix-SHAKE-384f` | hash profile sign | `profile/sign-19/Phoenix-SHAKE-384f__sign.json` |
| `Phoenix-SHAKE-384f` | hash profile verify | `profile/sign-19/Phoenix-SHAKE-384f__verify.json` |
| `Phoenix-SHAKE-384s` | KAT log (sha256 `07ceec279da5ed72…`) | `kat/sign-19/Phoenix-SHAKE-384s.log` |
| `Phoenix-SHAKE-384s` | timing keygen | `records/sign-19/Phoenix-SHAKE-384s__keygen.json` |
| `Phoenix-SHAKE-384s` | timing sign | `records/sign-19/Phoenix-SHAKE-384s__sign.json` |
| `Phoenix-SHAKE-384s` | timing verify | `records/sign-19/Phoenix-SHAKE-384s__verify.json` |
| `Phoenix-SHAKE-384s` | hash profile keygen | `profile/sign-19/Phoenix-SHAKE-384s__keygen.json` |
| `Phoenix-SHAKE-384s` | hash profile sign | `profile/sign-19/Phoenix-SHAKE-384s__sign.json` |
| `Phoenix-SHAKE-384s` | hash profile verify | `profile/sign-19/Phoenix-SHAKE-384s__verify.json` |
| `Phoenix-SHAKE-512f` | KAT log (sha256 `69ea45cc0affdc37…`) | `kat/sign-19/Phoenix-SHAKE-512f.log` |
| `Phoenix-SHAKE-512f` | timing keygen | `records/sign-19/Phoenix-SHAKE-512f__keygen.json` |
| `Phoenix-SHAKE-512f` | timing sign | `records/sign-19/Phoenix-SHAKE-512f__sign.json` |
| `Phoenix-SHAKE-512f` | timing verify | `records/sign-19/Phoenix-SHAKE-512f__verify.json` |
| `Phoenix-SHAKE-512f` | hash profile keygen | `profile/sign-19/Phoenix-SHAKE-512f__keygen.json` |
| `Phoenix-SHAKE-512f` | hash profile sign | `profile/sign-19/Phoenix-SHAKE-512f__sign.json` |
| `Phoenix-SHAKE-512f` | hash profile verify | `profile/sign-19/Phoenix-SHAKE-512f__verify.json` |
| `Phoenix-SHAKE-512s` | KAT log (sha256 `98c33d1867e1de95…`) | `kat/sign-19/Phoenix-SHAKE-512s.log` |
| `Phoenix-SHAKE-512s` | timing keygen | `records/sign-19/Phoenix-SHAKE-512s__keygen.json` |
| `Phoenix-SHAKE-512s` | timing sign | `records/sign-19/Phoenix-SHAKE-512s__sign.json` |
| `Phoenix-SHAKE-512s` | timing verify | `records/sign-19/Phoenix-SHAKE-512s__verify.json` |
| `Phoenix-SHAKE-512s` | hash profile keygen | `profile/sign-19/Phoenix-SHAKE-512s__keygen.json` |
| `Phoenix-SHAKE-512s` | hash profile sign | `profile/sign-19/Phoenix-SHAKE-512s__sign.json` |
| `Phoenix-SHAKE-512s` | hash profile verify | `profile/sign-19/Phoenix-SHAKE-512s__verify.json` |
| `Phoenix-SM3-128f` | KAT log (sha256 `38d839c3e2e37d5c…`) | `kat/sign-19/Phoenix-SM3-128f.log` |
| `Phoenix-SM3-128f` | timing keygen | `records/sign-19/Phoenix-SM3-128f__keygen.json` |
| `Phoenix-SM3-128f` | timing sign | `records/sign-19/Phoenix-SM3-128f__sign.json` |
| `Phoenix-SM3-128f` | timing verify | `records/sign-19/Phoenix-SM3-128f__verify.json` |
| `Phoenix-SM3-128f` | hash profile keygen | `profile/sign-19/Phoenix-SM3-128f__keygen.json` |
| `Phoenix-SM3-128f` | hash profile sign | `profile/sign-19/Phoenix-SM3-128f__sign.json` |
| `Phoenix-SM3-128f` | hash profile verify | `profile/sign-19/Phoenix-SM3-128f__verify.json` |
| `Phoenix-SM3-128s` | KAT log (sha256 `f166029062654da7…`) | `kat/sign-19/Phoenix-SM3-128s.log` |
| `Phoenix-SM3-128s` | timing keygen | `records/sign-19/Phoenix-SM3-128s__keygen.json` |
| `Phoenix-SM3-128s` | timing sign | `records/sign-19/Phoenix-SM3-128s__sign.json` |
| `Phoenix-SM3-128s` | timing verify | `records/sign-19/Phoenix-SM3-128s__verify.json` |
| `Phoenix-SM3-128s` | hash profile keygen | `profile/sign-19/Phoenix-SM3-128s__keygen.json` |
| `Phoenix-SM3-128s` | hash profile sign | `profile/sign-19/Phoenix-SM3-128s__sign.json` |
| `Phoenix-SM3-128s` | hash profile verify | `profile/sign-19/Phoenix-SM3-128s__verify.json` |
| `Phoenix-SM3-192f` | KAT log (sha256 `b7186300a7f7c32f…`) | `kat/sign-19/Phoenix-SM3-192f.log` |
| `Phoenix-SM3-192f` | timing keygen | `records/sign-19/Phoenix-SM3-192f__keygen.json` |
| `Phoenix-SM3-192f` | timing sign | `records/sign-19/Phoenix-SM3-192f__sign.json` |
| `Phoenix-SM3-192f` | timing verify | `records/sign-19/Phoenix-SM3-192f__verify.json` |
| `Phoenix-SM3-192f` | hash profile keygen | `profile/sign-19/Phoenix-SM3-192f__keygen.json` |
| `Phoenix-SM3-192f` | hash profile sign | `profile/sign-19/Phoenix-SM3-192f__sign.json` |
| `Phoenix-SM3-192f` | hash profile verify | `profile/sign-19/Phoenix-SM3-192f__verify.json` |
| `Phoenix-SM3-192s` | KAT log (sha256 `9d9954e8b1f5e881…`) | `kat/sign-19/Phoenix-SM3-192s.log` |
| `Phoenix-SM3-192s` | timing keygen | `records/sign-19/Phoenix-SM3-192s__keygen.json` |
| `Phoenix-SM3-192s` | timing sign | `records/sign-19/Phoenix-SM3-192s__sign.json` |
| `Phoenix-SM3-192s` | timing verify | `records/sign-19/Phoenix-SM3-192s__verify.json` |
| `Phoenix-SM3-192s` | hash profile keygen | `profile/sign-19/Phoenix-SM3-192s__keygen.json` |
| `Phoenix-SM3-192s` | hash profile sign | `profile/sign-19/Phoenix-SM3-192s__sign.json` |
| `Phoenix-SM3-192s` | hash profile verify | `profile/sign-19/Phoenix-SM3-192s__verify.json` |
| `Phoenix-SM3-256f` | KAT log (sha256 `5fb1e75749eca822…`) | `kat/sign-19/Phoenix-SM3-256f.log` |
| `Phoenix-SM3-256f` | timing keygen | `records/sign-19/Phoenix-SM3-256f__keygen.json` |
| `Phoenix-SM3-256f` | timing sign | `records/sign-19/Phoenix-SM3-256f__sign.json` |
| `Phoenix-SM3-256f` | timing verify | `records/sign-19/Phoenix-SM3-256f__verify.json` |
| `Phoenix-SM3-256f` | hash profile keygen | `profile/sign-19/Phoenix-SM3-256f__keygen.json` |
| `Phoenix-SM3-256f` | hash profile sign | `profile/sign-19/Phoenix-SM3-256f__sign.json` |
| `Phoenix-SM3-256f` | hash profile verify | `profile/sign-19/Phoenix-SM3-256f__verify.json` |
| `Phoenix-SM3-256s` | KAT log (sha256 `aeab9f70c73d28a6…`) | `kat/sign-19/Phoenix-SM3-256s.log` |
| `Phoenix-SM3-256s` | timing keygen | `records/sign-19/Phoenix-SM3-256s__keygen.json` |
| `Phoenix-SM3-256s` | timing sign | `records/sign-19/Phoenix-SM3-256s__sign.json` |
| `Phoenix-SM3-256s` | timing verify | `records/sign-19/Phoenix-SM3-256s__verify.json` |
| `Phoenix-SM3-256s` | hash profile keygen | `profile/sign-19/Phoenix-SM3-256s__keygen.json` |
| `Phoenix-SM3-256s` | hash profile sign | `profile/sign-19/Phoenix-SM3-256s__sign.json` |
| `Phoenix-SM3-256s` | hash profile verify | `profile/sign-19/Phoenix-SM3-256s__verify.json` |
| `Phoenix-SM3-384f` | KAT log (sha256 `f1454f14620520df…`) | `kat/sign-19/Phoenix-SM3-384f.log` |
| `Phoenix-SM3-384f` | timing keygen | `records/sign-19/Phoenix-SM3-384f__keygen.json` |
| `Phoenix-SM3-384f` | timing sign | `records/sign-19/Phoenix-SM3-384f__sign.json` |
| `Phoenix-SM3-384f` | timing verify | `records/sign-19/Phoenix-SM3-384f__verify.json` |
| `Phoenix-SM3-384f` | hash profile keygen | `profile/sign-19/Phoenix-SM3-384f__keygen.json` |
| `Phoenix-SM3-384f` | hash profile sign | `profile/sign-19/Phoenix-SM3-384f__sign.json` |
| `Phoenix-SM3-384f` | hash profile verify | `profile/sign-19/Phoenix-SM3-384f__verify.json` |
| `Phoenix-SM3-384s` | KAT log (sha256 `e95b1f9637acff3e…`) | `kat/sign-19/Phoenix-SM3-384s.log` |
| `Phoenix-SM3-384s` | timing keygen | `records/sign-19/Phoenix-SM3-384s__keygen.json` |
| `Phoenix-SM3-384s` | timing sign | `records/sign-19/Phoenix-SM3-384s__sign.json` |
| `Phoenix-SM3-384s` | timing verify | `records/sign-19/Phoenix-SM3-384s__verify.json` |
| `Phoenix-SM3-384s` | hash profile keygen | `profile/sign-19/Phoenix-SM3-384s__keygen.json` |
| `Phoenix-SM3-384s` | hash profile sign | `profile/sign-19/Phoenix-SM3-384s__sign.json` |
| `Phoenix-SM3-384s` | hash profile verify | `profile/sign-19/Phoenix-SM3-384s__verify.json` |
| `Phoenix-SM3-512f` | KAT log (sha256 `79d8c88f28eee52e…`) | `kat/sign-19/Phoenix-SM3-512f.log` |
| `Phoenix-SM3-512f` | timing keygen | `records/sign-19/Phoenix-SM3-512f__keygen.json` |
| `Phoenix-SM3-512f` | timing sign | `records/sign-19/Phoenix-SM3-512f__sign.json` |
| `Phoenix-SM3-512f` | timing verify | `records/sign-19/Phoenix-SM3-512f__verify.json` |
| `Phoenix-SM3-512f` | hash profile keygen | `profile/sign-19/Phoenix-SM3-512f__keygen.json` |
| `Phoenix-SM3-512f` | hash profile sign | `profile/sign-19/Phoenix-SM3-512f__sign.json` |
| `Phoenix-SM3-512f` | hash profile verify | `profile/sign-19/Phoenix-SM3-512f__verify.json` |
| `Phoenix-SM3-512s` | KAT log (sha256 `01bf6b38ffc539b5…`) | `kat/sign-19/Phoenix-SM3-512s.log` |
| `Phoenix-SM3-512s` | timing keygen | `records/sign-19/Phoenix-SM3-512s__keygen.json` |
| `Phoenix-SM3-512s` | timing sign | `records/sign-19/Phoenix-SM3-512s__sign.json` |
| `Phoenix-SM3-512s` | timing verify | `records/sign-19/Phoenix-SM3-512s__verify.json` |
| `Phoenix-SM3-512s` | hash profile keygen | `profile/sign-19/Phoenix-SM3-512s__keygen.json` |
| `Phoenix-SM3-512s` | hash profile sign | `profile/sign-19/Phoenix-SM3-512s__sign.json` |
| `Phoenix-SM3-512s` | hash profile verify | `profile/sign-19/Phoenix-SM3-512s__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

