# kex-05 Loom — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kex-05` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560625773236224.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key exchange
- Algorithm: Loom
- Implementation versions measured: reference
- Parameter sets: `LoomKEX-128`, `LoomKEX-256`, `LoomKEX-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kex-05/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `LoomKEX-128` | guide | PASS |
| `LoomKEX-256` | guide | PASS |
| `LoomKEX-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `LoomKEX-128` | exchange | 29.95 M | 11.1 ms | 90 | 11.1 ms | 195 (5 × 39) |
| `LoomKEX-128` | init_a | 8.15 M | 3.03 ms | 331 | 3.02 ms | 195 (5 × 39) |
| `LoomKEX-128` | init_b | 9.66 M | 3.59 ms | 279 | 3.58 ms | 195 (5 × 39) |
| `LoomKEX-128` | pass1 | 503.8 k | 187 µs | 5.35e+03 | 185 µs | 195 (5 × 39) |
| `LoomKEX-128` | pass2 | 566.8 k | 210 µs | 4.76e+03 | 207 µs | 195 (5 × 39) |
| `LoomKEX-128` | pass3 | 4.40 M | 1.63 ms | 612 | 1.63 ms | 195 (5 × 39) |
| `LoomKEX-128` | pass4 | 5.50 M | 2.04 ms | 490 | 2.04 ms | 195 (5 × 39) |
| `LoomKEX-128` | derive_a | 1.13 M | 420 µs | 2.38e+03 | 420 µs | 195 (5 × 39) |
| `LoomKEX-128` | derive_b | 6479 | 2.34 µs | 4.27e+05 | 2.34 µs | 195 (5 × 39) |
| `LoomKEX-256` | exchange | 37.19 M | 13.8 ms | 72.5 | 13.8 ms | 150 (5 × 30) |
| `LoomKEX-256` | init_a | 8.85 M | 3.28 ms | 304 | 3.28 ms | 150 (5 × 30) |
| `LoomKEX-256` | init_b | 9.57 M | 3.55 ms | 282 | 3.55 ms | 150 (5 × 30) |
| `LoomKEX-256` | pass1 | 698.1 k | 259 µs | 3.86e+03 | 257 µs | 150 (5 × 30) |
| `LoomKEX-256` | pass2 | 777.4 k | 288 µs | 3.47e+03 | 288 µs | 150 (5 × 30) |
| `LoomKEX-256` | pass3 | 7.12 M | 2.64 ms | 379 | 2.64 ms | 150 (5 × 30) |
| `LoomKEX-256` | pass4 | 8.59 M | 3.19 ms | 314 | 3.17 ms | 150 (5 × 30) |
| `LoomKEX-256` | derive_a | 1.57 M | 582 µs | 1.72e+03 | 581 µs | 150 (5 × 30) |
| `LoomKEX-256` | derive_b | 10.6 k | 3.94 µs | 2.54e+05 | 3.88 µs | 150 (5 × 30) |
| `LoomKEX-512` | exchange | 79.08 M | 29.3 ms | 34.1 | 29.3 ms | 110 (5 × 22) |
| `LoomKEX-512` | init_a | 18.31 M | 6.79 ms | 147 | 6.79 ms | 110 (5 × 22) |
| `LoomKEX-512` | init_b | 20.50 M | 7.6 ms | 131 | 7.61 ms | 110 (5 × 22) |
| `LoomKEX-512` | pass1 | 2.38 M | 881 µs | 1.13e+03 | 881 µs | 110 (5 × 22) |
| `LoomKEX-512` | pass2 | 2.88 M | 1.07 ms | 937 | 1.07 ms | 110 (5 × 22) |
| `LoomKEX-512` | pass3 | 14.77 M | 5.48 ms | 182 | 5.48 ms | 110 (5 × 22) |
| `LoomKEX-512` | pass4 | 17.32 M | 6.43 ms | 156 | 6.43 ms | 110 (5 × 22) |
| `LoomKEX-512` | derive_a | 2.89 M | 1.07 ms | 931 | 1.07 ms | 110 (5 × 22) |
| `LoomKEX-512` | derive_b | 21.6 k | 7.94 µs | 1.26e+05 | 7.95 µs | 110 (5 × 22) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `LoomKEX-128` | exchange | 103320 | 1672 KiB | 1740 KiB |
| `LoomKEX-128` | init_a | 103320 | 1672 KiB | 1740 KiB |
| `LoomKEX-128` | init_b | 103320 | 1672 KiB | 1740 KiB |
| `LoomKEX-128` | pass1 | 103320 | 3668 KiB | 3732 KiB |
| `LoomKEX-128` | pass2 | 103320 | 1672 KiB | 1740 KiB |
| `LoomKEX-128` | pass3 | 103320 | 1672 KiB | 1740 KiB |
| `LoomKEX-128` | pass4 | 103320 | 1672 KiB | 1740 KiB |
| `LoomKEX-128` | derive_a | 103320 | 1672 KiB | 1740 KiB |
| `LoomKEX-128` | derive_b | 103320 | 3652 KiB | 3716 KiB |
| `LoomKEX-256` | exchange | 107924 | 1816 KiB | 1884 KiB |
| `LoomKEX-256` | init_a | 107924 | 3716 KiB | 3780 KiB |
| `LoomKEX-256` | init_b | 107924 | 3832 KiB | 3896 KiB |
| `LoomKEX-256` | pass1 | 107924 | 1816 KiB | 1884 KiB |
| `LoomKEX-256` | pass2 | 107924 | 1816 KiB | 1884 KiB |
| `LoomKEX-256` | pass3 | 107924 | 3836 KiB | 3900 KiB |
| `LoomKEX-256` | pass4 | 107924 | 1816 KiB | 1884 KiB |
| `LoomKEX-256` | derive_a | 107924 | 3728 KiB | 3792 KiB |
| `LoomKEX-256` | derive_b | 107924 | 1816 KiB | 1884 KiB |
| `LoomKEX-512` | exchange | 112288 | 2148 KiB | 2224 KiB |
| `LoomKEX-512` | init_a | 112288 | 4036 KiB | 4100 KiB |
| `LoomKEX-512` | init_b | 112288 | 2148 KiB | 2224 KiB |
| `LoomKEX-512` | pass1 | 112288 | 4152 KiB | 4216 KiB |
| `LoomKEX-512` | pass2 | 112288 | 3968 KiB | 4032 KiB |
| `LoomKEX-512` | pass3 | 112288 | 4024 KiB | 4088 KiB |
| `LoomKEX-512` | pass4 | 112288 | 3920 KiB | 3984 KiB |
| `LoomKEX-512` | derive_a | 112288 | 3968 KiB | 4032 KiB |
| `LoomKEX-512` | derive_b | 112288 | 4132 KiB | 4196 KiB |

