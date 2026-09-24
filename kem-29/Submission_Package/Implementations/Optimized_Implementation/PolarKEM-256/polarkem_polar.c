/**
 * @file polarkem_polar.c
 * @brief Portable word-sliced implementation of the Polar transform.
 */
#include "polarkem_polar.h"

#include <stddef.h>
#include <string.h>

/*
 * Exact BEC(epsilon=1/2) information set.  Reliability values are compared as
 * exact rationals, the first 256 ranks are selected, and selected indices are
 * stored in numerical order.  This table is part of the serialized profile.
 */
static const uint16_t polarkem_info_positions[POLARKEM_MU_BITS] = {
    255U, 383U, 439U, 443U, 445U, 446U, 447U, 463U,
    471U, 475U, 477U, 478U, 479U, 487U, 491U, 492U,
    493U, 494U, 495U, 498U, 499U, 500U, 501U, 502U,
    503U, 504U, 505U, 506U, 507U, 508U, 509U, 510U,
    511U, 623U, 631U, 635U, 637U, 638U, 639U, 671U,
    687U, 695U, 698U, 699U, 700U, 701U, 702U, 703U,
    717U, 718U, 719U, 723U, 725U, 726U, 727U, 729U,
    730U, 731U, 732U, 733U, 734U, 735U, 739U, 741U,
    742U, 743U, 745U, 746U, 747U, 748U, 749U, 750U,
    751U, 753U, 754U, 755U, 756U, 757U, 758U, 759U,
    760U, 761U, 762U, 763U, 764U, 765U, 766U, 767U,
    799U, 811U, 813U, 814U, 815U, 819U, 821U, 822U,
    823U, 825U, 826U, 827U, 828U, 829U, 830U, 831U,
    839U, 843U, 845U, 846U, 847U, 851U, 853U, 854U,
    855U, 857U, 858U, 859U, 860U, 861U, 862U, 863U,
    867U, 869U, 870U, 871U, 873U, 874U, 875U, 876U,
    877U, 878U, 879U, 881U, 882U, 883U, 884U, 885U,
    886U, 887U, 888U, 889U, 890U, 891U, 892U, 893U,
    894U, 895U, 903U, 907U, 909U, 910U, 911U, 915U,
    917U, 918U, 919U, 920U, 921U, 922U, 923U, 924U,
    925U, 926U, 927U, 930U, 931U, 932U, 933U, 934U,
    935U, 936U, 937U, 938U, 939U, 940U, 941U, 942U,
    943U, 944U, 945U, 946U, 947U, 948U, 949U, 950U,
    951U, 952U, 953U, 954U, 955U, 956U, 957U, 958U,
    959U, 961U, 962U, 963U, 964U, 965U, 966U, 967U,
    968U, 969U, 970U, 971U, 972U, 973U, 974U, 975U,
    976U, 977U, 978U, 979U, 980U, 981U, 982U, 983U,
    984U, 985U, 986U, 987U, 988U, 989U, 990U, 991U,
    992U, 993U, 994U, 995U, 996U, 997U, 998U, 999U,
    1000U, 1001U, 1002U, 1003U, 1004U, 1005U, 1006U, 1007U,
    1008U, 1009U, 1010U, 1011U, 1012U, 1013U, 1014U, 1015U,
    1016U, 1017U, 1018U, 1019U, 1020U, 1021U, 1022U, 1023U
};

void polarkem_polar_transform(uint64_t words[POLARKEM_WORDS])
{
    size_t base;
    size_t j;
    size_t span_words;
    size_t w;

    /* Six butterfly layers contained entirely within each 64-bit word. */
    for (w = 0U; w < POLARKEM_WORDS; ++w) {
        uint64_t x = words[w];
        x ^= (x >> 1) & UINT64_C(0x5555555555555555);
        x ^= (x >> 2) & UINT64_C(0x3333333333333333);
        x ^= (x >> 4) & UINT64_C(0x0f0f0f0f0f0f0f0f);
        x ^= (x >> 8) & UINT64_C(0x00ff00ff00ff00ff);
        x ^= (x >> 16) & UINT64_C(0x0000ffff0000ffff);
        x ^= (x >> 32) & UINT64_C(0x00000000ffffffff);
        words[w] = x;
    }

    /* Remaining layers combine cache-adjacent groups of whole words. */
    for (span_words = 1U; span_words < POLARKEM_WORDS;
         span_words <<= 1U) {
        for (base = 0U; base < POLARKEM_WORDS;
             base += 2U * span_words) {
            for (j = 0U; j < span_words; ++j) {
                words[base + j] ^= words[base + span_words + j];
            }
        }
    }
}

void polarkem_polar_encode(uint64_t codeword[POLARKEM_WORDS],
                           const unsigned char mu[POLARKEM_MU_BYTES])
{
    size_t j;

    memset(codeword, 0, POLARKEM_WORDS * sizeof(codeword[0]));
    for (j = 0U; j < POLARKEM_MU_BITS; ++j) {
        const uint16_t position = polarkem_info_positions[j];
        const uint64_t bit = (uint64_t)((mu[j >> 3U] >> (j & 7U)) & 1U);
        codeword[position >> 6U] |= bit << (position & 63U);
    }
    polarkem_polar_transform(codeword);
}

void polarkem_polar_decode(unsigned char mu[POLARKEM_MU_BYTES],
                           const uint64_t codeword[POLARKEM_WORDS])
{
    uint64_t information[POLARKEM_WORDS];
    size_t j;

    memcpy(information, codeword, sizeof(information));
    polarkem_polar_transform(information);
    memset(mu, 0, POLARKEM_MU_BYTES);
    for (j = 0U; j < POLARKEM_MU_BITS; ++j) {
        const uint16_t position = polarkem_info_positions[j];
        const unsigned char bit = (unsigned char)(
            (information[position >> 6U] >> (position & 63U)) & UINT64_C(1));
        mu[j >> 3U] |= (unsigned char)(bit << (j & 7U));
    }
}
