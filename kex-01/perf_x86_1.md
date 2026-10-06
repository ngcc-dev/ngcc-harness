# kex-01 ADKEX (Authenticated Ding Key Exchange) — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kex-01` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560625202810880.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key exchange
- Algorithm: ADKEX (Authenticated Ding Key Exchange)
- Implementation versions measured: reference
- Parameter sets: `ADKEX-128`, `ADKEX-256`, `ADKEX-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kex-01/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `ADKEX-128` | guide | PASS |
| `ADKEX-256` | guide | PASS |
| `ADKEX-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `ADKEX-128` | exchange | 1.52 M | 727 µs | 1.37e+03 | 728 µs | 6815 (5 × 1363) |
| `ADKEX-128` | init_a | 301 | 48.1 ns | 2.08e+07 | 48 ns | 6815 (5 × 1363) |
| `ADKEX-128` | init_b | 202.4 k | 96.6 µs | 1.04e+04 | 96.6 µs | 6815 (5 × 1363) |
| `ADKEX-128` | pass1 | 429.5 k | 205 µs | 4.88e+03 | 205 µs | 6815 (5 × 1363) |
| `ADKEX-128` | pass2 | 559.1 k | 267 µs | 3.75e+03 | 267 µs | 6815 (5 × 1363) |
| `ADKEX-128` | derive_a | 332.6 k | 159 µs | 6.3e+03 | 159 µs | 6815 (5 × 1363) |
| `ADKEX-128` | derive_b | 3110 | 1.39 µs | 7.2e+05 | 1.39 µs | 6815 (5 × 1363) |
| `ADKEX-256` | exchange | 4.05 M | 1.94 ms | 517 | 1.94 ms | 2565 (5 × 513) |
| `ADKEX-256` | init_a | 315 | 49.2 ns | 2.03e+07 | 49 ns | 2565 (5 × 513) |
| `ADKEX-256` | init_b | 603.1 k | 295 µs | 3.39e+03 | 288 µs | 2565 (5 × 513) |
| `ADKEX-256` | pass1 | 1.22 M | 584 µs | 1.71e+03 | 584 µs | 2565 (5 × 513) |
| `ADKEX-256` | pass2 | 1.43 M | 681 µs | 1.47e+03 | 681 µs | 2565 (5 × 513) |
| `ADKEX-256` | derive_a | 805.0 k | 384 µs | 2.6e+03 | 384 µs | 2565 (5 × 513) |
| `ADKEX-256` | derive_b | 3187 | 1.42 µs | 7.05e+05 | 1.42 µs | 2565 (5 × 513) |
| `ADKEX-512` | exchange | 13.97 M | 6.67 ms | 150 | 6.67 ms | 745 (5 × 149) |
| `ADKEX-512` | init_a | 327 | 50.3 ns | 1.99e+07 | 50.6 ns | 745 (5 × 149) |
| `ADKEX-512` | init_b | 2.04 M | 1.03 ms | 973 | 976 µs | 745 (5 × 149) |
| `ADKEX-512` | pass1 | 4.18 M | 1.99 ms | 501 | 1.99 ms | 745 (5 × 149) |
| `ADKEX-512` | pass2 | 4.94 M | 2.36 ms | 424 | 2.36 ms | 745 (5 × 149) |
| `ADKEX-512` | derive_a | 2.82 M | 1.39 ms | 717 | 1.35 ms | 745 (5 × 149) |
| `ADKEX-512` | derive_b | 11.8 k | 5.54 µs | 1.81e+05 | 5.53 µs | 745 (5 × 149) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `ADKEX-128` | exchange | 34525 | 1740 KiB | 1832 KiB |
| `ADKEX-128` | init_a | 34525 | 1764 KiB | 1836 KiB |
| `ADKEX-128` | init_b | 34525 | 1716 KiB | 1844 KiB |
| `ADKEX-128` | pass1 | 34525 | 1704 KiB | 1832 KiB |
| `ADKEX-128` | pass2 | 34525 | 1768 KiB | 1856 KiB |
| `ADKEX-128` | derive_a | 34525 | 1768 KiB | 1848 KiB |
| `ADKEX-128` | derive_b | 34525 | 1760 KiB | 1844 KiB |
| `ADKEX-256` | exchange | 35589 | 1788 KiB | 1912 KiB |
| `ADKEX-256` | init_a | 35589 | 1808 KiB | 1872 KiB |
| `ADKEX-256` | init_b | 35589 | 1820 KiB | 1884 KiB |
| `ADKEX-256` | pass1 | 35589 | 1824 KiB | 1888 KiB |
| `ADKEX-256` | pass2 | 35589 | 1804 KiB | 1868 KiB |
| `ADKEX-256` | derive_a | 35589 | 1812 KiB | 1908 KiB |
| `ADKEX-256` | derive_b | 35589 | 1824 KiB | 1908 KiB |
| `ADKEX-512` | exchange | 36957 | 1944 KiB | 2020 KiB |
| `ADKEX-512` | init_a | 36957 | 1948 KiB | 2024 KiB |
| `ADKEX-512` | init_b | 36957 | 1912 KiB | 2036 KiB |
| `ADKEX-512` | pass1 | 36957 | 1944 KiB | 2036 KiB |
| `ADKEX-512` | pass2 | 36957 | 1908 KiB | 2044 KiB |
| `ADKEX-512` | derive_a | 36957 | 1932 KiB | 2008 KiB |
| `ADKEX-512` | derive_b | 36957 | 1920 KiB | 2028 KiB |

