
// Automatically generated modular arithmetic C code
// Command line : python monty.py 64
// 0x62af377936addf63a1c444dc3495ee857efbb63a67acbd8f9be1b2613d6d03b744ca4184f9080721939ce1b31b9aebfe4581c51d2a08996047c166dc811b8e3f
// Python Script by Mike Scott (Technology Innovation Institute, UAE, 2025)

#include <stdint.h>
#include <stdio.h>

#define sspint int64_t
#define spint uint64_t
#define udpint __uint128_t
#define dpint __uint128_t

#define Wordlength 64
#define Nlimbs 9
#define Radix 57
#define Nbits 511
#define Nbytes 64

#define MONTGOMERY
// propagate carries
static spint inline prop(spint *n) {
  int i;
  spint mask = ((spint)1 << 57u) - (spint)1;
  sspint carry = (sspint)n[0];
  carry >>= 57u;
  n[0] &= mask;
  for (i = 1; i < 8; i++) {
    carry += (sspint)n[i];
    n[i] = (spint)carry & mask;
    carry >>= 57u;
  }
  n[8] += (spint)carry;
  return -((n[8] >> 1) >> 62u);
}

// propagate carries and add p if negative, propagate carries again
static int inline flatten(spint *n) {
  spint carry = prop(n);
  n[0] += ((spint)0x1c166dc811b8e3fu) & carry;
  n[1] += ((spint)0xe28e95044cb023u) & carry;
  n[2] += ((spint)0x6cc6e6baff9160u) & carry;
  n[3] += ((spint)0x9f2100e432739cu) & carry;
  n[4] += ((spint)0x1d6d03b744ca418u) & carry;
  n[5] += ((spint)0x165ec7cdf0d9309u) & carry;
  n[6] += ((spint)0x1ba15fbeed8e99eu) & carry;
  n[7] += ((spint)0xc7438889b8692bu) & carry;
  n[8] += ((spint)0x62af377936addfu) & carry;
  (void)prop(n);
  return (int)(carry & 1);
}

// Montgomery final subtract
static int inline modfsb(spint *n) {
  n[0] -= (spint)0x1c166dc811b8e3fu;
  n[1] -= (spint)0xe28e95044cb023u;
  n[2] -= (spint)0x6cc6e6baff9160u;
  n[3] -= (spint)0x9f2100e432739cu;
  n[4] -= (spint)0x1d6d03b744ca418u;
  n[5] -= (spint)0x165ec7cdf0d9309u;
  n[6] -= (spint)0x1ba15fbeed8e99eu;
  n[7] -= (spint)0xc7438889b8692bu;
  n[8] -= (spint)0x62af377936addfu;
  return flatten(n);
}

// Modular addition - reduce less than 2p
static void inline modadd(const spint *a, const spint *b, spint *n) {
  spint carry;
  n[0] = a[0] + b[0];
  n[1] = a[1] + b[1];
  n[2] = a[2] + b[2];
  n[3] = a[3] + b[3];
  n[4] = a[4] + b[4];
  n[5] = a[5] + b[5];
  n[6] = a[6] + b[6];
  n[7] = a[7] + b[7];
  n[8] = a[8] + b[8];
  n[0] -= (spint)0x382cdb902371c7eu;
  n[1] -= (spint)0x1c51d2a08996046u;
  n[2] -= (spint)0xd98dcd75ff22c0u;
  n[3] -= (spint)0x13e4201c864e738u;
  n[4] -= (spint)0x3ada076e8994830u;
  n[5] -= (spint)0x2cbd8f9be1b2612u;
  n[6] -= (spint)0x3742bf7ddb1d33cu;
  n[7] -= (spint)0x18e87111370d256u;
  n[8] -= (spint)0xc55e6ef26d5bbeu;
  carry = prop(n);
  n[0] += ((spint)0x382cdb902371c7eu) & carry;
  n[1] += ((spint)0x1c51d2a08996046u) & carry;
  n[2] += ((spint)0xd98dcd75ff22c0u) & carry;
  n[3] += ((spint)0x13e4201c864e738u) & carry;
  n[4] += ((spint)0x3ada076e8994830u) & carry;
  n[5] += ((spint)0x2cbd8f9be1b2612u) & carry;
  n[6] += ((spint)0x3742bf7ddb1d33cu) & carry;
  n[7] += ((spint)0x18e87111370d256u) & carry;
  n[8] += ((spint)0xc55e6ef26d5bbeu) & carry;
  (void)prop(n);
}

// Modular subtraction - reduce less than 2p
static void inline modsub(const spint *a, const spint *b, spint *n) {
  spint carry;
  n[0] = a[0] - b[0];
  n[1] = a[1] - b[1];
  n[2] = a[2] - b[2];
  n[3] = a[3] - b[3];
  n[4] = a[4] - b[4];
  n[5] = a[5] - b[5];
  n[6] = a[6] - b[6];
  n[7] = a[7] - b[7];
  n[8] = a[8] - b[8];
  carry = prop(n);
  n[0] += ((spint)0x382cdb902371c7eu) & carry;
  n[1] += ((spint)0x1c51d2a08996046u) & carry;
  n[2] += ((spint)0xd98dcd75ff22c0u) & carry;
  n[3] += ((spint)0x13e4201c864e738u) & carry;
  n[4] += ((spint)0x3ada076e8994830u) & carry;
  n[5] += ((spint)0x2cbd8f9be1b2612u) & carry;
  n[6] += ((spint)0x3742bf7ddb1d33cu) & carry;
  n[7] += ((spint)0x18e87111370d256u) & carry;
  n[8] += ((spint)0xc55e6ef26d5bbeu) & carry;
  (void)prop(n);
}

