# Performance method and limitations — system arm_1

System arm_1: Qualcomm Snapdragon X Elite X1E-78-100 (Oryon), one core, clock pinned at 2.71 GHz (cpufreq performance, min = max), no SMT. [Summary](summary_arm_1.md).

## Measurement

- One Qualcomm Oryon core (CPU 2; fixed 2.71 GHz (governor performance, minimum = maximum), boost off, SMT none). Cycles come from the hardware counter (`perf_event_open`, user mode); time from `CLOCK_MONOTONIC_RAW`. All 2746 timing records were taken in this state (clock pinned at 2.71 GHz on the clusters of the timing cores: `performance` governor with minimum = maximum frequency, below the level at which the firmware's thermal limiter intervenes; boost off; no SMT; hardware cycle counter available), as stored in each record. A trial that ran below 95% of that clock was repeated, up to twice.
- Reference builds use the guide's flags `-std=c99 -Wpedantic -Wall -Wextra -O2` plus `-fPIC -D_GNU_SOURCE -Wno-error=implicit-function-declaration -Wno-error=incompatible-pointer-types -Wno-error=int-conversion -Wno-error=implicit-int` for shared libraries and pre-C99 declarations; an instance that fails to build or to pass its KATs that way is rebuilt with the harness defaults, and its page says so. Optimized builds use the guide's performance flags.
- Every library is checked against the submitted KAT vectors before timing. Instances whose vectors are not reproduced by the submitted code are still timed and are marked ⚠, with the identified cause on their candidate page (`performance/kat_issues.csv`); an instance without a reference source of its own is not timed. The DRNG is seeded with bytes 00..2f; signatures use a 64-byte message; hash inputs are the guide's S1–S8 lengths.
- Each operation is calibrated with one call, then measured in 5 trials. Trials normally run in separate processes (fresh address-space layout); if setup takes more than 60 s, the trials share one process. Each trial runs a fixed number of calls, targeting 0.5 s and at least 100 calls in total; if that would exceed 5 minutes, fewer calls are used, but never fewer than one per trial. No operation is cut short by a time limit.
- Reported cycles are the arithmetic mean over all timed calls (the guide's metric); the median of the trial means is also recorded as a robustness check.
- Key exchange: `exchange` covers both initialisations, every pass and both key derivations of one protocol run (no network time); single steps are timed call by call.

## Share of the ICCS placeholder functions

Each reference library is relinked with link-time wrappers (`-Wl,--wrap`) around `pseudohash`, `pseudoXOF`, `sm3hash` and the DRNG's `get_random_number`. Every call that crosses an object-file boundary is timed with the CPU tick counter and recorded with its input and output length; nested calls are not counted twice. The reported share is the time inside these functions divided by the time of the whole operation, measured in the same process on the same inputs as the benchmark. The wrappers cost a few tens of cycles per call. On this system the tick counter is `cntvct_el0` at 19.2 MHz, so a single short call is resolved only to about 52 ns; the rounding is unbiased and averages out over the many calls that make up a share.

The ICCS helpers `sm3hash`, `pseudohash` and `pseudoXOF` are also timed directly, as hash instances of `api/auxfunc.c` (`performance/iccs`, records under `iccs/`): same reference flags, driver, message lengths and planning as the hash candidates. Before timing, each is checked against an independent model on OpenSSL's SM3, and `sm3hash` against the GB/T 32905 examples. The summary divides each hash candidate's cycles by those of `pseudoXOF` with the same output width and message length. This is a direct, self-contained relative measurement, not an estimate of substituting that hash into a public-key scheme. Several hash submissions are not yet constant-time (e.g. table-based S-boxes), so their current timings are not production figures; the summary flags known cases.

## Limitations

- Static and peak memory are process-level proxies (ELF image, VmHWM), not isolated algorithm memory.
- The ICCS share covers only the three helper functions; candidates that implement their own SHAKE/AES/SM3 show that time as non-symmetric (see the survey).
- Link-time wrapping is not reliable with LTO, so shares are measured on reference builds.
- The complete functional test vectors are referenced by digest, not embedded.
- Very slow operations have fewer than 100 timed calls; their pages say how many. Operations too slow for more than one call use their calibration call as the measurement.
- Timing ran in parallel on cores 2, 3, 6, 7. The instances were divided among cores 2, 3, 7 by expected run time; the slowest instances (sign-30 TRINE-512-Balanced, sign-30 TRINE-512-ShortSig, sign-32 UVW-512) ran on a further core. All operations of one instance ran on the same core, and each record states its CPU. Cycle counts are comparable across these identical cores.
- A single key-exchange step is timed around each call, so its wall time includes the counter start/stop system calls (a floor of roughly a microsecond); its user-mode cycle count does not. The `exchange` figure has no such overhead.

