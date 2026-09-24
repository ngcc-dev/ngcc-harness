
/*
 * This file is not meant to be compiled independently, but to be
 * included (with #include) by another C file. The caller is supposed to
 * have already included <stdint.h> and <string.h>; moreover, the Q
 * macro should be defined to one of the supported modulus values.
 */

/*
 * This module implements operations modulo a prime q. It works for
 * a few specific primes (257, 769 and 3329); it could be extended to
 * other primes such that q < 65536 and q = 1 mod 256, provided that
 * the relevant constants are defined.
 *
 * Internally, all computations use a representation in 1..q range (i.e.
 * value 0 is represented by q, not by 0). Montgomery multiplication
 * uses R = 2^32, i.e. representation of x is x*2^32 mod q, in the 1..q
 * range. With R > q^2, Montgomery reduction can be done efficiently
 * since it does not require an extra conditional subtraction.
 *
 * Macro Q must be defined beforehand, to the modulus q.
 *   Q1I = -1/q mod 2^32
 *   R2  = 2^64 mod q
 *   T54 = 2^54 mod q
 *   T25 = 2^25 mod q
 *   T25iGM1 = iGM[1]/128 mod q
 * (iGM[1] is the second element of the table used for inverse NTT, see
 * below; iGM[] is in Montgomery representation, hence T25iGM1 is the
 * Montgomery representation of the actual value.)
 */

#ifdef Q

#if Q == 257
#define Q1I   16711935
#define Q1Ilo      255
#define Q1Ihi      255
#define R            1
#define R2           1
#define T54         64
#define T25        255
#define T25iGM1    225
#elif Q == 769
#define Q1I   452395775
#define Q1Ilo       767
#define Q1Ihi      6903
#define R            19
#define R2          361
#define T54         306
#define T25         655
#define T25iGM1     622
#elif Q == 3329
#define Q1I   2488732927
#define Q1Ilo       3327
#define Q1Ihi     -27561
#define R           1353
#define R2          2988
#define T54          276
#define T25         1441
#define T25iGM1     1932
#elif Q == 64513
#define Q1I   3354459135
#define Q1Ilo      -1025
#define Q1Ihi     -14352
#define R          14321
#define R2          4214
#define T54        57083
#define T25         7672
#define T25iGM1    22387
#else
#error Unsupported modulus Q.
#endif

/*
 * Square of q, as a uint32_t.
 */
#define Q2   ((uint32_t)((uint32_t)Q * (uint32_t)Q))

/*
 * q as a 16-bit signed integer.
 */
#if Q < 32767
#define Qs   Q
#else
#define Qs   (Q - 65536)
#endif

#else
#error This module must not be compiled separately.
#endif

#if __GNUC__ || __clang__
#define UNUSED   __attribute__ ((unused))
#else
#define UNUSED
#endif

UNUSED
static inline uint32_t
mq_add(uint32_t x, uint32_t y)
{
	/* Compute -(x+y) in the -q..q-2 range. */
	x = Q - (x + y);

	/* Add q if the value is strictly negative. Note that since
	   x <= q and y <= q, a negative value will have its
	   top 16 bits all equal to 1. */
	x += Q & (x >> 16);

	/* Since we have -(x+y) in the 0..q-1 range, we can get
	   x+y = -(-(x+y)) in the 1..q range. */
	return Q - x;
}

UNUSED
static inline uint32_t
mq_mul2(uint32_t x)
{
	/* Compute -2*x in the -q..q-2 range. */
	x = Q - (x << 1);

	/* Add q if the value is strictly negative. Note that since
	   x <= q, a negative value will have its top 16 bits all equal to 1. */
	x += Q & (x >> 16);

	/* Since we have -2*x in the 0..q-1 range, we can get
	   2*x = -(-2*x) in the 1..q range. */
	return Q - x;
}


UNUSED
static inline uint32_t
mq_sub(uint32_t x, uint32_t y)
{
	/* Get y-x in the -q+1..q-1 range. */
	y -= x;

	/* Add q if the value is strictly negative. New range is 0..q-1 */
	y += Q & (y >> 16);

	/* Return -(y-x) = x-y. */
	return Q - y;
}


UNUSED
static inline uint32_t
mq_neg(uint32_t x)
{
	x = Q - x;
	x += Q & ((x - 1) >> 16);
	return x;
}


/*
 * Given input x, compute x/2^32 modulo q. Returned value is in the 1..q
 * range.
 *
 * IF q <= 40504:
 * ==============
 *
 * This function works for all 1 <= x <= 2^32 + 2^16 - 1 - (2^16-1)*q.
 * The upper limit is, for the supported values of q:
 *     q      limit
 *    257   4278190336
 *    769   4244636416
 *   3329   4076866816
 * If q <= 19433, then the limit is greater than 8*q^2, so that we may
 * then add together up to 8 simple products (over the integers) in order
 * to mutualize the Montgomery reduction.
 *
 * IF q > 40504:
 * =============
 *
 * When q is larger than 40504, the validity range of the method above is
 * lower than q^2, which means that it is not sufficient to implement
 * multiplications. In that case, the function below uses a different
 * technique which involves a 32x32->64 multiplication, but works for all
 * values in the 1..2^32-1 range.
 */
UNUSED
static inline uint32_t
mq_montyred(uint32_t x)
{
#if Q <= 40504
	x *= Q1I;
	x = (x >> 16) * Q;
	return (x >> 16) + 1;
#else
	x *= Q1I;
	return (uint32_t)(((uint64_t)x * Q) >> 32) + 1;
#endif
}


/*
 * Given x and y, compute (x*y)/2^32 mod q; returned value is in 1..q.
 * Function works as long as 1 <= x*y <= 2^32 + 2^16 - 1 - (2^16-1)*q.
 */
UNUSED
static inline uint32_t
mq_montymul(uint32_t x, uint32_t y)
{
	return mq_montyred(x * y);
}



/*
 * Given x, compute x*2^32 mod q. This works for all x (including zero)
 * up to some limit, which depends on the modulus:
 *     q      limit
 *    257   4278190079
 *    769     11757226
 *   3329      1361084
 *  64513       954700
 */
UNUSED
static inline uint32_t
mq_tomonty(uint32_t x)
{
	return mq_montyred((x + Q) * R2);
}

/*
 * Given a signed integer x (in the -503109..+503109 range), obtain the
 * Montgomery representation of x (i.e. x*2^32 mod q, in the 1..q range).
 */
UNUSED
static inline uint32_t
mq_set(int32_t x)
{
	return mq_montyred((uint32_t)((int32_t)x
		+ (int32_t)Q * (1 + ((int32_t)503109 / Q))) * R2);
}

/*
 * Convert back an integer from Montgomery representation (in 1..q) into
 * an unsigned integer in normal representation, in the 0..q-1 range.
 */
UNUSED
static inline uint32_t
mq_unorm(uint32_t x)
{
	x = mq_montyred(x);
	x &= (uint32_t)(x - Q) >> 16;
	return x;
}

/*
 * Convert back an integer from Montgomery representation (in 1..q) into
 * a signed integer in normal representation, in the -((q-1)/2)..+((q-1)/2)
 * range.
 */
UNUSED
static inline int
mq_snorm(uint32_t x)
{
	x = mq_montyred(x);
	return (int)x - (int)(Q & ((uint32_t)((Q / 2) - x) >> 16));
}

#if defined CTL_AVX2 && CTL_AVX2 && Q <= 40504
#include "ctl_avx2_modq.h"
#endif

/*
 * Invert x modulo q. Input and output are both in Montgomery representation.
 * If x = 0, then 0 is returned.
 */
UNUSED
static inline uint32_t
mq_inv(uint32_t x)
{
	/*
	 * We use Fermat's little theorem: 1/x = x^(q-2) mod q.
	 * An efficient addition chain on the exponent is used; the
	 * chain depends on the modulus.
	 */

#if Q == 257

	uint32_t y;

	/* x -> x^3 */
	y = mq_montymul(x, x);
	x = mq_montymul(y, x);

	/* x^3 -> x^15 */
	y = mq_montymul(x, x);
	y = mq_montymul(y, y);
	x = mq_montymul(y, x);

	/* x^15 -> x^255 = 1/x */
	y = mq_montymul(x, x);
	y = mq_montymul(y, y);
	y = mq_montymul(y, y);
	y = mq_montymul(y, y);
	x = mq_montymul(y, x);

	return x;

#elif Q == 769

	uint32_t x2, x3, x5, x10, x13;

	x2 = mq_montymul(x, x);
	x3 = mq_montymul(x2, x);
	x5 = mq_montymul(x3, x2);
	x10 = mq_montymul(x5, x5);
	x13 = mq_montymul(x10, x3);
	x = mq_montymul(x13, x10);   /* x^23 */
	x = mq_montymul(x, x);       /* x^46 */
	x = mq_montymul(x, x);       /* x^92 */
	x = mq_montymul(x, x);       /* x^184 */
	x = mq_montymul(x, x);       /* x^368 */
	x = mq_montymul(x, x13);     /* x^381 */
	x = mq_montymul(x, x);       /* x^762 */
	x = mq_montymul(x, x5);      /* x^767 = 1/x */

	return x;

#elif Q == 3329

	uint32_t y, x8, x9;

	y = mq_montymul(x, x);
	y = mq_montymul(y, y);
	x8 = mq_montymul(y, y);
	x9 = mq_montymul(x8, x);
	x = mq_montymul(x8, x9);     /* x^17 */
	x = mq_montymul(x, x);       /* x^34 */
	x = mq_montymul(x, x);       /* x^68 */
	x = mq_montymul(x, x);       /* x^136 */
	x = mq_montymul(x, x);       /* x^272 */
	x = mq_montymul(x, x);       /* x^544 */
	x = mq_montymul(x, x9);      /* x^553 */
	y = mq_montymul(x, x);
	x = mq_montymul(x, y);       /* x^1659 */
	x = mq_montymul(x, x);       /* x^3318 */
	x = mq_montymul(x, x9);      /* x^3327 = 1/x */

	return x;

#elif Q == 64513

	uint32_t y, x3, x31;

	y = mq_montymul(x, x);      /* x^2 */
	x3 = mq_montymul(y, x);     /* x^3 */
	y = mq_montymul(x3, x3);    /* x^6 */
	y = mq_montymul(y, y);      /* x^12 */
	y = mq_montymul(y, x3);     /* x^15 */
	y = mq_montymul(y, y);      /* x^30 */
	x31 = mq_montymul(y, x);    /* x^31 */
	y = mq_montymul(x31, x31);  /* x^62 */
	y = mq_montymul(y, y);      /* x^124 */
	y = mq_montymul(y, y);      /* x^248 */
	y = mq_montymul(y, y);      /* x^496 */
	y = mq_montymul(y, y);      /* x^992 */
	y = mq_montymul(y, y);      /* x^1984 */
	y = mq_montymul(y, x31);    /* x^2015 */
	y = mq_montymul(y, y);      /* x^4030 */
	y = mq_montymul(y, y);      /* x^8060 */
	y = mq_montymul(y, y);      /* x^16120 */
	y = mq_montymul(y, y);      /* x^32240 */
	y = mq_montymul(y, y);      /* x^64480 */
	y = mq_montymul(y, x31);    /* x^64511 */

	return y;

#endif
}


