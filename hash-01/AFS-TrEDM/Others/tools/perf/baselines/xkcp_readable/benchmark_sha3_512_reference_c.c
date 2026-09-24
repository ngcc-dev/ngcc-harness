/*
 * SHA3-512 Reference C baseline.
 *
 * Source: official XKCP Standalone/CompactFIPS202/C/
 * Keccak-readable-and-compact.c.
 */
#define SHA3_BENCH_BACKEND "sha3-512-reference-c-xkcp-readable"
#define SHA3_BENCH_IMPLEMENTATION "SHA3-512 Reference C (XKCP readable)"
#define SHA3_BENCH_SOURCE_ORIGIN "official XKCP Standalone/CompactFIPS202/C/Keccak-readable-and-compact.c"
#define SHA3_BENCH_SOURCE_FILE "tools/perf/baselines/xkcp_readable/Keccak-readable-and-compact.c"
#define SHA3_BENCH_SOURCE_SHA256 "7d25b518f28b4b9be141495dde205621fdb528e880596ef9507d30ba1b937dc9"

#include "Keccak-readable-and-compact.c"
#include "../sha3_512_xkcp_bench_driver.h"
