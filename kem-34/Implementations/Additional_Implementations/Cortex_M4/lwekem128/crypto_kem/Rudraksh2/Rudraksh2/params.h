#ifndef PARAMS_H
#define PARAMS_H

#ifndef KEM_L
#define KEM_L 9 /* Change this for different security strengths */
#endif

/* Don't change parameters below this line */

#define KEM_NAMESPACE(s) kem_light_ref_##s

#define KEM_N 64
#define KEM_Q 3329

/**** Precomputed Values for Parametrisation ****/
// Bit widths
#define LOG2Q (12)
#define LOG2P (12)
#define LOG2T (6)
// Polynomial Compression
#define KEM_Q_2 (1664)				// round(KEM_Q/2)
#define KEM_Q_DENOM (20159)		// round(2^(32-LOG2T)/KEM_Q)
#define KEM_32_LOG2T (33554432) // 2^(32 - LOG2T - 1)
#define LOG2T_BITMASK (0x3f)	// Take the bottom LOG2T bits
#define LOG2P_BITMASK (0xfff) // Take the bottom LOG2P bits
#define KEM_Q_232 (1290168)		// floor((2^32/KEM_Q) + 1)
#define KEM_Q_230 (322542)		// floor((2^30/KEM_Q) + 1)
#define KEM_Q_INV (3327)			// -inverse_mod(KEM_Q, 2^16)
#define RLOG (18)							// rlog for modular reduction, previous 16
#define RMASK (0x3ffff)				// rlog for modular reduction, previous 0xffff
#define INV_2_Q (2807)				// 2^(-1) * 2^16 mod KEM_Q
// #define INV_2_Q (1749) // (2^16)^-1 / 2 mod KEM_Q
#define MONT (2285)		// 2^16 mod KEM_Q
#define MONT_2 (1674) // 2^18 * 2^18(1353) // 2^16 * 2^16 mod KEM_Q

// NTT
#define ROOT_OF_UNITY (33)

#define KEM_ETA 2

#define KEM_SYMBYTES 16 // 32   /* size in bytes of hashes, and seeds */
#define KEM_SSBYTES 16	// 32   /* size in bytes of shared key */

#define KEM_POLYBYTES (KEM_N * LOG2Q) / 8
#define KEM_POLYVECBYTES (KEM_L * KEM_POLYBYTES)

#define KEM_POLYCOMPRESSEDBYTES (KEM_N * LOG2T) / 8
#define KEM_POLYVECCOMPRESSEDBYTES (KEM_L * KEM_N * LOG2P) / 8

// #define KEM_POLYVECCOMPRESSEDBYTES KEM_POLYVECBYTES //XXX

#define KEM_INDCPA_MSGBYTES KEM_SYMBYTES
#define KEM_INDCPA_PUBLICKEYBYTES (KEM_POLYVECBYTES + KEM_SYMBYTES)
#define KEM_INDCPA_SECRETKEYBYTES (KEM_POLYVECBYTES)
#define KEM_INDCPA_BYTES (KEM_POLYVECCOMPRESSEDBYTES + KEM_POLYCOMPRESSEDBYTES)

#define KEM_PUBLICKEYBYTES (KEM_INDCPA_PUBLICKEYBYTES)
#define KEM_SECRETKEYBYTES                                                    \
	(KEM_INDCPA_SECRETKEYBYTES + KEM_INDCPA_PUBLICKEYBYTES                      \
	 + 2 * KEM_SYMBYTES) /* 32 bytes of additional space to save H(pk) */
#define KEM_CIPHERTEXTBYTES KEM_INDCPA_BYTES

// NTT CONSTANTS
// For in montgomery form
// omega^i
// #define ZETAS 
//	{ 1729, 749,	40,		2699, 848,	2642, 1432, 797,	569,	1062, 69, 
//		3136, 1746, 1919, 2786, 1089, 2393, 3033, 447,	56,		1355, 1339, 
//		1903, 1996, 2879, 882,	535,	283,	2508, 1476, 1235, 33,		2647, 
//		2998, 2402, 2513, 219,	2132, 1435, 1414, 1848, 1756, 1438, 1352, 
//		910,	2277, 2877, 464,	2617, 289,	1795, 632,	2474, 1025, 1010,
//		1320, 2681, 76,		2868, 650,	2102, 2055, 807 }

// For ntt + conversion to montgomery simultaneous
// 2^16 * 2^16 * omega^i
// #define ZETAS 
//	{ 2379, 1381, 856,	3163, 2168, 2609, 18,		3074, 858,	2087, 145, 
//		1862, 2077, 3116, 1030, 1999, 1941, 2321, 2242, 2530, 2365, 691, 
//		1442, 769,	357,	1564, 1462, 64,		1073, 2957, 3126, 1372, 2716, 
//		1572, 802,	1180, 26,		1682, 748,	2296, 265,	2291, 1478, 1635, 
//		2829, 1456, 980,	1940, 2074, 1524, 1794, 2872, 1677, 1961, 1640,
// 		1616, 2112, 2958, 2119, 594,	1040, 700,	3288 }

