#include <stdint.h>
#include "params.h"
#include "ntt.h"
#include "reduce.h"

#if KEM_MODE == 128 || KEM_MODE == 256

const int16_t zetas[128] = {
    171, 605, 688, 361, 583, 3, 250, 120, 640, 461, 223, 753, 626, 362, 
    432, 638, 694, 733, 76, 98, 203, 282, 430, 514, 178, 270, 199, 34, 
    400, 192, 620, 759, 689, 423, 645, 2, 114, 147, 715, 497, 600, 288, 
    161, 754, 683, 51, 405, 502, 170, 543, 648, 188, 719, 745, 307, 578, 
    263, 157, 523, 128, 375, 180, 389, 279, 428, 390, 202, 220, 236, 21, 
    212, 71, 635, 151, 23, 657, 537, 227, 717, 621, 244, 517, 532, 686, 
    652, 436, 703, 522, 477, 352, 624, 238, 493, 575, 495, 699, 209, 654, 670, 
    14, 29, 260, 391, 403, 355, 478, 358, 664, 167, 357, 528, 438, 421, 725, 
    691, 547, 419, 601, 611, 201, 303, 330, 585, 127, 318, 491, 416, 415};

const int16_t zetas_inv[128] = {
    354, 353, 278, 451, 642, 184, 439, 466, 568, 158, 168, 350, 222, 78, 
    44, 348, 331, 241, 412, 602, 105, 411, 291, 414, 366, 378, 509, 740, 755, 
    99, 115, 560, 70, 274, 194, 276, 531, 145, 417, 292, 247, 66, 333, 117, 83, 
    237, 252, 525, 148, 52, 542, 232, 112, 746, 618, 134, 698, 557, 748, 533, 
    549, 567, 379, 341, 490, 380, 589, 394, 641, 246, 612, 506, 191, 462, 24, 
    50, 581, 121, 226, 599, 267, 364, 718, 86, 15, 608, 481, 169, 272, 54, 622, 
    655, 767, 124, 346, 80, 10, 149, 577, 369, 735, 570, 499, 591, 255, 339, 
    487, 566, 671, 693, 36, 75, 131, 337, 407, 143, 16, 546, 308, 129, 649, 
    519, 766, 186, 408, 81, 164, 655};

