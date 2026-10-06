# kex-02 AFS-KEX — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kex-02` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560625362194432.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key exchange
- Algorithm: AFS-KEX
- Implementation versions measured: reference
- Parameter sets: `AFS_KEX_C128`, `AFS_KEX_C256`, `AFS_KEX_C512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kex-02/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `AFS_KEX_C128` | guide | PASS |
| `AFS_KEX_C256` | guide | PASS |
| `AFS_KEX_C512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `AFS_KEX_C128` | exchange | 2.81 M | 1.34 ms | 744 | 1.34 ms | 3670 (5 × 734) |
| `AFS_KEX_C128` | init_a | 519.4 k | 248 µs | 4.03e+03 | 248 µs | 3670 (5 × 734) |
| `AFS_KEX_C128` | init_b | 519.7 k | 248 µs | 4.03e+03 | 248 µs | 3670 (5 × 734) |
| `AFS_KEX_C128` | pass1 | 291.9 k | 139 µs | 7.18e+03 | 139 µs | 3670 (5 × 734) |
| `AFS_KEX_C128` | pass2 | 640.0 k | 306 µs | 3.27e+03 | 306 µs | 3670 (5 × 734) |
| `AFS_KEX_C128` | pass3 | 599.7 k | 286 µs | 3.49e+03 | 287 µs | 3670 (5 × 734) |
| `AFS_KEX_C128` | pass4 | 250.4 k | 120 µs | 8.37e+03 | 120 µs | 3670 (5 × 734) |
| `AFS_KEX_C128` | derive_a | 317 | 50.2 ns | 1.99e+07 | 50.1 ns | 3670 (5 × 734) |
| `AFS_KEX_C128` | derive_b | 316 | 50.4 ns | 1.98e+07 | 50.3 ns | 3670 (5 × 734) |
| `AFS_KEX_C256` | exchange | 6.52 M | 3.11 ms | 321 | 3.11 ms | 1535 (5 × 307) |
| `AFS_KEX_C256` | init_a | 1.24 M | 594 µs | 1.68e+03 | 593 µs | 1535 (5 × 307) |
| `AFS_KEX_C256` | init_b | 1.25 M | 595 µs | 1.68e+03 | 594 µs | 1535 (5 × 307) |
| `AFS_KEX_C256` | pass1 | 645.4 k | 308 µs | 3.24e+03 | 308 µs | 1535 (5 × 307) |
| `AFS_KEX_C256` | pass2 | 1.42 M | 678 µs | 1.47e+03 | 678 µs | 1535 (5 × 307) |
| `AFS_KEX_C256` | pass3 | 1.37 M | 653 µs | 1.53e+03 | 653 µs | 1535 (5 × 307) |
| `AFS_KEX_C256` | pass4 | 596.3 k | 292 µs | 3.42e+03 | 285 µs | 1535 (5 × 307) |
| `AFS_KEX_C256` | derive_a | 338 | 55.8 ns | 1.79e+07 | 56.3 ns | 1535 (5 × 307) |
| `AFS_KEX_C256` | derive_b | 334 | 53.5 ns | 1.87e+07 | 53.7 ns | 1535 (5 × 307) |
| `AFS_KEX_C512` | exchange | 22.17 M | 10.6 ms | 94.4 | 10.6 ms | 475 (5 × 95) |
| `AFS_KEX_C512` | init_a | 4.26 M | 2.03 ms | 492 | 2.03 ms | 475 (5 × 95) |
| `AFS_KEX_C512` | init_b | 4.26 M | 2.03 ms | 492 | 2.03 ms | 475 (5 × 95) |
| `AFS_KEX_C512` | pass1 | 2.18 M | 1.04 ms | 959 | 1.04 ms | 475 (5 × 95) |
| `AFS_KEX_C512` | pass2 | 4.74 M | 2.26 ms | 442 | 2.26 ms | 475 (5 × 95) |
| `AFS_KEX_C512` | pass3 | 4.65 M | 2.22 ms | 451 | 2.22 ms | 475 (5 × 95) |
| `AFS_KEX_C512` | pass4 | 2.09 M | 999 µs | 1e+03 | 998 µs | 475 (5 × 95) |
| `AFS_KEX_C512` | derive_a | 351 | 59.7 ns | 1.68e+07 | 58.9 ns | 475 (5 × 95) |
| `AFS_KEX_C512` | derive_b | 340 | 55.7 ns | 1.79e+07 | 56 ns | 475 (5 × 95) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `AFS_KEX_C128` | exchange | 32053 | 1748 KiB | 1836 KiB |
| `AFS_KEX_C128` | init_a | 32053 | 1744 KiB | 1808 KiB |
| `AFS_KEX_C128` | init_b | 32053 | 1760 KiB | 1856 KiB |
| `AFS_KEX_C128` | pass1 | 32053 | 1764 KiB | 1828 KiB |
| `AFS_KEX_C128` | pass2 | 32053 | 1748 KiB | 1852 KiB |
| `AFS_KEX_C128` | pass3 | 32053 | 1740 KiB | 1856 KiB |
| `AFS_KEX_C128` | pass4 | 32053 | 1764 KiB | 1852 KiB |
| `AFS_KEX_C128` | derive_a | 32053 | 1744 KiB | 1852 KiB |
| `AFS_KEX_C128` | derive_b | 32053 | 1768 KiB | 1832 KiB |
| `AFS_KEX_C256` | exchange | 40345 | 1800 KiB | 1916 KiB |
| `AFS_KEX_C256` | init_a | 40345 | 1800 KiB | 1896 KiB |
| `AFS_KEX_C256` | init_b | 40345 | 1816 KiB | 1884 KiB |
| `AFS_KEX_C256` | pass1 | 40345 | 1812 KiB | 1912 KiB |
| `AFS_KEX_C256` | pass2 | 40345 | 1824 KiB | 1904 KiB |
| `AFS_KEX_C256` | pass3 | 40345 | 1824 KiB | 1908 KiB |
| `AFS_KEX_C256` | pass4 | 40345 | 1824 KiB | 1892 KiB |
| `AFS_KEX_C256` | derive_a | 40345 | 1820 KiB | 1888 KiB |
| `AFS_KEX_C256` | derive_b | 40345 | 1792 KiB | 1860 KiB |
| `AFS_KEX_C512` | exchange | 41277 | 1912 KiB | 1980 KiB |
| `AFS_KEX_C512` | init_a | 41277 | 1916 KiB | 2016 KiB |
| `AFS_KEX_C512` | init_b | 41277 | 1912 KiB | 1980 KiB |
| `AFS_KEX_C512` | pass1 | 41277 | 1920 KiB | 2012 KiB |
| `AFS_KEX_C512` | pass2 | 41277 | 1908 KiB | 1976 KiB |
| `AFS_KEX_C512` | pass3 | 41277 | 1916 KiB | 1984 KiB |
| `AFS_KEX_C512` | pass4 | 41277 | 1920 KiB | 1988 KiB |
| `AFS_KEX_C512` | derive_a | 41277 | 1908 KiB | 1976 KiB |
| `AFS_KEX_C512` | derive_b | 41277 | 1928 KiB | 1996 KiB |