// Modular negation
static void inline modneg(const spint *b, spint *n) {
  spint carry;
  n[0] = (spint)0 - b[0];
  n[1] = (spint)0 - b[1];
  n[2] = (spint)0 - b[2];
  n[3] = (spint)0 - b[3];
  n[4] = (spint)0 - b[4];
  n[5] = (spint)0 - b[5];
  n[6] = (spint)0 - b[6];
  n[7] = (spint)0 - b[7];
  n[8] = (spint)0 - b[8];
  carry = prop(n);
  n[0] += ((spint)0x382cdb902371c7eu) & carry;
  n[1] += ((spint)0x1c51d2a08996046u) & carry;
  n[2] += ((spint)0xd98dcd75ff22c0u) & carry;
  n[3] += ((spint)0x13e4201c864e738u) & carry;
  n[4] += ((spint)0x3ada076e8994830u) & carry;
  n[5] += ((spint)0x2cbd8f9be1b2612u) & carry;
  n[6] += ((spint)0x3742bf7ddb1d33cu) & carry;
  n[7] += ((spint)0x18e87111370d256u) & carry;
  n[8] += ((spint)0xc55e6ef26d5bbeu) & carry;
  (void)prop(n);
}

// Overflow limit   = 340282366920938463463374607431768211456
// maximum possible = 288847309825165368551956829343810786
// Modular multiplication, c=a*b mod 2p
static void inline modmul(const spint *a, const spint *b, spint *c) {
  dpint t = 0;
  spint p0 = 0x1c166dc811b8e3fu;
  spint p1 = 0xe28e95044cb023u;
  spint p2 = 0x6cc6e6baff9160u;
  spint p3 = 0x9f2100e432739cu;
  spint p4 = 0x1d6d03b744ca418u;
  spint p5 = 0x165ec7cdf0d9309u;
  spint p6 = 0x1ba15fbeed8e99eu;
  spint p7 = 0xc7438889b8692bu;
  spint p8 = 0x62af377936addfu;
  spint q = ((spint)1 << 57u); // q is unsaturated radix
  spint mask = (spint)(q - (spint)1);
  spint ndash = 0x1840b8ce84a9e41u;
  t += (dpint)a[0] * b[0];
  spint v0 = (((spint)t * ndash) & mask);
  t += (dpint)v0 * (dpint)p0;
  t >>= 57;
  t += (dpint)a[0] * b[1];
  t += (dpint)a[1] * b[0];
  t += (dpint)v0 * (dpint)p1;
  spint v1 = (((spint)t * ndash) & mask);
  t += (dpint)v1 * (dpint)p0;
  t >>= 57;
  t += (dpint)a[0] * b[2];
  t += (dpint)a[1] * b[1];
  t += (dpint)a[2] * b[0];
  t += (dpint)v0 * (dpint)p2;
  t += (dpint)v1 * (dpint)p1;
  spint v2 = (((spint)t * ndash) & mask);
  t += (dpint)v2 * (dpint)p0;
  t >>= 57;
  t += (dpint)a[0] * b[3];
  t += (dpint)a[1] * b[2];
  t += (dpint)a[2] * b[1];
  t += (dpint)a[3] * b[0];
  t += (dpint)v0 * (dpint)p3;
  t += (dpint)v1 * (dpint)p2;
  t += (dpint)v2 * (dpint)p1;
  spint v3 = (((spint)t * ndash) & mask);
  t += (dpint)v3 * (dpint)p0;
  t >>= 57;
  t += (dpint)a[0] * b[4];
  t += (dpint)a[1] * b[3];
  t += (dpint)a[2] * b[2];
  t += (dpint)a[3] * b[1];
  t += (dpint)a[4] * b[0];
  t += (dpint)v0 * (dpint)p4;
  t += (dpint)v1 * (dpint)p3;
  t += (dpint)v2 * (dpint)p2;
  t += (dpint)v3 * (dpint)p1;
  spint v4 = (((spint)t * ndash) & mask);
  t += (dpint)v4 * (dpint)p0;
  t >>= 57;
  t += (dpint)a[0] * b[5];
  t += (dpint)a[1] * b[4];
  t += (dpint)a[2] * b[3];
  t += (dpint)a[3] * b[2];
  t += (dpint)a[4] * b[1];
  t += (dpint)a[5] * b[0];
  t += (dpint)v0 * (dpint)p5;
  t += (dpint)v1 * (dpint)p4;
  t += (dpint)v2 * (dpint)p3;
  t += (dpint)v3 * (dpint)p2;
  t += (dpint)v4 * (dpint)p1;
  spint v5 = (((spint)t * ndash) & mask);
  t += (dpint)v5 * (dpint)p0;
  t >>= 57;
  t += (dpint)a[0] * b[6];
  t += (dpint)a[1] * b[5];
  t += (dpint)a[2] * b[4];
  t += (dpint)a[3] * b[3];
  t += (dpint)a[4] * b[2];
  t += (dpint)a[5] * b[1];
  t += (dpint)a[6] * b[0];
  t += (dpint)v0 * (dpint)p6;
  t += (dpint)v1 * (dpint)p5;
  t += (dpint)v2 * (dpint)p4;
  t += (dpint)v3 * (dpint)p3;
  t += (dpint)v4 * (dpint)p2;
  t += (dpint)v5 * (dpint)p1;
  spint v6 = (((spint)t * ndash) & mask);
  t += (dpint)v6 * (dpint)p0;
  t >>= 57;
  t += (dpint)a[0] * b[7];
  t += (dpint)a[1] * b[6];
  t += (dpint)a[2] * b[5];
  t += (dpint)a[3] * b[4];
  t += (dpint)a[4] * b[3];
  t += (dpint)a[5] * b[2];
  t += (dpint)a[6] * b[1];
  t += (dpint)a[7] * b[0];
  t += (dpint)v0 * (dpint)p7;
  t += (dpint)v1 * (dpint)p6;
  t += (dpint)v2 * (dpint)p5;
  t += (dpint)v3 * (dpint)p4;
  t += (dpint)v4 * (dpint)p3;
  t += (dpint)v5 * (dpint)p2;
  t += (dpint)v6 * (dpint)p1;
  spint v7 = (((spint)t * ndash) & mask);
  t += (dpint)v7 * (dpint)p0;
  t >>= 57;
  t += (dpint)a[0] * b[8];
  t += (dpint)a[1] * b[7];
  t += (dpint)a[2] * b[6];
  t += (dpint)a[3] * b[5];
  t += (dpint)a[4] * b[4];
  t += (dpint)a[5] * b[3];
  t += (dpint)a[6] * b[2];
  t += (dpint)a[7] * b[1];
  t += (dpint)a[8] * b[0];
  t += (dpint)v0 * (dpint)p8;
  t += (dpint)v1 * (dpint)p7;
  t += (dpint)v2 * (dpint)p6;
  t += (dpint)v3 * (dpint)p5;
  t += (dpint)v4 * (dpint)p4;
  t += (dpint)v5 * (dpint)p3;
  t += (dpint)v6 * (dpint)p2;
  t += (dpint)v7 * (dpint)p1;
  spint v8 = (((spint)t * ndash) & mask);
  t += (dpint)v8 * (dpint)p0;
  t >>= 57;
  t += (dpint)a[1] * b[8];
  t += (dpint)a[2] * b[7];
  t += (dpint)a[3] * b[6];
  t += (dpint)a[4] * b[5];
  t += (dpint)a[5] * b[4];
  t += (dpint)a[6] * b[3];
  t += (dpint)a[7] * b[2];
  t += (dpint)a[8] * b[1];
  t += (dpint)v1 * (dpint)p8;
  t += (dpint)v2 * (dpint)p7;
  t += (dpint)v3 * (dpint)p6;
  t += (dpint)v4 * (dpint)p5;
  t += (dpint)v5 * (dpint)p4;
  t += (dpint)v6 * (dpint)p3;
  t += (dpint)v7 * (dpint)p2;
  t += (dpint)v8 * (dpint)p1;
  c[0] = ((spint)t & mask);
  t >>= 57;
  t += (dpint)a[2] * b[8];
  t += (dpint)a[3] * b[7];
  t += (dpint)a[4] * b[6];
  t += (dpint)a[5] * b[5];
  t += (dpint)a[6] * b[4];
  t += (dpint)a[7] * b[3];
  t += (dpint)a[8] * b[2];
  t += (dpint)v2 * (dpint)p8;
  t += (dpint)v3 * (dpint)p7;
  t += (dpint)v4 * (dpint)p6;
  t += (dpint)v5 * (dpint)p5;
  t += (dpint)v6 * (dpint)p4;
  t += (dpint)v7 * (dpint)p3;
  t += (dpint)v8 * (dpint)p2;
  c[1] = ((spint)t & mask);
  t >>= 57;
  t += (dpint)a[3] * b[8];
  t += (dpint)a[4] * b[7];
  t += (dpint)a[5] * b[6];
  t += (dpint)a[6] * b[5];
  t += (dpint)a[7] * b[4];
  t += (dpint)a[8] * b[3];
  t += (dpint)v3 * (dpint)p8;
  t += (dpint)v4 * (dpint)p7;
  t += (dpint)v5 * (dpint)p6;
  t += (dpint)v6 * (dpint)p5;
  t += (dpint)v7 * (dpint)p4;
  t += (dpint)v8 * (dpint)p3;
  c[2] = ((spint)t & mask);
  t >>= 57;
  t += (dpint)a[4] * b[8];
  t += (dpint)a[5] * b[7];
  t += (dpint)a[6] * b[6];
  t += (dpint)a[7] * b[5];
  t += (dpint)a[8] * b[4];
  t += (dpint)v4 * (dpint)p8;
  t += (dpint)v5 * (dpint)p7;
  t += (dpint)v6 * (dpint)p6;
  t += (dpint)v7 * (dpint)p5;
  t += (dpint)v8 * (dpint)p4;
  c[3] = ((spint)t & mask);
  t >>= 57;
  t += (dpint)a[5] * b[8];
  t += (dpint)a[6] * b[7];
  t += (dpint)a[7] * b[6];
  t += (dpint)a[8] * b[5];
  t += (dpint)v5 * (dpint)p8;
  t += (dpint)v6 * (dpint)p7;
  t += (dpint)v7 * (dpint)p6;
  t += (dpint)v8 * (dpint)p5;
  c[4] = ((spint)t & mask);
  t >>= 57;
  t += (dpint)a[6] * b[8];
  t += (dpint)a[7] * b[7];
  t += (dpint)a[8] * b[6];
  t += (dpint)v6 * (dpint)p8;
  t += (dpint)v7 * (dpint)p7;
  t += (dpint)v8 * (dpint)p6;
  c[5] = ((spint)t & mask);
  t >>= 57;
  t += (dpint)a[7] * b[8];
  t += (dpint)a[8] * b[7];
  t += (dpint)v7 * (dpint)p8;
  t += (dpint)v8 * (dpint)p7;
  c[6] = ((spint)t & mask);
  t >>= 57;
  t += (dpint)a[8] * b[8];
  t += (dpint)v8 * (dpint)p8;
  c[7] = ((spint)t & mask);
  t >>= 57;
  c[8] = (spint)t;
}

