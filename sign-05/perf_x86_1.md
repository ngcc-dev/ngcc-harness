# sign-05 Chinith — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-05` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561076774162432.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: Chinith
- Implementation versions measured: reference
- Parameter sets: `sm4th_d3_128f_loose`, `sm4th_d3_128f_tight`, `sm4th_d3_128s_loose`, `sm4th_d3_128s_tight`, `sm4th_em_d2_128f_loose`, `sm4th_em_d2_128f_tight`, `sm4th_em_d2_128s_loose`, `sm4th_em_d2_128s_tight`, `ublockith_d3_256f`, `ublockith_d3_256s`, `ublockith_em_d3_256f`, `ublockith_em_d3_256s`, `vistrutith_d3_512f`, `vistrutith_d3_512s`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-05/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `sm4th_d3_128f_loose` | guide | PASS |
| `sm4th_d3_128f_tight` | guide | PASS |
| `sm4th_d3_128s_loose` | guide | PASS |
| `sm4th_d3_128s_tight` | guide | PASS |
| `sm4th_em_d2_128f_loose` | guide | PASS |
| `sm4th_em_d2_128f_tight` | guide | PASS |
| `sm4th_em_d2_128s_loose` | guide | PASS |
| `sm4th_em_d2_128s_tight` | guide | PASS |
| `ublockith_d3_256f` | guide | PASS |
| `ublockith_d3_256s` | guide | PASS |
| `ublockith_em_d3_256f` | guide | PASS |
| `ublockith_em_d3_256s` | guide | PASS |
| `vistrutith_d3_512f` | guide | PASS |
| `vistrutith_d3_512s` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `sm4th_d3_128f_loose` | keygen | 12.0 k | 5.72 µs | 1.75e+05 | 5.72 µs | 100000 (5 × 20000) |
| `sm4th_d3_128f_loose` | sign | 125.86 M | 60.1 ms | 16.6 | 60.1 ms | 100 (5 × 20) |
| `sm4th_d3_128f_loose` | verify | 83.72 M | 40 ms | 25 | 40 ms | 130 (5 × 26) |
| `sm4th_d3_128f_tight` | keygen | 11.9 k | 5.7 µs | 1.76e+05 | 5.7 µs | 100000 (5 × 20000) |
| `sm4th_d3_128f_tight` | sign | 194.81 M | 93.1 ms | 10.7 | 93.1 ms | 100 (5 × 20) |
| `sm4th_d3_128f_tight` | verify | 120.51 M | 57.6 ms | 17.4 | 57.6 ms | 100 (5 × 20) |
| `sm4th_d3_128s_loose` | keygen | 12.0 k | 5.73 µs | 1.74e+05 | 5.72 µs | 100000 (5 × 20000) |
| `sm4th_d3_128s_loose` | sign | 354.25 M | 171 ms | 5.86 | 169 ms | 100 (5 × 20) |
| `sm4th_d3_128s_loose` | verify | 296.38 M | 143 ms | 6.99 | 141 ms | 100 (5 × 20) |
| `sm4th_d3_128s_tight` | keygen | 11.9 k | 5.7 µs | 1.75e+05 | 5.7 µs | 100000 (5 × 20000) |
| `sm4th_d3_128s_tight` | sign | 422.51 M | 203 ms | 4.93 | 202 ms | 100 (5 × 20) |
| `sm4th_d3_128s_tight` | verify | 338.39 M | 162 ms | 6.18 | 162 ms | 100 (5 × 20) |
| `sm4th_em_d2_128f_loose` | keygen | 11.9 k | 5.7 µs | 1.76e+05 | 5.69 µs | 100000 (5 × 20000) |
| `sm4th_em_d2_128f_loose` | sign | 29.92 M | 14.3 ms | 70 | 14.3 ms | 350 (5 × 70) |
| `sm4th_em_d2_128f_loose` | verify | 28.37 M | 13.6 ms | 73.8 | 13.6 ms | 370 (5 × 74) |
| `sm4th_em_d2_128f_tight` | keygen | 11.9 k | 5.71 µs | 1.75e+05 | 5.7 µs | 100000 (5 × 20000) |
| `sm4th_em_d2_128f_tight` | sign | 34.15 M | 16.3 ms | 61.3 | 16.3 ms | 310 (5 × 62) |
| `sm4th_em_d2_128f_tight` | verify | 32.63 M | 15.6 ms | 64.2 | 15.6 ms | 320 (5 × 64) |
| `sm4th_em_d2_128s_loose` | keygen | 11.9 k | 5.69 µs | 1.76e+05 | 5.69 µs | 100000 (5 × 20000) |
| `sm4th_em_d2_128s_loose` | sign | 196.83 M | 94.1 ms | 10.6 | 94.1 ms | 100 (5 × 20) |
| `sm4th_em_d2_128s_loose` | verify | 186.07 M | 88.9 ms | 11.2 | 89 ms | 100 (5 × 20) |
| `sm4th_em_d2_128s_tight` | keygen | 11.9 k | 5.68 µs | 1.76e+05 | 5.68 µs | 100000 (5 × 20000) |
| `sm4th_em_d2_128s_tight` | sign | 232.02 M | 114 ms | 8.8 | 114 ms | 100 (5 × 20) |
| `sm4th_em_d2_128s_tight` | verify | 220.22 M | 105 ms | 9.49 | 105 ms | 100 (5 × 20) |
| `ublockith_d3_256f` | keygen | 14.6 k | 6.99 µs | 1.43e+05 | 7 µs | 100000 (5 × 20000) |
| `ublockith_d3_256f` | sign | 351.34 M | 169 ms | 5.9 | 168 ms | 100 (5 × 20) |
| `ublockith_d3_256f` | verify | 407.09 M | 196 ms | 5.1 | 195 ms | 100 (5 × 20) |
| `ublockith_d3_256s` | keygen | 14.6 k | 6.99 µs | 1.43e+05 | 6.99 µs | 100000 (5 × 20000) |
| `ublockith_d3_256s` | sign | 1.08 G | 521 ms | 1.92 | 521 ms | 100 (5 × 20) |
| `ublockith_d3_256s` | verify | 1.14 G | 544 ms | 1.84 | 544 ms | 100 (5 × 20) |
| `ublockith_em_d3_256f` | keygen | 14.7 k | 7.01 µs | 1.43e+05 | 7.01 µs | 100000 (5 × 20000) |
| `ublockith_em_d3_256f` | sign | 267.51 M | 128 ms | 7.79 | 128 ms | 100 (5 × 20) |
| `ublockith_em_d3_256f` | verify | 310.14 M | 150 ms | 6.69 | 148 ms | 100 (5 × 20) |
| `ublockith_em_d3_256s` | keygen | 14.7 k | 7.01 µs | 1.43e+05 | 7 µs | 100000 (5 × 20000) |
| `ublockith_em_d3_256s` | sign | 897.43 M | 436 ms | 2.3 | 434 ms | 100 (5 × 20) |
| `ublockith_em_d3_256s` | verify | 923.48 M | 442 ms | 2.26 | 442 ms | 100 (5 × 20) |
| `vistrutith_d3_512f` | keygen | 15.9 k | 7.6 µs | 1.32e+05 | 7.61 µs | 100000 (5 × 20000) |
| `vistrutith_d3_512f` | sign | 6.37 G | 3.05 s | 0.328 | 3.05 s | 100 (5 × 20) |
| `vistrutith_d3_512f` | verify | 5.27 G | 2.52 s | 0.396 | 2.52 s | 100 (5 × 20) |
| `vistrutith_d3_512s` | keygen | 15.9 k | 7.59 µs | 1.32e+05 | 7.6 µs | 100000 (5 × 20000) |
| `vistrutith_d3_512s` | sign | 11.79 G | 5.67 s | 0.176 | 5.67 s | 100 (5 × 20) |
| `vistrutith_d3_512s` | verify | 10.70 G | 5.12 s | 0.195 | 5.12 s | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `sm4th_d3_128f_loose` | keygen | 143905 | 1756 KiB | 1836 KiB |
| `sm4th_d3_128f_loose` | sign | 143905 | 1772 KiB | 3316 KiB |
| `sm4th_d3_128f_loose` | verify | 143905 | 2264 KiB | 3260 KiB |
| `sm4th_d3_128f_tight` | keygen | 184837 | 1788 KiB | 1868 KiB |
| `sm4th_d3_128f_tight` | sign | 184837 | 1808 KiB | 3908 KiB |
| `sm4th_d3_128f_tight` | verify | 184837 | 2372 KiB | 4764 KiB |
| `sm4th_d3_128s_loose` | keygen | 143905 | 1764 KiB | 1844 KiB |
| `sm4th_d3_128s_loose` | sign | 143905 | 1784 KiB | 10600 KiB |
| `sm4th_d3_128s_loose` | verify | 143905 | 2460 KiB | 16500 KiB |
| `sm4th_d3_128s_tight` | keygen | 184837 | 1792 KiB | 1892 KiB |
| `sm4th_d3_128s_tight` | sign | 184837 | 1800 KiB | 15748 KiB |
| `sm4th_d3_128s_tight` | verify | 184837 | 2972 KiB | 15820 KiB |
| `sm4th_em_d2_128f_loose` | keygen | 104225 | 1768 KiB | 1852 KiB |
| `sm4th_em_d2_128f_loose` | sign | 104225 | 1768 KiB | 2780 KiB |
| `sm4th_em_d2_128f_loose` | verify | 104225 | 2172 KiB | 2772 KiB |
| `sm4th_em_d2_128f_tight` | keygen | 131853 | 1804 KiB | 1868 KiB |
| `sm4th_em_d2_128f_tight` | sign | 131853 | 1788 KiB | 3212 KiB |
| `sm4th_em_d2_128f_tight` | verify | 131853 | 2120 KiB | 3268 KiB |
| `sm4th_em_d2_128s_loose` | keygen | 104225 | 1744 KiB | 1848 KiB |
| `sm4th_em_d2_128s_loose` | sign | 104225 | 1764 KiB | 7800 KiB |
| `sm4th_em_d2_128s_loose` | verify | 104225 | 2152 KiB | 7848 KiB |
| `sm4th_em_d2_128s_tight` | keygen | 131853 | 1800 KiB | 1892 KiB |
| `sm4th_em_d2_128s_tight` | sign | 131853 | 1788 KiB | 12036 KiB |
| `sm4th_em_d2_128s_tight` | verify | 131853 | 2408 KiB | 12200 KiB |
| `ublockith_d3_256f` | keygen | 150513 | 1792 KiB | 1920 KiB |
| `ublockith_d3_256f` | sign | 150513 | 1848 KiB | 9284 KiB |
| `ublockith_d3_256f` | verify | 150513 | 2520 KiB | 13296 KiB |
| `ublockith_d3_256s` | keygen | 145913 | 1784 KiB | 1908 KiB |
| `ublockith_d3_256s` | sign | 145913 | 1748 KiB | 17148 KiB |
| `ublockith_d3_256s` | verify | 145913 | 6872 KiB | 17136 KiB |
| `ublockith_em_d3_256f` | keygen | 149537 | 1752 KiB | 1936 KiB |
| `ublockith_em_d3_256f` | sign | 149537 | 1836 KiB | 4232 KiB |
| `ublockith_em_d3_256f` | verify | 149537 | 2656 KiB | 3980 KiB |
| `ublockith_em_d3_256s` | keygen | 149537 | 1772 KiB | 1900 KiB |
| `ublockith_em_d3_256s` | sign | 149537 | 1816 KiB | 16380 KiB |
| `ublockith_em_d3_256s` | verify | 149537 | 6328 KiB | 17792 KiB |
| `vistrutith_d3_512f` | keygen | 191201 | 1908 KiB | 1992 KiB |
| `vistrutith_d3_512f` | sign | 191201 | 1920 KiB | 9332 KiB |
| `vistrutith_d3_512f` | verify | 191201 | 3556 KiB | 9104 KiB |
| `vistrutith_d3_512s` | keygen | 191201 | 1876 KiB | 1948 KiB |
| `vistrutith_d3_512s` | sign | 191201 | 1896 KiB | 55724 KiB |
| `vistrutith_d3_512s` | verify | 191201 | 11692 KiB | 63592 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `sm4th_d3_128f_loose` | 32 | 32 | 6724 |
| `sm4th_d3_128f_tight` | 32 | 32 | 11864 |
| `sm4th_d3_128s_loose` | 32 | 32 | 5056 |
| `sm4th_d3_128s_tight` | 32 | 32 | 9176 |
| `sm4th_em_d2_128f_loose` | 32 | 32 | 4932 |
| `sm4th_em_d2_128f_tight` | 32 | 32 | 9450 |
| `sm4th_em_d2_128s_loose` | 32 | 32 | 3818 |
| `sm4th_em_d2_128s_tight` | 32 | 32 | 7556 |
| `ublockith_d3_256f` | 64 | 64 | 31556 |
| `ublockith_d3_256s` | 64 | 64 | 24144 |
| `ublockith_em_d3_256f` | 64 | 64 | 25028 |
| `ublockith_em_d3_256s` | 64 | 64 | 19056 |
| `vistrutith_d3_512f` | 128 | 128 | 106788 |
| `vistrutith_d3_512s` | 128 | 128 | 83004 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **mixed** — random oracles via ICCS; seed-tree/VOLE PRG via SM4 / Ballet / Vistrutah

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `sm4th_d3_128f_loose` | keygen | 0.0% | 88% | drng 2.34 |
| `sm4th_d3_128f_loose` | sign | 2.7% | 0.0% | drng 1, pseudoXOF 2, sm3hash 352 |
| `sm4th_d3_128f_loose` | verify | 3.0% | 0.0% | pseudoXOF 2, sm3hash 20 |
| `sm4th_d3_128f_tight` | keygen | 0.0% | 88% | drng 2.34 |
| `sm4th_d3_128f_tight` | sign | 5.9% | 0.0% | drng 1, pseudoXOF 2, pseudohash 23, sm3hash 529 |
| `sm4th_d3_128f_tight` | verify | 8.3% | 0.0% | pseudoXOF 2, pseudohash 22, sm3hash 3 |
| `sm4th_d3_128s_loose` | keygen | 0.0% | 88% | drng 2.34 |
| `sm4th_d3_128s_loose` | sign | 11% | 0.0% | drng 1, pseudoXOF 2, sm3hash 7.98e+03 |
| `sm4th_d3_128s_loose` | verify | 5.3% | 0.0% | pseudoXOF 2, sm3hash 15 |
| `sm4th_d3_128s_tight` | keygen | 0.0% | 88% | drng 2.34 |
| `sm4th_d3_128s_tight` | sign | 21% | 0.0% | drng 1, pseudoXOF 2, pseudohash 16, sm3hash 4.43e+03 |
| `sm4th_d3_128s_tight` | verify | 22% | 0.0% | pseudoXOF 2, pseudohash 15, sm3hash 3 |
| `sm4th_em_d2_128f_loose` | keygen | 0.0% | 88% | drng 2.34 |
| `sm4th_em_d2_128f_loose` | sign | 11% | 0.0% | drng 1, pseudoXOF 303, sm3hash 19 |
| `sm4th_em_d2_128f_loose` | verify | 8.4% | 0.0% | pseudoXOF 4, sm3hash 18 |
| `sm4th_em_d2_128f_tight` | keygen | 0.0% | 89% | drng 2.34 |
| `sm4th_em_d2_128f_tight` | sign | 31% | 0.0% | drng 1, pseudoXOF 256, pseudohash 23, sm3hash 2 |
| `sm4th_em_d2_128f_tight` | verify | 30% | 0.0% | pseudoXOF 3, pseudohash 22, sm3hash 2 |
| `sm4th_em_d2_128s_loose` | keygen | 0.0% | 89% | drng 2.34 |
| `sm4th_em_d2_128s_loose` | sign | 14% | 0.0% | drng 1, pseudoXOF 4.26e+03, sm3hash 14 |
| `sm4th_em_d2_128s_loose` | verify | 8.3% | 0.0% | pseudoXOF 4, sm3hash 13 |
| `sm4th_em_d2_128s_tight` | keygen | 0.0% | 88% | drng 2.34 |
| `sm4th_em_d2_128s_tight` | sign | 36% | 0.0% | drng 1, pseudoXOF 2.95e+03, pseudohash 16, sm3hash 2 |
| `sm4th_em_d2_128s_tight` | verify | 34% | 0.0% | pseudoXOF 3, pseudohash 15, sm3hash 2 |
| `ublockith_d3_256f` | keygen | 0.0% | 72% | drng 2.34 |
| `ublockith_d3_256f` | sign | 6.7% | 0.0% | drng 1, pseudoXOF 2, pseudohash 35, sm3hash 109 |
| `ublockith_d3_256f` | verify | 5.7% | 0.0% | pseudoXOF 2, pseudohash 34, sm3hash 2 |
| `ublockith_d3_256s` | keygen | 0.0% | 72% | drng 2.34 |
| `ublockith_d3_256s` | sign | 16% | 0.0% | drng 1, pseudoXOF 2, pseudohash 25, sm3hash 50.3 |
| `ublockith_d3_256s` | verify | 15% | 0.0% | pseudoXOF 2, pseudohash 24, sm3hash 2 |
| `ublockith_em_d3_256f` | keygen | 0.0% | 72% | drng 2.34 |
| `ublockith_em_d3_256f` | sign | 8.8% | 0.0% | drng 1, pseudoXOF 2, pseudohash 35, sm3hash 251 |
| `ublockith_em_d3_256f` | verify | 7.2% | 0.0% | pseudoXOF 2, pseudohash 34, sm3hash 2 |
| `ublockith_em_d3_256s` | keygen | 0.0% | 72% | drng 2.34 |
| `ublockith_em_d3_256s` | sign | 21% | 0.0% | drng 1, pseudoXOF 2, pseudohash 25, sm3hash 3.75e+03 |
| `ublockith_em_d3_256s` | verify | 18% | 0.0% | pseudoXOF 2, pseudohash 24, sm3hash 2 |
| `vistrutith_d3_512f` | keygen | 0.0% | 87% | drng 2.34 |
| `vistrutith_d3_512f` | sign | 1.7% | 0.0% | drng 1, pseudoXOF 2, pseudohash 123, sm3hash 1 |
| `vistrutith_d3_512f` | verify | 2.0% | 0.0% | pseudoXOF 2, pseudohash 67, sm3hash 1 |
| `vistrutith_d3_512s` | keygen | 0.0% | 87% | drng 2.34 |
| `vistrutith_d3_512s` | sign | 6.6% | 0.0% | drng 1, pseudoXOF 2, pseudohash 69.7, sm3hash 1 |
| `vistrutith_d3_512s` | verify | 7.2% | 0.0% | pseudoXOF 2, pseudohash 47, sm3hash 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `sm4th_d3_128f_loose` | KAT log (sha256 `6bc8eaf9d5f2fa41…`) | `kat/sign-05/sm4th_d3_128f_loose.log` |
| `sm4th_d3_128f_loose` | timing keygen | `records/sign-05/sm4th_d3_128f_loose__keygen.json` |
| `sm4th_d3_128f_loose` | timing sign | `records/sign-05/sm4th_d3_128f_loose__sign.json` |
| `sm4th_d3_128f_loose` | timing verify | `records/sign-05/sm4th_d3_128f_loose__verify.json` |
| `sm4th_d3_128f_loose` | hash profile keygen | `profile/sign-05/sm4th_d3_128f_loose__keygen.json` |
| `sm4th_d3_128f_loose` | hash profile sign | `profile/sign-05/sm4th_d3_128f_loose__sign.json` |
| `sm4th_d3_128f_loose` | hash profile verify | `profile/sign-05/sm4th_d3_128f_loose__verify.json` |
| `sm4th_d3_128f_tight` | KAT log (sha256 `d13a1f2ba0dedb3a…`) | `kat/sign-05/sm4th_d3_128f_tight.log` |
| `sm4th_d3_128f_tight` | timing keygen | `records/sign-05/sm4th_d3_128f_tight__keygen.json` |
| `sm4th_d3_128f_tight` | timing sign | `records/sign-05/sm4th_d3_128f_tight__sign.json` |
| `sm4th_d3_128f_tight` | timing verify | `records/sign-05/sm4th_d3_128f_tight__verify.json` |
| `sm4th_d3_128f_tight` | hash profile keygen | `profile/sign-05/sm4th_d3_128f_tight__keygen.json` |
| `sm4th_d3_128f_tight` | hash profile sign | `profile/sign-05/sm4th_d3_128f_tight__sign.json` |
| `sm4th_d3_128f_tight` | hash profile verify | `profile/sign-05/sm4th_d3_128f_tight__verify.json` |
| `sm4th_d3_128s_loose` | KAT log (sha256 `d1b8bbe31ff1e6a3…`) | `kat/sign-05/sm4th_d3_128s_loose.log` |
| `sm4th_d3_128s_loose` | timing keygen | `records/sign-05/sm4th_d3_128s_loose__keygen.json` |
| `sm4th_d3_128s_loose` | timing sign | `records/sign-05/sm4th_d3_128s_loose__sign.json` |
| `sm4th_d3_128s_loose` | timing verify | `records/sign-05/sm4th_d3_128s_loose__verify.json` |
| `sm4th_d3_128s_loose` | hash profile keygen | `profile/sign-05/sm4th_d3_128s_loose__keygen.json` |
| `sm4th_d3_128s_loose` | hash profile sign | `profile/sign-05/sm4th_d3_128s_loose__sign.json` |
| `sm4th_d3_128s_loose` | hash profile verify | `profile/sign-05/sm4th_d3_128s_loose__verify.json` |
| `sm4th_d3_128s_tight` | KAT log (sha256 `2be015f8463d0b1d…`) | `kat/sign-05/sm4th_d3_128s_tight.log` |
| `sm4th_d3_128s_tight` | timing keygen | `records/sign-05/sm4th_d3_128s_tight__keygen.json` |
| `sm4th_d3_128s_tight` | timing sign | `records/sign-05/sm4th_d3_128s_tight__sign.json` |
| `sm4th_d3_128s_tight` | timing verify | `records/sign-05/sm4th_d3_128s_tight__verify.json` |
| `sm4th_d3_128s_tight` | hash profile keygen | `profile/sign-05/sm4th_d3_128s_tight__keygen.json` |
| `sm4th_d3_128s_tight` | hash profile sign | `profile/sign-05/sm4th_d3_128s_tight__sign.json` |
| `sm4th_d3_128s_tight` | hash profile verify | `profile/sign-05/sm4th_d3_128s_tight__verify.json` |
| `sm4th_em_d2_128f_loose` | KAT log (sha256 `02be1ed91a125359…`) | `kat/sign-05/sm4th_em_d2_128f_loose.log` |
| `sm4th_em_d2_128f_loose` | timing keygen | `records/sign-05/sm4th_em_d2_128f_loose__keygen.json` |
| `sm4th_em_d2_128f_loose` | timing sign | `records/sign-05/sm4th_em_d2_128f_loose__sign.json` |
| `sm4th_em_d2_128f_loose` | timing verify | `records/sign-05/sm4th_em_d2_128f_loose__verify.json` |
| `sm4th_em_d2_128f_loose` | hash profile keygen | `profile/sign-05/sm4th_em_d2_128f_loose__keygen.json` |
| `sm4th_em_d2_128f_loose` | hash profile sign | `profile/sign-05/sm4th_em_d2_128f_loose__sign.json` |
| `sm4th_em_d2_128f_loose` | hash profile verify | `profile/sign-05/sm4th_em_d2_128f_loose__verify.json` |
| `sm4th_em_d2_128f_tight` | KAT log (sha256 `af19563528946915…`) | `kat/sign-05/sm4th_em_d2_128f_tight.log` |
| `sm4th_em_d2_128f_tight` | timing keygen | `records/sign-05/sm4th_em_d2_128f_tight__keygen.json` |
| `sm4th_em_d2_128f_tight` | timing sign | `records/sign-05/sm4th_em_d2_128f_tight__sign.json` |
| `sm4th_em_d2_128f_tight` | timing verify | `records/sign-05/sm4th_em_d2_128f_tight__verify.json` |
| `sm4th_em_d2_128f_tight` | hash profile keygen | `profile/sign-05/sm4th_em_d2_128f_tight__keygen.json` |
| `sm4th_em_d2_128f_tight` | hash profile sign | `profile/sign-05/sm4th_em_d2_128f_tight__sign.json` |
| `sm4th_em_d2_128f_tight` | hash profile verify | `profile/sign-05/sm4th_em_d2_128f_tight__verify.json` |
| `sm4th_em_d2_128s_loose` | KAT log (sha256 `dc60df899d079f90…`) | `kat/sign-05/sm4th_em_d2_128s_loose.log` |
| `sm4th_em_d2_128s_loose` | timing keygen | `records/sign-05/sm4th_em_d2_128s_loose__keygen.json` |
| `sm4th_em_d2_128s_loose` | timing sign | `records/sign-05/sm4th_em_d2_128s_loose__sign.json` |
| `sm4th_em_d2_128s_loose` | timing verify | `records/sign-05/sm4th_em_d2_128s_loose__verify.json` |
| `sm4th_em_d2_128s_loose` | hash profile keygen | `profile/sign-05/sm4th_em_d2_128s_loose__keygen.json` |
| `sm4th_em_d2_128s_loose` | hash profile sign | `profile/sign-05/sm4th_em_d2_128s_loose__sign.json` |
| `sm4th_em_d2_128s_loose` | hash profile verify | `profile/sign-05/sm4th_em_d2_128s_loose__verify.json` |
| `sm4th_em_d2_128s_tight` | KAT log (sha256 `828383a77f982618…`) | `kat/sign-05/sm4th_em_d2_128s_tight.log` |
| `sm4th_em_d2_128s_tight` | timing keygen | `records/sign-05/sm4th_em_d2_128s_tight__keygen.json` |
| `sm4th_em_d2_128s_tight` | timing sign | `records/sign-05/sm4th_em_d2_128s_tight__sign.json` |
| `sm4th_em_d2_128s_tight` | timing verify | `records/sign-05/sm4th_em_d2_128s_tight__verify.json` |
| `sm4th_em_d2_128s_tight` | hash profile keygen | `profile/sign-05/sm4th_em_d2_128s_tight__keygen.json` |
| `sm4th_em_d2_128s_tight` | hash profile sign | `profile/sign-05/sm4th_em_d2_128s_tight__sign.json` |
| `sm4th_em_d2_128s_tight` | hash profile verify | `profile/sign-05/sm4th_em_d2_128s_tight__verify.json` |
| `ublockith_d3_256f` | KAT log (sha256 `4e4cbfb8202155f3…`) | `kat/sign-05/ublockith_d3_256f.log` |
| `ublockith_d3_256f` | timing keygen | `records/sign-05/ublockith_d3_256f__keygen.json` |
| `ublockith_d3_256f` | timing sign | `records/sign-05/ublockith_d3_256f__sign.json` |
| `ublockith_d3_256f` | timing verify | `records/sign-05/ublockith_d3_256f__verify.json` |
| `ublockith_d3_256f` | hash profile keygen | `profile/sign-05/ublockith_d3_256f__keygen.json` |
| `ublockith_d3_256f` | hash profile sign | `profile/sign-05/ublockith_d3_256f__sign.json` |
| `ublockith_d3_256f` | hash profile verify | `profile/sign-05/ublockith_d3_256f__verify.json` |
| `ublockith_d3_256s` | KAT log (sha256 `cc48efc0b26a0cac…`) | `kat/sign-05/ublockith_d3_256s.log` |
| `ublockith_d3_256s` | timing keygen | `records/sign-05/ublockith_d3_256s__keygen.json` |
| `ublockith_d3_256s` | timing sign | `records/sign-05/ublockith_d3_256s__sign.json` |
| `ublockith_d3_256s` | timing verify | `records/sign-05/ublockith_d3_256s__verify.json` |
| `ublockith_d3_256s` | hash profile keygen | `profile/sign-05/ublockith_d3_256s__keygen.json` |
| `ublockith_d3_256s` | hash profile sign | `profile/sign-05/ublockith_d3_256s__sign.json` |
| `ublockith_d3_256s` | hash profile verify | `profile/sign-05/ublockith_d3_256s__verify.json` |
| `ublockith_em_d3_256f` | KAT log (sha256 `f36ddf98437a22cb…`) | `kat/sign-05/ublockith_em_d3_256f.log` |
| `ublockith_em_d3_256f` | timing keygen | `records/sign-05/ublockith_em_d3_256f__keygen.json` |
| `ublockith_em_d3_256f` | timing sign | `records/sign-05/ublockith_em_d3_256f__sign.json` |
| `ublockith_em_d3_256f` | timing verify | `records/sign-05/ublockith_em_d3_256f__verify.json` |
| `ublockith_em_d3_256f` | hash profile keygen | `profile/sign-05/ublockith_em_d3_256f__keygen.json` |
| `ublockith_em_d3_256f` | hash profile sign | `profile/sign-05/ublockith_em_d3_256f__sign.json` |
| `ublockith_em_d3_256f` | hash profile verify | `profile/sign-05/ublockith_em_d3_256f__verify.json` |
| `ublockith_em_d3_256s` | KAT log (sha256 `b89809c29b12b12b…`) | `kat/sign-05/ublockith_em_d3_256s.log` |
| `ublockith_em_d3_256s` | timing keygen | `records/sign-05/ublockith_em_d3_256s__keygen.json` |
| `ublockith_em_d3_256s` | timing sign | `records/sign-05/ublockith_em_d3_256s__sign.json` |
| `ublockith_em_d3_256s` | timing verify | `records/sign-05/ublockith_em_d3_256s__verify.json` |
| `ublockith_em_d3_256s` | hash profile keygen | `profile/sign-05/ublockith_em_d3_256s__keygen.json` |
| `ublockith_em_d3_256s` | hash profile sign | `profile/sign-05/ublockith_em_d3_256s__sign.json` |
| `ublockith_em_d3_256s` | hash profile verify | `profile/sign-05/ublockith_em_d3_256s__verify.json` |
| `vistrutith_d3_512f` | KAT log (sha256 `5f12dc08029b13e8…`) | `kat/sign-05/vistrutith_d3_512f.log` |
| `vistrutith_d3_512f` | timing keygen | `records/sign-05/vistrutith_d3_512f__keygen.json` |
| `vistrutith_d3_512f` | timing sign | `records/sign-05/vistrutith_d3_512f__sign.json` |
| `vistrutith_d3_512f` | timing verify | `records/sign-05/vistrutith_d3_512f__verify.json` |
| `vistrutith_d3_512f` | hash profile keygen | `profile/sign-05/vistrutith_d3_512f__keygen.json` |
| `vistrutith_d3_512f` | hash profile sign | `profile/sign-05/vistrutith_d3_512f__sign.json` |
| `vistrutith_d3_512f` | hash profile verify | `profile/sign-05/vistrutith_d3_512f__verify.json` |
| `vistrutith_d3_512s` | KAT log (sha256 `a108d377c300aec2…`) | `kat/sign-05/vistrutith_d3_512s.log` |
| `vistrutith_d3_512s` | timing keygen | `records/sign-05/vistrutith_d3_512s__keygen.json` |
| `vistrutith_d3_512s` | timing sign | `records/sign-05/vistrutith_d3_512s__sign.json` |
| `vistrutith_d3_512s` | timing verify | `records/sign-05/vistrutith_d3_512s__verify.json` |
| `vistrutith_d3_512s` | hash profile keygen | `profile/sign-05/vistrutith_d3_512s__keygen.json` |
| `vistrutith_d3_512s` | hash profile sign | `profile/sign-05/vistrutith_d3_512s__sign.json` |
| `vistrutith_d3_512s` | hash profile verify | `profile/sign-05/vistrutith_d3_512s__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

