# kem-19 Lore — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-19` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560853788184576.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: Lore
- Implementation versions measured: reference
- Parameter sets: `Lore-SHAKE-L1`, `Lore-SHAKE-L2`, `Lore-SHAKE-L3`, `Lore-SHAKE-L4`, `Lore-SM3-L1`, `Lore-SM3-L2`, `Lore-SM3-L3`, `Lore-SM3-L4`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-19/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Lore-SHAKE-L1` | guide | PASS |
| `Lore-SHAKE-L2` | guide | PASS |
| `Lore-SHAKE-L3` | guide | PASS |
| `Lore-SHAKE-L4` | guide | PASS |
| `Lore-SM3-L1` | guide | PASS |
| `Lore-SM3-L2` | guide | PASS |
| `Lore-SM3-L3` | guide | PASS |
| `Lore-SM3-L4` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Lore-SHAKE-L1` | keygen | 173.0 k | 82.7 µs | 1.21e+04 | 82.7 µs | 52885 (5 × 10577) |
| `Lore-SHAKE-L1` | enc | 333.7 k | 159 µs | 6.27e+03 | 159 µs | 30425 (5 × 6085) |
| `Lore-SHAKE-L1` | dec | 392.1 k | 187 µs | 5.34e+03 | 187 µs | 25495 (5 × 5099) |
| `Lore-SHAKE-L2` | keygen | 717.4 k | 343 µs | 2.92e+03 | 343 µs | 13620 (5 × 2724) |
| `Lore-SHAKE-L2` | enc | 1.07 M | 510 µs | 1.96e+03 | 510 µs | 8910 (5 × 1782) |
| `Lore-SHAKE-L2` | dec | 1.35 M | 644 µs | 1.55e+03 | 644 µs | 7740 (5 × 1548) |
| `Lore-SHAKE-L3` | keygen | 1.58 M | 756 µs | 1.32e+03 | 761 µs | 6325 (5 × 1265) |
| `Lore-SHAKE-L3` | enc | 2.06 M | 986 µs | 1.01e+03 | 982 µs | 5195 (5 × 1039) |
| `Lore-SHAKE-L3` | dec | 2.35 M | 1.12 ms | 892 | 1.12 ms | 4400 (5 × 880) |
| `Lore-SHAKE-L4` | keygen | 2.71 M | 1.29 ms | 774 | 1.29 ms | 3745 (5 × 749) |
| `Lore-SHAKE-L4` | enc | 3.58 M | 1.71 ms | 585 | 1.71 ms | 2815 (5 × 563) |
| `Lore-SHAKE-L4` | dec | 4.28 M | 2.05 ms | 489 | 2.05 ms | 2415 (5 × 483) |
| `Lore-SM3-L1` | keygen | 240.3 k | 115 µs | 8.71e+03 | 115 µs | 37895 (5 × 7579) |
| `Lore-SM3-L1` | enc | 363.5 k | 174 µs | 5.76e+03 | 173 µs | 27255 (5 × 5451) |
| `Lore-SM3-L1` | dec | 416.7 k | 199 µs | 5.02e+03 | 199 µs | 24205 (5 × 4841) |
| `Lore-SM3-L2` | keygen | 1.06 M | 506 µs | 1.98e+03 | 506 µs | 9440 (5 × 1888) |
| `Lore-SM3-L2` | enc | 1.35 M | 646 µs | 1.55e+03 | 646 µs | 7660 (5 × 1532) |
| `Lore-SM3-L2` | dec | 1.62 M | 775 µs | 1.29e+03 | 775 µs | 6345 (5 × 1269) |
| `Lore-SM3-L3` | keygen | 2.38 M | 1.14 ms | 880 | 1.14 ms | 4370 (5 × 874) |
| `Lore-SM3-L3` | enc | 2.78 M | 1.33 ms | 753 | 1.33 ms | 3765 (5 × 753) |
| `Lore-SM3-L3` | dec | 3.05 M | 1.46 ms | 686 | 1.46 ms | 3300 (5 × 660) |
| `Lore-SM3-L4` | keygen | 5.19 M | 2.48 ms | 403 | 2.48 ms | 1980 (5 × 396) |
| `Lore-SM3-L4` | enc | 5.95 M | 2.84 ms | 352 | 2.84 ms | 1735 (5 × 347) |
| `Lore-SM3-L4` | dec | 6.65 M | 3.17 ms | 315 | 3.17 ms | 1555 (5 × 311) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Lore-SHAKE-L1` | keygen | 49869 | 1728 KiB | 1800 KiB |
| `Lore-SHAKE-L1` | enc | 49869 | 1720 KiB | 1824 KiB |
| `Lore-SHAKE-L1` | dec | 49869 | 1724 KiB | 1788 KiB |
| `Lore-SHAKE-L2` | keygen | 52245 | 1704 KiB | 1796 KiB |
| `Lore-SHAKE-L2` | enc | 52245 | 1860 KiB | 1932 KiB |
| `Lore-SHAKE-L2` | dec | 52245 | 1864 KiB | 1928 KiB |
| `Lore-SHAKE-L3` | keygen | 54293 | 1708 KiB | 1856 KiB |
| `Lore-SHAKE-L3` | enc | 54293 | 1864 KiB | 1928 KiB |
| `Lore-SHAKE-L3` | dec | 54293 | 1836 KiB | 1960 KiB |
| `Lore-SHAKE-L4` | keygen | 54357 | 1748 KiB | 1884 KiB |
| `Lore-SHAKE-L4` | enc | 54357 | 1920 KiB | 1984 KiB |
| `Lore-SHAKE-L4` | dec | 54357 | 1924 KiB | 2020 KiB |
| `Lore-SM3-L1` | keygen | 46541 | 1708 KiB | 1784 KiB |
| `Lore-SM3-L1` | enc | 46541 | 1724 KiB | 1812 KiB |
| `Lore-SM3-L1` | dec | 46541 | 1744 KiB | 1808 KiB |
| `Lore-SM3-L2` | keygen | 48885 | 1732 KiB | 1832 KiB |
| `Lore-SM3-L2` | enc | 48885 | 1848 KiB | 1916 KiB |
| `Lore-SM3-L2` | dec | 48885 | 1868 KiB | 1960 KiB |
| `Lore-SM3-L3` | keygen | 50901 | 1732 KiB | 1876 KiB |
| `Lore-SM3-L3` | enc | 50901 | 1880 KiB | 1944 KiB |
| `Lore-SM3-L3` | dec | 50901 | 1872 KiB | 1936 KiB |
| `Lore-SM3-L4` | keygen | 50965 | 1732 KiB | 1888 KiB |
| `Lore-SM3-L4` | enc | 50965 | 1904 KiB | 2024 KiB |
| `Lore-SM3-L4` | dec | 50965 | 1912 KiB | 1980 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `Lore-SHAKE-L1` | 610 | 2108 | 706 | 32 |
| `Lore-SHAKE-L2` | 1186 | 4518 | 1282 | 32 |
| `Lore-SHAKE-L3` | 1954 | 7736 | 2114 | 32 |
| `Lore-SHAKE-L4` | 2914 | 11432 | 3170 | 32 |
| `Lore-SM3-L1` | 610 | 2108 | 706 | 32 |
| `Lore-SM3-L2` | 1186 | 4518 | 1282 | 32 |
| `Lore-SM3-L3` | 1954 | 7736 | 2114 | 32 |
| `Lore-SM3-L4` | 2914 | 11432 | 3170 | 32 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **instance-dependent** — Lore-SM3: ICCS-only; Lore-SHAKE: own FIPS 202 (bypass); AVX2/NEON SIMD SM3 in auxfunc

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Lore-SHAKE-L1` | keygen | 0.0% | 3.6% | drng 1 |
| `Lore-SHAKE-L1` | enc | 0.0% | 1.4% | drng 1 |
| `Lore-SHAKE-L1` | dec | 0.0% | 0.0% | – |
| `Lore-SHAKE-L2` | keygen | 0.0% | 0.9% | drng 1 |
| `Lore-SHAKE-L2` | enc | 0.0% | 0.4% | drng 1 |
| `Lore-SHAKE-L2` | dec | 0.0% | 0.0% | – |
| `Lore-SHAKE-L3` | keygen | 0.0% | 0.4% | drng 1 |
| `Lore-SHAKE-L3` | enc | 0.0% | 0.3% | drng 1 |
| `Lore-SHAKE-L3` | dec | 0.0% | 0.0% | – |
| `Lore-SHAKE-L4` | keygen | 0.0% | 0.2% | drng 1 |
| `Lore-SHAKE-L4` | enc | 0.0% | 0.2% | drng 1 |
| `Lore-SHAKE-L4` | dec | 0.0% | 0.0% | – |
| `Lore-SM3-L1` | keygen | 73% | 2.6% | drng 1, pseudoXOF 5, sm3hash 1 |
| `Lore-SM3-L1` | enc | 68% | 1.3% | drng 1, pseudoXOF 7, sm3hash 3 |
| `Lore-SM3-L1` | dec | 60% | 0.0% | pseudoXOF 8, sm3hash 3 |
| `Lore-SM3-L2` | keygen | 60% | 0.6% | drng 1, pseudoXOF 14, sm3hash 1 |
| `Lore-SM3-L2` | enc | 53% | 0.4% | drng 1, pseudoXOF 16, sm3hash 3 |
| `Lore-SM3-L2` | dec | 44% | 0.0% | pseudoXOF 17, sm3hash 3 |
| `Lore-SM3-L3` | keygen | 58% | 0.3% | drng 1, pseudoXOF 29, sm3hash 1 |
| `Lore-SM3-L3` | enc | 53% | 0.2% | drng 1, pseudoXOF 31, sm3hash 3 |
| `Lore-SM3-L3` | dec | 48% | 0.0% | pseudoXOF 32, sm3hash 3 |
| `Lore-SM3-L4` | keygen | 64% | 0.1% | drng 1, pseudoXOF 56, sm3hash 1 |
| `Lore-SM3-L4` | enc | 59% | 0.1% | drng 1, pseudoXOF 58, sm3hash 3 |
| `Lore-SM3-L4` | dec | 52% | 0.0% | pseudoXOF 59, sm3hash 3 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Lore-SHAKE-L1` | KAT log (sha256 `1afaf20ea8b4ac0d…`) | `kat/kem-19/Lore-SHAKE-L1.log` |
| `Lore-SHAKE-L1` | timing dec | `records/kem-19/Lore-SHAKE-L1__dec.json` |
| `Lore-SHAKE-L1` | timing enc | `records/kem-19/Lore-SHAKE-L1__enc.json` |
| `Lore-SHAKE-L1` | timing keygen | `records/kem-19/Lore-SHAKE-L1__keygen.json` |
| `Lore-SHAKE-L1` | hash profile dec | `profile/kem-19/Lore-SHAKE-L1__dec.json` |
| `Lore-SHAKE-L1` | hash profile enc | `profile/kem-19/Lore-SHAKE-L1__enc.json` |
| `Lore-SHAKE-L1` | hash profile keygen | `profile/kem-19/Lore-SHAKE-L1__keygen.json` |
| `Lore-SHAKE-L2` | KAT log (sha256 `3eaaaf36c0d363fa…`) | `kat/kem-19/Lore-SHAKE-L2.log` |
| `Lore-SHAKE-L2` | timing dec | `records/kem-19/Lore-SHAKE-L2__dec.json` |
| `Lore-SHAKE-L2` | timing enc | `records/kem-19/Lore-SHAKE-L2__enc.json` |
| `Lore-SHAKE-L2` | timing keygen | `records/kem-19/Lore-SHAKE-L2__keygen.json` |
| `Lore-SHAKE-L2` | hash profile dec | `profile/kem-19/Lore-SHAKE-L2__dec.json` |
| `Lore-SHAKE-L2` | hash profile enc | `profile/kem-19/Lore-SHAKE-L2__enc.json` |
| `Lore-SHAKE-L2` | hash profile keygen | `profile/kem-19/Lore-SHAKE-L2__keygen.json` |
| `Lore-SHAKE-L3` | KAT log (sha256 `c488a2c03d92ad20…`) | `kat/kem-19/Lore-SHAKE-L3.log` |
| `Lore-SHAKE-L3` | timing dec | `records/kem-19/Lore-SHAKE-L3__dec.json` |
| `Lore-SHAKE-L3` | timing enc | `records/kem-19/Lore-SHAKE-L3__enc.json` |
| `Lore-SHAKE-L3` | timing keygen | `records/kem-19/Lore-SHAKE-L3__keygen.json` |
| `Lore-SHAKE-L3` | hash profile dec | `profile/kem-19/Lore-SHAKE-L3__dec.json` |
| `Lore-SHAKE-L3` | hash profile enc | `profile/kem-19/Lore-SHAKE-L3__enc.json` |
| `Lore-SHAKE-L3` | hash profile keygen | `profile/kem-19/Lore-SHAKE-L3__keygen.json` |
| `Lore-SHAKE-L4` | KAT log (sha256 `8a84de8aa4f99485…`) | `kat/kem-19/Lore-SHAKE-L4.log` |
| `Lore-SHAKE-L4` | timing dec | `records/kem-19/Lore-SHAKE-L4__dec.json` |
| `Lore-SHAKE-L4` | timing enc | `records/kem-19/Lore-SHAKE-L4__enc.json` |
| `Lore-SHAKE-L4` | timing keygen | `records/kem-19/Lore-SHAKE-L4__keygen.json` |
| `Lore-SHAKE-L4` | hash profile dec | `profile/kem-19/Lore-SHAKE-L4__dec.json` |
| `Lore-SHAKE-L4` | hash profile enc | `profile/kem-19/Lore-SHAKE-L4__enc.json` |
| `Lore-SHAKE-L4` | hash profile keygen | `profile/kem-19/Lore-SHAKE-L4__keygen.json` |
| `Lore-SM3-L1` | KAT log (sha256 `71de39651150d414…`) | `kat/kem-19/Lore-SM3-L1.log` |
| `Lore-SM3-L1` | timing dec | `records/kem-19/Lore-SM3-L1__dec.json` |
| `Lore-SM3-L1` | timing enc | `records/kem-19/Lore-SM3-L1__enc.json` |
| `Lore-SM3-L1` | timing keygen | `records/kem-19/Lore-SM3-L1__keygen.json` |
| `Lore-SM3-L1` | hash profile dec | `profile/kem-19/Lore-SM3-L1__dec.json` |
| `Lore-SM3-L1` | hash profile enc | `profile/kem-19/Lore-SM3-L1__enc.json` |
| `Lore-SM3-L1` | hash profile keygen | `profile/kem-19/Lore-SM3-L1__keygen.json` |
| `Lore-SM3-L2` | KAT log (sha256 `0682f77c99ee068e…`) | `kat/kem-19/Lore-SM3-L2.log` |
| `Lore-SM3-L2` | timing dec | `records/kem-19/Lore-SM3-L2__dec.json` |
| `Lore-SM3-L2` | timing enc | `records/kem-19/Lore-SM3-L2__enc.json` |
| `Lore-SM3-L2` | timing keygen | `records/kem-19/Lore-SM3-L2__keygen.json` |
| `Lore-SM3-L2` | hash profile dec | `profile/kem-19/Lore-SM3-L2__dec.json` |
| `Lore-SM3-L2` | hash profile enc | `profile/kem-19/Lore-SM3-L2__enc.json` |
| `Lore-SM3-L2` | hash profile keygen | `profile/kem-19/Lore-SM3-L2__keygen.json` |
| `Lore-SM3-L3` | KAT log (sha256 `da373ac026a9efb4…`) | `kat/kem-19/Lore-SM3-L3.log` |
| `Lore-SM3-L3` | timing dec | `records/kem-19/Lore-SM3-L3__dec.json` |
| `Lore-SM3-L3` | timing enc | `records/kem-19/Lore-SM3-L3__enc.json` |
| `Lore-SM3-L3` | timing keygen | `records/kem-19/Lore-SM3-L3__keygen.json` |
| `Lore-SM3-L3` | hash profile dec | `profile/kem-19/Lore-SM3-L3__dec.json` |
| `Lore-SM3-L3` | hash profile enc | `profile/kem-19/Lore-SM3-L3__enc.json` |
| `Lore-SM3-L3` | hash profile keygen | `profile/kem-19/Lore-SM3-L3__keygen.json` |
| `Lore-SM3-L4` | KAT log (sha256 `eb19dc0dc4844f16…`) | `kat/kem-19/Lore-SM3-L4.log` |
| `Lore-SM3-L4` | timing dec | `records/kem-19/Lore-SM3-L4__dec.json` |
| `Lore-SM3-L4` | timing enc | `records/kem-19/Lore-SM3-L4__enc.json` |
| `Lore-SM3-L4` | timing keygen | `records/kem-19/Lore-SM3-L4__keygen.json` |
| `Lore-SM3-L4` | hash profile dec | `profile/kem-19/Lore-SM3-L4__dec.json` |
| `Lore-SM3-L4` | hash profile enc | `profile/kem-19/Lore-SM3-L4__enc.json` |
| `Lore-SM3-L4` | hash profile keygen | `profile/kem-19/Lore-SM3-L4__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