// Modular squaring, c=a*a  mod 2p
static void inline modsqr(const spint *a, spint *c) {
  udpint tot;
  udpint t = 0;
  spint p0 = 0x1c166dc811b8e3fu;
  spint p1 = 0xe28e95044cb023u;
  spint p2 = 0x6cc6e6baff9160u;
  spint p3 = 0x9f2100e432739cu;
  spint p4 = 0x1d6d03b744ca418u;
  spint p5 = 0x165ec7cdf0d9309u;
  spint p6 = 0x1ba15fbeed8e99eu;
  spint p7 = 0xc7438889b8692bu;
  spint p8 = 0x62af377936addfu;
  spint q = ((spint)1 << 57u); // q is unsaturated radix
  spint mask = (spint)(q - (spint)1);
  spint ndash = 0x1840b8ce84a9e41u;
  tot = (udpint)a[0] * a[0];
  t = tot;
  spint v0 = (((spint)t * ndash) & mask);
  t += (udpint)v0 * p0;
  t >>= 57;
  tot = (udpint)a[0] * a[1];
  tot *= 2;
  t += tot;
  t += (udpint)v0 * p1;
  spint v1 = (((spint)t * ndash) & mask);
  t += (udpint)v1 * p0;
  t >>= 57;
  tot = (udpint)a[0] * a[2];
  tot *= 2;
  tot += (udpint)a[1] * a[1];
  t += tot;
  t += (udpint)v0 * p2;
  t += (udpint)v1 * p1;
  spint v2 = (((spint)t * ndash) & mask);
  t += (udpint)v2 * p0;
  t >>= 57;
  tot = (udpint)a[0] * a[3];
  tot += (udpint)a[1] * a[2];
  tot *= 2;
  t += tot;
  t += (udpint)v0 * p3;
  t += (udpint)v1 * p2;
  t += (udpint)v2 * p1;
  spint v3 = (((spint)t * ndash) & mask);
  t += (udpint)v3 * p0;
  t >>= 57;
  tot = (udpint)a[0] * a[4];
  tot += (udpint)a[1] * a[3];
  tot *= 2;
  tot += (udpint)a[2] * a[2];
  t += tot;
  t += (udpint)v0 * p4;
  t += (udpint)v1 * p3;
  t += (udpint)v2 * p2;
  t += (udpint)v3 * p1;
  spint v4 = (((spint)t * ndash) & mask);
  t += (udpint)v4 * p0;
  t >>= 57;
  tot = (udpint)a[0] * a[5];
  tot += (udpint)a[1] * a[4];
  tot += (udpint)a[2] * a[3];
  tot *= 2;
  t += tot;
  t += (udpint)v0 * p5;
  t += (udpint)v1 * p4;
  t += (udpint)v2 * p3;
  t += (udpint)v3 * p2;
  t += (udpint)v4 * p1;
  spint v5 = (((spint)t * ndash) & mask);
  t += (udpint)v5 * p0;
  t >>= 57;
  tot = (udpint)a[0] * a[6];
  tot += (udpint)a[1] * a[5];
  tot += (udpint)a[2] * a[4];
  tot *= 2;
  tot += (udpint)a[3] * a[3];
  t += tot;
  t += (udpint)v0 * p6;
  t += (udpint)v1 * p5;
  t += (udpint)v2 * p4;
  t += (udpint)v3 * p3;
  t += (udpint)v4 * p2;
  t += (udpint)v5 * p1;
  spint v6 = (((spint)t * ndash) & mask);
  t += (udpint)v6 * p0;
  t >>= 57;
  tot = (udpint)a[0] * a[7];
  tot += (udpint)a[1] * a[6];
  tot += (udpint)a[2] * a[5];
  tot += (udpint)a[3] * a[4];
  tot *= 2;
  t += tot;
  t += (udpint)v0 * p7;
  t += (udpint)v1 * p6;
  t += (udpint)v2 * p5;
  t += (udpint)v3 * p4;
  t += (udpint)v4 * p3;
  t += (udpint)v5 * p2;
  t += (udpint)v6 * p1;
  spint v7 = (((spint)t * ndash) & mask);
  t += (udpint)v7 * p0;
  t >>= 57;
  tot = (udpint)a[0] * a[8];
  tot += (udpint)a[1] * a[7];
  tot += (udpint)a[2] * a[6];
  tot += (udpint)a[3] * a[5];
  tot *= 2;
  tot += (udpint)a[4] * a[4];
  t += tot;
  t += (udpint)v0 * p8;
  t += (udpint)v1 * p7;
  t += (udpint)v2 * p6;
  t += (udpint)v3 * p5;
  t += (udpint)v4 * p4;
  t += (udpint)v5 * p3;
  t += (udpint)v6 * p2;
  t += (udpint)v7 * p1;
  spint v8 = (((spint)t * ndash) & mask);
  t += (udpint)v8 * p0;
  t >>= 57;
  tot = (udpint)a[1] * a[8];
  tot += (udpint)a[2] * a[7];
  tot += (udpint)a[3] * a[6];
  tot += (udpint)a[4] * a[5];
  tot *= 2;
  t += tot;
  t += (udpint)v1 * p8;
  t += (udpint)v2 * p7;
  t += (udpint)v3 * p6;
  t += (udpint)v4 * p5;
  t += (udpint)v5 * p4;
  t += (udpint)v6 * p3;
  t += (udpint)v7 * p2;
  t += (udpint)v8 * p1;
  c[0] = ((spint)t & mask);
  t >>= 57;
  tot = (udpint)a[2] * a[8];
  tot += (udpint)a[3] * a[7];
  tot += (udpint)a[4] * a[6];
  tot *= 2;
  tot += (udpint)a[5] * a[5];
  t += tot;
  t += (udpint)v2 * p8;
  t += (udpint)v3 * p7;
  t += (udpint)v4 * p6;
  t += (udpint)v5 * p5;
  t += (udpint)v6 * p4;
  t += (udpint)v7 * p3;
  t += (udpint)v8 * p2;
  c[1] = ((spint)t & mask);
  t >>= 57;
  tot = (udpint)a[3] * a[8];
  tot += (udpint)a[4] * a[7];
  tot += (udpint)a[5] * a[6];
  tot *= 2;
  t += tot;
  t += (udpint)v3 * p8;
  t += (udpint)v4 * p7;
  t += (udpint)v5 * p6;
  t += (udpint)v6 * p5;
  t += (udpint)v7 * p4;
  t += (udpint)v8 * p3;
  c[2] = ((spint)t & mask);
  t >>= 57;
  tot = (udpint)a[4] * a[8];
  tot += (udpint)a[5] * a[7];
  tot *= 2;
  tot += (udpint)a[6] * a[6];
  t += tot;
  t += (udpint)v4 * p8;
  t += (udpint)v5 * p7;
  t += (udpint)v6 * p6;
  t += (udpint)v7 * p5;
  t += (udpint)v8 * p4;
  c[3] = ((spint)t & mask);
  t >>= 57;
  tot = (udpint)a[5] * a[8];
  tot += (udpint)a[6] * a[7];
  tot *= 2;
  t += tot;
  t += (udpint)v5 * p8;
  t += (udpint)v6 * p7;
  t += (udpint)v7 * p6;
  t += (udpint)v8 * p5;
  c[4] = ((spint)t & mask);
  t >>= 57;
  tot = (udpint)a[6] * a[8];
  tot *= 2;
  tot += (udpint)a[7] * a[7];
  t += tot;
  t += (udpint)v6 * p8;
  t += (udpint)v7 * p7;
  t += (udpint)v8 * p6;
  c[5] = ((spint)t & mask);
  t >>= 57;
  tot = (udpint)a[7] * a[8];
  tot *= 2;
  t += tot;
  t += (udpint)v7 * p8;
  t += (udpint)v8 * p7;
  c[6] = ((spint)t & mask);
  t >>= 57;
  tot = (udpint)a[8] * a[8];
  t += tot;
  t += (udpint)v8 * p8;
  c[7] = ((spint)t & mask);
  t >>= 57;
  c[8] = (spint)t;
}