static const int16_t fqinv_table[769] = {
    0, 19, -375, -250, 197, -150, -125, -217, -286, 173, -75, -208, 322, -176, 276, -50, 
    -143, 363, -298, 1, 347, 184, -104, 168, 161, -30, -88, 314, 138, -238, -25, -49, 
    313, 187, -203, -351, -149, 146, -384, -315, -211, 113, 92, 376, -52, -273, 84, 344, 
    -304, -31, -15, 121, -44, 218, 157, 266, 69, -256, -119, 261, 372, -214, 360, -195, 
    -228, -189, -291, -367, 283, 56, 209, -303, 310, 74, 73, -10, -192, 190, 227, 302, 
    279, 361, -328, 167, 46, -235, 188, 177, -26, -259, 248, -135, 42, 240, 172, 154, 
    -152, -198, 369, -194, 377, -213, -324, -179, -22, -117, 109, -122, -306, 106, 133, 305, 
    -350, 41, -128, -274, 325, -105, -254, -58, 186, 51, -107, 294, 180, -6, 287, -236, 
    -114, -131, 290, -129, 239, 110, 201, -91, -243, -230, 28, -365, -280, 371, 233, -16, 
    155, 260, 37, 246, -348, -36, -5, 158, -96, -216, 95, 144, -271, 54, 151, 329, 
    -245, 24, -204, -349, -164, 345, -301, 83, 23, -191, 267, 171, 94, 9, -296, -224, 
    -13, 87, 255, -103, 124, 323, 317, 185, 21, 183, 120, 33, 86, -65, 77, -169, 
    -76, -231, -99, -63, -200, 4, -97, -282, -196, 134, 278, -34, -162, -285, 295, 275, 
    -11, 70, 326, -40, -330, -101, -61, 229, -153, -7, 53, 281, -318, -327, -232, -362, 
    -175, 253, -364, 78, -64, 215, -137, -193, -222, 142, 332, -85, -127, 357, -29, 132, 
    93, -252, -359, -136, 331, -160, 147, 355, 90, 312, -3, -340, -241, 225, -118, 178, 
    -57, -356, 319, -89, 145, 59, 320, 272, -265, -264, 55, 170, -284, -383, 339, -156, 
    263, -45, -115, 207, 14, 311, 202, 80, -140, 219, -199, 68, -268, -205, -8, 126, 
    -307, 338, 130, -66, -366, -370, 123, 206, -174, -321, -18, -342, 382, -166, 79, -71, 
    -48, 111, -108, -288, -337, -316, 72, 277, 249, 32, 27, -39, -309, 182, -220, 258, 
    262, -297, 12, 181, -102, 116, 210, -221, -82, 159, -212, 244, 234, 358, -343, -381, 
    -373, -308, 289, 270, -251, -354, -299, -334, 47, 165, -380, 20, -148, -163, -112, -35, 
    378, -379, -341, 247, -257, 237, 333, -242, 62, 81, -223, 17, -226, -139, -292, -67, 
    -374, 98, -293, 141, 60, -336, -368, -2, 43, 100, 352, -353, -346, -335, 300, -269, 
    -38, 38, 269, -300, 335, 346, 353, -352, -100, -43, 2, 368, 336, -60, -141, 293, 
    -98, 374, 67, 292, 139, 226, -17, 223, -81, -62, 242, -333, -237, 257, -247, 341, 
    379, -378, 35, 112, 163, 148, -20, 380, -165, -47, 334, 299, 354, 251, -270, -289, 
    308, 373, 381, 343, -358, -234, -244, 212, -159, 82, 221, -210, -116, 102, -181, -12, 
    297, -262, -258, 220, -182, 309, 39, -27, -32, -249, -277, -72, 316, 337, 288, 108, 
    -111, 48, 71, -79, 166, -382, 342, 18, 321, 174, -206, -123, 370, 366, 66, -130, 
    -338, 307, -126, 8, 205, 268, -68, 199, -219, 140, -80, -202, -311, -14, -207, 115, 
    45, -263, 156, -339, 383, 284, -170, -55, 264, 265, -272, -320, -59, -145, 89, -319, 
    356, 57, -178, 118, -225, 241, 340, 3, -312, -90, -355, -147, 160, -331, 136, 359, 
    252, -93, -132, 29, -357, 127, 85, -332, -142, 222, 193, 137, -215, 64, -78, 364, 
    -253, 175, 362, 232, 327, 318, -281, -53, 7, 153, -229, 61, 101, 330, 40, -326, 
    -70, 11, -275, -295, 285, 162, 34, -278, -134, 196, 282, 97, -4, 200, 63, 99, 
    231, 76, 169, -77, 65, -86, -33, -120, -183, -21, -185, -317, -323, -124, 103, -255, 
    -87, 13, 224, 296, -9, -94, -171, -267, 191, -23, -83, 301, -345, 164, 349, 204, 
    -24, 245, -329, -151, -54, 271, -144, -95, 216, 96, -158, 5, 36, 348, -246, -37, 
    -260, -155, 16, -233, -371, 280, 365, -28, 230, 243, 91, -201, -110, -239, 129, -290, 
    131, 114, 236, -287, 6, -180, -294, 107, -51, -186, 58, 254, 105, -325, 274, 128, 
    -41, 350, -305, -133, -106, 306, 122, -109, 117, 22, 179, 324, 213, -377, 194, -369, 
    198, 152, -154, -172, -240, -42, 135, -248, 259, 26, -177, -188, 235, -46, -167, 328, 
    -361, -279, -302, -227, -190, 192, 10, -73, -74, -310, 303, -209, -56, -283, 367, 291, 
    189, 228, 195, -360, 214, -372, -261, 119, 256, -69, -266, -157, -218, 44, -121, 15, 
    31, 304, -344, -84, 273, 52, -376, -92, -113, 211, 315, -385, -146, 149, 351, 203, 
    -187, -313, 49, 25, 238, -138, -314, 88, 30, -161, -168, 104, -184, -347, -1, 298, 
    -363, 143, 50, -276, 176, -322, 208, 75, -173, 286, 217, 125, 150, -197, 250, 375, 
    -19
};

