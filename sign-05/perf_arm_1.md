# sign-05 Chinith — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `sign-05` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561076774162432.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: Chinith
- Implementation versions measured: reference
- Parameter sets: `sm4th_d3_128f_loose`, `sm4th_d3_128f_tight`, `sm4th_d3_128s_loose`, `sm4th_d3_128s_tight`, `sm4th_em_d2_128f_loose`, `sm4th_em_d2_128f_tight`, `sm4th_em_d2_128s_loose`, `sm4th_em_d2_128s_tight`, `ublockith_d3_256f`, `ublockith_d3_256s`, `ublockith_em_d3_256f`, `ublockith_em_d3_256s`, `vistrutith_d3_512f`, `vistrutith_d3_512s`

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
| `sm4th_d3_128f_loose` | keygen | 11.2 k | 4.17 µs | 2.4e+05 | 4.09 µs | 100000 (5 × 20000) |
| `sm4th_d3_128f_loose` | sign | 95.61 M | 35.5 ms | 28.2 | 35.5 ms | 100 (5 × 20) |
| `sm4th_d3_128f_loose` | verify | 63.93 M | 23.7 ms | 42.2 | 23.7 ms | 135 (5 × 27) |
| `sm4th_d3_128f_tight` | keygen | 11.0 k | 4.09 µs | 2.45e+05 | 4.09 µs | 100000 (5 × 20000) |
| `sm4th_d3_128f_tight` | sign | 228.82 M | 84.9 ms | 11.8 | 84.9 ms | 100 (5 × 20) |
| `sm4th_d3_128f_tight` | verify | 135.98 M | 50.4 ms | 19.8 | 50.3 ms | 100 (5 × 20) |
| `sm4th_d3_128s_loose` | keygen | 11.0 k | 4.1 µs | 2.44e+05 | 4.09 µs | 100000 (5 × 20000) |
| `sm4th_d3_128s_loose` | sign | 275.70 M | 102 ms | 9.77 | 102 ms | 100 (5 × 20) |
| `sm4th_d3_128s_loose` | verify | 227.64 M | 84.5 ms | 11.8 | 84.5 ms | 100 (5 × 20) |
| `sm4th_d3_128s_tight` | keygen | 11.1 k | 4.13 µs | 2.42e+05 | 4.09 µs | 100000 (5 × 20000) |
| `sm4th_d3_128s_tight` | sign | 398.34 M | 148 ms | 6.77 | 147 ms | 100 (5 × 20) |
| `sm4th_d3_128s_tight` | verify | 296.32 M | 110 ms | 9.09 | 110 ms | 100 (5 × 20) |
| `sm4th_em_d2_128f_loose` | keygen | 11.0 k | 4.09 µs | 2.44e+05 | 4.09 µs | 100000 (5 × 20000) |
| `sm4th_em_d2_128f_loose` | sign | 24.05 M | 8.92 ms | 112 | 8.92 ms | 350 (5 × 70) |
| `sm4th_em_d2_128f_loose` | verify | 22.86 M | 8.48 ms | 118 | 8.46 ms | 375 (5 × 75) |
| `sm4th_em_d2_128f_tight` | keygen | 11.0 k | 4.1 µs | 2.44e+05 | 4.09 µs | 100000 (5 × 20000) |
| `sm4th_em_d2_128f_tight` | sign | 29.55 M | 11 ms | 91.1 | 11 ms | 270 (5 × 54) |
| `sm4th_em_d2_128f_tight` | verify | 28.23 M | 10.5 ms | 95.4 | 10.5 ms | 305 (5 × 61) |
| `sm4th_em_d2_128s_loose` | keygen | 11.0 k | 4.09 µs | 2.44e+05 | 4.09 µs | 100000 (5 × 20000) |
| `sm4th_em_d2_128s_loose` | sign | 160.24 M | 59.5 ms | 16.8 | 59 ms | 100 (5 × 20) |
| `sm4th_em_d2_128s_loose` | verify | 149.64 M | 55.5 ms | 18 | 55.2 ms | 100 (5 × 20) |
| `sm4th_em_d2_128s_tight` | keygen | 11.0 k | 4.09 µs | 2.44e+05 | 4.09 µs | 100000 (5 × 20000) |
| `sm4th_em_d2_128s_tight` | sign | 178.73 M | 66.4 ms | 15.1 | 66.2 ms | 100 (5 × 20) |
| `sm4th_em_d2_128s_tight` | verify | 169.37 M | 62.8 ms | 15.9 | 62.5 ms | 100 (5 × 20) |
| `ublockith_d3_256f` | keygen | 14.1 k | 5.22 µs | 1.92e+05 | 5.22 µs | 100000 (5 × 20000) |
| `ublockith_d3_256f` | sign | 469.86 M | 174 ms | 5.74 | 174 ms | 100 (5 × 20) |
| `ublockith_d3_256f` | verify | 572.71 M | 212 ms | 4.71 | 212 ms | 100 (5 × 20) |
| `ublockith_d3_256s` | keygen | 14.1 k | 5.22 µs | 1.92e+05 | 5.22 µs | 100000 (5 × 20000) |
| `ublockith_d3_256s` | sign | 1.01 G | 374 ms | 2.68 | 374 ms | 100 (5 × 20) |
| `ublockith_d3_256s` | verify | 1.11 G | 413 ms | 2.42 | 413 ms | 100 (5 × 20) |
| `ublockith_em_d3_256f` | keygen | 14.2 k | 5.27 µs | 1.9e+05 | 5.23 µs | 100000 (5 × 20000) |
| `ublockith_em_d3_256f` | sign | 351.83 M | 131 ms | 7.66 | 131 ms | 100 (5 × 20) |
| `ublockith_em_d3_256f` | verify | 429.75 M | 159 ms | 6.27 | 160 ms | 100 (5 × 20) |
| `ublockith_em_d3_256s` | keygen | 14.1 k | 5.23 µs | 1.91e+05 | 5.23 µs | 100000 (5 × 20000) |
| `ublockith_em_d3_256s` | sign | 825.61 M | 306 ms | 3.26 | 304 ms | 100 (5 × 20) |
| `ublockith_em_d3_256s` | verify | 883.88 M | 328 ms | 3.05 | 327 ms | 100 (5 × 20) |
| `vistrutith_d3_512f` | keygen | 14.5 k | 5.37 µs | 1.86e+05 | 5.37 µs | 100000 (5 × 20000) |
| `vistrutith_d3_512f` | sign | 6.02 G | 2.23 s | 0.448 | 2.23 s | 100 (5 × 20) |
| `vistrutith_d3_512f` | verify | 4.95 G | 1.84 s | 0.544 | 1.84 s | 100 (5 × 20) |
| `vistrutith_d3_512s` | keygen | 14.5 k | 5.37 µs | 1.86e+05 | 5.37 µs | 100000 (5 × 20000) |
| `vistrutith_d3_512s` | sign | 10.40 G | 3.86 s | 0.259 | 3.86 s | 95 (5 × 19) |
| `vistrutith_d3_512s` | verify | 9.37 G | 3.48 s | 0.287 | 3.46 s | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `sm4th_d3_128f_loose` | keygen | 130324 | 1488 KiB | 1552 KiB |
| `sm4th_d3_128f_loose` | sign | 130324 | 1488 KiB | 4928 KiB |
| `sm4th_d3_128f_loose` | verify | 130324 | 3800 KiB | 4768 KiB |
| `sm4th_d3_128f_tight` | keygen | 173052 | 1516 KiB | 1580 KiB |
| `sm4th_d3_128f_tight` | sign | 173052 | 1516 KiB | 6268 KiB |
| `sm4th_d3_128f_tight` | verify | 173052 | 4380 KiB | 5608 KiB |
| `sm4th_d3_128s_loose` | keygen | 130324 | 1484 KiB | 1548 KiB |
| `sm4th_d3_128s_loose` | sign | 130324 | 1488 KiB | 12456 KiB |
| `sm4th_d3_128s_loose` | verify | 130324 | 4156 KiB | 12424 KiB |
| `sm4th_d3_128s_tight` | keygen | 173052 | 1512 KiB | 1576 KiB |
| `sm4th_d3_128s_tight` | sign | 173052 | 1516 KiB | 17648 KiB |
| `sm4th_d3_128s_tight` | verify | 173052 | 4836 KiB | 17304 KiB |
| `sm4th_em_d2_128f_loose` | keygen | 91700 | 1480 KiB | 1544 KiB |
| `sm4th_em_d2_128f_loose` | sign | 91700 | 1480 KiB | 3816 KiB |
| `sm4th_em_d2_128f_loose` | verify | 91700 | 3888 KiB | 4160 KiB |
| `sm4th_em_d2_128f_tight` | keygen | 121084 | 1512 KiB | 1576 KiB |
| `sm4th_em_d2_128f_tight` | sign | 121084 | 1512 KiB | 4656 KiB |
| `sm4th_em_d2_128f_tight` | verify | 121084 | 4128 KiB | 4688 KiB |
| `sm4th_em_d2_128s_loose` | keygen | 91700 | 1476 KiB | 1540 KiB |
| `sm4th_em_d2_128s_loose` | sign | 91700 | 1476 KiB | 9716 KiB |
| `sm4th_em_d2_128s_loose` | verify | 91700 | 3880 KiB | 9896 KiB |
| `sm4th_em_d2_128s_tight` | keygen | 121084 | 1508 KiB | 1572 KiB |
| `sm4th_em_d2_128s_tight` | sign | 121084 | 1508 KiB | 18004 KiB |
| `sm4th_em_d2_128s_tight` | verify | 121084 | 5508 KiB | 13832 KiB |
| `ublockith_d3_256f` | keygen | 126944 | 1532 KiB | 1612 KiB |
| `ublockith_d3_256f` | sign | 126944 | 1548 KiB | 10900 KiB |
| `ublockith_d3_256f` | verify | 126944 | 5064 KiB | 14460 KiB |
| `ublockith_d3_256s` | keygen | 118184 | 1512 KiB | 1592 KiB |
| `ublockith_d3_256s` | sign | 118184 | 1532 KiB | 18264 KiB |
| `ublockith_d3_256s` | verify | 118184 | 8488 KiB | 19636 KiB |
| `ublockith_em_d3_256f` | keygen | 121672 | 1520 KiB | 1600 KiB |
| `ublockith_em_d3_256f` | sign | 121672 | 1532 KiB | 5268 KiB |
| `ublockith_em_d3_256f` | verify | 121672 | 5096 KiB | 5712 KiB |
| `ublockith_em_d3_256s` | keygen | 121672 | 1516 KiB | 1596 KiB |
| `ublockith_em_d3_256s` | sign | 121672 | 1532 KiB | 18212 KiB |
| `ublockith_em_d3_256s` | verify | 121672 | 7952 KiB | 18256 KiB |
| `vistrutith_d3_512f` | keygen | 144896 | 1624 KiB | 1696 KiB |
| `vistrutith_d3_512f` | sign | 144896 | 1628 KiB | 10884 KiB |
| `vistrutith_d3_512f` | verify | 144896 | 6212 KiB | 10996 KiB |
| `vistrutith_d3_512s` | keygen | 144896 | 1604 KiB | 1676 KiB |
| `vistrutith_d3_512s` | sign | 144896 | 1612 KiB | 57348 KiB |
| `vistrutith_d3_512s` | verify | 144896 | 13476 KiB | 65088 KiB |

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
| `sm4th_d3_128f_loose` | sign | 3.6% | 0.0% | drng 1, pseudoXOF 2, sm3hash 434 |
| `sm4th_d3_128f_loose` | verify | 3.7% | 0.0% | pseudoXOF 2, sm3hash 20 |
| `sm4th_d3_128f_tight` | keygen | 0.0% | 88% | drng 2.34 |
| `sm4th_d3_128f_tight` | sign | 4.7% | 0.0% | drng 1, pseudoXOF 2, pseudohash 23, sm3hash 497 |
| `sm4th_d3_128f_tight` | verify | 7.1% | 0.0% | pseudoXOF 2, pseudohash 22, sm3hash 3 |
| `sm4th_d3_128s_loose` | keygen | 0.0% | 88% | drng 2.34 |
| `sm4th_d3_128s_loose` | sign | 12% | 0.0% | drng 1, pseudoXOF 2, sm3hash 6.82e+03 |
| `sm4th_d3_128s_loose` | verify | 6.3% | 0.0% | pseudoXOF 2, sm3hash 15 |
| `sm4th_d3_128s_tight` | keygen | 0.0% | 88% | drng 2.34 |
| `sm4th_d3_128s_tight` | sign | 20% | 0.0% | drng 1, pseudoXOF 2, pseudohash 16, sm3hash 3.04e+03 |
| `sm4th_d3_128s_tight` | verify | 24% | 0.0% | pseudoXOF 2, pseudohash 15, sm3hash 3 |
| `sm4th_em_d2_128f_loose` | keygen | 0.0% | 88% | drng 2.34 |
| `sm4th_em_d2_128f_loose` | sign | 13% | 0.0% | drng 1, pseudoXOF 322, sm3hash 19 |
| `sm4th_em_d2_128f_loose` | verify | 10% | 0.0% | pseudoXOF 4, sm3hash 18 |
| `sm4th_em_d2_128f_tight` | keygen | 0.0% | 88% | drng 2.34 |
| `sm4th_em_d2_128f_tight` | sign | 34% | 0.0% | drng 1, pseudoXOF 298, pseudohash 23, sm3hash 2 |
| `sm4th_em_d2_128f_tight` | verify | 33% | 0.0% | pseudoXOF 3, pseudohash 22, sm3hash 2 |
| `sm4th_em_d2_128s_loose` | keygen | 0.0% | 88% | drng 2.34 |
| `sm4th_em_d2_128s_loose` | sign | 15% | 0.0% | drng 1, pseudoXOF 3.58e+03, sm3hash 14 |
| `sm4th_em_d2_128s_loose` | verify | 10% | 0.0% | pseudoXOF 4, sm3hash 13 |
| `sm4th_em_d2_128s_tight` | keygen | 0.0% | 88% | drng 2.34 |
| `sm4th_em_d2_128s_tight` | sign | 45% | 0.0% | drng 1, pseudoXOF 3.13e+03, pseudohash 16, sm3hash 2 |
| `sm4th_em_d2_128s_tight` | verify | 42% | 0.0% | pseudoXOF 3, pseudohash 15, sm3hash 2 |
| `ublockith_d3_256f` | keygen | 0.0% | 69% | drng 2.34 |
| `ublockith_d3_256f` | sign | 4.8% | 0.0% | drng 1, pseudoXOF 2, pseudohash 35, sm3hash 144 |
| `ublockith_d3_256f` | verify | 3.9% | 0.0% | pseudoXOF 2, pseudohash 34, sm3hash 2 |
| `ublockith_d3_256s` | keygen | 0.0% | 69% | drng 2.34 |
| `ublockith_d3_256s` | sign | 16% | 0.0% | drng 1, pseudoXOF 2, pseudohash 25, sm3hash 45 |
| `ublockith_d3_256s` | verify | 14% | 0.0% | pseudoXOF 2, pseudohash 24, sm3hash 2 |
| `ublockith_em_d3_256f` | keygen | 0.0% | 69% | drng 2.34 |
| `ublockith_em_d3_256f` | sign | 6.4% | 0.0% | drng 1, pseudoXOF 2, pseudohash 35, sm3hash 369 |
| `ublockith_em_d3_256f` | verify | 4.9% | 0.0% | pseudoXOF 2, pseudohash 34, sm3hash 2 |
| `ublockith_em_d3_256s` | keygen | 0.0% | 69% | drng 2.34 |
| `ublockith_em_d3_256s` | sign | 22% | 0.0% | drng 1, pseudoXOF 2, pseudohash 25, sm3hash 4.91e+03 |
| `ublockith_em_d3_256s` | verify | 18% | 0.0% | pseudoXOF 2, pseudohash 24, sm3hash 2 |
| `vistrutith_d3_512f` | keygen | 0.0% | 89% | drng 2.34 |
| `vistrutith_d3_512f` | sign | 1.7% | 0.0% | drng 1, pseudoXOF 2, pseudohash 123, sm3hash 1 |
| `vistrutith_d3_512f` | verify | 2.0% | 0.0% | pseudoXOF 2, pseudohash 67, sm3hash 1 |
| `vistrutith_d3_512s` | keygen | 0.0% | 89% | drng 2.34 |
| `vistrutith_d3_512s` | sign | 6.9% | 0.0% | drng 1, pseudoXOF 2, pseudohash 79, sm3hash 1 |
| `vistrutith_d3_512s` | verify | 7.7% | 0.0% | pseudoXOF 2, pseudohash 47, sm3hash 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `sm4th_d3_128f_loose` | KAT log (sha256 `0a6dd03a6796f619…`) | `kat/sign-05/sm4th_d3_128f_loose.log` |
| `sm4th_d3_128f_loose` | timing keygen | `records/sign-05/sm4th_d3_128f_loose__keygen.json` |
| `sm4th_d3_128f_loose` | timing sign | `records/sign-05/sm4th_d3_128f_loose__sign.json` |
| `sm4th_d3_128f_loose` | timing verify | `records/sign-05/sm4th_d3_128f_loose__verify.json` |
| `sm4th_d3_128f_loose` | hash profile keygen | `profile/sign-05/sm4th_d3_128f_loose__keygen.json` |
| `sm4th_d3_128f_loose` | hash profile sign | `profile/sign-05/sm4th_d3_128f_loose__sign.json` |
| `sm4th_d3_128f_loose` | hash profile verify | `profile/sign-05/sm4th_d3_128f_loose__verify.json` |
| `sm4th_d3_128f_tight` | KAT log (sha256 `23a9d5c681a21b49…`) | `kat/sign-05/sm4th_d3_128f_tight.log` |
| `sm4th_d3_128f_tight` | timing keygen | `records/sign-05/sm4th_d3_128f_tight__keygen.json` |
| `sm4th_d3_128f_tight` | timing sign | `records/sign-05/sm4th_d3_128f_tight__sign.json` |
| `sm4th_d3_128f_tight` | timing verify | `records/sign-05/sm4th_d3_128f_tight__verify.json` |
| `sm4th_d3_128f_tight` | hash profile keygen | `profile/sign-05/sm4th_d3_128f_tight__keygen.json` |
| `sm4th_d3_128f_tight` | hash profile sign | `profile/sign-05/sm4th_d3_128f_tight__sign.json` |
| `sm4th_d3_128f_tight` | hash profile verify | `profile/sign-05/sm4th_d3_128f_tight__verify.json` |
| `sm4th_d3_128s_loose` | KAT log (sha256 `b944341360b834cf…`) | `kat/sign-05/sm4th_d3_128s_loose.log` |
| `sm4th_d3_128s_loose` | timing keygen | `records/sign-05/sm4th_d3_128s_loose__keygen.json` |
| `sm4th_d3_128s_loose` | timing sign | `records/sign-05/sm4th_d3_128s_loose__sign.json` |
| `sm4th_d3_128s_loose` | timing verify | `records/sign-05/sm4th_d3_128s_loose__verify.json` |
| `sm4th_d3_128s_loose` | hash profile keygen | `profile/sign-05/sm4th_d3_128s_loose__keygen.json` |
| `sm4th_d3_128s_loose` | hash profile sign | `profile/sign-05/sm4th_d3_128s_loose__sign.json` |
| `sm4th_d3_128s_loose` | hash profile verify | `profile/sign-05/sm4th_d3_128s_loose__verify.json` |
| `sm4th_d3_128s_tight` | KAT log (sha256 `76775c7acb73bd28…`) | `kat/sign-05/sm4th_d3_128s_tight.log` |
| `sm4th_d3_128s_tight` | timing keygen | `records/sign-05/sm4th_d3_128s_tight__keygen.json` |
| `sm4th_d3_128s_tight` | timing sign | `records/sign-05/sm4th_d3_128s_tight__sign.json` |
| `sm4th_d3_128s_tight` | timing verify | `records/sign-05/sm4th_d3_128s_tight__verify.json` |
| `sm4th_d3_128s_tight` | hash profile keygen | `profile/sign-05/sm4th_d3_128s_tight__keygen.json` |
| `sm4th_d3_128s_tight` | hash profile sign | `profile/sign-05/sm4th_d3_128s_tight__sign.json` |
| `sm4th_d3_128s_tight` | hash profile verify | `profile/sign-05/sm4th_d3_128s_tight__verify.json` |
| `sm4th_em_d2_128f_loose` | KAT log (sha256 `5a000b036323042f…`) | `kat/sign-05/sm4th_em_d2_128f_loose.log` |
| `sm4th_em_d2_128f_loose` | timing keygen | `records/sign-05/sm4th_em_d2_128f_loose__keygen.json` |
| `sm4th_em_d2_128f_loose` | timing sign | `records/sign-05/sm4th_em_d2_128f_loose__sign.json` |
| `sm4th_em_d2_128f_loose` | timing verify | `records/sign-05/sm4th_em_d2_128f_loose__verify.json` |
| `sm4th_em_d2_128f_loose` | hash profile keygen | `profile/sign-05/sm4th_em_d2_128f_loose__keygen.json` |
| `sm4th_em_d2_128f_loose` | hash profile sign | `profile/sign-05/sm4th_em_d2_128f_loose__sign.json` |
| `sm4th_em_d2_128f_loose` | hash profile verify | `profile/sign-05/sm4th_em_d2_128f_loose__verify.json` |
| `sm4th_em_d2_128f_tight` | KAT log (sha256 `660ef19b7aa92d7c…`) | `kat/sign-05/sm4th_em_d2_128f_tight.log` |
| `sm4th_em_d2_128f_tight` | timing keygen | `records/sign-05/sm4th_em_d2_128f_tight__keygen.json` |
| `sm4th_em_d2_128f_tight` | timing sign | `records/sign-05/sm4th_em_d2_128f_tight__sign.json` |
| `sm4th_em_d2_128f_tight` | timing verify | `records/sign-05/sm4th_em_d2_128f_tight__verify.json` |
| `sm4th_em_d2_128f_tight` | hash profile keygen | `profile/sign-05/sm4th_em_d2_128f_tight__keygen.json` |
| `sm4th_em_d2_128f_tight` | hash profile sign | `profile/sign-05/sm4th_em_d2_128f_tight__sign.json` |
| `sm4th_em_d2_128f_tight` | hash profile verify | `profile/sign-05/sm4th_em_d2_128f_tight__verify.json` |
| `sm4th_em_d2_128s_loose` | KAT log (sha256 `de575cde44ba3bcb…`) | `kat/sign-05/sm4th_em_d2_128s_loose.log` |
| `sm4th_em_d2_128s_loose` | timing keygen | `records/sign-05/sm4th_em_d2_128s_loose__keygen.json` |
| `sm4th_em_d2_128s_loose` | timing sign | `records/sign-05/sm4th_em_d2_128s_loose__sign.json` |
| `sm4th_em_d2_128s_loose` | timing verify | `records/sign-05/sm4th_em_d2_128s_loose__verify.json` |
| `sm4th_em_d2_128s_loose` | hash profile keygen | `profile/sign-05/sm4th_em_d2_128s_loose__keygen.json` |
| `sm4th_em_d2_128s_loose` | hash profile sign | `profile/sign-05/sm4th_em_d2_128s_loose__sign.json` |
| `sm4th_em_d2_128s_loose` | hash profile verify | `profile/sign-05/sm4th_em_d2_128s_loose__verify.json` |
| `sm4th_em_d2_128s_tight` | KAT log (sha256 `2309ee80841339bc…`) | `kat/sign-05/sm4th_em_d2_128s_tight.log` |
| `sm4th_em_d2_128s_tight` | timing keygen | `records/sign-05/sm4th_em_d2_128s_tight__keygen.json` |
| `sm4th_em_d2_128s_tight` | timing sign | `records/sign-05/sm4th_em_d2_128s_tight__sign.json` |
| `sm4th_em_d2_128s_tight` | timing verify | `records/sign-05/sm4th_em_d2_128s_tight__verify.json` |
| `sm4th_em_d2_128s_tight` | hash profile keygen | `profile/sign-05/sm4th_em_d2_128s_tight__keygen.json` |
| `sm4th_em_d2_128s_tight` | hash profile sign | `profile/sign-05/sm4th_em_d2_128s_tight__sign.json` |
| `sm4th_em_d2_128s_tight` | hash profile verify | `profile/sign-05/sm4th_em_d2_128s_tight__verify.json` |
| `ublockith_d3_256f` | KAT log (sha256 `b015d7619af5ecd1…`) | `kat/sign-05/ublockith_d3_256f.log` |
| `ublockith_d3_256f` | timing keygen | `records/sign-05/ublockith_d3_256f__keygen.json` |
| `ublockith_d3_256f` | timing sign | `records/sign-05/ublockith_d3_256f__sign.json` |
| `ublockith_d3_256f` | timing verify | `records/sign-05/ublockith_d3_256f__verify.json` |
| `ublockith_d3_256f` | hash profile keygen | `profile/sign-05/ublockith_d3_256f__keygen.json` |
| `ublockith_d3_256f` | hash profile sign | `profile/sign-05/ublockith_d3_256f__sign.json` |
| `ublockith_d3_256f` | hash profile verify | `profile/sign-05/ublockith_d3_256f__verify.json` |
| `ublockith_d3_256s` | KAT log (sha256 `713d1fb479949e44…`) | `kat/sign-05/ublockith_d3_256s.log` |
| `ublockith_d3_256s` | timing keygen | `records/sign-05/ublockith_d3_256s__keygen.json` |
| `ublockith_d3_256s` | timing sign | `records/sign-05/ublockith_d3_256s__sign.json` |
| `ublockith_d3_256s` | timing verify | `records/sign-05/ublockith_d3_256s__verify.json` |
| `ublockith_d3_256s` | hash profile keygen | `profile/sign-05/ublockith_d3_256s__keygen.json` |
| `ublockith_d3_256s` | hash profile sign | `profile/sign-05/ublockith_d3_256s__sign.json` |
| `ublockith_d3_256s` | hash profile verify | `profile/sign-05/ublockith_d3_256s__verify.json` |
| `ublockith_em_d3_256f` | KAT log (sha256 `e7a29a1923c771e9…`) | `kat/sign-05/ublockith_em_d3_256f.log` |
| `ublockith_em_d3_256f` | timing keygen | `records/sign-05/ublockith_em_d3_256f__keygen.json` |
| `ublockith_em_d3_256f` | timing sign | `records/sign-05/ublockith_em_d3_256f__sign.json` |
| `ublockith_em_d3_256f` | timing verify | `records/sign-05/ublockith_em_d3_256f__verify.json` |
| `ublockith_em_d3_256f` | hash profile keygen | `profile/sign-05/ublockith_em_d3_256f__keygen.json` |
| `ublockith_em_d3_256f` | hash profile sign | `profile/sign-05/ublockith_em_d3_256f__sign.json` |
| `ublockith_em_d3_256f` | hash profile verify | `profile/sign-05/ublockith_em_d3_256f__verify.json` |
| `ublockith_em_d3_256s` | KAT log (sha256 `d6d975dc024f571c…`) | `kat/sign-05/ublockith_em_d3_256s.log` |
| `ublockith_em_d3_256s` | timing keygen | `records/sign-05/ublockith_em_d3_256s__keygen.json` |
| `ublockith_em_d3_256s` | timing sign | `records/sign-05/ublockith_em_d3_256s__sign.json` |
| `ublockith_em_d3_256s` | timing verify | `records/sign-05/ublockith_em_d3_256s__verify.json` |
| `ublockith_em_d3_256s` | hash profile keygen | `profile/sign-05/ublockith_em_d3_256s__keygen.json` |
| `ublockith_em_d3_256s` | hash profile sign | `profile/sign-05/ublockith_em_d3_256s__sign.json` |
| `ublockith_em_d3_256s` | hash profile verify | `profile/sign-05/ublockith_em_d3_256s__verify.json` |
| `vistrutith_d3_512f` | KAT log (sha256 `de28b2c9a10e3522…`) | `kat/sign-05/vistrutith_d3_512f.log` |
| `vistrutith_d3_512f` | timing keygen | `records/sign-05/vistrutith_d3_512f__keygen.json` |
| `vistrutith_d3_512f` | timing sign | `records/sign-05/vistrutith_d3_512f__sign.json` |
| `vistrutith_d3_512f` | timing verify | `records/sign-05/vistrutith_d3_512f__verify.json` |
| `vistrutith_d3_512f` | hash profile keygen | `profile/sign-05/vistrutith_d3_512f__keygen.json` |
| `vistrutith_d3_512f` | hash profile sign | `profile/sign-05/vistrutith_d3_512f__sign.json` |
| `vistrutith_d3_512f` | hash profile verify | `profile/sign-05/vistrutith_d3_512f__verify.json` |
| `vistrutith_d3_512s` | KAT log (sha256 `3862844b15c8123f…`) | `kat/sign-05/vistrutith_d3_512s.log` |
| `vistrutith_d3_512s` | timing keygen | `records/sign-05/vistrutith_d3_512s__keygen.json` |
| `vistrutith_d3_512s` | timing sign | `records/sign-05/vistrutith_d3_512s__sign.json` |
| `vistrutith_d3_512s` | timing verify | `records/sign-05/vistrutith_d3_512s__verify.json` |
| `vistrutith_d3_512s` | hash profile keygen | `profile/sign-05/vistrutith_d3_512s__keygen.json` |
| `vistrutith_d3_512s` | hash profile sign | `profile/sign-05/vistrutith_d3_512s__sign.json` |
| `vistrutith_d3_512s` | hash profile verify | `profile/sign-05/vistrutith_d3_512s__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