// copy
static void inline modcpy(const spint *a, spint *c) {
  int i;
  for (i = 0; i < 9; i++) {
    c[i] = a[i];
  }
}

// square n times
static void modnsqr(spint *a, int n) {
  int i;
  for (i = 0; i < n; i++) {
    modsqr(a, a);
  }
}

// Calculate progenitor
static void modpro(const spint *w, spint *z) {
  spint x[9];
  spint t0[9];
  spint t1[9];
  spint t2[9];
  spint t3[9];
  spint t4[9];
  spint t5[9];
  spint t6[9];
  spint t7[9];
  spint t8[9];
  spint t9[9];
  spint t10[9];
  spint t11[9];
  spint t12[9];
  spint t13[9];
  spint t14[9];
  spint t15[9];
  modcpy(w, x);
  modsqr(x, t10);
  modmul(x, t10, t5);
  modmul(t10, t5, t9);
  modmul(t10, t9, t0);
  modmul(t10, t0, t12);
  modmul(t10, t12, t4);
  modmul(t10, t4, t13);
  modmul(t10, t13, z);
  modmul(t10, z, t2);
  modmul(t10, t2, t11);
  modmul(t10, t11, t7);
  modmul(t10, t7, t1);
  modmul(t10, t1, t6);
  modmul(t10, t6, t3);
  modmul(t10, t3, t8);
  modmul(t10, t8, t14);
  modmul(t2, t14, t15);
  modmul(z, t15, t10);
  modnsqr(t15, 4);
  modmul(t7, t15, t15);
  modnsqr(t15, 5);
  modmul(z, t15, t15);
  modnsqr(t15, 7);
  modmul(t3, t15, t15);
  modnsqr(t15, 5);
  modmul(t1, t15, t15);
  modnsqr(t15, 4);
  modmul(t12, t15, t15);
  modnsqr(t15, 7);
  modmul(t3, t15, t15);
  modnsqr(t15, 6);
  modmul(t7, t15, t15);
  modnsqr(t15, 5);
  modmul(t1, t15, t15);
  modnsqr(t15, 6);
  modmul(t14, t15, t15);
  modnsqr(t15, 3);
  modmul(t5, t15, t15);
  modnsqr(t15, 8);
  modmul(t8, t15, t15);
  modnsqr(t15, 7);
  modmul(t0, t15, t15);
  modnsqr(t15, 8);
  modmul(t2, t15, t15);
  modnsqr(t15, 8);
  modmul(t11, t15, t15);
  modnsqr(t15, 4);
  modmul(t0, t15, t15);
  modnsqr(t15, 8);
  modmul(t13, t15, t15);
  modnsqr(t15, 6);
  modmul(t12, t15, t15);
  modnsqr(t15, 6);
  modmul(t1, t15, t15);
  modnsqr(t15, 5);
  modmul(t1, t15, t15);
  modnsqr(t15, 2);
  modmul(x, t15, t15);
  modnsqr(t15, 7);
  modmul(t9, t15, t15);
  modnsqr(t15, 7);
  modmul(t10, t15, t15);
  modnsqr(t15, 6);
  modmul(t14, t15, t15);
  modnsqr(t15, 6);
  modmul(t8, t15, t15);
  modnsqr(t15, 4);
  modmul(t4, t15, t15);
  modnsqr(t15, 8);
  modmul(t8, t15, t15);
  modnsqr(t15, 7);
  modmul(t6, t15, t15);
  modnsqr(t15, 5);
  modmul(t8, t15, t15);
  modnsqr(t15, 6);
  modmul(t6, t15, t15);
  modnsqr(t15, 5);
  modmul(z, t15, t15);
  modnsqr(t15, 3);
  modmul(t5, t15, t15);
  modnsqr(t15, 8);
  modmul(t14, t15, t14);
  modnsqr(t14, 7);
  modmul(t3, t14, t14);
  modnsqr(t14, 3);
  modmul(t0, t14, t14);
  modnsqr(t14, 9);
  modmul(t3, t14, t14);
  modnsqr(t14, 7);
  modmul(t11, t14, t14);
  modnsqr(t14, 9);
  modmul(t11, t14, t14);
  modnsqr(t14, 4);
  modmul(t13, t14, t13);
  modnsqr(t13, 6);
  modmul(t3, t13, t13);
  modnsqr(t13, 2);
  modmul(x, t13, t13);
  modnsqr(t13, 11);
  modmul(t8, t13, t13);
  modnsqr(t13, 5);
  modmul(t1, t13, t13);
  modnsqr(t13, 6);
  modmul(t2, t13, t13);
  modnsqr(t13, 7);
  modmul(t6, t13, t13);
  modnsqr(t13, 5);
  modmul(t12, t13, t12);
  modnsqr(t12, 7);
  modmul(t5, t12, t12);
  modnsqr(t12, 9);
  modmul(t11, t12, t12);
  modnsqr(t12, 3);
  modmul(t0, t12, t12);
  modnsqr(t12, 3);
  modmul(x, t12, t12);
  modnsqr(t12, 5);
  modmul(x, t12, t12);
  modnsqr(t12, 11);
  modmul(t0, t12, t12);
  modnsqr(t12, 3);
  modmul(x, t12, t12);
  modnsqr(t12, 9);
  modmul(t6, t12, t12);
  modnsqr(t12, 5);
  modmul(t0, t12, t12);
  modnsqr(t12, 5);
  modmul(t0, t12, t12);
  modnsqr(t12, 5);
  modmul(t0, t12, t12);
  modnsqr(t12, 9);
  modmul(t3, t12, t12);
  modnsqr(t12, 4);
  modmul(t5, t12, t12);
  modnsqr(t12, 8);
  modmul(t3, t12, t12);
  modnsqr(t12, 5);
  modmul(t11, t12, t11);
  modnsqr(t11, 6);
  modmul(t1, t11, t11);
  modnsqr(t11, 2);
  modmul(x, t11, t11);
  modnsqr(t11, 7);
  modmul(t10, t11, t10);
  modnsqr(t10, 3);
  modmul(t0, t10, t10);
  modnsqr(t10, 7);
  modmul(t2, t10, t10);
  modnsqr(t10, 3);
  modmul(t5, t10, t10);
  modnsqr(t10, 9);
  modmul(t0, t10, t10);
  modnsqr(t10, 6);
  modmul(t9, t10, t9);
  modnsqr(t9, 8);
  modmul(t8, t9, t8);
  modnsqr(t8, 7);
  modmul(t7, t8, t7);
  modnsqr(t7, 10);
  modmul(t2, t7, t7);
  modnsqr(t7, 7);
  modmul(t6, t7, t6);
  modnsqr(t6, 3);
  modmul(t5, t6, t5);
  modnsqr(t5, 11);
  modmul(t2, t5, t5);
  modnsqr(t5, 4);
  modmul(z, t5, t5);
  modnsqr(t5, 9);
  modmul(t4, t5, t4);
  modnsqr(t4, 7);
  modmul(t3, t4, t3);
  modnsqr(t3, 4);
  modmul(t0, t3, t3);
  modnsqr(t3, 3);
  modmul(x, t3, t3);
  modnsqr(t3, 11);
  modmul(t2, t3, t2);
  modnsqr(t2, 5);
  modmul(t1, t2, t1);
  modnsqr(t1, 6);
  modmul(t0, t1, t0);
  modnsqr(t0, 7);
  modmul(z, t0, z);
}

