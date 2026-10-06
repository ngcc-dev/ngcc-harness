# kex-05 Loom — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kex-05` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560625773236224.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key exchange
- Algorithm: Loom
- Implementation versions measured: reference
- Parameter sets: `LoomKEX-128`, `LoomKEX-256`, `LoomKEX-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kex-05/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `LoomKEX-128` | guide | PASS |
| `LoomKEX-256` | guide | PASS |
| `LoomKEX-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `LoomKEX-128` | exchange | 30.90 M | 14.8 ms | 67.8 | 14.8 ms | 230 (5 × 46) |
| `LoomKEX-128` | init_a | 8.60 M | 4.11 ms | 243 | 4.11 ms | 230 (5 × 46) |
| `LoomKEX-128` | init_b | 9.51 M | 4.54 ms | 220 | 4.54 ms | 230 (5 × 46) |
| `LoomKEX-128` | pass1 | 549.6 k | 262 µs | 3.81e+03 | 262 µs | 230 (5 × 46) |
| `LoomKEX-128` | pass2 | 621.9 k | 297 µs | 3.37e+03 | 297 µs | 230 (5 × 46) |
| `LoomKEX-128` | pass3 | 4.63 M | 2.21 ms | 452 | 2.21 ms | 230 (5 × 46) |
| `LoomKEX-128` | pass4 | 5.79 M | 2.76 ms | 362 | 2.76 ms | 230 (5 × 46) |
| `LoomKEX-128` | derive_a | 1.21 M | 576 µs | 1.74e+03 | 576 µs | 230 (5 × 46) |
| `LoomKEX-128` | derive_b | 4246 | 1.86 µs | 5.39e+05 | 1.84 µs | 230 (5 × 46) |
| `LoomKEX-256` | exchange | 38.83 M | 18.5 ms | 53.9 | 18.5 ms | 175 (5 × 35) |
| `LoomKEX-256` | init_a | 8.81 M | 4.21 ms | 238 | 4.21 ms | 175 (5 × 35) |
| `LoomKEX-256` | init_b | 10.35 M | 4.94 ms | 202 | 4.94 ms | 175 (5 × 35) |
| `LoomKEX-256` | pass1 | 764.7 k | 365 µs | 2.74e+03 | 365 µs | 175 (5 × 35) |
| `LoomKEX-256` | pass2 | 881.8 k | 421 µs | 2.38e+03 | 421 µs | 175 (5 × 35) |
| `LoomKEX-256` | pass3 | 7.42 M | 3.55 ms | 282 | 3.54 ms | 175 (5 × 35) |
| `LoomKEX-256` | pass4 | 8.93 M | 4.26 ms | 234 | 4.26 ms | 175 (5 × 35) |
| `LoomKEX-256` | derive_a | 1.67 M | 795 µs | 1.26e+03 | 795 µs | 175 (5 × 35) |
| `LoomKEX-256` | derive_b | 6333 | 2.85 µs | 3.51e+05 | 2.85 µs | 175 (5 × 35) |
| `LoomKEX-512` | exchange | 81.64 M | 39 ms | 25.6 | 39 ms | 125 (5 × 25) |
| `LoomKEX-512` | init_a | 19.24 M | 9.19 ms | 109 | 9.19 ms | 125 (5 × 25) |
| `LoomKEX-512` | init_b | 20.27 M | 9.69 ms | 103 | 9.69 ms | 125 (5 × 25) |
| `LoomKEX-512` | pass1 | 2.53 M | 1.22 ms | 819 | 1.22 ms | 125 (5 × 25) |
| `LoomKEX-512` | pass2 | 3.09 M | 1.48 ms | 677 | 1.48 ms | 125 (5 × 25) |
| `LoomKEX-512` | pass3 | 15.28 M | 7.31 ms | 137 | 7.31 ms | 125 (5 × 25) |
| `LoomKEX-512` | pass4 | 18.04 M | 8.64 ms | 116 | 8.63 ms | 125 (5 × 25) |
| `LoomKEX-512` | derive_a | 3.15 M | 1.5 ms | 665 | 1.5 ms | 125 (5 × 25) |
| `LoomKEX-512` | derive_b | 11.6 k | 5.43 µs | 1.84e+05 | 5.43 µs | 125 (5 × 25) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `LoomKEX-128` | exchange | 108105 | 1944 KiB | 2044 KiB |
| `LoomKEX-128` | init_a | 108105 | 1936 KiB | 2052 KiB |
| `LoomKEX-128` | init_b | 108105 | 1948 KiB | 2056 KiB |
| `LoomKEX-128` | pass1 | 108105 | 1968 KiB | 2060 KiB |
| `LoomKEX-128` | pass2 | 108105 | 1964 KiB | 2036 KiB |
| `LoomKEX-128` | pass3 | 108105 | 1964 KiB | 2036 KiB |
| `LoomKEX-128` | pass4 | 108105 | 1960 KiB | 2032 KiB |
| `LoomKEX-128` | derive_a | 108105 | 1948 KiB | 2020 KiB |
| `LoomKEX-128` | derive_b | 108105 | 1940 KiB | 2052 KiB |
| `LoomKEX-256` | exchange | 113917 | 2080 KiB | 2152 KiB |
| `LoomKEX-256` | init_a | 113917 | 2072 KiB | 2172 KiB |
| `LoomKEX-256` | init_b | 113917 | 2076 KiB | 2172 KiB |
| `LoomKEX-256` | pass1 | 113917 | 2084 KiB | 2156 KiB |
| `LoomKEX-256` | pass2 | 113917 | 2048 KiB | 2184 KiB |
| `LoomKEX-256` | pass3 | 113917 | 2088 KiB | 2196 KiB |
| `LoomKEX-256` | pass4 | 113917 | 2068 KiB | 2200 KiB |
| `LoomKEX-256` | derive_a | 113917 | 2100 KiB | 2172 KiB |
| `LoomKEX-256` | derive_b | 113917 | 2060 KiB | 2192 KiB |
| `LoomKEX-512` | exchange | 119277 | 2428 KiB | 2504 KiB |
| `LoomKEX-512` | init_a | 119277 | 2400 KiB | 2528 KiB |
| `LoomKEX-512` | init_b | 119277 | 2416 KiB | 2492 KiB |
| `LoomKEX-512` | pass1 | 119277 | 2404 KiB | 2520 KiB |
| `LoomKEX-512` | pass2 | 119277 | 2428 KiB | 2504 KiB |
| `LoomKEX-512` | pass3 | 119277 | 2424 KiB | 2516 KiB |
| `LoomKEX-512` | pass4 | 119277 | 2404 KiB | 2480 KiB |
| `LoomKEX-512` | derive_a | 119277 | 2416 KiB | 2532 KiB |
| `LoomKEX-512` | derive_b | 119277 | 2404 KiB | 2528 KiB |

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
| `LoomKEX-128` | exchange | 3.1% | 71% | drng 833, pseudoXOF 84 |
| `LoomKEX-256` | exchange | 3.4% | 69% | drng 939, pseudoXOF 63 |
| `LoomKEX-512` | exchange | 6.2% | 67% | drng 1.75e+03, pseudoXOF 67 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `LoomKEX-128` | KAT log (sha256 `5ee8305db770e4cb…`) | `kat/kex-05/LoomKEX-128.log` |
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
| `LoomKEX-256` | KAT log (sha256 `1246b71c4a1f2b2d…`) | `kat/kex-05/LoomKEX-256.log` |
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
| `LoomKEX-512` | KAT log (sha256 `43b12c64434096a8…`) | `kat/kex-05/LoomKEX-512.log` |
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