## 6. Transmission and storage overhead

Bandwidth counts all specified protocol messages and each required public key once. Public keys are transmitted bytes too. Certificates and transport framing are excluded. The published raw timing records are unchanged.

Only the responder's long-term public key is required; this exchange does not authenticate the initiator as a mutually authenticated protocol would.

| instance | passes | messages (bytes; raw API) | protocol-message bytes | public key A / B | bandwidth (bytes) | long-term sk (API cap) | shared secret |
|---|---|---|---|---|---|---|---|
| `ADKEX-128` | 2 | 1600 / 800 | 2400 | 0 / 800 | 3200 | 1600 | 32 |
| `ADKEX-256` | 2 | 3168 / 1600 | 4768 | 0 / 1568 | 6336 | 3136 | 32 |
| `ADKEX-512` | 2 | 6528 / 3136 | 9664 | 0 / 3392 | 13056 | 6784 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — default DKE_HASH=0; own SM3/HMAC compiled but unreachable; DKE_HASH=2 would switch to SHAKE

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `ADKEX-128` | exchange | 63% | 1.2% | drng 3, pseudoXOF 6, sm3hash 563 |
| `ADKEX-256` | exchange | 69% | 0.5% | drng 3, pseudoXOF 6, sm3hash 1.76e+03 |
| `ADKEX-512` | exchange | 81% | 0.2% | drng 3, pseudoXOF 10, pseudohash 6, sm3hash 3.7e+03 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `ADKEX-128` | KAT log (sha256 `230169395b7403b6…`) | `kat/kex-01/ADKEX-128.log` |
| `ADKEX-128` | timing derive_a | `records/kex-01/ADKEX-128__derive_a.json` |
| `ADKEX-128` | timing derive_b | `records/kex-01/ADKEX-128__derive_b.json` |
| `ADKEX-128` | timing exchange | `records/kex-01/ADKEX-128__exchange.json` |
| `ADKEX-128` | timing init_a | `records/kex-01/ADKEX-128__init_a.json` |
| `ADKEX-128` | timing init_b | `records/kex-01/ADKEX-128__init_b.json` |
| `ADKEX-128` | timing pass1 | `records/kex-01/ADKEX-128__pass1.json` |
| `ADKEX-128` | timing pass2 | `records/kex-01/ADKEX-128__pass2.json` |
| `ADKEX-128` | hash profile exchange | `profile/kex-01/ADKEX-128__exchange.json` |
| `ADKEX-256` | KAT log (sha256 `ccb2baf2aa90ea48…`) | `kat/kex-01/ADKEX-256.log` |
| `ADKEX-256` | timing derive_a | `records/kex-01/ADKEX-256__derive_a.json` |
| `ADKEX-256` | timing derive_b | `records/kex-01/ADKEX-256__derive_b.json` |
| `ADKEX-256` | timing exchange | `records/kex-01/ADKEX-256__exchange.json` |
| `ADKEX-256` | timing init_a | `records/kex-01/ADKEX-256__init_a.json` |
| `ADKEX-256` | timing init_b | `records/kex-01/ADKEX-256__init_b.json` |
| `ADKEX-256` | timing pass1 | `records/kex-01/ADKEX-256__pass1.json` |
| `ADKEX-256` | timing pass2 | `records/kex-01/ADKEX-256__pass2.json` |
| `ADKEX-256` | hash profile exchange | `profile/kex-01/ADKEX-256__exchange.json` |
| `ADKEX-512` | KAT log (sha256 `437526980a75426f…`) | `kat/kex-01/ADKEX-512.log` |
| `ADKEX-512` | timing derive_a | `records/kex-01/ADKEX-512__derive_a.json` |
| `ADKEX-512` | timing derive_b | `records/kex-01/ADKEX-512__derive_b.json` |
| `ADKEX-512` | timing exchange | `records/kex-01/ADKEX-512__exchange.json` |
| `ADKEX-512` | timing init_a | `records/kex-01/ADKEX-512__init_a.json` |
| `ADKEX-512` | timing init_b | `records/kex-01/ADKEX-512__init_b.json` |
| `ADKEX-512` | timing pass1 | `records/kex-01/ADKEX-512__pass1.json` |
| `ADKEX-512` | timing pass2 | `records/kex-01/ADKEX-512__pass2.json` |
| `ADKEX-512` | hash profile exchange | `profile/kex-01/ADKEX-512__exchange.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