/*
 * NTT Tables
 */

#if Q == 257

/* q = 257, w = 3, 1/w = 86 */

static const uint16_t GM[] = {
	   1,  241,   64,    4,  249,  128,    2,  225,  136,  137,
	 223,   30,  197,  189,   15,   17,   81,  246,   44,   67,
	 123,   88,  162,  235,  222,   46,   73,  117,   23,  146,
	 187,   92,    9,  113,   62,   36,  185,  124,   18,  226,
	 196,  205,  208,   13,  231,  159,  135,  153,  215,  158,
	 139,   89,   79,   21,  173,   59,  199,  157,  143,   25,
	 207,   29,  141,   57,    3,  209,  192,   12,  233,  127,
	   6,  161,  151,  154,  155,   90,   77,   53,   45,   51,
	 243,  224,  132,  201,  112,    7,  229,  191,  152,  138,
	 219,   94,   69,  181,   47,   19,   27,   82,  186,  108,
	  41,  115,   54,  164,   74,  101,  110,   39,  179,  220,
	 148,  202,  131,  217,  160,   10,  237,   63,    5,  177,
	  83,  214,  172,   75,  107,   87,  166,  171
};

static const uint16_t iGM[] = {
	   1,   16,  253,  193,   32,  255,  129,    8,  240,  242,
	  68,   60,  227,   34,  120,  121,  165,   70,  111,  234,
	 140,  184,  211,   35,   22,   95,  169,  134,  190,  213,
	  11,  176,  200,  116,  228,   50,  232,  114,  100,   58,
	 198,   84,  236,  178,  168,  118,   99,   42,  104,  122,
	  98,   26,  244,   49,   52,   61,   31,  239,  133,   72,
	 221,  195,  144,  248,   86,   91,  170,  150,  182,   85,
	  43,  174,   80,  252,  194,   20,  247,   97,   40,  126,
	  55,  109,   37,   78,  218,  147,  156,  183,   93,  203,
	 142,  216,  149,   71,  175,  230,  238,  210,   76,  188,
	 163,   38,  119,  105,   66,   28,  250,  145,   56,  125,
	  33,   14,  206,  212,  204,  180,  167,  102,  103,  106,
	  96,  251,  130,   24,  245,   65,   48,  254
};

static const uint16_t NX[] = {
	   3,  254,  209,   48,  192,   65,   12,  245,  233,   24,
	 127,  130,    6,  251,  161,   96,  151,  106,  154,  103,
	 155,  102,   90,  167,   77,  180,   53,  204,   45,  212,
	  51,  206,  243,   14,  224,   33,  132,  125,  201,   56,
	 112,  145,    7,  250,  229,   28,  191,   66,  152,  105,
	 138,  119,  219,   38,   94,  163,   69,  188,  181,   76,
	  47,  210,   19,  238,   27,  230,   82,  175,  186,   71,
	 108,  149,   41,  216,  115,  142,   54,  203,  164,   93,
	  74,  183,  101,  156,  110,  147,   39,  218,  179,   78,
	 220,   37,  148,  109,  202,   55,  131,  126,  217,   40,
	 160,   97,   10,  247,  237,   20,   63,  194,    5,  252,
	 177,   80,   83,  174,  214,   43,  172,   85,   75,  182,
	 107,  150,   87,  170,  166,   91,  171,   86
};


#elif Q == 769

/* q = 769, w = 343, 1/w = 630 */

static const uint16_t GM[] = {
	  19,  360,  211,  760,  455,  243,  277,  513,  155,  387,
	 669,   48,  393,  242,  317,  340,  447,  739,  431,  193,
	 667,  172,   41,  534,  692,  160,  521,  765,  544,  108,
	 294,  228,  617,  196,  619,   72,  205,  363,   91,  510,
	 298,  749,   31,  385,  701,  371,  540,  356,  269,  240,
	 397,  763,   47,  162,  441,  342,  616,  258,  446,   32,
	 262,  674,  724,  483,  365,  440,   87,  758,  727,  297,
	 424,  627,  104,  473,  305,  315,  224,  723,  302,  501,
	 290,  476,  185,   65,  388,  552,  221,  140,  504,  281,
	 295,  166,  494,  132,  103,  535,  156,  325,   73,   88,
	 336,  700,  453,  367,  706,   61,  636,  556,  515,  368,
	 660,  606,  756,   37,   58,  249,  741,  198,  539,  418,
	 582,   59,  716,  210,  662,  482,  714,  334
};

static const uint16_t iGM[] = {
	  19,  409,    9,  558,  256,  492,  526,  314,  429,  452,
	 527,  376,  721,  100,  382,  614,  541,  475,  661,  225,
	   4,  248,  609,   77,  235,  728,  597,  102,  576,  338,
	  30,  322,  286,   45,   95,  507,  737,  323,  511,  153,
	 427,  328,  607,  722,    6,  372,  529,  500,  413,  229,
	 398,   68,  384,  738,   20,  471,  259,  678,  406,  564,
	 697,  150,  573,  152,  435,   55,  287,  107,  559,   53,
	 710,  187,  351,  230,  571,   28,  520,  711,  732,   13,
	 163,  109,  401,  254,  213,  133,  708,   63,  402,  316,
	  69,  433,  681,  696,  444,  613,  234,  666,  637,  275,
	 603,  474,  488,  265,  629,  548,  217,  381,  704,  584,
	 293,  479,  268,  467,   46,  545,  454,  464,  296,  665,
	 142,  345,  472,   42,   11,  682,  329,  404
};

static const uint16_t NX[] = {
	 365,  404,  440,  329,   87,  682,  758,   11,  727,   42,
	 297,  472,  424,  345,  627,  142,  104,  665,  473,  296,
	 305,  464,  315,  454,  224,  545,  723,   46,  302,  467,
	 501,  268,  290,  479,  476,  293,  185,  584,   65,  704,
	 388,  381,  552,  217,  221,  548,  140,  629,  504,  265,
	 281,  488,  295,  474,  166,  603,  494,  275,  132,  637,
	 103,  666,  535,  234,  156,  613,  325,  444,   73,  696,
	  88,  681,  336,  433,  700,   69,  453,  316,  367,  402,
	 706,   63,   61,  708,  636,  133,  556,  213,  515,  254,
	 368,  401,  660,  109,  606,  163,  756,   13,   37,  732,
	  58,  711,  249,  520,  741,   28,  198,  571,  539,  230,
	 418,  351,  582,  187,   59,  710,  716,   53,  210,  559,
	 662,  107,  482,  287,  714,   55,  334,  435
};


#elif Q == 3329

/* q = 3329, w = 3061, 1/w = 2298 */

static const uint16_t GM[] = {
	1353, 2379, 1381,  856, 3163, 2609, 2168,   18,  255, 1467,
	1242,  213, 2471, 1252, 3184, 2299,  769, 1330,   64,  799,
	1564, 1008, 2957, 2638, 2972, 1941, 2256, 2365, 1867, 2242,
	 203, 1442, 1033, 1713, 1389, 1372, 1694, 2735,  457, 1180,
	2291, 2958, 1524, 1757, 1456,  700, 1961, 1647, 1217,  265,
	2716, 2074, 2289, 2829,   26, 1677, 2119, 1851, 2527, 1535,
	3288, 2349, 2581, 1689,  257, 1596, 2740,  293, 1211, 3207,
	1551, 1834, 1569, 2995,   44, 2838,  243,  693, 2241, 3062,
	 306, 3092, 2822, 2253,  302, 2834, 3155, 2093, 2464, 2465,
	1270, 2019, 2323, 1693, 2189, 3037, 2792,  318,  596, 1823,
	2081, 2729,  697,   15, 1877, 2887, 1035, 1842, 2614, 2153,
	 434, 1361,   86, 2218, 1163,  111, 2413,  840, 3019, 3308,
	1367, 3282, 1880, 1416, 1001, 2978,  724,   92
};