#elif KEM_MODE == 512

const int16_t zetas[128] = {
    2285, 2571, 2970, 1812, 1493, 1422, 287, 202, 3158, 622, 1577, 182, 962, 
    2127, 1855, 1468, 573, 2004, 264, 383, 2500, 1458, 1727, 3199, 2648, 1017, 
    732, 608, 1787, 411, 3124, 1758, 1223, 652, 2777, 1015, 2036, 1491, 3047, 
    1785, 516, 3321, 3009, 2663, 1711, 2167, 126, 1469, 2476, 3239, 3058, 830, 
    107, 1908, 3082, 2378, 2931, 961, 1821, 2604, 448, 2264, 677, 2054, 2226, 
    430, 555, 843, 2078, 871, 1550, 105, 422, 587, 177, 3094, 3038, 2869, 1574, 
    1653, 3083, 778, 1159, 3182, 2552, 1483, 2727, 1119, 1739, 644, 2457, 349, 
    418, 329, 3173, 3254, 817, 1097, 603, 610, 1322, 2044, 1864, 384, 2114, 3193, 
    1218, 1994, 2455, 220, 2142, 1670, 2144, 1799, 2051, 794, 1819, 2475, 2459, 
    478, 3221, 3021, 996, 991, 958, 1869, 1522, 1628};

const int16_t zetas_inv[128] = {
    1701, 1807, 1460, 2371, 2338, 2333, 308, 108, 2851, 870, 854, 1510, 2535, 
    1278, 1530, 1185, 1659, 1187, 3109, 874, 1335, 2111, 136, 1215, 2945, 1465, 
    1285, 2007, 2719, 2726, 2232, 2512, 75, 156, 3000, 2911, 2980, 872, 2685, 
    1590, 2210, 602, 1846, 777, 147, 2170, 2551, 246, 1676, 1755, 460, 291, 235, 
    3152, 2742, 2907, 3224, 1779, 2458, 1251, 2486, 2774, 2899, 1103, 1275, 2652, 
    1065, 2881, 725, 1508, 2368, 398, 951, 247, 1421, 3222, 2499, 271, 90, 853, 
    1860, 3203, 1162, 1618, 666, 320, 8, 2813, 1544, 282, 1838, 1293, 2314, 552, 
    2677, 2106, 1571, 205, 2918, 1542, 2721, 2597, 2312, 681, 130, 1602, 1871, 829, 
    2946, 3065, 1325, 2756, 1861, 1474, 1202, 2367, 3147, 1752, 2707, 171, 3127, 3042, 
    1907, 1836, 1517, 359, 758, 1441};

#endif

/*************************************************
* Name:        fqmul
*
* Description: Multiplication followed by Montgomery reduction
*
* Arguments:   - int16_t a: first factor
*              - int16_t b: second factor
*
* Returns 16-bit integer congruent to a*b*R^{-1} mod q
**************************************************/
static int16_t fqmul(int16_t a, int16_t b) {
    return montgomery_reduce((int32_t)a*b);
}

