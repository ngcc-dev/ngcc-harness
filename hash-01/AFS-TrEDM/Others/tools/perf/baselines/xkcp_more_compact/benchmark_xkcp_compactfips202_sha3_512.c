/*
 * SHA3-512 XKCP CompactFIPS202 baseline.
 *
 * Source: official XKCP Standalone/CompactFIPS202/C/Keccak-more-compact.c.
 * This is intentionally distinct from SHA3-512 Reference C, which uses the
 * readable-and-compact official source.
 */
#define SHA3_BENCH_BACKEND "xkcp-compactfips202-sha3-512-more-compact"
#define SHA3_BENCH_IMPLEMENTATION "SHA3-512 XKCP CompactFIPS202 (XKCP more-compact)"
#define SHA3_BENCH_SOURCE_ORIGIN "official XKCP Standalone/CompactFIPS202/C/Keccak-more-compact.c"
#define SHA3_BENCH_SOURCE_FILE "tools/perf/baselines/xkcp_more_compact/Keccak-more-compact.c"
#define SHA3_BENCH_SOURCE_SHA256 "089b4cbd9e9ec314ce9e00514dde4d90678f861b7d754e05e4726eb38cb702a2"

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#include "Keccak-more-compact.c"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#include "../sha3_512_xkcp_bench_driver.h"