static const uint16_t iGM[] = {
	1353,  950, 2473, 1948, 3311, 1161,  720,  166, 1030,  145,
	2077,  858, 3116, 2087, 1862, 3074, 1887, 3126, 1087, 1462,
	 964, 1073, 1388,  357,  691,  372, 2321, 1765, 2530, 3265,
	1999, 2560, 1640,  748,  980,   41, 1794,  802, 1478, 1210,
	1652, 3303,  500, 1040, 1255,  613, 3064, 2112, 1682, 1368,
	2629, 1873, 1572, 1805,  371, 1038, 2149, 2872,  594, 1635,
	1957, 1940, 1616, 2296, 3237, 2605,  351, 2328, 1913, 1449,
	  47, 1962,   21,  310, 2489,  916, 3218, 2166, 1111, 3243,
	1968, 2895, 1176,  715, 1487, 2294,  442, 1452, 3314, 2632,
	 600, 1248, 1506, 2733, 3011,  537,  292, 1140, 1636, 1006,
	1310, 2059,  864,  865, 1236,  174,  495, 3027, 1076,  507,
	 237, 3023,  267, 1088, 2636, 3086,  491, 3285,  334, 1760,
	1495, 1778,  122, 2118, 3036,  589, 1733, 3072
};

static const uint16_t NX[] = {
	 257, 3072, 1596, 1733, 2740,  589,  293, 3036, 1211, 2118,
	3207,  122, 1551, 1778, 1834, 1495, 1569, 1760, 2995,  334,
	  44, 3285, 2838,  491,  243, 3086,  693, 2636, 2241, 1088,
	3062,  267,  306, 3023, 3092,  237, 2822,  507, 2253, 1076,
	 302, 3027, 2834,  495, 3155,  174, 2093, 1236, 2464,  865,
	2465,  864, 1270, 2059, 2019, 1310, 2323, 1006, 1693, 1636,
	2189, 1140, 3037,  292, 2792,  537,  318, 3011,  596, 2733,
	1823, 1506, 2081, 1248, 2729,  600,  697, 2632,   15, 3314,
	1877, 1452, 2887,  442, 1035, 2294, 1842, 1487, 2614,  715,
	2153, 1176,  434, 2895, 1361, 1968,   86, 3243, 2218, 1111,
	1163, 2166,  111, 3218, 2413,  916,  840, 2489, 3019,  310,
	3308,   21, 1367, 1962, 3282,   47, 1880, 1449, 1416, 1913,
	1001, 2328, 2978,  351,  724, 2605,   92, 3237
};


#elif Q == 64513

/* q = 64513, w = 45056, 1/w = 10262 */

static const uint16_t GM[] = {
	14321, 37549, 22229, 48008, 45449, 33295, 30746, 44270,
	50769, 32369, 21408, 46914, 55118, 33528,  8851, 41654,
	44478, 35380, 27527, 36366, 20143, 11361, 25252, 30820,
	41567, 48474, 58272, 44960, 29104, 42082, 22935, 10681,
	16608, 19616, 30608, 23970, 62122, 49383, 19646, 21464,
	 7941, 26533, 36823, 19129, 10615,  9430, 56916, 53054,
	54464, 55130, 22562, 57724, 12296, 48209, 16446, 46274,
	56620,  8577, 29643, 46572, 32076, 11782, 63217, 19725,
	52463, 18832, 50012, 56584, 43011, 18731,  4127, 16186,
	10623, 36786, 24985, 53252, 33186,  1160, 35803, 14941,
	33449, 29563, 58600,  5322, 58637, 35074,  2844, 48108,
	30362, 21442, 17671,  9560, 18586,  9522, 54639, 40669,
	 3761, 54909, 44160, 44700,  7814, 11591, 51816, 32114,
	  598, 44958, 16267, 47057, 34571, 59975, 15546,   835,
	49003, 57754, 22131, 35462, 35445, 16507, 59171, 54723,
	33161, 12442, 46882, 62707, 60543, 36828, 56202, 63025
};

static const uint16_t iGM[] = {
	14321, 26964, 16505, 42284, 20243, 33767, 31218, 19064,
	22859, 55662, 30985,  9395, 17599, 43105, 32144, 13744,
	53832, 41578, 22431, 35409, 19553,  6241, 16039, 22946,
	33693, 39261, 53152, 44370, 28147, 36986, 29133, 20035,
	44788,  1296, 52731, 32437, 17941, 34870, 55936,  7893,
	18239, 48067, 16304, 52217,  6789, 41951,  9383, 10049,
	11459,  7597, 55083, 53898, 45384, 27690, 37980, 56572,
	43049, 44867, 15130,  2391, 40543, 33905, 44897, 47905,
	 1488,  8311, 27685,  3970,  1806, 17631, 52071, 31352,
	 9790,  5342, 48006, 29068, 29051, 42382,  6759, 15510,
	63678, 48967,  4538, 29942, 17456, 48246, 19555, 63915,
	32399, 12697, 52922, 56699, 19813, 20353,  9604, 60752,
	23844,  9874, 54991, 45927, 54953, 46842, 43071, 34151,
	16405, 61669, 29439,  5876, 59191,  5913, 34950, 31064,
	49572, 28710, 63353, 31327, 11261, 39528, 27727, 53890,
	48327, 60386, 45782, 21502,  7929, 14501, 45681, 12050
};

static const uint16_t NX[] = {
	52463, 12050, 18832, 45681, 50012, 14501, 56584,  7929,
	43011, 21502, 18731, 45782,  4127, 60386, 16186, 48327,
	10623, 53890, 36786, 27727, 24985, 39528, 53252, 11261,
	33186, 31327,  1160, 63353, 35803, 28710, 14941, 49572,
	33449, 31064, 29563, 34950, 58600,  5913,  5322, 59191,
	58637,  5876, 35074, 29439,  2844, 61669, 48108, 16405,
	30362, 34151, 21442, 43071, 17671, 46842,  9560, 54953,
	18586, 45927,  9522, 54991, 54639,  9874, 40669, 23844,
	 3761, 60752, 54909,  9604, 44160, 20353, 44700, 19813,
	 7814, 56699, 11591, 52922, 51816, 12697, 32114, 32399,
	  598, 63915, 44958, 19555, 16267, 48246, 47057, 17456,
	34571, 29942, 59975,  4538, 15546, 48967,   835, 63678,
	49003, 15510, 57754,  6759, 22131, 42382, 35462, 29051,
	35445, 29068, 16507, 48006, 59171,  5342, 54723,  9790,
	33161, 31352, 12442, 52071, 46882, 17631, 62707,  1806,
	60543,  3970, 36828, 27685, 56202,  8311, 63025,  1488
};


#endif


/*
 * Convert an array to (partial) NTT representation. This function
 * accepts all degrees from 2 (logn = 1) to 2048 (logn = 11). Source (a)
 * and destination (d) may overlap.
 */
UNUSED
static void
NTT(uint16_t *d, const uint16_t *a, unsigned logn)
{

	unsigned n, t, m, mm;

	n = 1u << logn;
	if (d != a) {
		memmove(d, a, n * sizeof *a);
	}
	mm = (logn <= 7) ? n : 128;
	t = n;
	for (m = 1; m < mm; m <<= 1) {
		unsigned ht, i, j1;

		ht = t >> 1;
		for (i = 0, j1 = 0; i < m; i ++, j1 += t) {
			unsigned j, j2;
			uint32_t s;

			s = GM[m + i];
			j2 = j1 + ht;
#if defined CTL_AVX2 && CTL_AVX2 && Q <= 40504
			if (ht >= 8) {
				mq_avx2_ntt_butterfly(d, j1, j2, ht, s);
				continue;
			}
#endif
			for (j = j1; j < j2; j ++) {
				uint32_t u, v;

				u = d[j];
				v = mq_montymul(d[j + ht], s);
				d[j] = mq_add(u, v);
				d[j + ht] = mq_sub(u, v);
			}
		}
		t = ht;
	}

}

/*
 * Apply the inverse (partial) NTT on an array; this reverts the effect
 * of the NTT() function. This function accepts all degrees from 2
 * (logn = 1) to 2048 (logn = 11). Source (a) and destination (d) may
 * overlap.
 */
UNUSED
static void
iNTT(uint16_t *d, const uint16_t *a, unsigned logn)
{

	unsigned n, t, m;
	uint32_t ni;

	n = 1u << logn;
	if (d != a) {
		memmove(d, a, n * sizeof *a);
	}
	if (logn <= 7) {
		t = 1;
		m = n;
	} else {
		t = 1u << (logn - 7);
		m = 128;
	}
	while (m > 1) {
		unsigned hm, dt, i, j1;

		hm = m >> 1;
		dt = t << 1;
		for (i = 0, j1 = 0; i < hm; i ++, j1 += dt) {
			unsigned j, j2;
			uint32_t s;

			j2 = j1 + t;
			s = iGM[hm + i];
#if defined CTL_AVX2 && CTL_AVX2 && Q <= 40504
			if (t >= 8) {
				mq_avx2_intt_butterfly(d, j1, j2, t, s);
				continue;
			}
#endif
			for (j = j1; j < j2; j ++) {
				uint32_t u, v;

				u = d[j];
				v = d[j + t];
				d[j] = mq_add(u, v);
#if Q <= 46341
				d[j + t] = mq_montyred((Q + u - v) * s);
#else
				d[j + t] = mq_montymul(mq_sub(u, v), s);
#endif
			}
		}
		t = dt;
		m = hm;
	}

	/*
	 * We need to divide by n, which we do by a multiplication by
	 * 1/n. Since we use Montgomery representation with R = 2^32,
	 * we use 2^32/n = 2^(32 - logn). We start with 2^54 mod q,
	 * then do a left shift by 10-logn (thus yielding a value equal
	 * to 2^(64-logn) modulo q), and apply a Montgomery reduction.
	 *
	 * However, for logn > 7, we have a partial NTT, and thus must
	 * stop at n = 128; i.e. with want ni = 2^25 mod q' (Montgomery
	 * representation of 1/128).
	 */
	if (logn <= 7) {
		ni = mq_montyred(T54 << (10 - logn));
	} else {
		ni = T25;
	}
#if defined CTL_AVX2 && CTL_AVX2 && Q <= 40504
	mq_avx2_poly_scale(d, ni, n);
#else
	for (m = 0; m < n; m ++) {
		d[m] = mq_montymul(d[m], ni);
	}
#endif

}

