/* Native check of the submitted bit-length API used by the K2K wrapper. */
#include <stdio.h>
#include <string.h>

#include "auxfunc.h"

int main(void)
{
    unsigned char first[128], suffix_changed[128], prefix_changed[128];
    unsigned char a[192], b[192], control[192];
    for (unsigned i = 0; i < sizeof first; ++i)
        first[i] = (unsigned char)i;
    memcpy(suffix_changed, first, sizeof first);
    memcpy(prefix_changed, first, sizeof first);
    suffix_changed[127] ^= 1;
    prefix_changed[0] ^= 1;
    if (pseudoXOF(sizeof a * 8, first, sizeof first, a) ||
        pseudoXOF(sizeof b * 8, suffix_changed, sizeof suffix_changed, b) ||
        pseudoXOF(sizeof control * 8, prefix_changed, sizeof prefix_changed, control))
        return 2;
    if (memcmp(a, b, sizeof a) || !memcmp(a, control, sizeof a))
        return 1;
    puts("CONFIRMED kex-03-7: submitted pseudoXOF ignores a changed byte after the first 16");
    return 0;
}