/*************************************************
* Name:        ntt
*
* Description: Inplace number-theoretic transform (NTT) in Rq
*              input is in standard order, output is in bitreversed order
*
* Arguments:   - int16_t r[N]: pointer to input/output vector of elements
*                                of Zq
**************************************************/
void ntt(int16_t r[N]) {
    unsigned int len, start, j, k;
    int16_t zeta, t;

#if KEM_MODE == 128
    k = 1;
    for (len = N/2; len >= 4; len >>= 1) {
        for (start = 0; start < N; start = j + len) {
            zeta = zetas[k++];
            for (j = start; j < start + len; ++j) {
                t = fqmul(zeta, r[j + len]);
                r[j + len] = r[j] - t;
                r[j] = r[j] + t;
            }
        }
    }
#elif KEM_MODE == 256
    k = 1;
    for (len = N/2; len >= 8; len >>= 1) {
        for (start = 0; start < N; start = j + len) {
            zeta = zetas[k++];
            for (j = start; j < start + len; ++j) {
                t = fqmul(zeta, r[j + len]);
                r[j + len] = r[j] - t;
                r[j] = r[j] + t;
            }
        }
    }
#elif KEM_MODE == 512
    k = 1;
    for (len = N/2; len >= 16; len >>= 1) {
        for (start = 0; start < N; start = j + len) {
            zeta = zetas[k++];
            for (j = start; j < start + len; ++j) {
                t = fqmul(zeta, r[j + len]);
                r[j + len] = r[j] - t;
                r[j] = r[j] + t;
            }
        }
    }
#else
#error "Invalid KEM_MODE"
#endif
}

/*************************************************
* Name:        invntt_tomont
*
* Description: Inplace inverse number-theoretic transform (NTT) in Rq
*              input is bitreversed order, output is in standard order
*              
* Arguments:   - int16_t r[N]: pointer to input/output vector of elements
*                                of Zq
 **************************************************/
void invntt_tomont(int16_t r[N]) {
    unsigned int start, len, j, k;
    int16_t t, zeta;

#if KEM_MODE == 128
    k = 0;
    for (len = 4; len <= N/2; len <<= 1) {
        for (start = 0; start < N; start = j +len) {
            zeta = zetas_inv[k++];
            for (j = start; j < start + len; ++j) {
                t = r[j];
                r[j] = freeze(t + r[j + len]);
                r[j + len] = t - r[j + len];
                r[j + len] = fqmul(zeta, r[j + len]);
            }
        }
    }
    
    for (j = 0; j < N; ++j) {
        r[j] = fqmul(r[j], zetas_inv[N/4 - 1]);
    }
#elif KEM_MODE == 256
    k = 0;
    for (len = 8; len <= N/2; len <<= 1) {
        for (start = 0; start < N; start = j +len) {
            zeta = zetas_inv[k++];
            for (j = start; j < start + len; ++j) {
                t = r[j];
                r[j] = freeze(t + r[j + len]);
                r[j + len] = t - r[j + len];
                r[j + len] = fqmul(zeta, r[j + len]);
            }
        }
    }
    
    for (j = 0; j < N; ++j) {
        r[j] = fqmul(r[j], zetas_inv[N/8 - 1]);
    }
#elif KEM_MODE == 512
    k = 0;
    for (len = 16; len <= N/2; len <<= 1) {
        for (start = 0; start < N; start = j +len) {
            zeta = zetas_inv[k++];
            for (j = start; j < start + len; ++j) {
                t = r[j];
                r[j] = freeze(t + r[j + len]);
                r[j + len] = t - r[j + len];
                r[j + len] = fqmul(zeta, r[j + len]);
            }
        }
    }
    
    for (j = 0; j < N; ++j) {
        r[j] = fqmul(r[j], zetas_inv[N/16 - 1]);
    }
#else
#error "Invalid KEM_MODE"
#endif
}

