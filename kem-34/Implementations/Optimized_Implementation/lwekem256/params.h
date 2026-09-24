#ifndef PARAMS_H
#define PARAMS_H

#ifndef KEM_L
#define KEM_L 9 /* Change this for different security strengths */
#endif

/* Don't change parameters below this line */

#define KEM_NAMESPACE(s) kem_light_ref_##s

#define KEM_N 128
#define KEM_Q 3329

/**** Precomputed Values for Parametrisation ****/
// Bit widths
#define LOG2Q (12)
#define LOG2P (11)
#define LOG2T (9)
// Polynomial Compression
#define KEM_Q_2 (1664)				// round(KEM_Q/2)
//#define KEM_Q_DENOM (2520)		// floor(2^(32-LOG2T)/KEM_Q)
#define KEM_Q_DENOM (161271)		// floor(2^(29)/KEM_Q)
//#define KEM_32_LOG2T (4194304)// 2^(32 - LOG2T - 1)
#define LOG2T_BITMASK (0x1ff) // Take the bottom LOG2T bits
#define LOG2P_BITMASK (0x7ff) // Take the bottom LOG2P bits
#define KEM_Q_232 (1290168)		// floor((2^32/KEM_Q) + 1)
#define KEM_Q_230 (322542)		// floor((2^30/KEM_Q) + 1)
#define KEM_Q_INV (3327)			// -inverse_mod(KEM_Q, 2^16)
//#define RLOG (20)							// rlog for modular reduction
#define RLOG (18)							// rlog for modular reduction
//#define RMASK (0x3ffff)				// rlog for modular reduction
#define INV_2_Q (2807)				// 2^(-1) * 2^16 mod KEM_Q
// #define INV_2_Q (1749) // (2^16)^-1 / 2 mod KEM_Q
#define MONT (2285)		// 2^16 mod KEM_Q
#define MONT_2 (1674) // 2^18 * 2^18 mod KEM_Q
//#define MONT_2 (152) // 2^18 * 2^18 mod KEM_Q

// NTT
#define ROOT_OF_UNITY (17)

#define KEM_ETA 1

#define KEM_SYMBYTES 32 // 32   /* size in bytes of hashes, and seeds */
#define KEM_SSBYTES 32	// 32   /* size in bytes of shared key */

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
// (pow(omega_inv,bitrev(i)) * mont) & KEM_Q [upto the first 32 elements]
//#define OMEGAS_INV_BITREV_MONTGOMERY                                          \
//	{ 990,	7427, 5047, 862,	4400, 578,	1095, 5538, 2299, 3336, 7197,         \
//		1319, 6804, 2025, 3823, 1595, 343,	7143, 6396, 7390, 671,	3583,         \
//		263,	1267, 1906, 4042, 3,		5213, 2497, 1749, 5134, 6100 }

// (pow(omega_inv,bit(i)) * n_inv * mont) & KEM_Q
//#define PSIS_INV_MONTGOMERY                                                   \
//	{ 4096, 4203, 4926, 5576, 636,	7456, 1710, 2366, 1989, 3318, 3971,         \
//		5153, 5387, 6681, 7600, 3688, 1159, 1945, 580,	3273, 4313, 4090,         \
//		7321, 2736, 6858, 110,	6845, 1745, 2100, 7083, 6081, 4479, 7437,         \
//		6463, 3112, 928,	6773, 756,	6544, 7105, 7450, 4828, 176,	3271,         \
//		2792, 3360, 5188, 5121, 4094, 2682, 4196, 3443, 3021, 4692, 4282,         \
//		7398, 3687, 4239, 1580, 3354, 625,	2931, 5376, 2156 }

// (pow(omega,bitrev(i)) * mont) & KEM_Q
// For in standard form
// 2^16 * omega^i
//#define ZETAS                                                                 \
//	{ 2571, 2970, 1812, 1493, 287,	1422, 202,	3158, 962,	1577, 1855, 622,    \
//		2127, 182,	1468, 573,	2648, 2500, 1787, 264,	732,	1727, 3124, 2004,   \
//		1017, 1458, 411,	383,	608,	3199, 1758, 1223, 2476, 516,	2931, 2036,   \
//		107,	1711, 448,	2777, 3058, 3009, 1821, 3047, 3082, 126,	677,	652,    \
//		3239, 3321, 961,	1491, 1908, 2167, 2264, 1015, 830,	2663, 2604, 1785,   \
//		2378, 1469, 2054, 2226, 817,	3083, 2144, 422,	2114, 1739, 3221, 2078,   \
//		1322, 2552, 1819, 3038, 2455, 418,	958,	555,	603,	1159, 2051, 177,    \
//		1218, 2457, 996,	1550, 1864, 2727, 2459, 1574, 2142, 3173, 1522, 430,    \
//		1097, 778,	1799, 587,	3193, 644,	3021, 871,	2044, 1483, 2475, 2869,   \
//		220,	329,	1869, 843,	610,	3182, 794,	3094, 1994, 349,	991,	105,    \
//		384,	1119, 478,	1653, 1670, 3254, 1628 }

