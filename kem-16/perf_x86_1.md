# kem-16 HARE — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-16` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560844942397440.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: HARE
- Implementation versions measured: reference
- Parameter sets: `HARE-128-kr`, `HARE-256-kr`, `HARE-384-kr`, `HARE-512-kr`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-16/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `HARE-128-kr` | guide | PASS |
| `HARE-256-kr` | guide | PASS |
| `HARE-384-kr` | guide | PASS |
| `HARE-512-kr` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `HARE-128-kr` | keygen | 5.89 M | 2.82 ms | 355 | 2.82 ms | 1740 (5 × 348) |
| `HARE-128-kr` | enc | 12.93 M | 6.18 ms | 162 | 6.18 ms | 810 (5 × 162) |
| `HARE-128-kr` | dec | 19.48 M | 9.3 ms | 107 | 9.3 ms | 535 (5 × 107) |
| `HARE-256-kr` | keygen | 26.13 M | 12.5 ms | 80.1 | 12.5 ms | 400 (5 × 80) |
| `HARE-256-kr` | enc | 55.20 M | 26.4 ms | 37.9 | 26.4 ms | 190 (5 × 38) |
| `HARE-256-kr` | dec | 84.79 M | 40.5 ms | 24.7 | 40.5 ms | 125 (5 × 25) |
| `HARE-384-kr` | keygen | 78.03 M | 37.3 ms | 26.8 | 37.3 ms | 135 (5 × 27) |
| `HARE-384-kr` | enc | 161.95 M | 77.4 ms | 12.9 | 77.4 ms | 100 (5 × 20) |
| `HARE-384-kr` | dec | 244.38 M | 117 ms | 8.56 | 117 ms | 100 (5 × 20) |
| `HARE-512-kr` | keygen | 166.96 M | 79.8 ms | 12.5 | 79.8 ms | 100 (5 × 20) |
| `HARE-512-kr` | enc | 342.73 M | 164 ms | 6.11 | 164 ms | 100 (5 × 20) |
| `HARE-512-kr` | dec | 519.62 M | 250 ms | 4.01 | 248 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `HARE-128-kr` | keygen | 50013 | 1724 KiB | 1852 KiB |
| `HARE-128-kr` | enc | 50013 | 1804 KiB | 1872 KiB |
| `HARE-128-kr` | dec | 50013 | 1824 KiB | 1892 KiB |
| `HARE-256-kr` | keygen | 50661 | 1748 KiB | 1992 KiB |
| `HARE-256-kr` | enc | 50661 | 1928 KiB | 2028 KiB |
| `HARE-256-kr` | dec | 50661 | 1960 KiB | 2048 KiB |
| `HARE-384-kr` | keygen | 51013 | 1748 KiB | 2096 KiB |
| `HARE-384-kr` | enc | 51013 | 2124 KiB | 2196 KiB |
| `HARE-384-kr` | dec | 51013 | 2204 KiB | 2272 KiB |
| `HARE-512-kr` | keygen | 51917 | 1804 KiB | 2336 KiB |
| `HARE-512-kr` | enc | 51917 | 2376 KiB | 2448 KiB |
| `HARE-512-kr` | dec | 51917 | 2540 KiB | 2624 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `HARE-128-kr` | 2629 | 2677 | 4688 | 16 |
| `HARE-256-kr` | 6580 | 6676 | 11790 | 32 |
| `HARE-384-kr` | 13157 | 13301 | 23693 | 48 |
| `HARE-512-kr` | 21812 | 22004 | 39294 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — pseudoXOF only (recomputes prefix per call); lives under _shared/

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `HARE-128-kr` | keygen | 3.0% | 0.1% | drng 1, pseudoXOF 6.41 |
| `HARE-128-kr` | enc | 2.3% | 0.1% | drng 2, pseudoXOF 6 |
| `HARE-128-kr` | dec | 2.1% | 0.0% | pseudoXOF 8 |
| `HARE-256-kr` | keygen | 1.5% | 0.0% | drng 1, pseudoXOF 6.47 |
| `HARE-256-kr` | enc | 1.1% | 0.0% | drng 2, pseudoXOF 6 |
| `HARE-256-kr` | dec | 1.1% | 0.0% | pseudoXOF 8 |
| `HARE-384-kr` | keygen | 1.1% | 0.0% | drng 1, pseudoXOF 7.52 |
| `HARE-384-kr` | enc | 0.9% | 0.0% | drng 2, pseudoXOF 6 |
| `HARE-384-kr` | dec | 1.0% | 0.0% | pseudoXOF 9 |
| `HARE-512-kr` | keygen | 1.5% | 0.0% | drng 1, pseudoXOF 7.54 |
| `HARE-512-kr` | enc | 1.0% | 0.0% | drng 2, pseudoXOF 6 |
| `HARE-512-kr` | dec | 1.0% | 0.0% | pseudoXOF 9 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `HARE-128-kr` | KAT log (sha256 `bf17fa8aef5af610…`) | `kat/kem-16/HARE-128-kr.log` |
| `HARE-128-kr` | timing dec | `records/kem-16/HARE-128-kr__dec.json` |
| `HARE-128-kr` | timing enc | `records/kem-16/HARE-128-kr__enc.json` |
| `HARE-128-kr` | timing keygen | `records/kem-16/HARE-128-kr__keygen.json` |
| `HARE-128-kr` | hash profile dec | `profile/kem-16/HARE-128-kr__dec.json` |
| `HARE-128-kr` | hash profile enc | `profile/kem-16/HARE-128-kr__enc.json` |
| `HARE-128-kr` | hash profile keygen | `profile/kem-16/HARE-128-kr__keygen.json` |
| `HARE-256-kr` | KAT log (sha256 `64fea5eaebf9c25a…`) | `kat/kem-16/HARE-256-kr.log` |
| `HARE-256-kr` | timing dec | `records/kem-16/HARE-256-kr__dec.json` |
| `HARE-256-kr` | timing enc | `records/kem-16/HARE-256-kr__enc.json` |
| `HARE-256-kr` | timing keygen | `records/kem-16/HARE-256-kr__keygen.json` |
| `HARE-256-kr` | hash profile dec | `profile/kem-16/HARE-256-kr__dec.json` |
| `HARE-256-kr` | hash profile enc | `profile/kem-16/HARE-256-kr__enc.json` |
| `HARE-256-kr` | hash profile keygen | `profile/kem-16/HARE-256-kr__keygen.json` |
| `HARE-384-kr` | KAT log (sha256 `be4e5f21f7c669ff…`) | `kat/kem-16/HARE-384-kr.log` |
| `HARE-384-kr` | timing dec | `records/kem-16/HARE-384-kr__dec.json` |
| `HARE-384-kr` | timing enc | `records/kem-16/HARE-384-kr__enc.json` |
| `HARE-384-kr` | timing keygen | `records/kem-16/HARE-384-kr__keygen.json` |
| `HARE-384-kr` | hash profile dec | `profile/kem-16/HARE-384-kr__dec.json` |
| `HARE-384-kr` | hash profile enc | `profile/kem-16/HARE-384-kr__enc.json` |
| `HARE-384-kr` | hash profile keygen | `profile/kem-16/HARE-384-kr__keygen.json` |
| `HARE-512-kr` | KAT log (sha256 `ef300357ea81be4a…`) | `kat/kem-16/HARE-512-kr.log` |
| `HARE-512-kr` | timing dec | `records/kem-16/HARE-512-kr__dec.json` |
| `HARE-512-kr` | timing enc | `records/kem-16/HARE-512-kr__enc.json` |
| `HARE-512-kr` | timing keygen | `records/kem-16/HARE-512-kr__keygen.json` |
| `HARE-512-kr` | hash profile dec | `profile/kem-16/HARE-512-kr__dec.json` |
| `HARE-512-kr` | hash profile enc | `profile/kem-16/HARE-512-kr__enc.json` |
| `HARE-512-kr` | hash profile keygen | `profile/kem-16/HARE-512-kr__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