/*************************************************
* Name:        fqinv
*
* Description: Inversion
*
* Arguments:   - int16_t a: first factor a = x * R mod q
*
* Returns 16-bit integer congruent to x^{-1} * R mod q
**************************************************/
int16_t fqinv(int16_t a)
{
#if KEM_MODE == 128 || KEM_MODE == 256
    a = freeze(a);
    return fqinv_table[a];
#elif KEM_MODE == 512
    int16_t t1;
    // q - 2 = 3327 = 110011111111
    t1 = a;                    // 1
    t1 = fqmul(t1, t1);        // 10
    t1 = fqmul(t1, a);         // 11
    t1 = fqmul(t1, t1);        // 110
    t1 = fqmul(t1, t1);        // 1100
    t1 = fqmul(t1, t1);        // 11000 
    t1 = fqmul(t1, a);         // 11001
    t1 = fqmul(t1, t1);        // 110010
    t1 = fqmul(t1, a);         // 110011
    t1 = fqmul(t1, t1);        // 1100110
    t1 = fqmul(t1, a);         // 1100111
    t1 = fqmul(t1, t1);        // 11001110
    t1 = fqmul(t1, a);         // 11001111
    t1 = fqmul(t1, t1);        // 110011110
    t1 = fqmul(t1, a);         // 110011111
    t1 = fqmul(t1, t1);        // 1100111110
    t1 = fqmul(t1, a);         // 1100111111
    t1 = fqmul(t1, t1);        // 11001111110
    t1 = fqmul(t1, a);         // 11001111111
    t1 = fqmul(t1, t1);        // 110011111110
    t1 = fqmul(t1, a);         // 110011111111
    return t1;
#else
#error "Invalid KEM_MODE"
#endif
}

#if KEM_MODE != 512
/*************************************************
* Name:        basemul_K
*
* Description: Recursive Karatsuba multiplication of 2*half-element polynomials.
*              half=2 is the base case (4 elements).
*
* Arguments:   - int half: half-size (2, 4, 8, or 16)
*              - int16_t *r: output array (2*half elements)
*              - const int16_t *a, *b: input arrays
*              - int16_t zeta: twiddle factor
**************************************************/
static void basemul_K(int half, int16_t *r, const int16_t *a, const int16_t *b, int16_t zeta)
{
    if (half == 2) {
        int16_t lo[3], hi[3], mid[3];
        int16_t sa0 = a[0] + a[2], sa1 = a[1] + a[3];
        int16_t sb0 = b[0] + b[2], sb1 = b[1] + b[3];

        lo[0]  = fqmul(a[0], b[0]);
        lo[2]  = fqmul(a[1], b[1]);
        lo[1]  = fqmul(a[0] + a[1], b[0] + b[1]) - lo[0] - lo[2];

        hi[0]  = fqmul(a[2], b[2]);
        hi[2]  = fqmul(a[3], b[3]);
        hi[1]  = fqmul(a[2] + a[3], b[2] + b[3]) - hi[0] - hi[2];

        mid[0] = fqmul(sa0, sb0);
        mid[2] = fqmul(sa1, sb1);
        mid[1] = fqmul(sa0 + sa1, sb0 + sb1) - mid[0] - mid[2];

        r[0] = freeze(lo[0] + fqmul(zeta, hi[0] + mid[2] - lo[2] - hi[2]));
        r[1] = freeze(lo[1] + fqmul(zeta, hi[1]));
        r[2] = freeze(lo[2] + fqmul(zeta, hi[2]) + mid[0] - lo[0] - hi[0]);
        r[3] = freeze(mid[1] - lo[1] - hi[1]);
    } else {
        int sub_half = half / 2;
        int16_t ea[half], oa[half], eb[half], ob[half];
        int16_t lo[half], hi[half], mid[half], cross[half];
        int16_t sa[half], sb[half], yhi[half];

        for (int i = 0; i < half; i++) {
            ea[i] = a[2*i];      oa[i] = a[2*i + 1];
            eb[i] = b[2*i];      ob[i] = b[2*i + 1];
        }

        basemul_K(sub_half, lo, ea, eb, zeta);
        basemul_K(sub_half, hi, oa, ob, zeta);

        for (int i = 0; i < half; i++) {
            sa[i] = freeze_centered(ea[i] + oa[i]);
            sb[i] = freeze_centered(eb[i] + ob[i]);
        }
        basemul_K(sub_half, mid, sa, sb, zeta);

        for (int i = 0; i < half; i++)
            cross[i] = mid[i] - lo[i] - hi[i];

        yhi[0] = fqmul(zeta, hi[half - 1]);
        for (int i = 1; i < half; i++)
            yhi[i] = hi[i - 1];

        for (int i = 0; i < half; i++) {
            r[2*i]     = freeze(lo[i] + yhi[i]);
            r[2*i + 1] = freeze(cross[i]);
        }
    }
}

