# kex-09 TriQ-KEX — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kex-09` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560626335272960.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key exchange
- Algorithm: TriQ-KEX
- Implementation versions measured: reference
- Parameter sets: `TriQ-KEX-128`, `TriQ-KEX-256`, `TriQ-KEX-384`, `TriQ-KEX-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kex-09/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `TriQ-KEX-128` | guide | PASS |
| `TriQ-KEX-256` | guide | PASS |
| `TriQ-KEX-384` | guide | PASS |
| `TriQ-KEX-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `TriQ-KEX-128` | exchange | 60.66 M | 29 ms | 34.5 | 29 ms | 175 (5 × 35) |
| `TriQ-KEX-128` | init_a | 3.61 M | 1.73 ms | 579 | 1.73 ms | 175 (5 × 35) |
| `TriQ-KEX-128` | init_b | 3.62 M | 1.73 ms | 578 | 1.73 ms | 175 (5 × 35) |
| `TriQ-KEX-128` | pass1 | 10.80 M | 5.17 ms | 194 | 5.16 ms | 175 (5 × 35) |
| `TriQ-KEX-128` | pass2 | 25.42 M | 12.1 ms | 82.4 | 12.1 ms | 175 (5 × 35) |
| `TriQ-KEX-128` | derive_a | 15.99 M | 7.64 ms | 131 | 7.64 ms | 175 (5 × 35) |
| `TriQ-KEX-128` | derive_b | 1.17 M | 577 µs | 1.73e+03 | 562 µs | 175 (5 × 35) |
| `TriQ-KEX-256` | exchange | 343.23 M | 165 ms | 6.05 | 164 ms | 100 (5 × 20) |
| `TriQ-KEX-256` | init_a | 21.00 M | 10.2 ms | 98.5 | 10 ms | 100 (5 × 20) |
| `TriQ-KEX-256` | init_b | 21.00 M | 10 ms | 99.6 | 10 ms | 100 (5 × 20) |
| `TriQ-KEX-256` | pass1 | 62.90 M | 30.1 ms | 33.3 | 30.1 ms | 100 (5 × 20) |
| `TriQ-KEX-256` | pass2 | 146.90 M | 70.8 ms | 14.1 | 70.2 ms | 100 (5 × 20) |
| `TriQ-KEX-256` | derive_a | 87.77 M | 42.4 ms | 23.6 | 42 ms | 100 (5 × 20) |
| `TriQ-KEX-256` | derive_b | 3.64 M | 1.76 ms | 568 | 1.76 ms | 100 (5 × 20) |
| `TriQ-KEX-384` | exchange | 934.75 M | 450 ms | 2.22 | 447 ms | 100 (5 × 20) |
| `TriQ-KEX-384` | init_a | 56.52 M | 27 ms | 37 | 27 ms | 100 (5 × 20) |
| `TriQ-KEX-384` | init_b | 56.52 M | 27 ms | 37 | 27 ms | 100 (5 × 20) |
| `TriQ-KEX-384` | pass1 | 169.54 M | 81.6 ms | 12.3 | 81 ms | 100 (5 × 20) |
| `TriQ-KEX-384` | pass2 | 396.78 M | 191 ms | 5.25 | 190 ms | 100 (5 × 20) |
| `TriQ-KEX-384` | derive_a | 241.64 M | 116 ms | 8.64 | 116 ms | 100 (5 × 20) |
| `TriQ-KEX-384` | derive_b | 14.14 M | 6.94 ms | 144 | 6.92 ms | 100 (5 × 20) |
| `TriQ-KEX-512` | exchange | 1.96 G | 943 ms | 1.06 | 945 ms | 100 (5 × 20) |
| `TriQ-KEX-512` | init_a | 120.15 M | 57.6 ms | 17.4 | 57.4 ms | 100 (5 × 20) |
| `TriQ-KEX-512` | init_b | 120.27 M | 57.5 ms | 17.4 | 57.4 ms | 100 (5 × 20) |
| `TriQ-KEX-512` | pass1 | 359.70 M | 172 ms | 5.82 | 172 ms | 100 (5 × 20) |
| `TriQ-KEX-512` | pass2 | 838.72 M | 403 ms | 2.48 | 401 ms | 100 (5 × 20) |
| `TriQ-KEX-512` | derive_a | 502.09 M | 243 ms | 4.12 | 240 ms | 100 (5 × 20) |
| `TriQ-KEX-512` | derive_b | 22.91 M | 11.2 ms | 89.4 | 11.2 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `TriQ-KEX-128` | exchange | 47069 | 2080 KiB | 2188 KiB |
| `TriQ-KEX-128` | init_a | 47069 | 2056 KiB | 2176 KiB |
| `TriQ-KEX-128` | init_b | 47069 | 2068 KiB | 2148 KiB |
| `TriQ-KEX-128` | pass1 | 47069 | 2064 KiB | 2144 KiB |
| `TriQ-KEX-128` | pass2 | 47069 | 2056 KiB | 2184 KiB |
| `TriQ-KEX-128` | derive_a | 47069 | 2056 KiB | 2136 KiB |
| `TriQ-KEX-128` | derive_b | 47069 | 2052 KiB | 2132 KiB |
| `TriQ-KEX-256` | exchange | 50677 | 2660 KiB | 2792 KiB |
| `TriQ-KEX-256` | init_a | 50677 | 2684 KiB | 2800 KiB |
| `TriQ-KEX-256` | init_b | 50677 | 2712 KiB | 2804 KiB |
| `TriQ-KEX-256` | pass1 | 50677 | 2684 KiB | 2752 KiB |
| `TriQ-KEX-256` | pass2 | 50677 | 2588 KiB | 2716 KiB |
| `TriQ-KEX-256` | derive_a | 50677 | 2684 KiB | 2800 KiB |
| `TriQ-KEX-256` | derive_b | 50677 | 2708 KiB | 2776 KiB |
| `TriQ-KEX-384` | exchange | 59797 | 3432 KiB | 3592 KiB |
| `TriQ-KEX-384` | init_a | 59797 | 3440 KiB | 3600 KiB |
| `TriQ-KEX-384` | init_b | 59797 | 3424 KiB | 3580 KiB |
| `TriQ-KEX-384` | pass1 | 59797 | 3416 KiB | 3552 KiB |
| `TriQ-KEX-384` | pass2 | 59797 | 3440 KiB | 3604 KiB |
| `TriQ-KEX-384` | derive_a | 59797 | 3444 KiB | 3600 KiB |
| `TriQ-KEX-384` | derive_b | 59797 | 3428 KiB | 3520 KiB |
| `TriQ-KEX-512` | exchange | 67393 | 3008 KiB | 3140 KiB |
| `TriQ-KEX-512` | init_a | 67393 | 3028 KiB | 3220 KiB |
| `TriQ-KEX-512` | init_b | 67393 | 3008 KiB | 3200 KiB |
| `TriQ-KEX-512` | pass1 | 67393 | 3008 KiB | 3216 KiB |
| `TriQ-KEX-512` | pass2 | 67393 | 2992 KiB | 3224 KiB |
| `TriQ-KEX-512` | derive_a | 67393 | 2996 KiB | 3152 KiB |
| `TriQ-KEX-512` | derive_b | 67393 | 3020 KiB | 3236 KiB |

