# kex-06 MAMBA-NIKE — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kex-06` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560625907453952.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key exchange
- Algorithm: MAMBA-NIKE
- Implementation versions measured: reference
- Parameter sets: `MAMBA-NIKE-128`, `MAMBA-NIKE-192`, `MAMBA-NIKE-256`, `MAMBA-NIKE-384`, `MAMBA-NIKE-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kex-06/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `MAMBA-NIKE-128` | guide | PASS |
| `MAMBA-NIKE-192` | guide | PASS |
| `MAMBA-NIKE-256` | guide | PASS |
| `MAMBA-NIKE-384` | guide | PASS |
| `MAMBA-NIKE-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `MAMBA-NIKE-128` | exchange | 6.09 M | 2.92 ms | 343 | 2.92 ms | 1760 (5 × 352) |
| `MAMBA-NIKE-128` | init_a | 1.29 M | 617 µs | 1.62e+03 | 616 µs | 1760 (5 × 352) |
| `MAMBA-NIKE-128` | init_b | 1.28 M | 615 µs | 1.63e+03 | 614 µs | 1760 (5 × 352) |
| `MAMBA-NIKE-128` | pass1 | 2.49 M | 1.19 ms | 837 | 1.19 ms | 1760 (5 × 352) |
| `MAMBA-NIKE-128` | derive_a | 4276 | 1.94 µs | 5.16e+05 | 1.95 µs | 1760 (5 × 352) |
| `MAMBA-NIKE-128` | derive_b | 1.30 M | 622 µs | 1.61e+03 | 621 µs | 1760 (5 × 352) |
| `MAMBA-NIKE-192` | exchange | 6.14 M | 2.94 ms | 340 | 2.93 ms | 1680 (5 × 336) |
| `MAMBA-NIKE-192` | init_a | 1.30 M | 622 µs | 1.61e+03 | 622 µs | 1680 (5 × 336) |
| `MAMBA-NIKE-192` | init_b | 1.29 M | 620 µs | 1.61e+03 | 620 µs | 1680 (5 × 336) |
| `MAMBA-NIKE-192` | pass1 | 2.53 M | 1.21 ms | 826 | 1.21 ms | 1680 (5 × 336) |
| `MAMBA-NIKE-192` | derive_a | 4231 | 1.92 µs | 5.2e+05 | 1.94 µs | 1680 (5 × 336) |
| `MAMBA-NIKE-192` | derive_b | 1.29 M | 619 µs | 1.61e+03 | 621 µs | 1680 (5 × 336) |
| `MAMBA-NIKE-256` | exchange | 6.20 M | 2.97 ms | 337 | 2.96 ms | 1785 (5 × 357) |
| `MAMBA-NIKE-256` | init_a | 1.31 M | 658 µs | 1.52e+03 | 628 µs | 1785 (5 × 357) |
| `MAMBA-NIKE-256` | init_b | 1.32 M | 633 µs | 1.58e+03 | 633 µs | 1785 (5 × 357) |
| `MAMBA-NIKE-256` | pass1 | 2.54 M | 1.22 ms | 821 | 1.22 ms | 1785 (5 × 357) |
| `MAMBA-NIKE-256` | derive_a | 4196 | 1.9 µs | 5.25e+05 | 1.9 µs | 1785 (5 × 357) |
| `MAMBA-NIKE-256` | derive_b | 1.30 M | 623 µs | 1.61e+03 | 622 µs | 1785 (5 × 357) |
| `MAMBA-NIKE-384` | exchange | 12.94 M | 6.2 ms | 161 | 6.19 ms | 810 (5 × 162) |
| `MAMBA-NIKE-384` | init_a | 2.62 M | 1.25 ms | 797 | 1.26 ms | 810 (5 × 162) |
| `MAMBA-NIKE-384` | init_b | 2.67 M | 1.28 ms | 781 | 1.28 ms | 810 (5 × 162) |
| `MAMBA-NIKE-384` | pass1 | 5.31 M | 2.54 ms | 393 | 2.55 ms | 810 (5 × 162) |
| `MAMBA-NIKE-384` | derive_a | 7858 | 3.61 µs | 2.77e+05 | 3.63 µs | 810 (5 × 162) |
| `MAMBA-NIKE-384` | derive_b | 2.74 M | 1.35 ms | 741 | 1.31 ms | 810 (5 × 162) |
| `MAMBA-NIKE-512` | exchange | 13.11 M | 6.28 ms | 159 | 6.29 ms | 800 (5 × 160) |
| `MAMBA-NIKE-512` | init_a | 2.73 M | 1.31 ms | 764 | 1.31 ms | 800 (5 × 160) |
| `MAMBA-NIKE-512` | init_b | 2.73 M | 1.31 ms | 764 | 1.31 ms | 800 (5 × 160) |
| `MAMBA-NIKE-512` | pass1 | 5.35 M | 2.56 ms | 390 | 2.56 ms | 800 (5 × 160) |
| `MAMBA-NIKE-512` | derive_a | 7819 | 3.6 µs | 2.78e+05 | 3.58 µs | 800 (5 × 160) |
| `MAMBA-NIKE-512` | derive_b | 2.74 M | 1.32 ms | 760 | 1.31 ms | 800 (5 × 160) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `MAMBA-NIKE-128` | exchange | 43901 | 2060 KiB | 2132 KiB |
| `MAMBA-NIKE-128` | init_a | 43901 | 2052 KiB | 2124 KiB |
| `MAMBA-NIKE-128` | init_b | 43901 | 2060 KiB | 2148 KiB |
| `MAMBA-NIKE-128` | pass1 | 43901 | 2048 KiB | 2148 KiB |
| `MAMBA-NIKE-128` | derive_a | 43901 | 2032 KiB | 2132 KiB |
| `MAMBA-NIKE-128` | derive_b | 43901 | 2060 KiB | 2144 KiB |
| `MAMBA-NIKE-192` | exchange | 43885 | 2028 KiB | 2156 KiB |
| `MAMBA-NIKE-192` | init_a | 43885 | 2052 KiB | 2136 KiB |
| `MAMBA-NIKE-192` | init_b | 43885 | 2040 KiB | 2136 KiB |
| `MAMBA-NIKE-192` | pass1 | 43885 | 2044 KiB | 2136 KiB |
| `MAMBA-NIKE-192` | derive_a | 43885 | 2044 KiB | 2156 KiB |
| `MAMBA-NIKE-192` | derive_b | 43885 | 2036 KiB | 2156 KiB |
| `MAMBA-NIKE-256` | exchange | 43949 | 2044 KiB | 2112 KiB |
| `MAMBA-NIKE-256` | init_a | 43949 | 2048 KiB | 2116 KiB |
| `MAMBA-NIKE-256` | init_b | 43949 | 2064 KiB | 2148 KiB |
| `MAMBA-NIKE-256` | pass1 | 43949 | 2048 KiB | 2136 KiB |
| `MAMBA-NIKE-256` | derive_a | 43949 | 2056 KiB | 2124 KiB |
| `MAMBA-NIKE-256` | derive_b | 43949 | 2040 KiB | 2144 KiB |
| `MAMBA-NIKE-384` | exchange | 43885 | 2408 KiB | 2480 KiB |
| `MAMBA-NIKE-384` | init_a | 43885 | 2392 KiB | 2500 KiB |
| `MAMBA-NIKE-384` | init_b | 43885 | 2400 KiB | 2484 KiB |
| `MAMBA-NIKE-384` | pass1 | 43885 | 2408 KiB | 2492 KiB |
| `MAMBA-NIKE-384` | derive_a | 43885 | 2404 KiB | 2492 KiB |
| `MAMBA-NIKE-384` | derive_b | 43885 | 2396 KiB | 2464 KiB |
| `MAMBA-NIKE-512` | exchange | 43949 | 2380 KiB | 2452 KiB |
| `MAMBA-NIKE-512` | init_a | 43949 | 2392 KiB | 2500 KiB |
| `MAMBA-NIKE-512` | init_b | 43949 | 2312 KiB | 2448 KiB |
| `MAMBA-NIKE-512` | pass1 | 43949 | 2408 KiB | 2496 KiB |
| `MAMBA-NIKE-512` | derive_a | 43949 | 2400 KiB | 2500 KiB |
| `MAMBA-NIKE-512` | derive_b | 43949 | 2388 KiB | 2508 KiB |