/*************************************************
* Name:        baseinv_K
*
* Description: Recursive Karatsuba inversion of 2*half-element polynomials.
*              half=2 is the base case (4 elements).
*
* Arguments:   - int half: half-size (2, 4, 8, or 16)
*              - int16_t *b: output array (2*half elements)
*              - const int16_t *a: input array
*              - int16_t zeta: twiddle factor
* Returns:     0 on success, non-zero on failure
**************************************************/
static int baseinv_K(int half, int16_t *b, const int16_t *a, int16_t zeta)
{
    if (half == 2) {
        int r1;
        int32_t t0, t1, t2;
        uint32_t x;

        t0 = fqmul(a[2], a[2]) - fqmul(2 * a[1], a[3]);
        t0 = fqmul(a[0], a[0]) + fqmul(t0, zeta);
        t1 = fqmul(a[3], a[3]);
        t1 = fqmul(2 * a[0], a[2]) - fqmul(a[1], a[1]) - fqmul(t1, zeta);

        t2 = fqmul(t1, t1);
        t2 = fqmul(t0, t0) - fqmul(t2, zeta);

        t2 = fqinv(t2);

        x  = (uint32_t)t2;
        r1 = (-(uint64_t)x) >> 63;

        t0 = montgomery_reduce(fqmul(t0, t2));
        t1 = montgomery_reduce(fqmul(t1, t2));
        t0 = montgomery_reduce(t0);
        t1 = montgomery_reduce(t1);

        t2 = fqmul(t1, zeta);

        b[0] =  fqmul(a[0], t0) - fqmul(a[2], t2);
        b[1] = -fqmul(a[1], t0) + fqmul(a[3], t2);
        b[2] =  fqmul(a[2], t0) - fqmul(a[0], t1);
        b[3] = -fqmul(a[3], t0) + fqmul(a[1], t1);

        return r1 - 1;
    } else {
        int sub_half = half / 2;
        int16_t e[half], o[half];
        int16_t e2[half], o2[half], yo2[half];
        int16_t norm[half], ninv[half];
        int16_t p[half], q[half];
        int ret;

        for (int i = 0; i < half; i++) {
            e[i] = a[2*i];
            o[i] = a[2*i + 1];
        }

        basemul_K(sub_half, e2, e, e, zeta);
        basemul_K(sub_half, o2, o, o, zeta);

        yo2[0] = fqmul(zeta, o2[half - 1]);
        for (int i = 1; i < half; i++)
            yo2[i] = o2[i - 1];

        for (int i = 0; i < half; i++)
            norm[i] = freeze(e2[i] - yo2[i]);

        ret = baseinv_K(sub_half, ninv, norm, zeta);

        basemul_K(sub_half, p, e, ninv, zeta);
        basemul_K(sub_half, q, o, ninv, zeta);

        for (int i = 0; i < half; i++) {
            b[2*i]     =  p[i];
            b[2*i + 1] = -q[i];
        }

        return ret;
    }
}
#endif /* KEM_MODE != 512 */

#if KEM_MODE == 512
static void basemul4(int16_t r[4],
                     const int16_t a[4],
                     const int16_t b[4],
                     int16_t zeta)
{
    int16_t af[4], bf[4];
    unsigned int i;
    for (i = 0; i < 4; i++) {
        af[i] = freeze_centered(a[i]);
        bf[i] = freeze_centered(b[i]);
    }

    int16_t lo[3], hi[3], mid[3];
    int16_t sa0 = af[0] + af[2], sa1 = af[1] + af[3];
    int16_t sb0 = bf[0] + bf[2], sb1 = bf[1] + bf[3];

    lo[0]  = fqmul(af[0], bf[0]);
    lo[2]  = fqmul(af[1], bf[1]);
    lo[1]  = fqmul(af[0] + af[1], bf[0] + bf[1]) - lo[0] - lo[2];

    hi[0]  = fqmul(af[2], bf[2]);
    hi[2]  = fqmul(af[3], bf[3]);
    hi[1]  = fqmul(af[2] + af[3], bf[2] + bf[3]) - hi[0] - hi[2];

    mid[0] = fqmul(sa0, sb0);
    mid[2] = fqmul(sa1, sb1);
    mid[1] = fqmul(sa0 + sa1, sb0 + sb1) - mid[0] - mid[2];

    r[0] = freeze(lo[0] + fqmul(zeta, hi[0] + mid[2] - lo[2] - hi[2]));
    r[1] = freeze(lo[1] + fqmul(zeta, hi[1]));
    r[2] = freeze(lo[2] + fqmul(zeta, hi[2]) + mid[0] - lo[0] - hi[0]);
    r[3] = freeze(mid[1] - lo[1] - hi[1]);
}

