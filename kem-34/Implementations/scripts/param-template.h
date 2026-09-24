#ifndef PARAMS_H
#define PARAMS_H

#ifndef KEM_L
#define KEM_L $KEM_L /* Change this for different security strengths */
#endif

/* Don't change parameters below this line */

#define KEM_NAMESPACE(s) kem_light_ref_##s

#define KEM_N $KEM_N
#define KEM_Q $KEM_Q

/**** Precomputed Values for Parametrisation ****/
// Bit widths
#define LOG2Q ($LOG2Q)
#define LOG2P ($LOG2P)
#define LOG2T ($LOG2T)
// Polynomial Compression
#define KEM_Q_2 ($KEM_Q_2)						 // floor(KEM_Q/2)
#define KEM_Q_DENOM ($KEM_Q_DENOM)		 // floor(2^(32-LOG2T)/KEM_Q)
#define LOG2T_BITMASK ($LOG2T_BITMASK) // Take the bottom LOG2T bits
#define LOG2P_BITMASK ($LOG2P_BITMASK) // Take the bottom LOG2P bits
#define KEM_Q_232 ($KEM_Q_232)				 // floor((2^32/KEM_Q) + 1)
#define KEM_Q_230 ($KEM_Q_230)				 // floor((2^30/KEM_Q) + 1)
#define KEM_Q_INV ($KEM_Q_INV)				 // -inverse_mod(KEM_Q, 2^16)
#define RLOG ($RLOG)									 // rlog for modular reduction
#define RMASK ($RMASK)								 // rlog for modular reduction
#define INV_2_Q ($INV_2_Q)						 // 2^(-1) * 2^16 mod KEM_Q
// #define INV_2_Q (1749) // (2^16)^-1 / 2 mod KEM_Q
#define MONT ($MONT)		 // 2^16 mod KEM_Q
#define MONT_2 ($MONT_2) // 2^16 * 2^16 mod KEM_Q

// NTT
#define ROOT_OF_UNITY ($ROOT_OF_UNITY)

#define KEM_ETA $KEM_ETA

#define KEM_SYMBYTES                                                          \
	$KEM_SYMBYTES // 32   /* size in bytes of hashes, and seeds */
#define KEM_SSBYTES $KEM_SSBYTES // 32   /* size in bytes of shared key */

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
#define OMEGAS_INV_BITREV_MONTGOMERY                                          \
	{ 990,	7427, 5047, 862,	4400, 578,	1095, 5538, 2299, 3336, 7197,         \
		1319, 6804, 2025, 3823, 1595, 343,	7143, 6396, 7390, 671,	3583,         \
		263,	1267, 1906, 4042, 3,		5213, 2497, 1749, 5134, 6100 }

// (pow(omega_inv,bit(i)) * n_inv * mont) & KEM_Q
#define PSIS_INV_MONTGOMERY                                                   \
	{ 4096, 4203, 4926, 5576, 636,	7456, 1710, 2366, 1989, 3318, 3971,         \
		5153, 5387, 6681, 7600, 3688, 1159, 1945, 580,	3273, 4313, 4090,         \
		7321, 2736, 6858, 110,	6845, 1745, 2100, 7083, 6081, 4479, 7437,         \
		6463, 3112, 928,	6773, 756,	6544, 7105, 7450, 4828, 176,	3271,         \
		2792, 3360, 5188, 5121, 4094, 2682, 4196, 3443, 3021, 4692, 4282,         \
		7398, 3687, 4239, 1580, 3354, 625,	2931, 5376, 2156 }

// (pow(omega,bitrev(i)) * mont) & KEM_Q
// For in standard form
// 2^16 * omega^i
#define ZETAS $ZETAS

// Inverse zetas for invntt
// For in standard form
// 2^16 / 2 * omega^-i
#define INV_ZETAS $INV_ZETAS

#endif
