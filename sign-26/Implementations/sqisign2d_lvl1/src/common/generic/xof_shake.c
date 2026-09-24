#include <fips202.h>
#include <sqisign_xof.h>

int
sqisign_xof(unsigned char *out,
            size_t out_len_bytes,
            const unsigned char *in,
            size_t in_len_bytes)
{
    SHAKE256(out, out_len_bytes, in, in_len_bytes);
    return 0;
}