static int baseinv4(int16_t b[4], const int16_t a[4], int16_t zeta)
{
    int r1;
    int32_t t0, t1, t2;
    uint32_t x;

    t0 = fqmul(a[2], a[2]) - fqmul(2 * a[1], a[3]);
    t0 = fqmul(a[0], a[0]) + fqmul(t0, zeta);
    t1 = fqmul(a[3], a[3]);
    t1 = fqmul(2 * a[0], a[2]) - fqmul(a[1], a[1]) - fqmul(t1, zeta);

    t2 = fqmul(t1, t1);
    t2 = fqmul(t0, t0) - fqmul(t2, zeta);

    t2 = fqinv(t2);

    x  = (uint32_t)t2;
    r1 = (-(uint64_t)x) >> 63;

    t0 = montgomery_reduce(fqmul(t0, t2));
    t1 = montgomery_reduce(fqmul(t1, t2));
    t0 = montgomery_reduce(t0);
    t1 = montgomery_reduce(t1);

    t2 = fqmul(t1, zeta);

    b[0] =  fqmul(a[0], t0) - fqmul(a[2], t2);
    b[1] = -fqmul(a[1], t0) + fqmul(a[3], t2);
    b[2] =  fqmul(a[2], t0) - fqmul(a[0], t1);
    b[3] = -fqmul(a[3], t0) + fqmul(a[1], t1);

    return r1 - 1;
}

static void basemul8(int16_t r[8],
                     const int16_t a[8],
                     const int16_t b[8],
                     int16_t zeta)
{
    int16_t ea[4], oa[4], eb[4], ob[4];
    int16_t lo[4], hi[4], mid[4], cross[4];
    int16_t sa[4], sb[4], yhi[4];
    unsigned int i;

    for (i = 0; i < 4; i++) {
        ea[i] = a[2*i];      oa[i] = a[2*i + 1];
        eb[i] = b[2*i];      ob[i] = b[2*i + 1];
    }

    basemul4(lo,  ea, eb, zeta);
    basemul4(hi,  oa, ob, zeta);

    for (i = 0; i < 4; i++) {
        sa[i] = freeze_centered(ea[i] + oa[i]);
        sb[i] = freeze_centered(eb[i] + ob[i]);
    }
    basemul4(mid, sa, sb, zeta);

    for (i = 0; i < 4; i++)
        cross[i] = mid[i] - lo[i] - hi[i];

    yhi[0] = fqmul(zeta, hi[3]);
    yhi[1] = hi[0];
    yhi[2] = hi[1];
    yhi[3] = hi[2];

    for (i = 0; i < 4; i++) {
        r[2*i]     = freeze(lo[i] + yhi[i]);
        r[2*i + 1] = freeze(cross[i]);
    }
}

static int baseinv8(int16_t b[8], const int16_t a[8], int16_t zeta)
{
    int16_t e[4], o[4];
    int16_t e2[4], o2[4], yo2[4];
    int16_t norm[4], ninv[4];
    int16_t p[4], q[4];
    int ret;
    unsigned int i;

    for (i = 0; i < 4; i++) {
        e[i] = a[2*i];
        o[i] = a[2*i + 1];
    }

    basemul4(e2, e, e, zeta);
    basemul4(o2, o, o, zeta);

    yo2[0] = fqmul(zeta, o2[3]);
    yo2[1] = o2[0];
    yo2[2] = o2[1];
    yo2[3] = o2[2];

    for (i = 0; i < 4; i++)
        norm[i] = freeze(e2[i] - yo2[i]);

    ret = baseinv4(ninv, norm, zeta);

    basemul4(p, e, ninv, zeta);
    basemul4(q, o, ninv, zeta);

    for (i = 0; i < 4; i++) {
        b[2*i]     =  p[i];
        b[2*i + 1] = -q[i];
    }
    return ret;
}

