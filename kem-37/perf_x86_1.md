# kem-37 TriQ-KEM — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-37` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560881558671360.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: TriQ-KEM
- Implementation versions measured: reference
- Parameter sets: `TriQ-KEM-128`, `TriQ-KEM-256`, `TriQ-KEM-384`, `TriQ-KEM-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-37/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `TriQ-KEM-128` | guide | PASS |
| `TriQ-KEM-256` | guide | PASS |
| `TriQ-KEM-384` | guide | PASS |
| `TriQ-KEM-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `TriQ-KEM-128` | keygen | 3.92 M | 1.87 ms | 534 | 1.87 ms | 2625 (5 × 525) |
| `TriQ-KEM-128` | enc | 7.41 M | 3.54 ms | 282 | 3.54 ms | 1400 (5 × 280) |
| `TriQ-KEM-128` | dec | 11.21 M | 5.35 ms | 187 | 5.35 ms | 930 (5 × 186) |
| `TriQ-KEM-256` | keygen | 21.48 M | 10.3 ms | 97.5 | 10.3 ms | 485 (5 × 97) |
| `TriQ-KEM-256` | enc | 42.49 M | 20.3 ms | 49.3 | 20.3 ms | 250 (5 × 50) |
| `TriQ-KEM-256` | dec | 63.63 M | 30.4 ms | 32.9 | 30.4 ms | 165 (5 × 33) |
| `TriQ-KEM-384` | keygen | 57.31 M | 27.4 ms | 36.5 | 27.4 ms | 185 (5 × 37) |
| `TriQ-KEM-384` | enc | 113.81 M | 54.4 ms | 18.4 | 54.4 ms | 100 (5 × 20) |
| `TriQ-KEM-384` | dec | 171.18 M | 81.8 ms | 12.2 | 81.8 ms | 100 (5 × 20) |
| `TriQ-KEM-512` | keygen | 122.30 M | 58.4 ms | 17.1 | 58.4 ms | 100 (5 × 20) |
| `TriQ-KEM-512` | enc | 242.32 M | 116 ms | 8.64 | 116 ms | 100 (5 × 20) |
| `TriQ-KEM-512` | dec | 362.24 M | 173 ms | 5.78 | 173 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `TriQ-KEM-128` | keygen | 42477 | 1724 KiB | 1840 KiB |
| `TriQ-KEM-128` | enc | 42477 | 1776 KiB | 1844 KiB |
| `TriQ-KEM-128` | dec | 42477 | 1780 KiB | 1872 KiB |
| `TriQ-KEM-256` | keygen | 45957 | 1736 KiB | 1972 KiB |
| `TriQ-KEM-256` | enc | 45957 | 1892 KiB | 2004 KiB |
| `TriQ-KEM-256` | dec | 45957 | 1916 KiB | 2004 KiB |
| `TriQ-KEM-384` | keygen | 54893 | 1768 KiB | 2100 KiB |
| `TriQ-KEM-384` | enc | 54893 | 2064 KiB | 2184 KiB |
| `TriQ-KEM-384` | dec | 54893 | 2144 KiB | 2228 KiB |
| `TriQ-KEM-512` | keygen | 62041 | 1808 KiB | 2272 KiB |
| `TriQ-KEM-512` | enc | 62041 | 2300 KiB | 2408 KiB |
| `TriQ-KEM-512` | dec | 62041 | 2384 KiB | 2480 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `TriQ-KEM-128` | 2054 | 2102 | 4086 | 16 |
| `TriQ-KEM-256` | 6328 | 6424 | 12472 | 32 |
| `TriQ-KEM-384` | 12255 | 12399 | 24335 | 48 |
| `TriQ-KEM-512` | 19768 | 19960 | 39320 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `TriQ-KEM-128` | keygen | 12% | 0.3% | drng 2, pseudoXOF 8, sm3hash 1 |
| `TriQ-KEM-128` | enc | 6.1% | 0.2% | drng 2, pseudoXOF 6.02, sm3hash 2 |
| `TriQ-KEM-128` | dec | 4.6% | 0.0% | pseudoXOF 6, sm3hash 3 |
| `TriQ-KEM-256` | keygen | 4.1% | 0.1% | drng 2, pseudoXOF 9.1 |
| `TriQ-KEM-256` | enc | 2.9% | 0.0% | drng 2, pseudoXOF 8, pseudohash 1, sm3hash 1 |
| `TriQ-KEM-256` | dec | 2.3% | 0.0% | pseudoXOF 8, pseudohash 1, sm3hash 2 |
| `TriQ-KEM-384` | keygen | 2.9% | 0.0% | drng 2, pseudoXOF 10.9 |
| `TriQ-KEM-384` | enc | 2.0% | 0.0% | drng 2, pseudoXOF 9, pseudohash 1 |
| `TriQ-KEM-384` | dec | 1.9% | 0.0% | pseudoXOF 10, pseudohash 1 |
| `TriQ-KEM-512` | keygen | 3.7% | 0.0% | drng 2, pseudoXOF 10.9 |
| `TriQ-KEM-512` | enc | 2.6% | 0.0% | drng 2, pseudoXOF 11.2, pseudohash 1 |
| `TriQ-KEM-512` | dec | 2.2% | 0.0% | pseudoXOF 12, pseudohash 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `TriQ-KEM-128` | KAT log (sha256 `440a979fa2a19c57…`) | `kat/kem-37/TriQ-KEM-128.log` |
| `TriQ-KEM-128` | timing dec | `records/kem-37/TriQ-KEM-128__dec.json` |
| `TriQ-KEM-128` | timing enc | `records/kem-37/TriQ-KEM-128__enc.json` |
| `TriQ-KEM-128` | timing keygen | `records/kem-37/TriQ-KEM-128__keygen.json` |
| `TriQ-KEM-128` | hash profile dec | `profile/kem-37/TriQ-KEM-128__dec.json` |
| `TriQ-KEM-128` | hash profile enc | `profile/kem-37/TriQ-KEM-128__enc.json` |
| `TriQ-KEM-128` | hash profile keygen | `profile/kem-37/TriQ-KEM-128__keygen.json` |
| `TriQ-KEM-256` | KAT log (sha256 `10b6c7648cd3bbb5…`) | `kat/kem-37/TriQ-KEM-256.log` |
| `TriQ-KEM-256` | timing dec | `records/kem-37/TriQ-KEM-256__dec.json` |
| `TriQ-KEM-256` | timing enc | `records/kem-37/TriQ-KEM-256__enc.json` |
| `TriQ-KEM-256` | timing keygen | `records/kem-37/TriQ-KEM-256__keygen.json` |
| `TriQ-KEM-256` | hash profile dec | `profile/kem-37/TriQ-KEM-256__dec.json` |
| `TriQ-KEM-256` | hash profile enc | `profile/kem-37/TriQ-KEM-256__enc.json` |
| `TriQ-KEM-256` | hash profile keygen | `profile/kem-37/TriQ-KEM-256__keygen.json` |
| `TriQ-KEM-384` | KAT log (sha256 `bbf322220cf9be79…`) | `kat/kem-37/TriQ-KEM-384.log` |
| `TriQ-KEM-384` | timing dec | `records/kem-37/TriQ-KEM-384__dec.json` |
| `TriQ-KEM-384` | timing enc | `records/kem-37/TriQ-KEM-384__enc.json` |
| `TriQ-KEM-384` | timing keygen | `records/kem-37/TriQ-KEM-384__keygen.json` |
| `TriQ-KEM-384` | hash profile dec | `profile/kem-37/TriQ-KEM-384__dec.json` |
| `TriQ-KEM-384` | hash profile enc | `profile/kem-37/TriQ-KEM-384__enc.json` |
| `TriQ-KEM-384` | hash profile keygen | `profile/kem-37/TriQ-KEM-384__keygen.json` |
| `TriQ-KEM-512` | KAT log (sha256 `5894523014ae0d3b…`) | `kat/kem-37/TriQ-KEM-512.log` |
| `TriQ-KEM-512` | timing dec | `records/kem-37/TriQ-KEM-512__dec.json` |
| `TriQ-KEM-512` | timing enc | `records/kem-37/TriQ-KEM-512__enc.json` |
| `TriQ-KEM-512` | timing keygen | `records/kem-37/TriQ-KEM-512__keygen.json` |
| `TriQ-KEM-512` | hash profile dec | `profile/kem-37/TriQ-KEM-512__dec.json` |
| `TriQ-KEM-512` | hash profile enc | `profile/kem-37/TriQ-KEM-512__enc.json` |
| `TriQ-KEM-512` | hash profile keygen | `profile/kem-37/TriQ-KEM-512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