## 6. Transmission and storage overhead

Bandwidth counts all specified protocol messages and each required public key once. Public keys are transmitted bytes too. Certificates and transport framing are excluded. The published raw timing records are unchanged.

Only the responder's long-term public key is required; this exchange does not authenticate the initiator as a mutually authenticated protocol would.

| instance | passes | messages (bytes; raw API) | protocol-message bytes | public key A / B | bandwidth (bytes) | long-term sk (API cap) | shared secret |
|---|---|---|---|---|---|---|---|
| `MAMBA-NIKE-128` | 1 | 1568 | 1568 | 0 / 1184 | 2752 | 3232 | 32 |
| `MAMBA-NIKE-192` | 1 | 1568 | 1568 | 0 / 1312 | 2880 | 3360 | 32 |
| `MAMBA-NIKE-256` | 1 | 1568 | 1568 | 0 / 1312 | 2880 | 3360 | 32 |
| `MAMBA-NIKE-384` | 1 | 3360 | 3360 | 0 / 2848 | 6208 | 6944 | 48 |
| `MAMBA-NIKE-512` | 1 | 3360 | 3360 | 0 / 2848 | 6208 | 6944 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **bypass** — SHAKE128/256 + ChaCha20; DRNG only with -DKAT_BUILD, otherwise /dev/urandom

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `MAMBA-NIKE-128` | exchange | 0.0% | 0.5% | drng 6 |
| `MAMBA-NIKE-192` | exchange | 0.0% | 0.5% | drng 6 |
| `MAMBA-NIKE-256` | exchange | 0.0% | 0.5% | drng 6 |
| `MAMBA-NIKE-384` | exchange | 0.0% | 0.2% | drng 6 |
| `MAMBA-NIKE-512` | exchange | 0.0% | 0.2% | drng 6 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `MAMBA-NIKE-128` | KAT log (sha256 `0228be5a1b75f7e7…`) | `kat/kex-06/MAMBA-NIKE-128.log` |
| `MAMBA-NIKE-128` | timing derive_a | `records/kex-06/MAMBA-NIKE-128__derive_a.json` |
| `MAMBA-NIKE-128` | timing derive_b | `records/kex-06/MAMBA-NIKE-128__derive_b.json` |
| `MAMBA-NIKE-128` | timing exchange | `records/kex-06/MAMBA-NIKE-128__exchange.json` |
| `MAMBA-NIKE-128` | timing init_a | `records/kex-06/MAMBA-NIKE-128__init_a.json` |
| `MAMBA-NIKE-128` | timing init_b | `records/kex-06/MAMBA-NIKE-128__init_b.json` |
| `MAMBA-NIKE-128` | timing pass1 | `records/kex-06/MAMBA-NIKE-128__pass1.json` |
| `MAMBA-NIKE-128` | hash profile exchange | `profile/kex-06/MAMBA-NIKE-128__exchange.json` |
| `MAMBA-NIKE-192` | KAT log (sha256 `c0ca4648299977a1…`) | `kat/kex-06/MAMBA-NIKE-192.log` |
| `MAMBA-NIKE-192` | timing derive_a | `records/kex-06/MAMBA-NIKE-192__derive_a.json` |
| `MAMBA-NIKE-192` | timing derive_b | `records/kex-06/MAMBA-NIKE-192__derive_b.json` |
| `MAMBA-NIKE-192` | timing exchange | `records/kex-06/MAMBA-NIKE-192__exchange.json` |
| `MAMBA-NIKE-192` | timing init_a | `records/kex-06/MAMBA-NIKE-192__init_a.json` |
| `MAMBA-NIKE-192` | timing init_b | `records/kex-06/MAMBA-NIKE-192__init_b.json` |
| `MAMBA-NIKE-192` | timing pass1 | `records/kex-06/MAMBA-NIKE-192__pass1.json` |
| `MAMBA-NIKE-192` | hash profile exchange | `profile/kex-06/MAMBA-NIKE-192__exchange.json` |
| `MAMBA-NIKE-256` | KAT log (sha256 `f92ef0f1f12d2904…`) | `kat/kex-06/MAMBA-NIKE-256.log` |
| `MAMBA-NIKE-256` | timing derive_a | `records/kex-06/MAMBA-NIKE-256__derive_a.json` |
| `MAMBA-NIKE-256` | timing derive_b | `records/kex-06/MAMBA-NIKE-256__derive_b.json` |
| `MAMBA-NIKE-256` | timing exchange | `records/kex-06/MAMBA-NIKE-256__exchange.json` |
| `MAMBA-NIKE-256` | timing init_a | `records/kex-06/MAMBA-NIKE-256__init_a.json` |
| `MAMBA-NIKE-256` | timing init_b | `records/kex-06/MAMBA-NIKE-256__init_b.json` |
| `MAMBA-NIKE-256` | timing pass1 | `records/kex-06/MAMBA-NIKE-256__pass1.json` |
| `MAMBA-NIKE-256` | hash profile exchange | `profile/kex-06/MAMBA-NIKE-256__exchange.json` |
| `MAMBA-NIKE-384` | KAT log (sha256 `a543699cc51547e8…`) | `kat/kex-06/MAMBA-NIKE-384.log` |
| `MAMBA-NIKE-384` | timing derive_a | `records/kex-06/MAMBA-NIKE-384__derive_a.json` |
| `MAMBA-NIKE-384` | timing derive_b | `records/kex-06/MAMBA-NIKE-384__derive_b.json` |
| `MAMBA-NIKE-384` | timing exchange | `records/kex-06/MAMBA-NIKE-384__exchange.json` |
| `MAMBA-NIKE-384` | timing init_a | `records/kex-06/MAMBA-NIKE-384__init_a.json` |
| `MAMBA-NIKE-384` | timing init_b | `records/kex-06/MAMBA-NIKE-384__init_b.json` |
| `MAMBA-NIKE-384` | timing pass1 | `records/kex-06/MAMBA-NIKE-384__pass1.json` |
| `MAMBA-NIKE-384` | hash profile exchange | `profile/kex-06/MAMBA-NIKE-384__exchange.json` |
| `MAMBA-NIKE-512` | KAT log (sha256 `e5689c47200de2c0…`) | `kat/kex-06/MAMBA-NIKE-512.log` |
| `MAMBA-NIKE-512` | timing derive_a | `records/kex-06/MAMBA-NIKE-512__derive_a.json` |
| `MAMBA-NIKE-512` | timing derive_b | `records/kex-06/MAMBA-NIKE-512__derive_b.json` |
| `MAMBA-NIKE-512` | timing exchange | `records/kex-06/MAMBA-NIKE-512__exchange.json` |
| `MAMBA-NIKE-512` | timing init_a | `records/kex-06/MAMBA-NIKE-512__init_a.json` |
| `MAMBA-NIKE-512` | timing init_b | `records/kex-06/MAMBA-NIKE-512__init_b.json` |
| `MAMBA-NIKE-512` | timing pass1 | `records/kex-06/MAMBA-NIKE-512__pass1.json` |
| `MAMBA-NIKE-512` | hash profile exchange | `profile/kex-06/MAMBA-NIKE-512__exchange.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

