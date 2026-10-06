# kem-06 BRA — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-06` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560843625385984.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: BRA
- Implementation versions measured: reference
- Parameter sets: `BRA-128`, `BRA-256`, `BRA-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-06/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `BRA-128` | guide | PASS |
| `BRA-256` | guide | PASS |
| `BRA-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `BRA-128` | keygen | 2.04 M | 976 µs | 1.02e+03 | 976 µs | 4890 (5 × 978) |
| `BRA-128` | enc | 4.52 M | 2.16 ms | 463 | 2.16 ms | 2295 (5 × 459) |
| `BRA-128` | dec | 50.59 M | 24.2 ms | 41.4 | 24.2 ms | 210 (5 × 42) |
| `BRA-256` | keygen | 4.37 M | 2.09 ms | 479 | 2.09 ms | 2345 (5 × 469) |
| `BRA-256` | enc | 9.38 M | 4.48 ms | 223 | 4.48 ms | 1110 (5 × 222) |
| `BRA-256` | dec | 102.18 M | 48.8 ms | 20.5 | 48.8 ms | 105 (5 × 21) |
| `BRA-512` | keygen | 8.37 M | 4 ms | 250 | 4 ms | 1235 (5 × 247) |
| `BRA-512` | enc | 18.55 M | 8.86 ms | 113 | 8.86 ms | 565 (5 × 113) |
| `BRA-512` | dec | 262.02 M | 125 ms | 7.99 | 125 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `BRA-128` | keygen | 137009 | 1808 KiB | 1904 KiB |
| `BRA-128` | enc | 137009 | 1828 KiB | 1940 KiB |
| `BRA-128` | dec | 137009 | 1844 KiB | 1944 KiB |
| `BRA-256` | keygen | 137105 | 1808 KiB | 1928 KiB |
| `BRA-256` | enc | 137105 | 1852 KiB | 1924 KiB |
| `BRA-256` | dec | 137105 | 1868 KiB | 1980 KiB |
| `BRA-512` | keygen | 137177 | 1796 KiB | 1948 KiB |
| `BRA-512` | enc | 137177 | 1908 KiB | 1980 KiB |
| `BRA-512` | dec | 137177 | 1928 KiB | 2004 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `BRA-128` | 1078 | 1176 | 2092 | 64 |
| `BRA-256` | 1735 | 1841 | 3406 | 64 |
| `BRA-512` | 3573 | 3717 | 7082 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — pseudohash for G/H; seed expansion via per-seed ICCS DRNG instances; XKCP Keccak dead

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `BRA-128` | keygen | 0.0% | 7.7% | drng 10 |
| `BRA-128` | enc | 4.5% | 3.8% | drng 11, pseudohash 2 |
| `BRA-128` | dec | 0.4% | 0.7% | drng 17, pseudohash 2 |
| `BRA-256` | keygen | 0.0% | 5.2% | drng 10.3 |
| `BRA-256` | enc | 3.4% | 2.6% | drng 11.4, pseudohash 2 |
| `BRA-256` | dec | 0.3% | 0.5% | drng 18, pseudohash 2 |
| `BRA-512` | keygen | 0.0% | 4.7% | drng 10 |
| `BRA-512` | enc | 3.4% | 2.3% | drng 11, pseudohash 2 |
| `BRA-512` | dec | 0.2% | 0.4% | drng 17, pseudohash 2 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `BRA-128` | KAT log (sha256 `83cfcc4b15427a33…`) | `kat/kem-06/BRA-128.log` |
| `BRA-128` | timing dec | `records/kem-06/BRA-128__dec.json` |
| `BRA-128` | timing enc | `records/kem-06/BRA-128__enc.json` |
| `BRA-128` | timing keygen | `records/kem-06/BRA-128__keygen.json` |
| `BRA-128` | hash profile dec | `profile/kem-06/BRA-128__dec.json` |
| `BRA-128` | hash profile enc | `profile/kem-06/BRA-128__enc.json` |
| `BRA-128` | hash profile keygen | `profile/kem-06/BRA-128__keygen.json` |
| `BRA-256` | KAT log (sha256 `ae1bdf0662bc64c2…`) | `kat/kem-06/BRA-256.log` |
| `BRA-256` | timing dec | `records/kem-06/BRA-256__dec.json` |
| `BRA-256` | timing enc | `records/kem-06/BRA-256__enc.json` |
| `BRA-256` | timing keygen | `records/kem-06/BRA-256__keygen.json` |
| `BRA-256` | hash profile dec | `profile/kem-06/BRA-256__dec.json` |
| `BRA-256` | hash profile enc | `profile/kem-06/BRA-256__enc.json` |
| `BRA-256` | hash profile keygen | `profile/kem-06/BRA-256__keygen.json` |
| `BRA-512` | KAT log (sha256 `23c014a046ebea39…`) | `kat/kem-06/BRA-512.log` |
| `BRA-512` | timing dec | `records/kem-06/BRA-512__dec.json` |
| `BRA-512` | timing enc | `records/kem-06/BRA-512__enc.json` |
| `BRA-512` | timing keygen | `records/kem-06/BRA-512__keygen.json` |
| `BRA-512` | hash profile dec | `profile/kem-06/BRA-512__dec.json` |
| `BRA-512` | hash profile enc | `profile/kem-06/BRA-512__enc.json` |
| `BRA-512` | hash profile keygen | `profile/kem-06/BRA-512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

