/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares the ZEN key-encapsulation mechanism layer for the optimized ZEN-512 instance.
*/
#ifndef NTT_H
#define NTT_H

#include <stdint.h>
#include "params.h"
#define QINV -767
#define MONT 171

#ifdef __cplusplus
extern "C"
{
#endif

    static const int16_t f[128] =
    {
        171, 605, 688, 361, 186, 766, 519, 649, 461, 129, 753, 546, 407, 626, 131, 432, 
        693, 671, 36, 694, 430, 514, 282, 566, 735, 199, 178, 270, 759, 149, 369, 577, 
        147, 655, 497, 54, 767, 645, 689, 423, 86, 718, 364, 267, 161, 754, 288, 169, 
        191, 307, 719, 745, 599, 226, 121, 581, 389, 279, 180, 394, 612, 263, 641, 523, 
        746, 112, 618, 635, 717, 621, 227, 232, 698, 212, 236, 21, 341, 379, 567, 549, 
        352, 292, 238, 145, 194, 493, 70, 495, 117, 333, 66, 247, 532, 686, 517, 525, 
        331, 528, 167, 357, 414, 291, 411, 105, 654, 560, 14, 99, 509, 29, 366, 391, 
        451, 278, 353, 354, 585, 127, 330, 466, 222, 691, 421, 725, 201, 158, 350, 168
    };

    static const int16_t fn[128] = 
    {
        601, 419, 611, 568, 44, 348, 78, 547, 303, 439, 642, 184, 415, 416, 491, 318, 
        378, 403, 740, 260, 670, 755, 209, 115, 664, 358, 478, 355, 412, 602, 241, 438, 
        244, 252, 83, 237, 522, 703, 436, 652, 274, 699, 276, 575, 624, 531, 477, 417, 
        220, 202, 390, 428, 748, 533, 557, 71, 537, 542, 148, 52, 134, 151, 657, 23, 
        246, 128, 506, 157, 375, 589, 490, 380, 188, 648, 543, 170, 24, 50, 462, 578, 
        600, 481, 15, 608, 502, 405, 51, 683, 346, 80, 124, 2, 715, 272, 114, 622, 
        192, 400, 620, 10, 499, 591, 570, 34, 203, 487, 255, 339, 75, 733, 98, 76, 
        337, 638, 143, 362, 223, 16, 640, 308, 120, 250, 3, 583, 408, 81, 164, 655
    };

    static const int16_t qinv[ZEN_Q] = 
    {
        0, 1, 385, 513, 577, 154, 641, 110, 673, 171, 77, 70, 705, 355, 55, 564, 
        721, 181, 470, 81, 423, 293, 35, 535, 737, 646, 562, 57, 412, 716, 282, 645, 
        745, 536, 475, 22, 235, 291, 425, 631, 596, 694, 531, 465, 402, 188, 652, 180, 
        753, 565, 323, 573, 281, 740, 413, 14, 206, 27, 358, 378, 141, 353, 707, 354, 
        757, 71, 268, 264, 622, 691, 11, 65, 502, 611, 530, 728, 597, 10, 700, 623, 
        298, 19, 347, 454, 650, 190, 617, 495, 201, 553, 94, 600, 326, 215, 90, 170, 
        761, 111, 667, 435, 546, 434, 671, 112, 525, 520, 370, 115, 591, 127, 7, 97, 
        103, 245, 398, 107, 179, 723, 189, 685, 455, 286, 561, 744, 738, 283, 177, 109, 
        763, 155, 420, 317, 134, 451, 132, 319, 311, 595, 730, 426, 390, 60, 417, 242, 
        251, 297, 690, 701, 265, 160, 364, 494, 683, 191, 5, 129, 350, 529, 696, 503, 
        149, 406, 394, 184, 558, 261, 227, 571, 325, 678, 95, 9, 693, 729, 632, 312, 
        485, 126, 661, 116, 47, 17, 300, 374, 163, 212, 492, 366, 45, 118, 85, 153, 
        765, 514, 440, 280, 718, 324, 602, 228, 273, 88, 217, 322, 720, 754, 56, 743, 
        647, 287, 260, 605, 185, 278, 442, 93, 680, 202, 448, 460, 388, 428, 433, 669, 
        436, 499, 507, 166, 199, 497, 438, 516, 474, 736, 746, 36, 479, 464, 727, 695, 
        612, 351, 143, 519, 665, 113, 372, 302, 369, 664, 526, 144, 473, 538, 439, 576, 
        766, 386, 462, 481, 210, 165, 543, 500, 67, 148, 610, 697, 66, 506, 544, 437, 
        540, 200, 682, 618, 365, 583, 213, 328, 195, 52, 30, 125, 593, 313, 121, 209, 
        510, 463, 533, 37, 345, 21, 735, 537, 517, 145, 80, 751, 182, 396, 247, 401, 
        726, 532, 480, 511, 387, 550, 449, 136, 175, 285, 649, 686, 348, 131, 636, 135, 
        459, 551, 203, 50, 197, 168, 92, 555, 279, 575, 515, 539, 498, 545, 670, 668, 
        547, 429, 339, 338, 432, 548, 389, 630, 731, 292, 749, 82, 316, 639, 156, 241, 
        627, 61, 63, 13, 715, 741, 58, 392, 408, 409, 393, 608, 150, 276, 187, 725, 
        466, 248, 106, 655, 246, 468, 183, 607, 407, 410, 59, 629, 427, 549, 461, 512, 
        767, 2, 257, 308, 220, 342, 140, 710, 359, 362, 162, 586, 301, 523, 114, 663, 
        521, 303, 44, 582, 493, 619, 161, 376, 360, 361, 377, 711, 28, 54, 756, 706, 
        708, 142, 528, 613, 130, 453, 687, 20, 477, 38, 139, 380, 221, 337, 431, 430, 
        340, 222, 101, 99, 224, 271, 230, 254, 194, 490, 214, 677, 601, 572, 719, 566, 
        218, 310, 634, 133, 638, 421, 83, 120, 484, 594, 633, 320, 219, 382, 258, 289, 
        237, 43, 368, 522, 373, 587, 18, 689, 624, 252, 232, 34, 748, 424, 732, 236, 
        306, 259, 560, 648, 456, 176, 644, 739, 717, 574, 441, 556, 186, 404, 151, 87, 
        569, 229, 332, 225, 263, 703, 72, 159, 621, 702, 269, 226, 604, 559, 288, 307, 
        383, 3, 193, 330, 231, 296, 625, 243, 105, 400, 467, 397, 656, 104, 250, 626, 
        418, 157, 74, 42, 305, 290, 733, 23, 33, 295, 253, 331, 272, 570, 603, 262, 
        270, 333, 100, 336, 341, 381, 309, 321, 567, 89, 676, 327, 491, 584, 164, 509, 
        482, 122, 26, 713, 15, 49, 447, 552, 681, 496, 541, 167, 445, 51, 489, 329, 
        255, 4, 616, 684, 651, 724, 403, 277, 557, 606, 395, 469, 752, 722, 653, 108, 
        643, 284, 457, 137, 40, 76, 760, 674, 91, 444, 198, 542, 508, 211, 585, 375, 
        363, 620, 266, 73, 240, 419, 640, 764, 578, 86, 275, 405, 609, 504, 68, 79, 
        472, 518, 527, 352, 709, 379, 343, 39, 174, 458, 450, 637, 318, 635, 452, 349, 
        614, 6, 660, 592, 486, 31, 25, 208, 483, 314, 84, 580, 46, 590, 662, 371, 
        524, 666, 672, 762, 642, 178, 654, 399, 249, 244, 657, 98, 335, 223, 334, 102, 
        658, 8, 599, 679, 554, 443, 169, 675, 216, 568, 274, 152, 579, 119, 315, 422, 
        750, 471, 146, 69, 759, 172, 41, 239, 158, 267, 704, 758, 78, 147, 505, 501, 
        698, 12, 415, 62, 416, 628, 391, 411, 742, 563, 755, 356, 29, 488, 196, 446, 
        204, 16, 589, 117, 581, 367, 304, 238, 75, 173, 138, 344, 478, 534, 747, 294, 
        233, 24, 124, 487, 53, 357, 712, 207, 123, 32, 234, 734, 476, 346, 688, 299, 
        588, 48, 205, 714, 414, 64, 699, 692, 598, 96, 659, 128, 615, 192, 256, 384, 
        768
    };

    /// @brief Reduce a 32-bit integer modulo ZEN_Q using Montgomery reduction
    /// @param[in] a Input 32-bit integer to be reduced
    /// @return 16-bit integer reduced modulo ZEN_Q in Montgomery domain
    int16_t montgomery_reduce(int32_t a);

    /// @brief Compute the forward Number Theoretic Transform (NTT) of a polynomial
    /// @param[in,out] a Base address of input polynomial coefficient array; replaced by its NTT representation
    /// @return None
    void poly_ntt(int16_t *a);

    /// @brief Compute the forward Number Theoretic Transform (NTT) of a polynomial and map coefficients into the standard range modulo ZEN_Q
    /// @param[in,out] a Base address of input polynomial coefficient array; replaced by its NTT representation
    /// @return None
    void poly_ntt_mq(int16_t *a);

    /// @brief Compute the inverse Number Theoretic Transform (INTT) of a polynomial
    /// @param[in,out] a Base address of input polynomial coefficient array in NTT domain; replaced by the polynomial in coefficient domain
    /// @return None    
    void poly_intt(int16_t *a);

    /// @brief Perform block-wise polynomial multiplication in the NTT domain
    /// @param[out] r Base address of output polynomial coefficient array
    /// @param[in] a Base address of first input polynomial coefficient array in NTT domain
    /// @param[in] b Base address of second input polynomial coefficient array in NTT domain
    /// @return None
    void poly_basemul_ntt(int16_t *r, int16_t *a, int16_t *b);

    /// @brief Perform block-wise polynomial multiplication in the NTT domain and reduce coefficients modulo ZEN_Q
    /// @param[out] r Base address of output polynomial coefficient array
    /// @param[in] a Base address of first input polynomial coefficient array in NTT domain
    /// @param[in] b Base address of second input polynomial coefficient array in NTT domain
    /// @return None
    void poly_basemul_ntt_mq(int16_t *r,  int16_t *a,  int16_t *b);

    /// @brief Perform block-wise inversion of a polynomial in the NTT domain
    /// @param[out] r Base address of output polynomial coefficient array
    /// @param[in] a Base address of input polynomial coefficient array in NTT domain
    /// @return None
    void poly_baseinv_ntt(int16_t *r, int16_t *a);

#ifdef __cplusplus
}
#endif
#endif