/*
 * Polynomial addition (works both in NTT and normal representations).
 */
UNUSED
static void
mq_poly_add(uint16_t *d, const uint16_t *a, const uint16_t *b, unsigned logn)
{
	size_t u, n;

	n = (size_t)1 << logn;
#if defined CTL_AVX2 && CTL_AVX2 && Q <= 40504
	mq_avx2_poly_add(d, a, b, n);
#else
	for (u = 0; u < n; u ++) {
		d[u] = mq_add(a[u], b[u]);
	}
#endif
}

/*
 * Polynomial addition (works both in NTT and normal representations).
 */
UNUSED
static void
mq_poly_sub(uint16_t *d, const uint16_t *a, const uint16_t *b, unsigned logn)
{
	size_t u, n;

	n = (size_t)1 << logn;
#if defined CTL_AVX2 && CTL_AVX2 && Q <= 40504
	mq_avx2_poly_sub(d, a, b, n);
#else
	for (u = 0; u < n; u ++) {
		d[u] = mq_sub(a[u], b[u]);
	}
#endif
}

/*
 * Multiplication of a polynomial by a constant c (modulo q). The constant
 * is provided as a normal signed integer.
 */
UNUSED
static void
mq_poly_mulconst(uint16_t *d, const uint16_t *a, int c, unsigned logn)
{
	size_t u, n;
	uint32_t cc;

	n = (size_t)1 << logn;
	cc = mq_set(c);
#if defined CTL_AVX2 && CTL_AVX2 && Q <= 40504
	if (d != a) {
		memmove(d, a, n * sizeof *a);
	}
	mq_avx2_poly_scale(d, cc, n);
#else
	for (u = 0; u < n; u ++) {
		d[u] = mq_montymul(a[u], cc);
	}
#endif
}

/*
 * Polynomial multiplication (NTT only).
 */
UNUSED
static void
mq_poly_mul_ntt(uint16_t *d, const uint16_t *a, const uint16_t *b,
	unsigned logn)
{

	size_t u;

	if (logn <= 7) {
		size_t n;

		n = (size_t)1 << logn;
#if defined CTL_AVX2 && CTL_AVX2 && Q <= 40504
		mq_avx2_poly_mul(d, a, b, n);
#else
		for (u = 0; u < n; u ++) {
			d[u] = mq_montymul(a[u], b[u]);
		}
#endif
		return;
	}

	switch (logn) {
	case 8:
		for (u = 0; u < 256; u += 2) {
			uint32_t a0, a1, b0, b1;

			a0 = a[u];
			a1 = a[u + 1];
			b0 = b[u];
			b1 = b[u + 1];
#if Q <= 19433
			d[u] = mq_montyred(
				a0 * b0 + mq_montymul(a1, b1) * NX[u >> 1]);
			d[u + 1] = mq_montyred(
				a1 * b0 + a0 * b1);
#else
			d[u] = mq_add(
				mq_montymul(a0, b0),
				mq_montymul(mq_montymul(a1, b1), NX[u >> 1]));
			d[u + 1] = mq_add(
				mq_montymul(a1, b0),
				mq_montymul(a0, b1));
#endif
		}
		break;
	case 9:
		for (u = 0; u < 512; u += 4) {
			uint32_t a0, a1, a2, a3, b0, b1, b2, b3, x;

			a0 = a[u];
			a1 = a[u + 1];
			a2 = a[u + 2];
			a3 = a[u + 3];
			b0 = b[u];
			b1 = b[u + 1];
			b2 = b[u + 2];
			b3 = b[u + 3];
			x = NX[u >> 2];
#if Q <= 19433
			d[u] = mq_montyred(a0 * b0
				+ x * mq_montyred(
					a1 * b3 + a2 * b2 + a3 * b1));
			d[u + 1] = mq_montyred(
				a0 * b1 + a1 * b0
				+ x * mq_montyred(a2 * b3 + a3 * b2));
			d[u + 2] = mq_montyred(
				a0 * b2 + a1 * b1 + a2 * b0
				+ x * mq_montyred(a3 * b3));
			d[u + 3] = mq_montyred(
				a0 * b3 + a1 * b2 + a2 * b1 + a3 * b0);
#else
			d[u] = mq_add(
				mq_montymul(a0, b0),
				mq_montymul(x,
					mq_add(
						mq_add(
							mq_montymul(a1, b3),
							mq_montymul(a2, b2)),
						mq_montymul(a3, b1))));
			d[u + 1] = mq_add(
				mq_add(
					mq_montymul(a0, b1),
					mq_montymul(a1, b0)),
				mq_montymul(
					x,
					mq_add(
						mq_montymul(a2, b3),
						mq_montymul(a3, b2))));
			d[u + 2] = mq_add(
				mq_add(
					mq_add(
						mq_montymul(a0, b2),
						mq_montymul(a1, b1)),
					mq_montymul(a2, b0)),
				mq_montymul(x, mq_montymul(a3, b3)));
			d[u + 3] = mq_add(
				mq_add(
					mq_montymul(a0, b3),
					mq_montymul(a1, b2)),
				mq_add(
					mq_montymul(a2, b1),
					mq_montymul(a3, b0)));
#endif
		}
		break;
	case 10:
		for (u = 0; u < 1024; u += 8) {
			uint32_t a0, a1, a2, a3, a4, a5, a6, a7;
			uint32_t b0, b1, b2, b3, b4, b5, b6, b7;
			uint32_t x;

			a0 = a[u];
			a1 = a[u + 1];
			a2 = a[u + 2];
			a3 = a[u + 3];
			a4 = a[u + 4];
			a5 = a[u + 5];
			a6 = a[u + 6];
			a7 = a[u + 7];
			b0 = b[u];
			b1 = b[u + 1];
			b2 = b[u + 2];
			b3 = b[u + 3];
			b4 = b[u + 4];
			b5 = b[u + 5];
			b6 = b[u + 6];
			b7 = b[u + 7];
			x = NX[u >> 3];
#if Q <= 19433
			d[u] = mq_montyred(
				a0 * b0
				+ x * mq_montyred(
					a1 * b7 + a2 * b6 + a3 * b5 + a4 * b4
					+ a5 * b3 + a6 * b2 + a7 * b1));
			d[u + 1] = mq_montyred(
				a0 * b1 + a1 * b0
				+ x * mq_montyred(
					a2 * b7 + a3 * b6 + a4 * b5
					+ a5 * b4 + a6 * b3 + a7 * b2));
			d[u + 2] = mq_montyred(
				a0 * b2 + a1 * b1 + a2 * b0
				+ x * mq_montyred(
					a3 * b7 + a4 * b6 + a5 * b5
					+ a6 * b4 + a7 * b3));
			d[u + 3] = mq_montyred(
				a0 * b3 + a1 * b2 + a2 * b1 + a3 * b0
				+ x * mq_montyred(
					a4 * b7 + a5 * b6 + a6 * b5 + a7 * b4));
			d[u + 4] = mq_montyred(
				a0 * b4 + a1 * b3 + a2 * b2 + a3 * b1 + a4 * b0
				+ x * mq_montyred(
					a5 * b7 + a6 * b6 + a7 * b5));
			d[u + 5] = mq_montyred(
				a0 * b5 + a1 * b4 + a2 * b3 + a3 * b2
				+ a4 * b1 + a5 * b0
				+ x * mq_montyred(a6 * b7 + a7 * b6));
			d[u + 6] = mq_montyred(
				a0 * b6 + a1 * b5 + a2 * b4 + a3 * b3
				+ a4 * b2 + a5 * b1 + a6 * b0
				+ x * mq_montyred(a7 * b7));
			d[u + 7] = mq_montyred(
				a0 * b7 + a1 * b6 + a2 * b5 + a3 * b4
				+ a4 * b3 + a5 * b2 + a6 * b1 + a7 * b0);
#else
			d[u] = mq_add(
				mq_montymul(a0, b0),
				mq_montymul(x, mq_add(
					mq_add(
						mq_add(
							mq_montymul(a1, b7),
							mq_montymul(a2, b6)),
						mq_add(
							mq_montymul(a3, b5),
							mq_montymul(a4, b4))),
					mq_add(
						mq_add(
							mq_montymul(a5, b3),
							mq_montymul(a6, b2)),
						mq_montymul(a7, b1)))));
			d[u + 1] = mq_add(
				mq_add(
					mq_montymul(a0, b1),
					mq_montymul(a1, b0)),
				mq_montymul(x, mq_add(
					mq_add(
						mq_add(
							mq_montymul(a2, b7),
							mq_montymul(a3, b6)),
						mq_add(
							mq_montymul(a4, b5),
							mq_montymul(a5, b4))),
					mq_add(
						mq_montymul(a6, b3),
						mq_montymul(a7, b2)))));
			d[u + 2] = mq_add(
				mq_add(
					mq_add(
						mq_montymul(a0, b2),
						mq_montymul(a1, b1)),
					mq_montymul(a2, b0)),
				mq_montymul(x, mq_add(
					mq_add(
						mq_add(
							mq_montymul(a3, b7),
							mq_montymul(a4, b6)),
						mq_add(
							mq_montymul(a5, b5),
							mq_montymul(a6, b4))),
					mq_montymul(a7, b3))));
			d[u + 3] = mq_add(
				mq_add(
					mq_add(
						mq_montymul(a0, b3),
						mq_montymul(a1, b2)),
					mq_add(
						mq_montymul(a2, b1),
						mq_montymul(a3, b0))),
				mq_montymul(x, mq_add(
					mq_add(
						mq_montymul(a4, b7),
						mq_montymul(a5, b6)),
					mq_add(
						mq_montymul(a6, b5),
						mq_montymul(a7, b4)))));
			d[u + 4] = mq_add(
				mq_add(
					mq_add(
						mq_add(
							mq_montymul(a0, b4),
							mq_montymul(a1, b3)),
						mq_add(
							mq_montymul(a2, b2),
							mq_montymul(a3, b1))),
					mq_montymul(a4, b0)),
				mq_montymul(x, mq_add(
					mq_add(
						mq_montymul(a5, b7),
						mq_montymul(a6, b6)),
					mq_montymul(a7, b5))));
			d[u + 5] = mq_add(
				mq_add(
					mq_add(
						mq_add(
							mq_montymul(a0, b5),
							mq_montymul(a1, b4)),
						mq_add(
							mq_montymul(a2, b3),
							mq_montymul(a3, b2))),
					mq_add(
						mq_montymul(a4, b1),
						mq_montymul(a5, b0))),
				mq_montymul(x, mq_add(
					mq_montymul(a6, b7),
					mq_montymul(a7, b6))));
			d[u + 6] = mq_add(
				mq_add(
					mq_add(
						mq_add(
							mq_montymul(a0, b6),
							mq_montymul(a1, b5)),
						mq_add(
							mq_montymul(a2, b4),
							mq_montymul(a3, b3))),
					mq_add(
						mq_add(
							mq_montymul(a4, b2),
							mq_montymul(a5, b1)),
						mq_montymul(a6, b0))),
				mq_montymul(x, mq_montymul(a7, b7)));
			d[u + 7] = mq_add(
				mq_add(
					mq_add(
						mq_montymul(a0, b7),
						mq_montymul(a1, b6)),
					mq_add(
						mq_montymul(a2, b5),
						mq_montymul(a3, b4))),
				mq_add(
					mq_add(
						mq_montymul(a4, b3),
						mq_montymul(a5, b2)),
					mq_add(
						mq_montymul(a6, b1),
						mq_montymul(a7, b0))));
#endif
		}
		break;
	case 11:
#if Q == 3329
		for (u = 0; u < 2048; u += 16) {
			uint32_t x;
			int i, j;

			x = NX[u >> 4];

			/*
			 * For q = 3329 a full 16-term raw product sum is still
			 * inside mq_montyred()'s validity range. This removes most
			 * per-product reductions in the degree-15 NTT blocks.
			 */
			for (i = 0; i < 16; i ++) {
				uint32_t sum_plain, sum_x, sx;

				sum_plain = 0;
				sum_x = 0;
				for (j = 0; j <= i; j ++) {
					sum_plain += (uint32_t)a[u + j]
						* (uint32_t)b[u + i - j];
				}
				for (j = i + 1; j < 16; j ++) {
					sum_x += (uint32_t)a[u + j]
						* (uint32_t)b[u + 16 + i - j];
				}
				sx = (i == 15) ? Q : mq_montyred(sum_x);
				d[u + i] = mq_montyred(sum_plain + x * sx);
			}
		}
#else
		/* For logn = 11 (N=2048), handle groups of 16 */
		for (u = 0; u < 2048; u += 16) {
			uint32_t x;
			int i, j;

			x = NX[u >> 4];

			/*
			 * We compute the convolution product modulo (X^16 - x).
			 * The formula is:
			 * d[u+i] = sum_{j=0}^{i} (a[u+j]*b[u+i-j]) 
			 *        + x * sum_{j=i+1}^{15} (a[u+j]*b[u+16+i-j])
			 */
			for (i = 0; i < 16; i++) {
				uint32_t sum_plain = Q;
				uint32_t sum_x = Q;

				for (j = 0; j <= i; j++) {
					uint32_t prod = mq_montymul(a[u + j], b[u + i - j]);
					sum_plain = mq_add(sum_plain, prod);
				}

				for (j = i + 1; j < 16; j++) {
					uint32_t prod = mq_montymul(a[u + j], b[u + 16 + i - j]);
					sum_x = mq_add(sum_x, prod);
				}

				sum_x = mq_montymul(x, sum_x);
				d[u + i] = mq_add(sum_plain, sum_x);
			}
		}
#endif
		break;
	}

}