// calculate inverse, provide progenitor h if available
static void modinv(const spint *x, const spint *h, spint *z) {
  spint s[9];
  spint t[9];
  if (h == NULL) {
    modpro(x, t);
  } else {
    modcpy(h, t);
  }
  modcpy(x, s);
  modnsqr(t, 2);
  modmul(s, t, z);
}

// Convert m to n-residue form, n=nres(m)
static void nres(const spint *m, spint *n) {
  const spint c[9] = {
      0x168ffa06cd3d351u, 0x60213451e77702u,  0xde48727537f354u,
      0x1bb5cb50f081716u, 0x1082c53c69e264eu, 0x54e3c02f7d721u,
      0x2fa04b8949c29fu,  0x3efff906413701u,  0x535e5c00ce4773u};
  modmul(m, c, n);
}

// Convert n back to normal form, m=redc(n)
static void redc(const spint *n, spint *m) {
  int i;
  spint c[9];
  c[0] = 1;
  for (i = 1; i < 9; i++) {
    c[i] = 0;
  }
  modmul(n, c, m);
  (void)modfsb(m);
}

// is unity?
static int modis1(const spint *a) {
  int i;
  spint c[9];
  spint c0;
  spint d = 0;
  redc(a, c);
  for (i = 1; i < 9; i++) {
    d |= c[i];
  }
  c0 = (spint)c[0];
  return ((spint)1 & ((d - (spint)1) >> 57u) &
          (((c0 ^ (spint)1) - (spint)1) >> 57u));
}