// Inverse zetas for invntt
// For in standard form
// 2^16 / 2 * omega^-i
#define INV_ZETAS                                                             \
	{ 2515, 1702, 2494, 838,	3090, 1105, 3137, 1612, 1169, 1490, 2332, 1782,   \
		2932, 1738, 3024, 1243, 730,	1500, 3219, 230,	427,	923,	2307, 1229,   \
		154,	3007, 68,		1371, 765,	2940, 1116, 3114, 2568, 78,		2258, 2542,   \
		435,	301,	2397, 2554, 2831, 436,	2720, 1576, 639,	1085, 1363, 1387,   \
		2850, 3120, 437,	1810, 755,	2053, 2668, 2290, 54,		795,	2272, 3118,   \
		2257, 123,	1256, 2216, 2302, 930,	2140, 772,	2027, 333,	2914, 1157,   \
		2197, 581,	2375, 919,	1184, 4,		45,		3003, 1326, 3266, 1788, 141,    \
		754,	160,	1800, 276,	3105, 809,	1611, 2311, 199,	3071, 2091, 1053,   \
		2450, 65,		3025, 1473, 1459, 2600, 1156, 2327, 1767, 801,	2963, 3197,   \
		771,	2079, 2005, 1378, 2595, 3238, 601,	3018, 737,	876,	2848, 1750,   \
		3228, 2618, 1521, 918,	2423, 1844, 379 }

#define OMEGAS_INV_BITREV_MONTGOMERY { 2482, 3032, 2739, 1436, 2521, 2181, 970,	686,	786,	2567, 1479, 2810, 2601, 350,	841,	684,	2955, 820,	1685, 2839, 897,	401, 2590, 2724, 520,	3079, 826,	3316, 1797, 2273, 1971, 1037, 1771, 621,	931,	1537, 2900, 2703, 2814, 1592, 475,	988,	2355, 2901, 9,		1084, 360,	83,		782,	2825, 1319, 3143, 2664, 1280, 32, 1265, 2847, 1128, 694,	1843, 2598, 2208, 721,	1766 }

#define PSIS_INV_MONTGOMERY { 2048, 2862, 560,	2187, 3066, 572,	2971, 2133, 2867, 3106, 966,	3190, 3125, 3317, 2545, 933,	1034, 3194, 1167, 3006, 3310, 978,	645,	2192, 2283, 2680, 3095, 1357, 3213, 189,	2361, 1118, 2024, 1294, 2426, 926, 2796, 2906, 2325, 2095, 1494, 1067, 2021, 1098, 1827, 2849, 1930, 701, 1412, 1258, 74,		396,	2569, 2501, 2497, 1126, 1437, 672,	627,	1016, 2018, 902,	1228, 1443, 1064, 1825, 499,	421,	1983, 3054, 3117, 575, 3167, 2732, 944,	643,	3171, 774,	633,	1408, 3216, 385,	2960, 2524, 2890, 170,	10,		1763, 887,	248,	1777, 692,	824,	2790, 2514, 1127, 2612, 3091, 3315, 195,	2753, 2316, 1507, 3026, 178,	2752, 1141, 2417, 338,	999,	2017, 3056, 2138, 2084, 1885, 1090, 2414, 142,	400,	611, 2190, 3262, 1171, 1048, 2999, 1743, 690,	1803 }

#define ZETAS { 2482, 297,	1893, 590,	2643, 2359, 1148, 808,	2645, 2488, 2979, 728, 519,	1850, 762,	2543, 2292, 1358, 1056, 1532, 13,		2503, 250,	2809, 605,	739,	2928, 2432, 490,	1644, 2509, 374,	1563, 2608, 1121, 731, 1486, 2635, 2201, 482,	2064, 3297, 2049, 665,	186,	2010, 504,	2547, 3246, 2969, 2245, 3320, 428,	974,	2341, 2854, 1737, 515,	626,	429, 1792, 2398, 2708, 1558, 2246, 1720, 2220, 43,		1654, 155,	2871, 420, 1688, 2348, 708,	2389, 2165, 1489, 2967, 3283, 2345, 3112, 1307, 2741, 221,	2603, 921,	1147, 298,	2576, 3170, 1396, 1672, 1316, 2705, 3029, 3268, 1059, 2412, 2440, 1959, 1518, 798,	1536, 1798, 2785, 1543, 1318, 3162, 880,	1910, 22,		1918, 538,	1546, 3176, 618,	3242, 3178, 1912, 2897, 2097, 655,	635,	503,	818,	2759, 3183 }
#endif