/*
 * Invert one degree-15 block modulo y^16 - x, with coefficients in
 * Montgomery representation. This is used only for logn = 11.
 */
static int
mq_poly_inv_ntt_log11(uint16_t *d, const uint16_t *a)
{
	size_t blk, i, j, k;

	for (blk = 0; blk < 2048; blk += 16) {
		uint32_t mat[16][17];
		uint32_t x;

		x = NX[blk >> 4];
		for (i = 0; i < 16; i ++) {
			for (j = 0; j < 16; j ++) {
				uint32_t v;

				if (j <= i) {
					v = a[blk + i - j];
				} else {
					v = mq_montymul(x, a[blk + 16 + i - j]);
				}
				mat[i][j] = v;
			}
			mat[i][16] = (i == 0) ? R : Q;
		}

		for (j = 0; j < 16; j ++) {
			size_t piv;
			uint32_t invp;

			piv = j;
			while (piv < 16 && mat[piv][j] == Q) {
				piv ++;
			}
			if (piv == 16) {
				return 0;
			}
			if (piv != j) {
				for (k = j; k < 17; k ++) {
					uint32_t t;

					t = mat[j][k];
					mat[j][k] = mat[piv][k];
					mat[piv][k] = t;
				}
			}

			invp = mq_inv(mat[j][j]);
			for (k = j; k < 17; k ++) {
				mat[j][k] = mq_montymul(mat[j][k], invp);
			}

			for (i = 0; i < 16; i ++) {
				uint32_t f;

				if (i == j) {
					continue;
				}
				f = mat[i][j];
				if (f == Q) {
					continue;
				}
				for (k = j; k < 17; k ++) {
					mat[i][k] = mq_sub(mat[i][k],
						mq_montymul(f, mat[j][k]));
				}
			}
		}

		for (i = 0; i < 16; i ++) {
			d[blk + i] = (uint16_t)mat[i][16];
		}
	}

	return 1;
}

/*
 * Polynomial inversion (NTT only). Returned value is 1 on success, 0
 * on failure; a failure is reported if the polynomial is not invertible.
 * On failure, the contents are unpredictable.
 */