// Inverse zetas for invntt
// For in standard form
// 2^16 / 2 * omega^-i
#define INV_ZETAS                                                             \
	{ 1800, 772,	2003, 3071, 2375, 3053, 1302, 3266, 1238, 2410, 3105,         \
		333,	1541, 1053, 1184, 2520, 415,	141,	2302, 3325, 1611, 1157,         \
		2575, 2399, 45,		1018, 1132, 160,	2140, 326,	199,	581,	2173,         \
		1473, 879,	1250, 2963, 1002, 1459, 3264, 1324, 3197, 1562, 2600,         \
		304,	1951, 771,	2528, 2848, 3018, 2595, 2453, 2728, 1750, 737,          \
		3238, 1808, 918,	3228, 711,	906,	1485, 379 }

// For in montgomery form
// omega^-i
// #define INV_ZETAS 
//	{ 2522, 1274, 1227, 2679, 461,	3253, 648,	2009, 2319, 2304, 855, 
//		2697, 1534, 3040, 712,	2865, 452,	1052, 2419, 1977, 1891, 1573, 
//		1481, 1915, 1894, 1197, 3110, 816,	927,	331,	682,	3296, 2094, 
//		1853, 821,	3046, 2794, 2447, 450,	1333, 1426, 1990, 1974, 3273, 
//		2882, 296,	936,	2240, 543,	1410, 1583, 193,	3260, 2267, 2760,
//\ 		2532, 1897, 687,	2481, 630,	3289, 2580, 1600 }

// For ntt + conversion to montgomery simultaneous
// 2^16 * 2^16 / 2 * omega^i
// #define INV_ZETAS 
//	{ 1685, 2979, 2809, 3032, 605,	1850, 2273, 2521, 2509, 684,	826, 
//		1893, 2432, 2567, 2292, 2359, 2839, 2601, 250,	847,	2590, 519, 
//		1532, 2181, 2955, 2488, 3316, 2739, 2928, 2543, 1971, 2643, 1766,
//		186,	1128, 3297, 2598, 2547, 1486, 1280, 2608, 1319, 482,	2064, 
//		2208, 504,	694,	665,	2814, 1771, 626,	2398, 1592, 621,	2900,
//\ 		1792, 3320, 360,	2245, 83,		2901, 974,	475 }

// #define INV_ZETAS 
//	{ 53,		1125, 2147, 1668, 671,	236,	1492, 1646, 1209, 1606, 674, 
//		3189, 3121, 547,	242,	740,	1575, 2340, 3001, 2271, 1662, 1423, 
//		307,	361,	251,	2941, 3133, 2372, 100,	3002, 1036, 2205, 506, 
//		1780, 1130, 1054, 3063, 2038, 1406, 1117, 653,	1705, 353,	1926, 
//		512,	1709, 2525, 2856, 942,	2630, 2268, 1328, 2492, 144,	190,
//\ 		898, 2169, 3123, 1582, 3300, 3278, 1625, 2040 }


// (pow(omega_inv,bitrev(i)) * mont) & KEM_Q [upto the first 32 elements]
#define OMEGAS_INV_BITREV_MONTGOMERY { 2482, 3032, 590,	1893, 1148, 2521, 686, 2359, 2810, 1850, 786, 2567, 841,	684,	2979, 2601, 739,	605, 401,	 2432, 374,	 2509, 1685, 2839, 1797, 2273, 1358, 2292, 13,	 826,	 2809, 250 }
// (pow(omega_inv,bit(i)) * n_inv * mont) & KEM_Q
#define PSIS_INV_MONTGOMERY { 767,	225,	1520, 1761, 356,	2331, 3097, 800,	226,	713,	526,1630, 2874, 995,	2653, 2905, 2610, 987,	2451, 2798, 2405, 3301, 2622, 1290, 947,	1441, 2263, 1380, 1555, 148,	408,	315,	2128, 468,	1830, 1266, 341,	1120, 1648, 1664, 2068, 2282, 2692, 1393, 1051, 738,	325,	716,	1434, 1254, 38,		2624, 3005, 1806, 660, 20,		505,	1932, 2177, 873,	1237, 441,	316,	1321 }
// 2^16 * omega^i
#define ZETAS { 2482, 297,	1436, 2739, 970,	2643, 808,	2181, 728,	350,	2645, 2488, 762,	2543, 1479, 519,	3079, 520,	2503, 3316, 1037, 1971, 1056, 1532, 490,	1644, 820,	2955, 897,	2928, 2724, 2590, 2010, 3143, 782,	504,	2049, 665,	32,		2064, 721,	1563, 731,	2208, 1843, 694,	2201, 482,	1737, 515,	2703, 2900, 931,	1792, 1558, 621,	2854, 988,	428,	974,	2969, 83,		9,		2245 }
#endif
