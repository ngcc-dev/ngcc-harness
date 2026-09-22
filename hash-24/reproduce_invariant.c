#include <stdint.h>
#include <stdio.h>

#include "QSH/Implementations/03_Implementations/1_Reference_Implementation/QSH-512/CryptHash_AlgorithmInstance.c"

int main(void)
{
    static const uint32_t expected[4][4] = {
        {0x5274eb55, 0xad9c50ab, 0x039e35d2, 0xb8f59a35},
        {0xdc2c37c5, 0x5d520afa, 0xf2695a51, 0xd15768b7},
        {0xca2300d8, 0x4db0f0a2, 0xa426efbb, 0x80c568e1},
        {0xae1e5625, 0xdd788116, 0xeb37e469, 0x6733d638}
    };
    qsh_params p;
    uint64_t s[64];
    unsigned x0, x1, x2;

    if (select_params(512, &p) != 0)
        return 1;
    for (x0 = 0; x0 < 4; x0++)
        for (x1 = 0; x1 < 4; x1++)
            for (x2 = 0; x2 < 4; x2++)
                s[idx(x0, x1, x2)] = 4 * x0 + x1;

    E3(s, &p);
    for (x0 = 0; x0 < 4; x0++)
        for (x1 = 0; x1 < 4; x1++)
            for (x2 = 0; x2 < 4; x2++)
                if (s[idx(x0, x1, x2)] != expected[x0][x1]) {
                    puts("FAIL: output left the translation-invariant subspace");
                    return 1;
                }

    puts("CONFIRMED: full QSH-512 permutation preserves the 512-bit subspace");
    puts("random-permutation probability: 2^-1536");
    return 0;
}