UNUSED
static int
mq_poly_inv_ntt(uint16_t *d, const uint16_t *a, unsigned logn)
{

	size_t u, n;
	uint32_t z;

	z = (uint32_t)-1;
	if (logn <= 7) {
		n = (size_t)1 << logn;
		for (u = 0; u < n; u ++) {
			z &= a[u] - Q;
			d[u] = mq_inv(a[u]);
		}
		return (int)(z >> 31);
	}

	/*
	 * For larger degrees, we split polynomial a[] into its odd and
	 * even coefficients:
	 *    a = a0(X^2) + X*a1(X^2)
	 * With a0 and a1 being half-degree polynomials (they operate
	 * modulo X^(n/2)+1).
	 *
	 * We then define an adjoint:
	 *    a' = a0(X^2) - X*a1(X^2)
	 * This yields:
	 *    a*a' = (a0^2)(X^2) - X^2*(a1^2)(X^2)
	 *         = (a0^2 - X*a1^2)(X^2)
	 * i.e. a*a' is a half-degree polynomial (composed with X^2).
	 *
	 * If we can invert a*a', then:
	 *    1/a = (1/(a*a')) * a'
	 * It can be shown that a*a' is invertible if and only if a is
	 * invertible.
	 *
	 * Thus, to invert a polynomial modulo X^n+1, we just have to
	 * invert another polynomial modulo X^(n/2)+1. We can apply this
	 * process recursively to get down to degree 128 (logn = 7),
	 * that we can handle with coefficient-wise inversion in NTT
	 * representation.
	 */
	switch (logn) {

	case 8:
		for (u = 0; u < 256; u += 2) {
			uint32_t a0, a1, c;

			a0 = a[u];
			a1 = a[u + 1];
#if Q <= 19433
			c = mq_montyred(a1 * a1);
			c = mq_montyred(Q2 + a0 * a0 - NX[u >> 1] * c);
#else
			c = mq_montymul(a1, a1);
			c = mq_sub(
				mq_montymul(a0, a0),
				mq_montymul(NX[u >> 1], c));
#endif
			z &= c - Q;
			c = mq_inv(c);
#if Q <= 19433
			d[u] = mq_montyred(a0 * c);
			d[u + 1] = mq_montyred(a1 * (2 * Q - c));
#else
			d[u] = mq_montymul(a0, c);
			d[u + 1] = mq_neg(mq_montymul(a1, c));
#endif
		}
		return (int)(z >> 31);

	case 9:
		for (u = 0; u < 512; u += 4) {
			uint32_t a0, a1, a2, a3, b0, b1, c, x;

			a0 = a[u];
			a1 = a[u + 1];
			a2 = a[u + 2];
			a3 = a[u + 3];
			x = NX[u >> 2];

#if Q <= 19433
			b0 = mq_montyred(a0 * a0 + x * mq_montyred(
				2 * Q2 + a2 * a2 - 2 * a1 * a3));
			b1 = mq_montyred(2 * Q2
				+ 2 * a0 * a2 - a1 * a1
				- x * mq_montyred(a3 * a3));
			c = mq_inv(mq_montyred(
				Q2 + b0 * b0 - x * mq_montyred(b1 * b1)));
			z &= c - Q;
			b0 = mq_montyred(b0 * c);
			b1 = mq_montyred(b1 * (2 * Q - c));

			d[u] = mq_montyred(a0 * b0 + x * mq_montyred(a2 * b1));
			d[u + 1] = mq_montyred(3 * Q2
				- a1 * b0 - x * mq_montyred(a3 * b1));
			d[u + 2] = mq_montyred(a2 * b0 + a0 * b1);
			d[u + 3] = mq_montyred(3 * Q2 - a3 * b0 - a1 * b1);
#else
			b0 = mq_add(
				mq_montymul(a0, a0),
				mq_montymul(x, mq_sub(
					mq_montymul(a2, a2),
					mq_mul2(mq_montymul(a1, a3)))));
			b1 = mq_sub(
				mq_mul2(mq_montymul(a0, a2)),
				mq_add(
					mq_montymul(a1, a1),
					mq_montymul(x, mq_montymul(a3, a3))));
			c = mq_inv(mq_sub(
				mq_montymul(b0, b0),
				mq_montymul(x, mq_montymul(b1, b1))));
			z &= c - Q;
			b0 = mq_montymul(b0, c);
			b1 = mq_neg(mq_montymul(b1, c));

			d[u] = mq_add(
				mq_montymul(a0, b0),
				mq_montymul(x, mq_montymul(a2, b1)));
			d[u + 1] = mq_neg(mq_add(
				mq_montymul(a1, b0),
				mq_montymul(x, mq_montymul(a3, b1))));
			d[u + 2] = mq_add(
				mq_montymul(a2, b0),
				mq_montymul(a0, b1));
			d[u + 3] = mq_neg(mq_add(
				mq_montymul(a3, b0),
				mq_montymul(a1, b1)));
#endif
		}
		return (int)(z >> 31);

	case 10:
		for (u = 0; u < 1024; u += 8) {
			uint32_t a0, a1, a2, a3, a4, a5, a6, a7;
			uint32_t b0, b1, b2, b3, c0, c1, e, x;
			uint32_t f0, f1, f2, f3;

			a0 = a[u];
			a1 = a[u + 1];
			a2 = a[u + 2];
			a3 = a[u + 3];
			a4 = a[u + 4];
			a5 = a[u + 5];
			a6 = a[u + 6];
			a7 = a[u + 7];
			x = NX[u >> 3];

#if Q <= 19433
			b0 = mq_montyred(a0 * a0 + x * mq_montyred(
				4 * Q2 + a4 * a4
				+ 2 * (a2 * a6 - a1 * a7 - a3 * a5)));
			b1 = mq_montyred(Q2 + 2 * a0 * a2 - a1 * a1
				+ x * mq_montyred(3 * Q2 - a5 * a5
				+ 2 * (a4 * a6 - a3 * a7)));
			b2 = mq_montyred(2 * Q2 + a2 * a2
				+ 2 * (a0 * a4 - a1 * a3)
				+ x * mq_montyred(2 * Q2
				+ a6 * a6 - 2 * a5 * a7));
			b3 = mq_montyred(4 * Q2 - a3 * a3
				+ 2 * (a0 * a6 + a2 * a4 - a1 * a5)
				- x * mq_montyred(a7 * a7));

			c0 = mq_montyred(b0 * b0 + x * mq_montyred(
				2 * Q2 + b2 * b2 - 2 * b1 * b3));
			c1 = mq_montyred(2 * Q2
				+ 2 * b0 * b2 - b1 * b1
				- x * mq_montyred(b3 * b3));
			e = mq_inv(mq_montyred(
				Q2 + c0 * c0 - x * mq_montyred(c1 * c1)));
			z &= e - Q;
			c0 = mq_montyred(c0 * e);
			c1 = mq_montyred(c1 * (2 * Q - e));

			f0 = mq_montyred(b0 * c0 + x * mq_montyred(b2 * c1));
			f1 = mq_montyred(3 * Q2
				- b1 * c0 - x * mq_montyred(b3 * c1));
			f2 = mq_montyred(b2 * c0 + b0 * c1);
			f3 = mq_montyred(3 * Q2 - b3 * c0 - b1 * c1);

			d[u] = mq_montyred(a0 * f0 + x * mq_montyred(
				a2 * f3 + a4 * f2 + a6 * f1));
			d[u + 1] = mq_montyred(3 * Q2 - a1 * f0
				- x * mq_montyred(a3 * f3 + a5 * f2 + a7 * f1));
			d[u + 2] = mq_montyred(a0 * f1 + a2 * f0
				+ x * mq_montyred(a4 * f3 + a6 * f2));
			d[u + 3] = mq_montyred(4 * Q2 - a1 * f1 - a3 * f0
				- x * mq_montyred(a5 * f3 + a7 * f2));
			d[u + 4] = mq_montyred(a0 * f2 + a2 * f1
				+ a4 * f0 + x * mq_montyred(a6 * f3));
			d[u + 5] = mq_montyred(5 * Q2 - a1 * f2 - a3 * f1
				- a5 * f0 - x * mq_montyred(a7 * f3));
			d[u + 6] = mq_montyred(a0 * f3 + a2 * f2
				+ a4 * f1 + a6 * f0);
			d[u + 7] = mq_montyred(5 * Q2 - a1 * f3 - a3 * f2
				- a5 * f1 - a7 * f0);
#else
			b0 = mq_add(
				mq_montymul(a0, a0),
				mq_montymul(x, mq_add(
					mq_montymul(a4, a4),
					mq_mul2(mq_sub(
						mq_montymul(a2, a6),
						mq_add(
							mq_montymul(a1, a7),
							mq_montymul(a3, a5)))))));
			b1 = mq_add(
				mq_sub(
					mq_mul2(mq_montymul(a0, a2)),
					mq_montymul(a1, a1)),
				mq_montymul(x, mq_sub(
					mq_mul2(mq_sub(
						mq_montymul(a4, a6),
						mq_montymul(a3, a7))),
					mq_montymul(a5, a5))));
			b2 = mq_add(
				mq_add(
					mq_montymul(a2, a2),
					mq_mul2(mq_sub(
						mq_montymul(a0, a4),
						mq_montymul(a1, a3)))),
				mq_montymul(x, mq_sub(
					mq_montymul(a6, a6),
					mq_mul2(mq_montymul(a5, a7)))));
			b3 = mq_sub(
				mq_mul2(mq_sub(
					mq_add(
						mq_montymul(a0, a6),
						mq_montymul(a2, a4)),
					mq_montymul(a1, a5))),
				mq_add(
					mq_montymul(a3, a3),
					mq_montymul(x, mq_montymul(a7, a7))));

			c0 = mq_add(
				mq_montymul(b0, b0),
				mq_montymul(x, mq_sub(
					mq_montymul(b2, b2),
					mq_mul2(mq_montymul(b1, b3)))));
			c1 = mq_sub(
				mq_mul2(mq_montymul(b0, b2)),
				mq_add(
					mq_montymul(b1, b1),
					mq_montymul(x, mq_montymul(b3, b3))));
			e = mq_inv(mq_sub(
				mq_montymul(c0, c0),
				mq_montymul(x, mq_montymul(c1, c1))));
			z &= e - Q;
			c0 = mq_montymul(c0, e);
			c1 = mq_neg(mq_montymul(c1, e));

			f0 = mq_add(
				mq_montymul(b0, c0),
				mq_montymul(x, mq_montymul(b2, c1)));
			f1 = mq_neg(mq_add(
				mq_montymul(b1, c0),
				mq_montymul(x, mq_montymul(b3, c1))));
			f2 = mq_add(
				mq_montymul(b2, c0),
				mq_montymul(b0, c1));
			f3 = mq_neg(mq_add(
				mq_montymul(b3, c0),
				mq_montymul(b1, c1)));

			d[u] = mq_add(
				mq_montymul(a0, f0),
				mq_montymul(x, mq_add(
					mq_add(
						mq_montymul(a2, f3),
						mq_montymul(a4, f2)),
					mq_montymul(a6, f1))));
			d[u + 1] = mq_neg(mq_add(
				mq_montymul(a1, f0),
				mq_montymul(x, mq_add(
					mq_add(
						mq_montymul(a3, f3),
						mq_montymul(a5, f2)),
					mq_montymul(a7, f1)))));
			d[u + 2] = mq_add(
				mq_add(
					mq_montymul(a0, f1),
					mq_montymul(a2, f0)),
				mq_montymul(x, mq_add(
					mq_montymul(a4, f3),
					mq_montymul(a6, f2))));
			d[u + 3] = mq_neg(mq_add(
				mq_add(
					mq_montymul(a1, f1),
					mq_montymul(a3, f0)),
				mq_montymul(x, mq_add(
					mq_montymul(a5, f3),
					mq_montymul(a7, f2)))));
			d[u + 4] = mq_add(
				mq_add(
					mq_montymul(a0, f2),
					mq_montymul(a2, f1)),
				mq_add(
					mq_montymul(a4, f0),
					mq_montymul(x, mq_montymul(a6, f3))));
			d[u + 5] = mq_neg(mq_add(
				mq_add(
					mq_montymul(a1, f2),
					mq_montymul(a3, f1)),
				mq_add(
					mq_montymul(a5, f0),
					mq_montymul(x, mq_montymul(a7, f3)))));
			d[u + 6] = mq_add(
				mq_add(
					mq_montymul(a0, f3),
					mq_montymul(a2, f2)),
				mq_add(
					mq_montymul(a4, f1),
					mq_montymul(a6, f0)));
			d[u + 7] = mq_neg(mq_add(
				mq_add(
					mq_montymul(a1, f3),
					mq_montymul(a3, f2)),
				mq_add(
					mq_montymul(a5, f1),
					mq_montymul(a7, f0))));
#endif
		}
		return (int)(z >> 31);
		
	case 11:
		return mq_poly_inv_ntt_log11(d, a);

	default:
		/* normally unreachable if logn is correct */
		return 0;
	}

}

/*
 * TTx[] contains the NTT representation of 1+X+X^2+X^3+...+X^(n-1) for
 * degree n = 2^x (for 1 <= x <= 7).
 */

#if Q == 257

static const uint16_t TT1[] = {
	 242,   17
};

static const uint16_t TT2[] = {
	  53,  174,   85,  206
};

static const uint16_t TT3[] = {
	 143,  220,   87,    4,  255,  172,   39,  116
};