// is zero?
static int modis0(const spint *a) {
  int i;
  spint c[9];
  spint d = 0;
  redc(a, c);
  for (i = 0; i < 9; i++) {
    d |= c[i];
  }
  return ((spint)1 & ((d - (spint)1) >> 57u));
}

// set to zero
static void modzer(spint *a) {
  int i;
  for (i = 0; i < 9; i++) {
    a[i] = 0;
  }
}

// set to one
static void modone(spint *a) {
  int i;
  a[0] = 1;
  for (i = 1; i < 9; i++) {
    a[i] = 0;
  }
  nres(a, a);
}

// set to integer
static void modint(int x, spint *a) {
  int i;
  a[0] = (spint)x;
  for (i = 1; i < 9; i++) {
    a[i] = 0;
  }
  nres(a, a);
}

// Modular multiplication by an integer, c=a*b mod 2p
// uses special method for trinomials, otherwise Barrett-Dhem reduction
static void inline modmli(const spint *a, int b, spint *c) {
  spint p0 = 0x1c166dc811b8e3fu;
  spint p1 = 0xe28e95044cb023u;
  spint p2 = 0x6cc6e6baff9160u;
  spint p3 = 0x9f2100e432739cu;
  spint p4 = 0x1d6d03b744ca418u;
  spint p5 = 0x165ec7cdf0d9309u;
  spint p6 = 0x1ba15fbeed8e99eu;
  spint p7 = 0xc7438889b8692bu;
  spint p8 = 0x62af377936addfu;
  spint mask = ((spint)1 << 57u) - (spint)1;
  udpint t = 0;
  spint q, h, r = 0x29818b9f87748ea;
  t += (udpint)a[0] * (udpint)b;
  c[0] = (spint)t & mask;
  t = t >> 57u;
  t += (udpint)a[1] * (udpint)b;
  c[1] = (spint)t & mask;
  t = t >> 57u;
  t += (udpint)a[2] * (udpint)b;
  c[2] = (spint)t & mask;
  t = t >> 57u;
  t += (udpint)a[3] * (udpint)b;
  c[3] = (spint)t & mask;
  t = t >> 57u;
  t += (udpint)a[4] * (udpint)b;
  c[4] = (spint)t & mask;
  t = t >> 57u;
  t += (udpint)a[5] * (udpint)b;
  c[5] = (spint)t & mask;
  t = t >> 57u;
  t += (udpint)a[6] * (udpint)b;
  c[6] = (spint)t & mask;
  t = t >> 57u;
  t += (udpint)a[7] * (udpint)b;
  c[7] = (spint)t & mask;
  t = t >> 57u;
  t += (udpint)a[8] * (udpint)b;
  c[8] = (spint)t & mask;

  // Barrett-Dhem reduction
  h = (spint)(t >> 48u);
  q = (spint)(((udpint)h * (udpint)r) >> 64u);
  t = (udpint)q * (udpint)p0;
  c[0] -= (spint)t & mask;
  c[1] -= (spint)(t >> 57u);
  t = (udpint)q * (udpint)p1;
  c[1] -= (spint)t & mask;
  c[2] -= (spint)(t >> 57u);
  t = (udpint)q * (udpint)p2;
  c[2] -= (spint)t & mask;
  c[3] -= (spint)(t >> 57u);
  t = (udpint)q * (udpint)p3;
  c[3] -= (spint)t & mask;
  c[4] -= (spint)(t >> 57u);
  t = (udpint)q * (udpint)p4;
  c[4] -= (spint)t & mask;
  c[5] -= (spint)(t >> 57u);
  t = (udpint)q * (udpint)p5;
  c[5] -= (spint)t & mask;
  c[6] -= (spint)(t >> 57u);
  t = (udpint)q * (udpint)p6;
  c[6] -= (spint)t & mask;
  c[7] -= (spint)(t >> 57u);
  t = (udpint)q * (udpint)p7;
  c[7] -= (spint)t & mask;
  c[8] -= (spint)(t >> 57u);
  c[8] = (c[8] - (q * p8)) & mask;
  (void)prop(c);
}