## 6. Transmission and storage overhead

Bandwidth counts all specified protocol messages and each required public key once. Public keys are transmitted bytes too. Certificates and transport framing are excluded. The published raw timing records are unchanged.

| instance | passes | messages (bytes; raw API) | protocol-message bytes | public key A / B | bandwidth (bytes) | long-term sk (API cap) | shared secret |
|---|---|---|---|---|---|---|---|
| `LoomKEX-128` | 4 | 792 / 776 / 1235 / 1235 | 4038 | 1264 / 1264 | 6566 | 2288 | 16 |
| `LoomKEX-256` | 4 | 1352 / 1384 / 2485 / 2485 | 7706 | 1952 / 1952 | 11610 | 3680 | 32 |
| `LoomKEX-512` | 4 | 2952 / 3016 / 5101 / 5101 | 16170 | 3648 / 3648 | 23466 | 7104 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — grep hits for AES/Keccak/OpenSSL are not reachable in the built library

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `LoomKEX-128` | exchange | 3.2% | 70% | drng 808, pseudoXOF 84 |
| `LoomKEX-256` | exchange | 3.6% | 68% | drng 912, pseudoXOF 63 |
| `LoomKEX-512` | exchange | 6.1% | 66% | drng 1.77e+03, pseudoXOF 67 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `LoomKEX-128` | KAT log (sha256 `ca6cdab8b040f619…`) | `kat/kex-05/LoomKEX-128.log` |
| `LoomKEX-128` | timing derive_a | `records/kex-05/LoomKEX-128__derive_a.json` |
| `LoomKEX-128` | timing derive_b | `records/kex-05/LoomKEX-128__derive_b.json` |
| `LoomKEX-128` | timing exchange | `records/kex-05/LoomKEX-128__exchange.json` |
| `LoomKEX-128` | timing init_a | `records/kex-05/LoomKEX-128__init_a.json` |
| `LoomKEX-128` | timing init_b | `records/kex-05/LoomKEX-128__init_b.json` |
| `LoomKEX-128` | timing pass1 | `records/kex-05/LoomKEX-128__pass1.json` |
| `LoomKEX-128` | timing pass2 | `records/kex-05/LoomKEX-128__pass2.json` |
| `LoomKEX-128` | timing pass3 | `records/kex-05/LoomKEX-128__pass3.json` |
| `LoomKEX-128` | timing pass4 | `records/kex-05/LoomKEX-128__pass4.json` |
| `LoomKEX-128` | hash profile exchange | `profile/kex-05/LoomKEX-128__exchange.json` |
| `LoomKEX-256` | KAT log (sha256 `1fd101fa673ee4df…`) | `kat/kex-05/LoomKEX-256.log` |
| `LoomKEX-256` | timing derive_a | `records/kex-05/LoomKEX-256__derive_a.json` |
| `LoomKEX-256` | timing derive_b | `records/kex-05/LoomKEX-256__derive_b.json` |
| `LoomKEX-256` | timing exchange | `records/kex-05/LoomKEX-256__exchange.json` |
| `LoomKEX-256` | timing init_a | `records/kex-05/LoomKEX-256__init_a.json` |
| `LoomKEX-256` | timing init_b | `records/kex-05/LoomKEX-256__init_b.json` |
| `LoomKEX-256` | timing pass1 | `records/kex-05/LoomKEX-256__pass1.json` |
| `LoomKEX-256` | timing pass2 | `records/kex-05/LoomKEX-256__pass2.json` |
| `LoomKEX-256` | timing pass3 | `records/kex-05/LoomKEX-256__pass3.json` |
| `LoomKEX-256` | timing pass4 | `records/kex-05/LoomKEX-256__pass4.json` |
| `LoomKEX-256` | hash profile exchange | `profile/kex-05/LoomKEX-256__exchange.json` |
| `LoomKEX-512` | KAT log (sha256 `4222b89ae0c2d697…`) | `kat/kex-05/LoomKEX-512.log` |
| `LoomKEX-512` | timing derive_a | `records/kex-05/LoomKEX-512__derive_a.json` |
| `LoomKEX-512` | timing derive_b | `records/kex-05/LoomKEX-512__derive_b.json` |
| `LoomKEX-512` | timing exchange | `records/kex-05/LoomKEX-512__exchange.json` |
| `LoomKEX-512` | timing init_a | `records/kex-05/LoomKEX-512__init_a.json` |
| `LoomKEX-512` | timing init_b | `records/kex-05/LoomKEX-512__init_b.json` |
| `LoomKEX-512` | timing pass1 | `records/kex-05/LoomKEX-512__pass1.json` |
| `LoomKEX-512` | timing pass2 | `records/kex-05/LoomKEX-512__pass2.json` |
| `LoomKEX-512` | timing pass3 | `records/kex-05/LoomKEX-512__pass3.json` |
| `LoomKEX-512` | timing pass4 | `records/kex-05/LoomKEX-512__pass4.json` |
| `LoomKEX-512` | hash profile exchange | `profile/kex-05/LoomKEX-512__exchange.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

