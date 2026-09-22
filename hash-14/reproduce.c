#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Built with -Dstatic= against the submitted generic implementation so that
 * the specification-level Laurus[c,fid] entry point can be exercised. */
void laurus(uint8_t *out, uint64_t outlen, const uint8_t *in,
            uint64_t inlen, uint64_t c, uint64_t fid);

static int equal_domain_outputs(uint64_t message_bits)
{
    uint8_t message[128] = {0};
    uint8_t hash_domain[128];
    uint8_t xof_domain[128];

    laurus(hash_domain, 1024, message, message_bits, 1024, 0);
    laurus(xof_domain, 1024, message, message_bits, 1024, 1);
    return memcmp(hash_domain, xof_domain, sizeof hash_domain) == 0;
}

int main(void)
{
    if (!equal_domain_outputs(768)) {
        fputs("Laurus[1024,0/1]: witness did not reproduce\n", stderr);
        return 1;
    }
    if (equal_domain_outputs(512)) {
        fputs("Laurus[1024,0/1]: short-message control failed\n", stderr);
        return 1;
    }
    puts("Laurus[1024,0/1]: EQUAL at 768 bits; DISTINCT at 512 bits");
    return 0;
}
