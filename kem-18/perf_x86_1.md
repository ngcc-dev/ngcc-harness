# kem-18 LoongKEM — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-18` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560853666549760.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: LoongKEM
- Implementation versions measured: reference
- Parameter sets: `Loong128`, `Loong256`, `Loong384`, `Loong512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-18/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Loong128` | guide | PASS |
| `Loong256` | guide | PASS |
| `Loong384` | guide | PASS |
| `Loong512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Loong128` | keygen | 6.26 M | 2.99 ms | 334 | 2.99 ms | 1605 (5 × 321) |
| `Loong128` | enc | 6.34 M | 3.03 ms | 330 | 3.03 ms | 1650 (5 × 330) |
| `Loong128` | dec | 6.42 M | 3.06 ms | 326 | 3.06 ms | 1640 (5 × 328) |
| `Loong256` | keygen | 15.94 M | 7.63 ms | 131 | 7.52 ms | 645 (5 × 129) |
| `Loong256` | enc | 15.84 M | 7.58 ms | 132 | 7.58 ms | 655 (5 × 131) |
| `Loong256` | dec | 16.15 M | 7.73 ms | 129 | 7.68 ms | 650 (5 × 130) |
| `Loong384` | keygen | 46.99 M | 22.4 ms | 44.5 | 22.5 ms | 220 (5 × 44) |
| `Loong384` | enc | 47.40 M | 22.6 ms | 44.2 | 22.6 ms | 220 (5 × 44) |
| `Loong384` | dec | 47.89 M | 22.9 ms | 43.7 | 22.9 ms | 220 (5 × 44) |
| `Loong512` | keygen | 75.46 M | 36.1 ms | 27.7 | 36.1 ms | 140 (5 × 28) |
| `Loong512` | enc | 75.94 M | 36.3 ms | 27.5 | 36.3 ms | 140 (5 × 28) |
| `Loong512` | dec | 77.37 M | 37 ms | 27 | 36.8 ms | 140 (5 × 28) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Loong128` | keygen | 27293 | 1684 KiB | 2080 KiB |
| `Loong128` | enc | 27293 | 1888 KiB | 2016 KiB |
| `Loong128` | dec | 27293 | 1992 KiB | 2092 KiB |
| `Loong256` | keygen | 28537 | 1708 KiB | 2508 KiB |
| `Loong256` | enc | 28537 | 2420 KiB | 2512 KiB |
| `Loong256` | dec | 28537 | 2436 KiB | 2552 KiB |
| `Loong384` | keygen | 27425 | 1716 KiB | 3036 KiB |
| `Loong384` | enc | 27425 | 2988 KiB | 3084 KiB |
| `Loong384` | dec | 27425 | 2972 KiB | 3104 KiB |
| `Loong512` | keygen | 28741 | 1728 KiB | 3868 KiB |
| `Loong512` | enc | 28741 | 3796 KiB | 3908 KiB |
| `Loong512` | dec | 28741 | 3872 KiB | 3952 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `Loong128` | 1472 | 1848 | 1512 | 16 |
| `Loong256` | 3248 | 3888 | 3520 | 32 |
| `Loong384` | 6444 | 7340 | 6680 | 48 |
| `Loong512` | 10640 | 11832 | 10848 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Loong128` | keygen | 79% | 0.2% | drng 2, pseudoXOF 3, sm3hash 1 |
| `Loong128` | enc | 78% | 0.1% | drng 1, pseudoXOF 5, sm3hash 1 |
| `Loong128` | dec | 77% | 0.0% | pseudoXOF 6 |
| `Loong256` | keygen | 79% | 0.1% | drng 2, pseudoXOF 3, sm3hash 1 |
| `Loong256` | enc | 78% | 0.0% | drng 1, pseudoXOF 5, sm3hash 1 |
| `Loong256` | dec | 77% | 0.0% | pseudoXOF 6 |
| `Loong384` | keygen | 86% | 0.0% | drng 2, pseudoXOF 3, sm3hash 1 |
| `Loong384` | enc | 85% | 0.0% | drng 1, pseudoXOF 5, sm3hash 1 |
| `Loong384` | dec | 85% | 0.0% | pseudoXOF 6 |
| `Loong512` | keygen | 87% | 0.0% | drng 2, pseudoXOF 3, sm3hash 1 |
| `Loong512` | enc | 87% | 0.0% | drng 1, pseudoXOF 5, sm3hash 1 |
| `Loong512` | dec | 86% | 0.0% | pseudoXOF 6 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Loong128` | KAT log (sha256 `1cfd7322649130cd…`) | `kat/kem-18/Loong128.log` |
| `Loong128` | timing dec | `records/kem-18/Loong128__dec.json` |
| `Loong128` | timing enc | `records/kem-18/Loong128__enc.json` |
| `Loong128` | timing keygen | `records/kem-18/Loong128__keygen.json` |
| `Loong128` | hash profile dec | `profile/kem-18/Loong128__dec.json` |
| `Loong128` | hash profile enc | `profile/kem-18/Loong128__enc.json` |
| `Loong128` | hash profile keygen | `profile/kem-18/Loong128__keygen.json` |
| `Loong256` | KAT log (sha256 `79b118010d356bf2…`) | `kat/kem-18/Loong256.log` |
| `Loong256` | timing dec | `records/kem-18/Loong256__dec.json` |
| `Loong256` | timing enc | `records/kem-18/Loong256__enc.json` |
| `Loong256` | timing keygen | `records/kem-18/Loong256__keygen.json` |
| `Loong256` | hash profile dec | `profile/kem-18/Loong256__dec.json` |
| `Loong256` | hash profile enc | `profile/kem-18/Loong256__enc.json` |
| `Loong256` | hash profile keygen | `profile/kem-18/Loong256__keygen.json` |
| `Loong384` | KAT log (sha256 `d511a3737e93b339…`) | `kat/kem-18/Loong384.log` |
| `Loong384` | timing dec | `records/kem-18/Loong384__dec.json` |
| `Loong384` | timing enc | `records/kem-18/Loong384__enc.json` |
| `Loong384` | timing keygen | `records/kem-18/Loong384__keygen.json` |
| `Loong384` | hash profile dec | `profile/kem-18/Loong384__dec.json` |
| `Loong384` | hash profile enc | `profile/kem-18/Loong384__enc.json` |
| `Loong384` | hash profile keygen | `profile/kem-18/Loong384__keygen.json` |
| `Loong512` | KAT log (sha256 `248a9f4af1426223…`) | `kat/kem-18/Loong512.log` |
| `Loong512` | timing dec | `records/kem-18/Loong512__dec.json` |
| `Loong512` | timing enc | `records/kem-18/Loong512__enc.json` |
| `Loong512` | timing keygen | `records/kem-18/Loong512__keygen.json` |
| `Loong512` | hash profile dec | `profile/kem-18/Loong512__dec.json` |
| `Loong512` | hash profile enc | `profile/kem-18/Loong512__enc.json` |
| `Loong512` | hash profile keygen | `profile/kem-18/Loong512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

