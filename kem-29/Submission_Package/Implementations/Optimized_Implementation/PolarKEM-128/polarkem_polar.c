/**
 * @file polarkem_polar.c
 * @brief Portable word-sliced implementation of the Polar transform.
 */
#include "polarkem_polar.h"

#include <stddef.h>
#include <string.h>

/*
 * Exact BEC(epsilon=1/2) information set.  Reliability values are compared as
 * exact rationals, the first 128 ranks are selected, and selected indices are
 * stored in numerical order.  This table is part of the serialized profile.
 */
static const uint16_t polarkem_info_positions[POLARKEM_MU_BITS] = {
    191U, 223U, 237U, 238U, 239U, 243U, 245U, 246U,
    247U, 249U, 250U, 251U, 252U, 253U, 254U, 255U,
    317U, 318U, 319U, 343U, 347U, 349U, 350U, 351U,
    359U, 363U, 365U, 366U, 367U, 371U, 373U, 374U,
    375U, 376U, 377U, 378U, 379U, 380U, 381U, 382U,
    383U, 399U, 407U, 411U, 413U, 414U, 415U, 422U,
    423U, 425U, 426U, 427U, 428U, 429U, 430U, 431U,
    433U, 434U, 435U, 436U, 437U, 438U, 439U, 440U,
    441U, 442U, 443U, 444U, 445U, 446U, 447U, 451U,
    453U, 454U, 455U, 457U, 458U, 459U, 460U, 461U,
    462U, 463U, 465U, 466U, 467U, 468U, 469U, 470U,
    471U, 472U, 473U, 474U, 475U, 476U, 477U, 478U,
    479U, 481U, 482U, 483U, 484U, 485U, 486U, 487U,
    488U, 489U, 490U, 491U, 492U, 493U, 494U, 495U,
    496U, 497U, 498U, 499U, 500U, 501U, 502U, 503U,
    504U, 505U, 506U, 507U, 508U, 509U, 510U, 511U
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