## 6. Transmission and storage overhead

Bandwidth counts all specified protocol messages and each required public key once. Public keys are transmitted bytes too. Certificates and transport framing are excluded. The published raw timing records are unchanged.

AFS-KEX Figure 3 sends fresh composite keys in passes 1 and 2. Its specified protocol-message totals are 3,136/6,080/12,288 bytes; the submitted API instead puts those keys in pre-distributed public-key buffers and sends 1,568/2,944/6,016 bytes as protocol messages. Both arrangements give the same bandwidth totals. Separately, the submitted pass 4 emits nothing but does not assign its output length, so the benchmark's capacity-initialized length creates a phantom raw message. [The size audit](../performance/EXTERNAL_SIZE_AUDIT.md) explains the accounting.

| instance | passes | messages (bytes; raw API) | protocol-message bytes | public key A / B | bandwidth (bytes) | long-term sk (API cap) | shared secret |
|---|---|---|---|---|---|---|---|
| `AFS_KEX_C128` | 4 | 768 / 784 / 16 / 1568 | 3136 | 784 / 784 | 4704 | 3170 | 16 |
| `AFS_KEX_C256` | 4 | 1440 / 1472 / 32 / 2944 | 6080 | 1568 / 1568 | 9216 | 6338 | 32 |
| `AFS_KEX_C512` | 4 | 2944 / 3008 / 64 / 6016 | 12288 | 3136 / 3136 | 18560 | 12674 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — optimized AVX2 auxfunc.c adds an SM3 counter-block fast path

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `AFS_KEX_C128` | exchange | 63% | 1.0% | drng 6, pseudoXOF 88.3, sm3hash 6 |
| `AFS_KEX_C256` | exchange | 73% | 0.5% | drng 6, pseudoXOF 249, pseudohash 6 |
| `AFS_KEX_C512` | exchange | 76% | 0.2% | drng 6, pseudoXOF 248, pseudohash 6 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `AFS_KEX_C128` | KAT log (sha256 `44a7a639ded42986…`) | `kat/kex-02/AFS_KEX_C128.log` |
| `AFS_KEX_C128` | timing derive_a | `records/kex-02/AFS_KEX_C128__derive_a.json` |
| `AFS_KEX_C128` | timing derive_b | `records/kex-02/AFS_KEX_C128__derive_b.json` |
| `AFS_KEX_C128` | timing exchange | `records/kex-02/AFS_KEX_C128__exchange.json` |
| `AFS_KEX_C128` | timing init_a | `records/kex-02/AFS_KEX_C128__init_a.json` |
| `AFS_KEX_C128` | timing init_b | `records/kex-02/AFS_KEX_C128__init_b.json` |
| `AFS_KEX_C128` | timing pass1 | `records/kex-02/AFS_KEX_C128__pass1.json` |
| `AFS_KEX_C128` | timing pass2 | `records/kex-02/AFS_KEX_C128__pass2.json` |
| `AFS_KEX_C128` | timing pass3 | `records/kex-02/AFS_KEX_C128__pass3.json` |
| `AFS_KEX_C128` | timing pass4 | `records/kex-02/AFS_KEX_C128__pass4.json` |
| `AFS_KEX_C128` | hash profile exchange | `profile/kex-02/AFS_KEX_C128__exchange.json` |
| `AFS_KEX_C256` | KAT log (sha256 `5ad0fe3331e2a187…`) | `kat/kex-02/AFS_KEX_C256.log` |
| `AFS_KEX_C256` | timing derive_a | `records/kex-02/AFS_KEX_C256__derive_a.json` |
| `AFS_KEX_C256` | timing derive_b | `records/kex-02/AFS_KEX_C256__derive_b.json` |
| `AFS_KEX_C256` | timing exchange | `records/kex-02/AFS_KEX_C256__exchange.json` |
| `AFS_KEX_C256` | timing init_a | `records/kex-02/AFS_KEX_C256__init_a.json` |
| `AFS_KEX_C256` | timing init_b | `records/kex-02/AFS_KEX_C256__init_b.json` |
| `AFS_KEX_C256` | timing pass1 | `records/kex-02/AFS_KEX_C256__pass1.json` |
| `AFS_KEX_C256` | timing pass2 | `records/kex-02/AFS_KEX_C256__pass2.json` |
| `AFS_KEX_C256` | timing pass3 | `records/kex-02/AFS_KEX_C256__pass3.json` |
| `AFS_KEX_C256` | timing pass4 | `records/kex-02/AFS_KEX_C256__pass4.json` |
| `AFS_KEX_C256` | hash profile exchange | `profile/kex-02/AFS_KEX_C256__exchange.json` |
| `AFS_KEX_C512` | KAT log (sha256 `bfe24e66d2fb9898…`) | `kat/kex-02/AFS_KEX_C512.log` |
| `AFS_KEX_C512` | timing derive_a | `records/kex-02/AFS_KEX_C512__derive_a.json` |
| `AFS_KEX_C512` | timing derive_b | `records/kex-02/AFS_KEX_C512__derive_b.json` |
| `AFS_KEX_C512` | timing exchange | `records/kex-02/AFS_KEX_C512__exchange.json` |
| `AFS_KEX_C512` | timing init_a | `records/kex-02/AFS_KEX_C512__init_a.json` |
| `AFS_KEX_C512` | timing init_b | `records/kex-02/AFS_KEX_C512__init_b.json` |
| `AFS_KEX_C512` | timing pass1 | `records/kex-02/AFS_KEX_C512__pass1.json` |
| `AFS_KEX_C512` | timing pass2 | `records/kex-02/AFS_KEX_C512__pass2.json` |
| `AFS_KEX_C512` | timing pass3 | `records/kex-02/AFS_KEX_C512__pass3.json` |
| `AFS_KEX_C512` | timing pass4 | `records/kex-02/AFS_KEX_C512__pass4.json` |
| `AFS_KEX_C512` | hash profile exchange | `profile/kex-02/AFS_KEX_C512__exchange.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

