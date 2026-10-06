# kem-20 MAMBA-Frost — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-20` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560853918208000.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: MAMBA-Frost
- Implementation versions measured: reference
- Parameter sets: `MAMBA-Frost-128`, `MAMBA-Frost-192`, `MAMBA-Frost-256`, `MAMBA-Frost-384`, `MAMBA-Frost-512`, `MAMBA-Frost-CC-128`, `MAMBA-Frost-CC-192`, `MAMBA-Frost-CC-256`, `MAMBA-Frost-CC-384`, `MAMBA-Frost-CC-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-20/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `MAMBA-Frost-128` | guide | PASS |
| `MAMBA-Frost-192` | guide | PASS |
| `MAMBA-Frost-256` | guide | PASS |
| `MAMBA-Frost-384` | guide | PASS |
| `MAMBA-Frost-512` | guide | PASS |
| `MAMBA-Frost-CC-128` | guide | PASS |
| `MAMBA-Frost-CC-192` | guide | PASS |
| `MAMBA-Frost-CC-256` | guide | PASS |
| `MAMBA-Frost-CC-384` | guide | PASS |
| `MAMBA-Frost-CC-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `MAMBA-Frost-128` | keygen | 38.49 M | 18.4 ms | 54.4 | 18.4 ms | 275 (5 × 55) |
| `MAMBA-Frost-128` | enc | 43.04 M | 20.6 ms | 48.6 | 20.5 ms | 245 (5 × 49) |
| `MAMBA-Frost-128` | dec | 43.47 M | 20.8 ms | 48.1 | 20.8 ms | 240 (5 × 48) |
| `MAMBA-Frost-192` | keygen | 112.69 M | 53.8 ms | 18.6 | 53.8 ms | 100 (5 × 20) |
| `MAMBA-Frost-192` | enc | 119.72 M | 57.2 ms | 17.5 | 57.3 ms | 100 (5 × 20) |
| `MAMBA-Frost-192` | dec | 120.24 M | 57.4 ms | 17.4 | 57.4 ms | 100 (5 × 20) |
| `MAMBA-Frost-256` | keygen | 241.73 M | 116 ms | 8.66 | 116 ms | 100 (5 × 20) |
| `MAMBA-Frost-256` | enc | 257.78 M | 123 ms | 8.12 | 123 ms | 100 (5 × 20) |
| `MAMBA-Frost-256` | dec | 258.75 M | 124 ms | 8.09 | 124 ms | 100 (5 × 20) |
| `MAMBA-Frost-384` | keygen | 538.40 M | 259 ms | 3.87 | 258 ms | 100 (5 × 20) |
| `MAMBA-Frost-384` | enc | 593.01 M | 283 ms | 3.53 | 282 ms | 100 (5 × 20) |
| `MAMBA-Frost-384` | dec | 595.55 M | 286 ms | 3.5 | 285 ms | 100 (5 × 20) |
| `MAMBA-Frost-512` | keygen | 972.04 M | 467 ms | 2.14 | 469 ms | 100 (5 × 20) |
| `MAMBA-Frost-512` | enc | 990.48 M | 476 ms | 2.1 | 477 ms | 100 (5 × 20) |
| `MAMBA-Frost-512` | dec | 992.50 M | 477 ms | 2.1 | 474 ms | 100 (5 × 20) |
| `MAMBA-Frost-CC-128` | keygen | 38.45 M | 18.4 ms | 54.4 | 18.4 ms | 270 (5 × 54) |
| `MAMBA-Frost-CC-128` | enc | 43.00 M | 20.6 ms | 48.7 | 20.6 ms | 245 (5 × 49) |
| `MAMBA-Frost-CC-128` | dec | 43.36 M | 20.7 ms | 48.3 | 20.7 ms | 245 (5 × 49) |
| `MAMBA-Frost-CC-192` | keygen | 111.68 M | 53.4 ms | 18.7 | 53.5 ms | 100 (5 × 20) |
| `MAMBA-Frost-CC-192` | enc | 118.93 M | 56.8 ms | 17.6 | 56.8 ms | 100 (5 × 20) |
| `MAMBA-Frost-CC-192` | dec | 120.02 M | 57.3 ms | 17.4 | 57.4 ms | 100 (5 × 20) |
| `MAMBA-Frost-CC-256` | keygen | 241.47 M | 115 ms | 8.67 | 115 ms | 100 (5 × 20) |
| `MAMBA-Frost-CC-256` | enc | 255.32 M | 122 ms | 8.2 | 122 ms | 100 (5 × 20) |
| `MAMBA-Frost-CC-256` | dec | 258.07 M | 123 ms | 8.11 | 123 ms | 100 (5 × 20) |
| `MAMBA-Frost-CC-384` | keygen | 541.61 M | 260 ms | 3.84 | 259 ms | 100 (5 × 20) |
| `MAMBA-Frost-CC-384` | enc | 571.29 M | 274 ms | 3.64 | 275 ms | 100 (5 × 20) |
| `MAMBA-Frost-CC-384` | dec | 572.50 M | 275 ms | 3.64 | 274 ms | 100 (5 × 20) |
| `MAMBA-Frost-CC-512` | keygen | 998.62 M | 477 ms | 2.1 | 477 ms | 100 (5 × 20) |
| `MAMBA-Frost-CC-512` | enc | 972.28 M | 464 ms | 2.15 | 465 ms | 100 (5 × 20) |
| `MAMBA-Frost-CC-512` | dec | 973.29 M | 465 ms | 2.15 | 466 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `MAMBA-Frost-128` | keygen | 33973 | 1668 KiB | 2336 KiB |
| `MAMBA-Frost-128` | enc | 33973 | 2288 KiB | 2392 KiB |
| `MAMBA-Frost-128` | dec | 33973 | 2328 KiB | 2400 KiB |
| `MAMBA-Frost-192` | keygen | 34101 | 1736 KiB | 3400 KiB |
| `MAMBA-Frost-192` | enc | 34101 | 3368 KiB | 3452 KiB |
| `MAMBA-Frost-192` | dec | 34101 | 3384 KiB | 3456 KiB |
| `MAMBA-Frost-256` | keygen | 34101 | 1768 KiB | 5148 KiB |
| `MAMBA-Frost-256` | enc | 34101 | 5152 KiB | 5220 KiB |
| `MAMBA-Frost-256` | dec | 34101 | 5216 KiB | 5328 KiB |
| `MAMBA-Frost-384` | keygen | 34349 | 1796 KiB | 9264 KiB |
| `MAMBA-Frost-384` | enc | 34349 | 9412 KiB | 9516 KiB |
| `MAMBA-Frost-384` | dec | 34349 | 9576 KiB | 9652 KiB |
| `MAMBA-Frost-512` | keygen | 34285 | 1784 KiB | 15276 KiB |
| `MAMBA-Frost-512` | enc | 34285 | 2488 KiB | 15028 KiB |
| `MAMBA-Frost-512` | dec | 34285 | 2764 KiB | 14964 KiB |
| `MAMBA-Frost-CC-128` | keygen | 34109 | 1708 KiB | 2360 KiB |
| `MAMBA-Frost-CC-128` | enc | 34109 | 2300 KiB | 2388 KiB |
| `MAMBA-Frost-CC-128` | dec | 34109 | 2328 KiB | 2412 KiB |
| `MAMBA-Frost-CC-192` | keygen | 34237 | 1720 KiB | 3376 KiB |
| `MAMBA-Frost-CC-192` | enc | 34237 | 3352 KiB | 3424 KiB |
| `MAMBA-Frost-CC-192` | dec | 34237 | 3400 KiB | 3484 KiB |
| `MAMBA-Frost-CC-256` | keygen | 34237 | 1756 KiB | 5256 KiB |
| `MAMBA-Frost-CC-256` | enc | 34237 | 5176 KiB | 5264 KiB |
| `MAMBA-Frost-CC-256` | dec | 34237 | 5200 KiB | 5328 KiB |
| `MAMBA-Frost-CC-384` | keygen | 34677 | 1800 KiB | 9344 KiB |
| `MAMBA-Frost-CC-384` | enc | 34677 | 9392 KiB | 9480 KiB |
| `MAMBA-Frost-CC-384` | dec | 34677 | 9524 KiB | 9588 KiB |
| `MAMBA-Frost-CC-512` | keygen | 34677 | 1872 KiB | 15524 KiB |
| `MAMBA-Frost-CC-512` | enc | 34677 | 2400 KiB | 15172 KiB |
| `MAMBA-Frost-CC-512` | dec | 34677 | 2616 KiB | 15204 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `MAMBA-Frost-128` | 5152 | 6736 | 5192 | 16 |
| `MAMBA-Frost-192` | 9712 | 11528 | 9760 | 24 |
| `MAMBA-Frost-256` | 16776 | 19416 | 15552 | 32 |
| `MAMBA-Frost-384` | 25096 | 29032 | 37736 | 48 |
| `MAMBA-Frost-512` | 36432 | 41728 | 72944 | 64 |
| `MAMBA-Frost-CC-128` | 5152 | 6736 | 5192 | 16 |
| `MAMBA-Frost-CC-192` | 9712 | 11528 | 9760 | 24 |
| `MAMBA-Frost-CC-256` | 16776 | 19416 | 15552 | 32 |
| `MAMBA-Frost-CC-384` | 37628 | 43492 | 25204 | 48 |
| `MAMBA-Frost-CC-512` | 72832 | 83328 | 36544 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **mixed** — all SHAKE calls are pseudoXOF shims; matrix A via own AES-128 with -D_AES128_FOR_A_

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `MAMBA-Frost-128` | keygen | 2.2% | 0.0% | drng 1, pseudoXOF 4 |
| `MAMBA-Frost-128` | enc | 3.2% | 0.0% | drng 1, pseudoXOF 7 |
| `MAMBA-Frost-128` | dec | 3.8% | 0.0% | pseudoXOF 8 |
| `MAMBA-Frost-192` | keygen | 1.3% | 0.0% | drng 1, pseudoXOF 4 |
| `MAMBA-Frost-192` | enc | 2.0% | 0.0% | drng 1, pseudoXOF 7 |
| `MAMBA-Frost-192` | dec | 2.4% | 0.0% | pseudoXOF 8 |
| `MAMBA-Frost-256` | keygen | 1.3% | 0.0% | drng 1, pseudoXOF 4 |
| `MAMBA-Frost-256` | enc | 1.7% | 0.0% | drng 1, pseudoXOF 7 |
| `MAMBA-Frost-256` | dec | 2.0% | 0.0% | pseudoXOF 8 |
| `MAMBA-Frost-384` | keygen | 0.9% | 0.0% | drng 1, pseudoXOF 4 |
| `MAMBA-Frost-384` | enc | 1.6% | 0.0% | drng 1, pseudoXOF 7 |
| `MAMBA-Frost-384` | dec | 1.9% | 0.0% | pseudoXOF 8 |
| `MAMBA-Frost-512` | keygen | 0.9% | 0.0% | drng 1, pseudoXOF 4 |
| `MAMBA-Frost-512` | enc | 2.1% | 0.0% | drng 1, pseudoXOF 7 |
| `MAMBA-Frost-512` | dec | 2.4% | 0.0% | pseudoXOF 8 |
| `MAMBA-Frost-CC-128` | keygen | 2.2% | 0.0% | drng 1, pseudoXOF 4 |
| `MAMBA-Frost-CC-128` | enc | 3.2% | 0.0% | drng 1, pseudoXOF 7 |
| `MAMBA-Frost-CC-128` | dec | 3.8% | 0.0% | pseudoXOF 8 |
| `MAMBA-Frost-CC-192` | keygen | 1.3% | 0.0% | drng 1, pseudoXOF 4 |
| `MAMBA-Frost-CC-192` | enc | 2.0% | 0.0% | drng 1, pseudoXOF 7 |
| `MAMBA-Frost-CC-192` | dec | 2.4% | 0.0% | pseudoXOF 8 |
| `MAMBA-Frost-CC-256` | keygen | 1.3% | 0.0% | drng 1, pseudoXOF 4 |
| `MAMBA-Frost-CC-256` | enc | 1.7% | 0.0% | drng 1, pseudoXOF 7 |
| `MAMBA-Frost-CC-256` | dec | 2.0% | 0.0% | pseudoXOF 8 |
| `MAMBA-Frost-CC-384` | keygen | 1.3% | 0.0% | drng 1, pseudoXOF 4 |
| `MAMBA-Frost-CC-384` | enc | 1.4% | 0.0% | drng 1, pseudoXOF 7 |
| `MAMBA-Frost-CC-384` | dec | 1.5% | 0.0% | pseudoXOF 8 |
| `MAMBA-Frost-CC-512` | keygen | 1.6% | 0.0% | drng 1, pseudoXOF 4 |
| `MAMBA-Frost-CC-512` | enc | 1.5% | 0.0% | drng 1, pseudoXOF 7 |
| `MAMBA-Frost-CC-512` | dec | 1.5% | 0.0% | pseudoXOF 8 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `MAMBA-Frost-128` | KAT log (sha256 `8090bcff3acb2f59…`) | `kat/kem-20/MAMBA-Frost-128.log` |
| `MAMBA-Frost-128` | timing dec | `records/kem-20/MAMBA-Frost-128__dec.json` |
| `MAMBA-Frost-128` | timing enc | `records/kem-20/MAMBA-Frost-128__enc.json` |
| `MAMBA-Frost-128` | timing keygen | `records/kem-20/MAMBA-Frost-128__keygen.json` |
| `MAMBA-Frost-128` | hash profile dec | `profile/kem-20/MAMBA-Frost-128__dec.json` |
| `MAMBA-Frost-128` | hash profile enc | `profile/kem-20/MAMBA-Frost-128__enc.json` |
| `MAMBA-Frost-128` | hash profile keygen | `profile/kem-20/MAMBA-Frost-128__keygen.json` |
| `MAMBA-Frost-192` | KAT log (sha256 `b2f9fb067a588cd9…`) | `kat/kem-20/MAMBA-Frost-192.log` |
| `MAMBA-Frost-192` | timing dec | `records/kem-20/MAMBA-Frost-192__dec.json` |
| `MAMBA-Frost-192` | timing enc | `records/kem-20/MAMBA-Frost-192__enc.json` |
| `MAMBA-Frost-192` | timing keygen | `records/kem-20/MAMBA-Frost-192__keygen.json` |
| `MAMBA-Frost-192` | hash profile dec | `profile/kem-20/MAMBA-Frost-192__dec.json` |
| `MAMBA-Frost-192` | hash profile enc | `profile/kem-20/MAMBA-Frost-192__enc.json` |
| `MAMBA-Frost-192` | hash profile keygen | `profile/kem-20/MAMBA-Frost-192__keygen.json` |
| `MAMBA-Frost-256` | KAT log (sha256 `f390adac33d822c8…`) | `kat/kem-20/MAMBA-Frost-256.log` |
| `MAMBA-Frost-256` | timing dec | `records/kem-20/MAMBA-Frost-256__dec.json` |
| `MAMBA-Frost-256` | timing enc | `records/kem-20/MAMBA-Frost-256__enc.json` |
| `MAMBA-Frost-256` | timing keygen | `records/kem-20/MAMBA-Frost-256__keygen.json` |
| `MAMBA-Frost-256` | hash profile dec | `profile/kem-20/MAMBA-Frost-256__dec.json` |
| `MAMBA-Frost-256` | hash profile enc | `profile/kem-20/MAMBA-Frost-256__enc.json` |
| `MAMBA-Frost-256` | hash profile keygen | `profile/kem-20/MAMBA-Frost-256__keygen.json` |
| `MAMBA-Frost-384` | KAT log (sha256 `39a1ff58781834b2…`) | `kat/kem-20/MAMBA-Frost-384.log` |
| `MAMBA-Frost-384` | timing dec | `records/kem-20/MAMBA-Frost-384__dec.json` |
| `MAMBA-Frost-384` | timing enc | `records/kem-20/MAMBA-Frost-384__enc.json` |
| `MAMBA-Frost-384` | timing keygen | `records/kem-20/MAMBA-Frost-384__keygen.json` |
| `MAMBA-Frost-384` | hash profile dec | `profile/kem-20/MAMBA-Frost-384__dec.json` |
| `MAMBA-Frost-384` | hash profile enc | `profile/kem-20/MAMBA-Frost-384__enc.json` |
| `MAMBA-Frost-384` | hash profile keygen | `profile/kem-20/MAMBA-Frost-384__keygen.json` |
| `MAMBA-Frost-512` | KAT log (sha256 `8c24563da71c0fcf…`) | `kat/kem-20/MAMBA-Frost-512.log` |
| `MAMBA-Frost-512` | timing dec | `records/kem-20/MAMBA-Frost-512__dec.json` |
| `MAMBA-Frost-512` | timing enc | `records/kem-20/MAMBA-Frost-512__enc.json` |
| `MAMBA-Frost-512` | timing keygen | `records/kem-20/MAMBA-Frost-512__keygen.json` |
| `MAMBA-Frost-512` | hash profile dec | `profile/kem-20/MAMBA-Frost-512__dec.json` |
| `MAMBA-Frost-512` | hash profile enc | `profile/kem-20/MAMBA-Frost-512__enc.json` |
| `MAMBA-Frost-512` | hash profile keygen | `profile/kem-20/MAMBA-Frost-512__keygen.json` |
| `MAMBA-Frost-CC-128` | KAT log (sha256 `76270eca0e7383ee…`) | `kat/kem-20/MAMBA-Frost-CC-128.log` |
| `MAMBA-Frost-CC-128` | timing dec | `records/kem-20/MAMBA-Frost-CC-128__dec.json` |
| `MAMBA-Frost-CC-128` | timing enc | `records/kem-20/MAMBA-Frost-CC-128__enc.json` |
| `MAMBA-Frost-CC-128` | timing keygen | `records/kem-20/MAMBA-Frost-CC-128__keygen.json` |
| `MAMBA-Frost-CC-128` | hash profile dec | `profile/kem-20/MAMBA-Frost-CC-128__dec.json` |
| `MAMBA-Frost-CC-128` | hash profile enc | `profile/kem-20/MAMBA-Frost-CC-128__enc.json` |
| `MAMBA-Frost-CC-128` | hash profile keygen | `profile/kem-20/MAMBA-Frost-CC-128__keygen.json` |
| `MAMBA-Frost-CC-192` | KAT log (sha256 `fb9fd18fb53a9e95…`) | `kat/kem-20/MAMBA-Frost-CC-192.log` |
| `MAMBA-Frost-CC-192` | timing dec | `records/kem-20/MAMBA-Frost-CC-192__dec.json` |
| `MAMBA-Frost-CC-192` | timing enc | `records/kem-20/MAMBA-Frost-CC-192__enc.json` |
| `MAMBA-Frost-CC-192` | timing keygen | `records/kem-20/MAMBA-Frost-CC-192__keygen.json` |
| `MAMBA-Frost-CC-192` | hash profile dec | `profile/kem-20/MAMBA-Frost-CC-192__dec.json` |
| `MAMBA-Frost-CC-192` | hash profile enc | `profile/kem-20/MAMBA-Frost-CC-192__enc.json` |
| `MAMBA-Frost-CC-192` | hash profile keygen | `profile/kem-20/MAMBA-Frost-CC-192__keygen.json` |
| `MAMBA-Frost-CC-256` | KAT log (sha256 `e8028e5741692f04…`) | `kat/kem-20/MAMBA-Frost-CC-256.log` |
| `MAMBA-Frost-CC-256` | timing dec | `records/kem-20/MAMBA-Frost-CC-256__dec.json` |
| `MAMBA-Frost-CC-256` | timing enc | `records/kem-20/MAMBA-Frost-CC-256__enc.json` |
| `MAMBA-Frost-CC-256` | timing keygen | `records/kem-20/MAMBA-Frost-CC-256__keygen.json` |
| `MAMBA-Frost-CC-256` | hash profile dec | `profile/kem-20/MAMBA-Frost-CC-256__dec.json` |
| `MAMBA-Frost-CC-256` | hash profile enc | `profile/kem-20/MAMBA-Frost-CC-256__enc.json` |
| `MAMBA-Frost-CC-256` | hash profile keygen | `profile/kem-20/MAMBA-Frost-CC-256__keygen.json` |
| `MAMBA-Frost-CC-384` | KAT log (sha256 `5536f26fe0a5df5e…`) | `kat/kem-20/MAMBA-Frost-CC-384.log` |
| `MAMBA-Frost-CC-384` | timing dec | `records/kem-20/MAMBA-Frost-CC-384__dec.json` |
| `MAMBA-Frost-CC-384` | timing enc | `records/kem-20/MAMBA-Frost-CC-384__enc.json` |
| `MAMBA-Frost-CC-384` | timing keygen | `records/kem-20/MAMBA-Frost-CC-384__keygen.json` |
| `MAMBA-Frost-CC-384` | hash profile dec | `profile/kem-20/MAMBA-Frost-CC-384__dec.json` |
| `MAMBA-Frost-CC-384` | hash profile enc | `profile/kem-20/MAMBA-Frost-CC-384__enc.json` |
| `MAMBA-Frost-CC-384` | hash profile keygen | `profile/kem-20/MAMBA-Frost-CC-384__keygen.json` |
| `MAMBA-Frost-CC-512` | KAT log (sha256 `e35e49091c3beb5f…`) | `kat/kem-20/MAMBA-Frost-CC-512.log` |
| `MAMBA-Frost-CC-512` | timing dec | `records/kem-20/MAMBA-Frost-CC-512__dec.json` |
| `MAMBA-Frost-CC-512` | timing enc | `records/kem-20/MAMBA-Frost-CC-512__enc.json` |
| `MAMBA-Frost-CC-512` | timing keygen | `records/kem-20/MAMBA-Frost-CC-512__keygen.json` |
| `MAMBA-Frost-CC-512` | hash profile dec | `profile/kem-20/MAMBA-Frost-CC-512__dec.json` |
| `MAMBA-Frost-CC-512` | hash profile enc | `profile/kem-20/MAMBA-Frost-CC-512__enc.json` |
| `MAMBA-Frost-CC-512` | hash profile keygen | `profile/kem-20/MAMBA-Frost-CC-512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

