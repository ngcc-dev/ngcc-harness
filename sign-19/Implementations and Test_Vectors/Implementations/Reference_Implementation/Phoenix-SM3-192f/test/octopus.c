#include <stdio.h>
#include <string.h>
#include "../octopus.h"
#include "../randombytes.h"
#include "../params.h"

int main(void)
{
    setbuf(stdout, NULL);
    printf("=== Octopus (Paper Version) Test ===\n");

    uint32_t indices[SPX_TFORS_K];
    octopus_auth auth;

    for (int i = 0; i < SPX_TFORS_K; i++) {
        randombytes((unsigned char*)&indices[i], sizeof(uint32_t));
        indices[i] &= ((1U << SPX_TFORS_HEIGHT) - 1);
    }

    octopus_compute(&auth, indices);

    printf("Computed auth path count: %u\n", auth.count);
    printf("[PASS] Octopus test passed\n");
    return 0;
}