static void basemul16(int16_t r[16],
                      const int16_t a[16],
                      const int16_t b[16],
                      int16_t zeta)
{
    int16_t ea[8], oa[8], eb[8], ob[8];
    int16_t lo[8], hi[8], mid[8], cross[8];
    int16_t sa[8], sb[8], yhi[8];
    unsigned int i;

    for (i = 0; i < 8; i++) {
        ea[i] = a[2*i];      oa[i] = a[2*i + 1];
        eb[i] = b[2*i];      ob[i] = b[2*i + 1];
    }

    basemul8(lo,  ea, eb, zeta);
    basemul8(hi,  oa, ob, zeta);

    for (i = 0; i < 8; i++) {
        sa[i] = freeze_centered(ea[i] + oa[i]);
        sb[i] = freeze_centered(eb[i] + ob[i]);
    }
    basemul8(mid, sa, sb, zeta);

    for (i = 0; i < 8; i++)
        cross[i] = mid[i] - lo[i] - hi[i];

    yhi[0] = fqmul(zeta, hi[7]);
    for (i = 1; i < 8; i++)
        yhi[i] = hi[i - 1];

    for (i = 0; i < 8; i++) {
        r[2*i]     = freeze(lo[i] + yhi[i]);
        r[2*i + 1] = freeze(cross[i]);
    }
}

static int baseinv16(int16_t b[16], const int16_t a[16], int16_t zeta)
{
    int16_t e[8], o[8];
    int16_t e2[8], o2[8], yo2[8];
    int16_t norm[8], ninv[8];
    int16_t p[8], q[8];
    int ret;
    unsigned int i;

    for (i = 0; i < 8; i++) {
        e[i] = a[2*i];
        o[i] = a[2*i + 1];
    }

    basemul8(e2, e, e, zeta);
    basemul8(o2, o, o, zeta);

    yo2[0] = fqmul(zeta, o2[7]);
    for (i = 1; i < 8; i++)
        yo2[i] = o2[i - 1];

    for (i = 0; i < 8; i++)
        norm[i] = freeze(e2[i] - yo2[i]);

    ret = baseinv8(ninv, norm, zeta);

    basemul8(p, e, ninv, zeta);
    basemul8(q, o, ninv, zeta);

    for (i = 0; i < 8; i++) {
        b[2*i]     =  p[i];
        b[2*i + 1] = -q[i];
    }
    return ret;
}
#endif

#if KEM_MODE == 128
void basemul(int16_t r[4],
             const int16_t a[4],
             const int16_t b[4],
             int16_t zeta)
{
    basemul_K(2, r, a, b, zeta);
}

int baseinv(int16_t b[4], const int16_t a[4], int16_t zeta)
{
    return baseinv_K(2, b, a, zeta);
}
#elif KEM_MODE == 256
void basemul(int16_t r[8],
             const int16_t a[8],
             const int16_t b[8],
             int16_t zeta)
{
    basemul_K(4, r, a, b, zeta);
}

int baseinv(int16_t b[8], const int16_t a[8], int16_t zeta)
{
    return baseinv_K(4, b, a, zeta);
}
#elif KEM_MODE == 512
void basemul(int16_t r[16],
             const int16_t a[16],
             const int16_t b[16],
             int16_t zeta)
{
    basemul16(r, a, b, zeta);
}

int baseinv(int16_t b[16], const int16_t a[16], int16_t zeta)
{
    return baseinv16(b, a, zeta);
}
#else
#error "Invalid KEM_MODE"
#endif