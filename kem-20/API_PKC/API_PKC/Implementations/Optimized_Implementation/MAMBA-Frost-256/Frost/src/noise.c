/********************************************************************************************
* MAMBA-Frost: unstructured LWQ-Z key encapsulation mechanism.
*
* Abstract: secret and ephemeral sampling functions.
*
*********************************************************************************************/

#include <stddef.h>
#include <stdint.h>


void frost_sample_n(uint16_t *s, const size_t n)
{ // Fills vector s with n samples from centered binomial distribution B_eta.
  // Input: pseudo-random 16-bit values passed in s. The input is overwritten by the output.
#ifndef PARAMS_ETA
#define PARAMS_ETA 2
#endif
    for (size_t i = 0; i < n; ++i) {
        uint16_t x = s[i];
        uint16_t a = 0;
        uint16_t b = 0;
        for (unsigned int j = 0; j < PARAMS_ETA; ++j) {
            a = (uint16_t)(a + ((x >> j) & 0x1u));
            b = (uint16_t)(b + ((x >> (PARAMS_ETA + j)) & 0x1u));
        }
        s[i] = (uint16_t)(a - b);
    }
}
