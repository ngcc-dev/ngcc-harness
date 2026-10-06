# kex-04 DKEX (Ding Key Exchange) — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kex-04` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560625651601408.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key exchange
- Algorithm: DKEX (Ding Key Exchange)
- Implementation versions measured: reference
- Parameter sets: `DKEX-128`, `DKEX-256`, `DKEX-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kex-04/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `DKEX-128` | guide | PASS |
| `DKEX-256` | guide | PASS |
| `DKEX-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `DKEX-128` | exchange | 3.84 M | 1.84 ms | 544 | 1.84 ms | 3000 (5 × 600) |
| `DKEX-128` | init_a | 267.8 k | 132 µs | 7.58e+03 | 128 µs | 3000 (5 × 600) |
| `DKEX-128` | init_b | 267.3 k | 128 µs | 7.84e+03 | 127 µs | 3000 (5 × 600) |
| `DKEX-128` | pass1 | 203.3 k | 97 µs | 1.03e+04 | 97 µs | 3000 (5 × 600) |
| `DKEX-128` | pass2 | 1.39 M | 662 µs | 1.51e+03 | 662 µs | 3000 (5 × 600) |
| `DKEX-128` | pass3 | 1.48 M | 718 µs | 1.39e+03 | 705 µs | 3000 (5 × 600) |
| `DKEX-128` | derive_a | 3243 | 1.49 µs | 6.69e+05 | 1.44 µs | 3000 (5 × 600) |
| `DKEX-128` | derive_b | 287.6 k | 137 µs | 7.28e+03 | 137 µs | 3000 (5 × 600) |
| `DKEX-256` | exchange | 8.70 M | 4.16 ms | 240 | 4.16 ms | 1030 (5 × 206) |
| `DKEX-256` | init_a | 717.9 k | 343 µs | 2.92e+03 | 343 µs | 1030 (5 × 206) |
| `DKEX-256` | init_b | 724.7 k | 346 µs | 2.89e+03 | 346 µs | 1030 (5 × 206) |
| `DKEX-256` | pass1 | 604.8 k | 289 µs | 3.46e+03 | 289 µs | 1030 (5 × 206) |
| `DKEX-256` | pass2 | 2.90 M | 1.39 ms | 721 | 1.39 ms | 1030 (5 × 206) |
| `DKEX-256` | pass3 | 3.02 M | 1.44 ms | 693 | 1.44 ms | 1030 (5 × 206) |
| `DKEX-256` | derive_a | 3242 | 1.44 µs | 6.95e+05 | 1.44 µs | 1030 (5 × 206) |
| `DKEX-256` | derive_b | 752.3 k | 359 µs | 2.78e+03 | 359 µs | 1030 (5 × 206) |
| `DKEX-512` | exchange | 12.24 M | 5.85 ms | 171 | 5.85 ms | 890 (5 × 178) |
| `DKEX-512` | init_a | 730.9 k | 349 µs | 2.86e+03 | 349 µs | 890 (5 × 178) |
| `DKEX-512` | init_b | 734.3 k | 351 µs | 2.85e+03 | 350 µs | 890 (5 × 178) |
| `DKEX-512` | pass1 | 2.06 M | 1.01 ms | 991 | 984 µs | 890 (5 × 178) |
| `DKEX-512` | pass2 | 4.71 M | 2.25 ms | 444 | 2.25 ms | 890 (5 × 178) |
| `DKEX-512` | pass3 | 3.28 M | 1.57 ms | 637 | 1.57 ms | 890 (5 × 178) |
| `DKEX-512` | derive_a | 9114 | 4.24 µs | 2.36e+05 | 4.25 µs | 890 (5 × 178) |
| `DKEX-512` | derive_b | 768.0 k | 367 µs | 2.73e+03 | 367 µs | 890 (5 × 178) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `DKEX-128` | exchange | 57265 | 1896 KiB | 1960 KiB |
| `DKEX-128` | init_a | 57265 | 1884 KiB | 1948 KiB |
| `DKEX-128` | init_b | 57265 | 1888 KiB | 1952 KiB |
| `DKEX-128` | pass1 | 57265 | 1900 KiB | 1980 KiB |
| `DKEX-128` | pass2 | 57265 | 1876 KiB | 1984 KiB |
| `DKEX-128` | pass3 | 57265 | 1896 KiB | 1960 KiB |
| `DKEX-128` | derive_a | 57265 | 1892 KiB | 1956 KiB |
| `DKEX-128` | derive_b | 57265 | 1876 KiB | 1964 KiB |
| `DKEX-256` | exchange | 58249 | 2068 KiB | 2132 KiB |
| `DKEX-256` | init_a | 58249 | 2056 KiB | 2120 KiB |
| `DKEX-256` | init_b | 58249 | 2072 KiB | 2144 KiB |
| `DKEX-256` | pass1 | 58249 | 2064 KiB | 2128 KiB |
| `DKEX-256` | pass2 | 58249 | 2048 KiB | 2112 KiB |
| `DKEX-256` | pass3 | 58249 | 2044 KiB | 2140 KiB |
| `DKEX-256` | derive_a | 58249 | 2048 KiB | 2112 KiB |
| `DKEX-256` | derive_b | 58249 | 1980 KiB | 2108 KiB |
| `DKEX-512` | exchange | 59281 | 2124 KiB | 2200 KiB |
| `DKEX-512` | init_a | 59281 | 2108 KiB | 2212 KiB |
| `DKEX-512` | init_b | 59281 | 2132 KiB | 2212 KiB |
| `DKEX-512` | pass1 | 59281 | 2104 KiB | 2232 KiB |
| `DKEX-512` | pass2 | 59281 | 2104 KiB | 2212 KiB |
| `DKEX-512` | pass3 | 59281 | 2124 KiB | 2200 KiB |
| `DKEX-512` | derive_a | 59281 | 2124 KiB | 2212 KiB |
| `DKEX-512` | derive_b | 59281 | 2124 KiB | 2200 KiB |