// Test for quadratic residue
static int modqr(const spint *h, const spint *x) {
  spint r[9];
  if (h == NULL) {
    modpro(x, r);
    modsqr(r, r);
  } else {
    modsqr(h, r);
  }
  modmul(r, x, r);
  return modis1(r) | modis0(x);
}

// conditional move g to f if d=1
// strongly recommend inlining be disabled using compiler specific syntax
static void __attribute__((noinline))
modcmv(int b, const spint *g, volatile spint *f) {
  int i;
  spint c0, c1, s, t, w, aux;
  static spint R = 0;
  R += 0x3cc3c33c5aa5a55au;
  w = R;
  c0 = (~b) & (w + 1);
  c1 = b + w;
  for (i = 0; i < 9; i++) {
    s = g[i];
    t = f[i];
    f[i] = aux = c0 * t + c1 * s;
    f[i] = aux - w * (t + s);
  }
}

// conditional swap g and f if d=1
// strongly recommend inlining be disabled using compiler specific syntax
static void __attribute__((noinline))
modcsw(int b, volatile spint *g, volatile spint *f) {
  int i;
  spint c0, c1, s, t, w, v, aux;
  static spint R = 0;
  R += 0x3cc3c33c5aa5a55au;
  w = R;
  c0 = (~b) & (w + 1);
  c1 = b + w;
  for (i = 0; i < 9; i++) {
    s = g[i];
    t = f[i];
    v = w * (t + s);
    f[i] = aux = c0 * t + c1 * s;
    f[i] = aux - v;
    g[i] = aux = c0 * s + c1 * t;
    g[i] = aux - v;
  }
}

// Modular square root, provide progenitor h if available, NULL if not
static void modsqrt(const spint *x, const spint *h, spint *r) {
  spint s[9];
  spint y[9];
  if (h == NULL) {
    modpro(x, y);
  } else {
    modcpy(h, y);
  }
  modmul(y, x, s);
  modcpy(s, r);
}

// shift left by less than a word
static void modshl(unsigned int n, spint *a) {
  int i;
  a[8] = ((a[8] << n)) | (a[7] >> (57u - n));
  for (i = 7; i > 0; i--) {
    a[i] = ((a[i] << n) & (spint)0x1ffffffffffffff) | (a[i - 1] >> (57u - n));
  }
  a[0] = (a[0] << n) & (spint)0x1ffffffffffffff;
}

// shift right by less than a word. Return shifted out part
static int modshr(unsigned int n, spint *a) {
  int i;
  spint r = a[0] & (((spint)1 << n) - (spint)1);
  for (i = 0; i < 8; i++) {
    a[i] = (a[i] >> n) | ((a[i + 1] << (57u - n)) & (spint)0x1ffffffffffffff);
  }
  a[8] = a[8] >> n;
  return r;
}