static const uint16_t TT4[] = {
	  59,  227,   34,  149,  213,  218,  124,  141,  118,  135,
	  41,   46,  110,  225,   32,  200
};

static const uint16_t TT5[] = {
	 212,  163,   43,  154,  245,   80,  109,  189,  198,  228,
	 127,   52,  166,   82,  123,  159,  100,  136,  177,   93,
	 207,  132,   31,   61,   70,  150,  179,   14,  105,  216,
	  96,   47
};

static const uint16_t TT6[] = {
	  64,  103,   78,  248,  139,  204,   44,    7,   81,  152,
	 234,  183,   15,  203,  241,  137,  199,  197,  194,    5,
	  72,  182,  214,  147,  219,  113,   13,  151,   23,  223,
	  71,  247,   12,  188,   36,  236,  108,  246,  146,   40,
	 112,   45,   77,  187,  254,   65,   62,   60,  122,   18,
	  56,  244,   76,   25,  107,  178,  252,  215,   55,  120,
	  11,  181,  156,  195
};

static const uint16_t TT7[] = {
	 256,  129,   42,  164,  148,    8,  140,   99,  144,  134,
	 155,  253,   51,   37,  106,  165,  233,  186,  173,  131,
	  10,  201,  205,  161,  142,  145,  168,  238,   35,  190,
	 185,   89,  240,  158,  121,   16,  102,   29,  239,   28,
	 169,  232,  171,  193,  133,   38,  211,   83,   97,   84,
	  30,  196,   33,  250,  210,   92,   68,  235,  237,  209,
	  67,   75,   57,  180,   79,  202,  184,  192,   50,   22,
	  24,  191,  167,   49,    9,  226,   63,  229,  175,  162,
	 176,   48,  221,  126,   66,   88,   27,   90,  231,   20,
	 230,  157,  243,  138,  101,   19,  170,   74,   69,  224,
	  21,   91,  114,  117,   98,   54,   58,  249,  128,   86,
	  73,   26,   94,  153,  222,  208,    6,  104,  125,  115,
	 160,  119,  251,  111,   95,  217,  130,    3
};

#elif Q == 769

static const uint16_t TT1[] = {
	 379,  428
};

static const uint16_t TT2[] = {
	 581,  177,  630,  226
};

static const uint16_t TT3[] = {
	 531,  631,  498,  625,  182,  309,  176,  276
};

static const uint16_t TT4[] = {
	   6,  287,  370,  123,  103,  124,  585,  665,  142,  222,
	 683,  704,  684,  437,  520,   32
};

static const uint16_t TT5[] = {
	 390,  391,  360,  214,  547,  193,  482,  533,  400,  575,
	 518,  499,  107,  294,  130,  431,  376,  677,  513,  700,
	 308,  289,  232,  407,  274,  325,  614,  260,  593,  447,
	 416,  417
};

static const uint16_t TT6[] = {
	 346,  434,  539,  243,  432,  288,  175,  253,   54,  271,
	 521,  634,  524,  440,  755,  311,   36,  764,  334,   47,
	  68,  199,  330,  668,  574,  409,  247,  341,  344,  685,
	 169,  693,  114,  638,  122,  463,  466,  560,  398,  233,
	 139,  477,  608,  739,  760,  473,   43,    2,  496,   52,
	 367,  283,  173,  286,  536,  753,  554,  632,  519,  375,
	 564,  268,  373,  461
};

static const uint16_t TT7[] = {
	 598,   94,  528,  340,   12,  297,  588,  667,  327,  537,
	  14,  562,  640,  479,  143,  363,  471,  406,   56,  486,
	 304,  738,  460,   39,   64,  215,  508,  372,  492,  249,
	 174,  448,  545,  296,  234,  525,  672,  765,  653,  210,
	 121,   15,  557,  610,  202,  458,  369,  198,  582,  566,
	 144,  674,  237,  257,  649,   33,   60,  628,  749,  621,
	 559,  548,   91,  526,  281,  716,  259,  248,  186,   58,
	 179,  747,    5,  158,  550,  570,  133,  663,  241,  225,
	 609,  438,  349,  605,  197,  250,   23,  686,  597,  154,
	  42,  135,  282,  573,  511,  262,  359,  633,  558,  315,
	 435,  299,  592,  743,  768,  347,   69,  503,  321,  751,
	 401,  336,  444,  664,  328,  167,  245,   24,  270,  480,
	 140,  219,  510,   26,  467,  279,  713,  209
};

#elif Q == 3329

static const uint16_t TT1[] = {
	 403, 2303
};

static const uint16_t TT2[] = {
	2640, 1495, 1211,   66
};

static const uint16_t TT3[] = {
	 611, 1340, 3091, 3228, 2807, 2944, 1366, 2095
};

static const uint16_t TT4[] = {
	3007, 1544,  298, 2382, 2843,   10,  636, 2491,  215, 2070,
	2696, 3192,  324, 2408, 1162, 3028
};

static const uint16_t TT5[] = {
	2792, 3222, 1273, 1815, 1407, 2518, 2150, 2614,  303, 2054,
	2970,  379,  594,  678, 2700, 2282,  424,    6, 2028, 2112,
	2327, 3065,  652, 2403,   92,  556,  188, 1299,  891, 1433,
	2813, 3243
};

static const uint16_t TT6[] = {
	3098, 2486, 1315, 1800, 3163, 2712, 1788, 1842,    2, 2812,
	 357, 1350, 1582, 2718,  150, 1749, 3060,  875, 1695, 2413,
	2418,  193, 2655, 1432, 1558, 2959, 2446, 2239,  472, 1599,
	 727,  508, 2198, 1979, 1107, 2234,  467,  260, 3076, 1148,
	1274,   51, 2513,  288,  293, 1011, 1831, 2975,  957, 2556,
	3317, 1124, 1356, 2349, 3223, 2704,  864,  918, 3323, 2872,
	 906, 1391,  220, 2937
};

static const uint16_t TT7[] = {
	1755, 1112,  222, 1421, 1803,  827,  684, 2916, 2574,  423,
	1936,  159, 1483, 2093, 2929,  755, 1082, 2251, 2263,   32,
	 531,  183, 2639,   61, 1283, 1881, 3217, 2219,  893, 2736,
	2297, 1201, 2667,  124,  456, 1294, 2108, 1282,  526,  971,
	1260,  247, 3289,  426, 2535, 2775, 3069, 3124, 2176,  940,
	1719,  870,   72, 1491,   81, 1068,  591,  353, 1784, 1414,
	3067, 1716, 1230, 3115, 2920, 1476,  990, 2968, 1292,  922,
	2353, 2115, 1638, 2625, 1215, 2634, 1836,  987, 1766,  530,
	2911, 2966, 3260,  171, 2280, 2746, 2459, 1446, 1735, 2180,
	1424,  598, 1412, 2250, 2582,   39, 1505,  409, 3299, 1813,
	 487, 2818,  825, 1423, 2645,   67, 2523, 2175, 2674,  443,
	 455, 1624, 1951, 3106,  613, 1223, 2547,  770, 2283,  132,
	3119, 2022, 1879,  903, 1285, 2484, 1594,  951
};

#elif Q == 64513

static const uint16_t TT1[] = {
	51870, 41285
};

static const uint16_t TT2[] = {
	57594, 46146, 47009, 35561
};

static const uint16_t TT3[] = {
	17815, 32860, 20468,  7311, 21331,  8174, 60295, 10827
};

static const uint16_t TT4[] = {
	50374, 49769, 28753, 36967, 35100,  5836, 59024, 20111,
	 8531, 34131, 22806, 58055, 56188, 64402, 43386, 42781
};

static const uint16_t TT5[] = {
	63672, 37076, 51977, 47561, 16345, 41161, 55429, 18505,
	 4032,  1655,  8808,  2864, 49976,  3559, 31777,  8445,
	20197, 61378, 25083, 43179, 25778, 19834, 26987, 24610,
	10137, 37726, 51994, 12297, 45594, 41178, 56079, 29483
};

static const uint16_t TT6[] = {
	12126, 50705, 11707, 62445, 49627, 54327, 59852, 35270,
	17310, 15380, 16703,  1106, 27633, 18712, 23743, 13267,
	 3682,  4382, 45431, 22392, 41204, 40925,  4775,   953,
	44949, 55003, 49689, 21942, 18267, 45287, 28338, 53065,
	40090,   304, 47868, 10375,  6700, 43466, 38152, 48206,
	27689, 23867, 52230, 51951,  6250, 47724, 24260, 24960,
	15375,  4899,  9930,  1009, 27536, 11939, 13262, 11332,
	57885, 33303, 38828, 43528, 30710, 16935, 42450, 16516
};

static const uint16_t TT7[] = {
	  585, 23667, 32462,  4435, 60735, 27192, 42895, 17482,
	50967, 48287, 45874, 62780, 44098, 11093,  4354,  1673,
	13505, 21115, 18884, 11876,  9364, 24042, 53145, 13580,
	59318, 60461,  1231, 36193, 43707,  3779, 57840, 33207,
	52870, 19007, 29145, 44132, 59648, 31214, 32727, 12057,
	37267, 45141, 39280, 42570, 46442, 27621, 59365,  7054,
	  836, 24549, 14177, 31316, 16482, 18383, 16899, 26985,
	59232, 41815, 10205, 15856, 24715, 31961, 44768, 61362,
	31793, 48387, 61194,  3927, 12786, 18437, 51340, 33923,
	 1657, 11743, 10259, 12160, 61839, 14465,  4093, 27806,
	21588, 33790,  1021, 46713, 50585, 53875, 48014, 55888,
	16585, 60428, 61941, 33507, 49023, 64010,  9635, 40285,
	59948, 35315, 24863, 49448, 56962, 27411, 32694, 33837,
	15062, 40010,  4600, 19278, 16766,  9758,  7527, 15137,
	26969, 24288, 17549, 49057, 30375, 47281, 44868, 42188,
	11160, 50260,  1450, 32420, 24207, 60693,  4975, 28057
};