## 6. Transmission and storage overhead

Bandwidth counts all specified protocol messages and each required public key once. Public keys are transmitted bytes too. Certificates and transport framing are excluded. The published raw timing records are unchanged.

| instance | passes | messages (bytes; raw API) | protocol-message bytes | public key A / B | bandwidth (bytes) | long-term sk (API cap) | shared secret |
|---|---|---|---|---|---|---|---|
| `DKEX-128` | 3 | 800 / 3188 / 2420 | 6408 | 1312 / 1312 | 9032 | 2560 | 32 |
| `DKEX-256` | 3 | 1568 / 6195 / 4627 | 12390 | 2592 / 2592 | 17574 | 4896 | 32 |
| `DKEX-512` | 3 | 3392 / 7699 / 4627 | 15718 | 2592 / 2592 | 20902 | 4896 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **mixed** — KEX core via ICCS; embedded ML-DSA uses its own SHAKE

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `DKEX-128` | exchange | 12% | 0.5% | drng 4, pseudoXOF 1, sm3hash 186 |
| `DKEX-256` | exchange | 14% | 0.2% | drng 4, pseudoXOF 1, sm3hash 585 |
| `DKEX-512` | exchange | 36% | 0.2% | drng 4, pseudoXOF 5, sm3hash 1.23e+03 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `DKEX-128` | KAT log (sha256 `7159b0d28cca8ce6…`) | `kat/kex-04/DKEX-128.log` |
| `DKEX-128` | timing derive_a | `records/kex-04/DKEX-128__derive_a.json` |
| `DKEX-128` | timing derive_b | `records/kex-04/DKEX-128__derive_b.json` |
| `DKEX-128` | timing exchange | `records/kex-04/DKEX-128__exchange.json` |
| `DKEX-128` | timing init_a | `records/kex-04/DKEX-128__init_a.json` |
| `DKEX-128` | timing init_b | `records/kex-04/DKEX-128__init_b.json` |
| `DKEX-128` | timing pass1 | `records/kex-04/DKEX-128__pass1.json` |
| `DKEX-128` | timing pass2 | `records/kex-04/DKEX-128__pass2.json` |
| `DKEX-128` | timing pass3 | `records/kex-04/DKEX-128__pass3.json` |
| `DKEX-128` | hash profile exchange | `profile/kex-04/DKEX-128__exchange.json` |
| `DKEX-256` | KAT log (sha256 `0edc6b7375877b75…`) | `kat/kex-04/DKEX-256.log` |
| `DKEX-256` | timing derive_a | `records/kex-04/DKEX-256__derive_a.json` |
| `DKEX-256` | timing derive_b | `records/kex-04/DKEX-256__derive_b.json` |
| `DKEX-256` | timing exchange | `records/kex-04/DKEX-256__exchange.json` |
| `DKEX-256` | timing init_a | `records/kex-04/DKEX-256__init_a.json` |
| `DKEX-256` | timing init_b | `records/kex-04/DKEX-256__init_b.json` |
| `DKEX-256` | timing pass1 | `records/kex-04/DKEX-256__pass1.json` |
| `DKEX-256` | timing pass2 | `records/kex-04/DKEX-256__pass2.json` |
| `DKEX-256` | timing pass3 | `records/kex-04/DKEX-256__pass3.json` |
| `DKEX-256` | hash profile exchange | `profile/kex-04/DKEX-256__exchange.json` |
| `DKEX-512` | KAT log (sha256 `1fe8edad8396601a…`) | `kat/kex-04/DKEX-512.log` |
| `DKEX-512` | timing derive_a | `records/kex-04/DKEX-512__derive_a.json` |
| `DKEX-512` | timing derive_b | `records/kex-04/DKEX-512__derive_b.json` |
| `DKEX-512` | timing exchange | `records/kex-04/DKEX-512__exchange.json` |
| `DKEX-512` | timing init_a | `records/kex-04/DKEX-512__init_a.json` |
| `DKEX-512` | timing init_b | `records/kex-04/DKEX-512__init_b.json` |
| `DKEX-512` | timing pass1 | `records/kex-04/DKEX-512__pass1.json` |
| `DKEX-512` | timing pass2 | `records/kex-04/DKEX-512__pass2.json` |
| `DKEX-512` | timing pass3 | `records/kex-04/DKEX-512__pass3.json` |
| `DKEX-512` | hash profile exchange | `profile/kex-04/DKEX-512__exchange.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