// set a= 2^r
static void mod2r(unsigned int r, spint *a) {
  unsigned int n = r / 57u;
  unsigned int m = r % 57u;
  modzer(a);
  if (r >= 64 * 8)
    return;
  a[n] = 1;
  a[n] <<= m;
  nres(a, a);
}

// export to byte array
static void modexp(const spint *a, char *b) {
  int i;
  spint c[9];
  redc(a, c);
  for (i = 63; i >= 0; i--) {
    b[i] = c[0] & (spint)0xff;
    (void)modshr(8, c);
  }
}

// import from byte array
// returns 1 if in range, else 0
static int modimp(const char *b, spint *a) {
  int i, res;
  for (i = 0; i < 9; i++) {
    a[i] = 0;
  }
  for (i = 0; i < 64; i++) {
    modshl(8, a);
    a[0] += (spint)(unsigned char)b[i];
  }
  res = modfsb(a);
  nres(a, a);
  return res;
}

// determine sign
static int modsign(const spint *a) {
  spint c[9];
  redc(a, c);
  return c[0] % 2;
}

// return true if equal
static int modcmp(const spint *a, const spint *b) {
  spint c[9], d[9];
  int i, eq = 1;
  redc(a, c);
  redc(b, d);
  for (i = 0; i < 9; i++) {
    eq &= (((c[i] ^ d[i]) - 1) >> 57) & 1;
  }
  return eq;
}

// clang-format on
/******************************************************************************
 API functions calling generated code above
 ******************************************************************************/

#include <fp.h>

void
fp_set(fp_t *x, const digit_t val)
{
    modint((int)val, *x);
}

// void fp_set(fp_t* x, const digit_t val)
// { // Set field element x = val, where val has wordsize

//     (*x)[0] = val;
//     for (unsigned int i = 1; i < NWORDS_FIELD; i++) {
//         (*x)[i] = 0;
//     }
// }

bool
fp_is_equal(const fp_t *a, const fp_t *b)
{
    // return -(uint32_t)modcmp(*a, *b);
    return (bool)modcmp(*a, *b);
}

bool
fp_is_zero(const fp_t *a)
{
    // return -(uint32_t)modis0(*a);
    return (bool)modis0(*a);
}

uint32_t
fp_is_zero_mask(const fp_t *a)
{
    return -(uint32_t)modis0(*a);
}

void
fp_copy(fp_t *out, const fp_t *a)
{
    modcpy(*a, *out);
}

void
fp_add(fp_t *out, const fp_t *a, const fp_t *b)
{
    modadd(*a, *b, *out);
}

void
fp_sub(fp_t *out, const fp_t *a, const fp_t *b)
{
    modsub(*a, *b, *out);
}

void
fp_neg(fp_t *out, const fp_t *a)
{
    modneg(*a, *out);
}

void
fp_sqr(fp_t *out, const fp_t *a)
{
    modsqr(*a, *out);
}

void
fp_mul(fp_t *out, const fp_t *a, const fp_t *b)
{
    modmul(*a, *b, *out);
}

void
fp_inv(fp_t *x)
{
    modinv(*x, NULL, *x);
}

bool
fp_is_square(const fp_t *a)
{
    return (bool)modqr(NULL, *a);
}

void
fp_sqrt(fp_t *a)
{
    modsqrt(*a, NULL, *a);
}

void
fp_exp3div4(fp_t *out, const fp_t *a)
{
    modpro(*a, *out);
}

void
fp_tomont(fp_t* out, const fp_t* a)
{
    nres(*a, *out);
}

void
fp_frommont(fp_t* out, const fp_t* a)
{
    redc(*a, *out);
}

void
fp_mont_setone(fp_t *x)
{
    modone(*x);
}

void fp_swap(digit_t* P, digit_t* Q, const digit_t option)
{ // If option = 0 then P <- P and Q <- Q, else if option = 0xFF...FF then P <- Q and Q <- P
    digit_t temp;

    for (int i = 0; i < NWORDS_FIELD; i++) {
        temp = option & (P[i] ^ Q[i]);
        P[i] = temp ^ P[i];
        Q[i] = temp ^ Q[i];
    }
}

/*
 * If ctl == 0x00000000, then *d is set to a0
 * If ctl == 0xFFFFFFFF, then *d is set to a1
 * ctl MUST be either 0x00000000 or 0xFFFFFFFF.
 */
void fp_select(fp_t *d, const fp_t *a0, const fp_t *a1, uint32_t ctl)
{
    digit_t cw = (int32_t)ctl;
    for (unsigned int i = 0; i < NWORDS_FIELD; i++) {
        (*d)[i] = (*a0)[i] ^ (cw & ((*a0)[i] ^ (*a1)[i]));
    }
}

void
fp_encode(void *dst, const fp_t *a)
{
    // Modified version of modexp()
    int i;
    spint c[9];
    redc(*a, c);
    for (i = 0; i < 64; i++) {
        ((char *)dst)[i] = c[0] & (spint)0xff;
        (void)modshr(8, c);
    }
}

uint32_t
fp_decode(fp_t *d, const void *src)
{
    // Modified version of modimp()
    int i;
    spint res;
    const unsigned char *b = src;
    for (i = 0; i < 9; i++) {
        (*d)[i] = 0;
    }
    for (i = 63; i >= 0; i--) { //?
        modshl(8, *d);
        (*d)[0] += (spint)b[i];
    }
    res = (spint)-modfsb(*d);
    nres(*d, *d);
    // If the value was canonical then res = -1; otherwise, res = 0
    for (i = 0; i < 9; i++) {
        (*d)[i] &= res;
    }
    return (uint32_t)res;
}