## 6. Transmission and storage overhead

Bandwidth counts all specified protocol messages and each required public key once. Public keys are transmitted bytes too. Certificates and transport framing are excluded. The published raw timing records are unchanged.

| instance | passes | messages (bytes; raw API) | protocol-message bytes | public key A / B | bandwidth (bytes) | long-term sk (API cap) | shared secret |
|---|---|---|---|---|---|---|---|
| `TriQ-KEX-128` | 2 | 6148 / 8180 | 14328 | 2054 / 2054 | 18436 | 2134 | 16 |
| `TriQ-KEX-256` | 2 | 18808 / 24952 | 43760 | 6328 / 6328 | 56416 | 6488 | 32 |
| `TriQ-KEX-384` | 2 | 36598 / 48678 | 85276 | 12255 / 12255 | 109786 | 12495 | 48 |
| `TriQ-KEX-512` | 2 | 59096 / 78648 | 137744 | 19768 / 19768 | 177280 | 20088 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `TriQ-KEX-128` | exchange | 7.0% | 0.1% | drng 14, pseudoXOF 74.8 |
| `TriQ-KEX-256` | exchange | 3.6% | 0.0% | drng 14, pseudoXOF 65.4, pseudohash 9 |
| `TriQ-KEX-384` | exchange | 4.4% | 0.0% | drng 14, pseudoXOF 70.7, pseudohash 9 |
| `TriQ-KEX-512` | exchange | 3.8% | 0.0% | drng 14, pseudoXOF 63, pseudohash 17 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `TriQ-KEX-128` | KAT log (sha256 `5b85598ad4855dd5…`) | `kat/kex-09/TriQ-KEX-128.log` |
| `TriQ-KEX-128` | timing derive_a | `records/kex-09/TriQ-KEX-128__derive_a.json` |
| `TriQ-KEX-128` | timing derive_b | `records/kex-09/TriQ-KEX-128__derive_b.json` |
| `TriQ-KEX-128` | timing exchange | `records/kex-09/TriQ-KEX-128__exchange.json` |
| `TriQ-KEX-128` | timing init_a | `records/kex-09/TriQ-KEX-128__init_a.json` |
| `TriQ-KEX-128` | timing init_b | `records/kex-09/TriQ-KEX-128__init_b.json` |
| `TriQ-KEX-128` | timing pass1 | `records/kex-09/TriQ-KEX-128__pass1.json` |
| `TriQ-KEX-128` | timing pass2 | `records/kex-09/TriQ-KEX-128__pass2.json` |
| `TriQ-KEX-128` | hash profile exchange | `profile/kex-09/TriQ-KEX-128__exchange.json` |
| `TriQ-KEX-256` | KAT log (sha256 `671cd82218afaac3…`) | `kat/kex-09/TriQ-KEX-256.log` |
| `TriQ-KEX-256` | timing derive_a | `records/kex-09/TriQ-KEX-256__derive_a.json` |
| `TriQ-KEX-256` | timing derive_b | `records/kex-09/TriQ-KEX-256__derive_b.json` |
| `TriQ-KEX-256` | timing exchange | `records/kex-09/TriQ-KEX-256__exchange.json` |
| `TriQ-KEX-256` | timing init_a | `records/kex-09/TriQ-KEX-256__init_a.json` |
| `TriQ-KEX-256` | timing init_b | `records/kex-09/TriQ-KEX-256__init_b.json` |
| `TriQ-KEX-256` | timing pass1 | `records/kex-09/TriQ-KEX-256__pass1.json` |
| `TriQ-KEX-256` | timing pass2 | `records/kex-09/TriQ-KEX-256__pass2.json` |
| `TriQ-KEX-256` | hash profile exchange | `profile/kex-09/TriQ-KEX-256__exchange.json` |
| `TriQ-KEX-384` | KAT log (sha256 `6ec4d0ea4abfe2cc…`) | `kat/kex-09/TriQ-KEX-384.log` |
| `TriQ-KEX-384` | timing derive_a | `records/kex-09/TriQ-KEX-384__derive_a.json` |
| `TriQ-KEX-384` | timing derive_b | `records/kex-09/TriQ-KEX-384__derive_b.json` |
| `TriQ-KEX-384` | timing exchange | `records/kex-09/TriQ-KEX-384__exchange.json` |
| `TriQ-KEX-384` | timing init_a | `records/kex-09/TriQ-KEX-384__init_a.json` |
| `TriQ-KEX-384` | timing init_b | `records/kex-09/TriQ-KEX-384__init_b.json` |
| `TriQ-KEX-384` | timing pass1 | `records/kex-09/TriQ-KEX-384__pass1.json` |
| `TriQ-KEX-384` | timing pass2 | `records/kex-09/TriQ-KEX-384__pass2.json` |
| `TriQ-KEX-384` | hash profile exchange | `profile/kex-09/TriQ-KEX-384__exchange.json` |
| `TriQ-KEX-512` | KAT log (sha256 `e61678802e93fa99…`) | `kat/kex-09/TriQ-KEX-512.log` |
| `TriQ-KEX-512` | timing derive_a | `records/kex-09/TriQ-KEX-512__derive_a.json` |
| `TriQ-KEX-512` | timing derive_b | `records/kex-09/TriQ-KEX-512__derive_b.json` |
| `TriQ-KEX-512` | timing exchange | `records/kex-09/TriQ-KEX-512__exchange.json` |
| `TriQ-KEX-512` | timing init_a | `records/kex-09/TriQ-KEX-512__init_a.json` |
| `TriQ-KEX-512` | timing init_b | `records/kex-09/TriQ-KEX-512__init_b.json` |
| `TriQ-KEX-512` | timing pass1 | `records/kex-09/TriQ-KEX-512__pass1.json` |
| `TriQ-KEX-512` | timing pass2 | `records/kex-09/TriQ-KEX-512__pass2.json` |
| `TriQ-KEX-512` | hash profile exchange | `profile/kex-09/TriQ-KEX-512__exchange.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