#endif

/*
 * Multiply a polynomial 'a' by 'ones', with 'ones' being the polynomial
 * 1+X+X^2+X^3+...+X^n. Source and destination are in NTT representation.
 */
UNUSED
static void
mq_poly_mul_ones_ntt(uint16_t *d, const uint16_t *a, unsigned logn)
{
	size_t u;

	switch (logn) {
	case 1:
		mq_poly_mul_ntt(d, a, TT1, logn);
		break;
	case 2:
		mq_poly_mul_ntt(d, a, TT2, logn);
		break;
	case 3:
		mq_poly_mul_ntt(d, a, TT3, logn);
		break;
	case 4:
		mq_poly_mul_ntt(d, a, TT4, logn);
		break;
	case 5:
		mq_poly_mul_ntt(d, a, TT5, logn);
		break;
	case 6:
		mq_poly_mul_ntt(d, a, TT6, logn);
		break;
	case 7:
		mq_poly_mul_ntt(d, a, TT7, logn);
		break;
	case 8:
		for (u = 0; u < 256; u += 2) {
			uint32_t a0, a1, b;

			a0 = a[u];
			a1 = a[u + 1];
			b = TT7[u >> 1];
#if Q <= 19433
			d[u] = mq_montyred(
				b * (a0 + mq_montyred(a1 * NX[u >> 1])));
			d[u + 1] = mq_montyred(b * (a0 + a1));
#else
			d[u] = mq_montymul(b, mq_add(a0,
				mq_montymul(a1, NX[u >> 1])));
			d[u + 1] = mq_montymul(b, mq_add(a0, a1));
#endif
		}
		break;
	case 9:
		for (u = 0; u < 512; u += 4) {
			uint32_t a0, a1, a2, a3, b, x;

			a0 = a[u];
			a1 = a[u + 1];
			a2 = a[u + 2];
			a3 = a[u + 3];
			b = TT7[u >> 2];
			x = NX[u >> 2];
#if Q <= 19433
			d[u] = mq_montyred(
				b * (a0 + mq_montyred(x * (a1 + a2 + a3))));
			d[u + 1] = mq_montyred(
				b * (a0 + a1 + mq_montyred(x * (a2 + a3))));
			d[u + 2] = mq_montyred(
				b * (a0 + a1 + a2 + mq_montyred(x * a3)));
			d[u + 3] = mq_montyred(
				b * (a0 + a1 + a2 + a3));
#else
			d[u] = mq_montymul(b, mq_add(a0, mq_montymul(x,
				mq_add(mq_add(a1, a2), a3))));
			d[u + 1] = mq_montymul(b, mq_add(
				mq_add(a0, a1),
				mq_montymul(x, mq_add(a2, a3))));
			d[u + 2] = mq_montymul(b, mq_add(
				mq_add(a0, a1),
				mq_add(a2, mq_montymul(x, a3))));
			d[u + 3] = mq_montymul(b, mq_add(
				mq_add(a0, a1), mq_add(a2, a3)));
#endif
		}
		break;
	case 10:
		for (u = 0; u < 1024; u += 8) {
			uint32_t a0, a1, a2, a3, a4, a5, a6, a7;
			uint32_t b, x;

			a0 = a[u];
			a1 = a[u + 1];
			a2 = a[u + 2];
			a3 = a[u + 3];
			a4 = a[u + 4];
			a5 = a[u + 5];
			a6 = a[u + 6];
			a7 = a[u + 7];
			b = TT7[u >> 3];
			x = NX[u >> 3];
#if Q <= 19433
			d[u] = mq_montyred(
				b * (a0 + mq_montyred(
				x * (a1 + a2 + a3 + a4 + a5 + a6 + a7))));
			d[u + 1] = mq_montyred(
				b * (a0 + a1 + mq_montyred(
				x * (a2 + a3 + a4 + a5 + a6 + a7))));
			d[u + 2] = mq_montyred(
				b * (a0 + a1 + a2 + mq_montyred(
				x * (a3 + a4 + a5 + a6 + a7))));
			d[u + 3] = mq_montyred(
				b * (a0 + a1 + a2 + a3 + mq_montyred(
				x * (a4 + a5 + a6 + a7))));
			d[u + 4] = mq_montyred(
				b * (a0 + a1 + a2 + a3 + a4 + mq_montyred(
				x * (a5 + a6 + a7))));
			d[u + 5] = mq_montyred(
				b * (a0 + a1 + a2 + a3 + a4 + a5 + mq_montyred(
				x * (a6 + a7))));
			d[u + 6] = mq_montyred(
				b * (a0 + a1 + a2 + a3 + a4 + a5 + a6
				+ mq_montyred(x * a7)));
			d[u + 7] = mq_montyred(
				b * (a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7));
#else
			d[u] = mq_montymul(b, mq_add(a0, mq_montymul(x,
				mq_add(
					mq_add(
						mq_add(a1, a2),
						mq_add(a3, a4)),
					mq_add(
						mq_add(a5, a6),
						a7)))));
			d[u + 1] = mq_montymul(b, mq_add(
				mq_add(a0, a1),
				mq_montymul(x, mq_add(
					mq_add(
						mq_add(a2, a3),
						mq_add(a4, a5)),
					mq_add(a6, a7)))));
			d[u + 2] = mq_montymul(b, mq_add(
				mq_add(
					mq_add(a0, a1),
					a2),
				mq_montymul(x, mq_add(
					mq_add(
						mq_add(a3, a4),
						mq_add(a5, a6)),
					a7))));
			d[u + 3] = mq_montymul(b, mq_add(
				mq_add(
					mq_add(a0, a1),
					mq_add(a2, a3)),
				mq_montymul(x, mq_add(
					mq_add(a4, a5),
					mq_add(a6, a7)))));
			d[u + 4] = mq_montymul(b, mq_add(
				mq_add(
					mq_add(
						mq_add(a0, a1),
						mq_add(a2, a3)),
					a4),
				mq_montymul(x, mq_add(
					mq_add(a5, a6),
					a7))));
			d[u + 5] = mq_montymul(b, mq_add(
				mq_add(
					mq_add(
						mq_add(a0, a1),
						mq_add(a2, a3)),
					mq_add(a4, a5)),
				mq_montymul(x, mq_add(a6, a7))));
			d[u + 6] = mq_montymul(b, mq_add(
				mq_add(
					mq_add(
						mq_add(a0, a1),
						mq_add(a2, a3)),
					mq_add(
						mq_add(a4, a5),
						a6)),
				mq_montymul(x, a7)));
			d[u + 7] = mq_montymul(b, mq_add(
				mq_add(
					mq_add(a0, a1),
					mq_add(a2, a3)),
				mq_add(
					mq_add(a4, a5),
					mq_add(a6, a7))));
#endif
		}
		break;
	case 11:
		for (u = 0; u < 2048; u += 16) {
			uint32_t b, x, sum_x, sum_plain;
			int i, j;

			b = TT7[u >> 4];
			x = NX[u >> 4];

			/* 
			 * We want to compute:
			 * d[u+i] = b * ( sum_{j=0}^{i} a[u+j] + x * sum_{j=i+1}^{15} a[u+j] )
			 * We do this safely using the modular addition functions for any Q.
			 */
			for (i = 0; i < 16; i++) {
				sum_plain = Q;
				for (j = 0; j <= i; j++) {
					sum_plain = mq_add(sum_plain, a[u + j]);
				}

				sum_x = Q;
				for (j = i + 1; j < 16; j++) {
					sum_x = mq_add(sum_x, a[u + j]);
				}

				/* Multiply the tail sum by x */
				sum_x = mq_montymul(x, sum_x);

				/* Add both parts and multiply by b */
				d[u + i] = mq_montymul(b, mq_add(sum_plain, sum_x));
			}
		}
		break;
	}
}

/*
 * Add a constant value (in Montgomery representation) to a polynomial,
 * in NTT representation.
 */
UNUSED
static void
mq_poly_addconst_ntt(uint16_t *d, const uint16_t *a, uint32_t c, unsigned logn)
{

	size_t u, n;

	switch (logn) {
	case 8:
		memmove(d, a, 256 * sizeof *a);
#if defined CTL_AVX2 && CTL_AVX2 && Q <= 40504
		mq_avx2_poly_addconst_stride(d, c, 256, 2);
#else
		for (u = 0; u < 256; u += 2) {
			d[u] = mq_add(d[u], c);
		}
#endif
		break;

	case 9:
		memmove(d, a, 512 * sizeof *a);
#if defined CTL_AVX2 && CTL_AVX2 && Q <= 40504
		mq_avx2_poly_addconst_stride(d, c, 512, 4);
#else
		for (u = 0; u < 512; u += 4) {
			d[u] = mq_add(d[u], c);
		}
#endif
		break;

	case 10:
		memmove(d, a, 1024 * sizeof *a);
#if defined CTL_AVX2 && CTL_AVX2 && Q <= 40504
		mq_avx2_poly_addconst_stride(d, c, 1024, 8);
#else
		for (u = 0; u < 1024; u += 8) {
			d[u] = mq_add(d[u], c);
		}
#endif
		break;
		
	case 11:
		memmove(d, a, 2048 * sizeof *a);
#if defined CTL_AVX2 && CTL_AVX2 && Q <= 40504
		mq_avx2_poly_addconst_stride(d, c, 2048, 16);
#else
		for (u = 0; u < 2048; u += 16) {
			d[u] = mq_add(d[u], c);
		}
#endif
		break;
		
	default:
		n = (size_t)1 << logn;
		for (u = 0; u < n; u ++) {
			d[u] = mq_add(a[u], c);
		}
		break;
	}

}
